#include "ui/script/lua_parser.h"

#include <cstring>

#include "ui/script/lua_token.h"

namespace immirror::ui::script {
namespace {

NodePtr Make(Node::Kind kind) {
    NodePtr node(new Node());
    node->kind = kind;
    return node;
}

/* 二元运算符优先级, 0 表示不是二元运算符. */
int32_t PrecedenceOf(const std::string& op) {
    if (op == "or") {
        return 1;
    }
    if (op == "and") {
        return 2;
    }
    if (op == "==" || op == "~=" || op == "<" || op == "<=" || op == ">" || op == ">=") {
        return 3;
    }
    if (op == "..") {
        return 4;
    }
    if (op == "+" || op == "-") {
        return 5;
    }
    if (op == "*" || op == "/" || op == "%") {
        return 6;
    }
    return 0;
}

/* 递归下降解析器: 语句与表达式两层, 块用语句列表表示. */
class Parser {
public:
    Parser(std::vector<Token> tokens, ParseStats* stats) : tokens_(std::move(tokens)), stats_(stats) {}

    bool Run(std::vector<NodePtr>& out, std::string* error) {
        error_ = error;
        if (!ParseBlock(out, false)) {
            return false;
        }
        if (Peek().kind != Token::Kind::kEnd) {
            *error_ = "语句结束后仍有多余内容";
            return false;
        }
        return true;
    }

private:
    const Token& Peek(int32_t offset = 0) const {
        const size_t index = position_ + static_cast<size_t>(offset < 0 ? 0 : offset);
        return index < tokens_.size() ? tokens_[index] : tokens_.back();
    }
    bool CheckSymbol(const char* symbol) const {
        return Peek().kind == Token::Kind::kSymbol && Peek().text == symbol;
    }
    bool CheckKeyword(const char* word) const {
        return Peek().kind == Token::Kind::kKeyword && Peek().text == word;
    }
    bool AcceptSymbol(const char* symbol) {
        if (!CheckSymbol(symbol)) {
            return false;
        }
        position_ += 1;
        return true;
    }
    bool AcceptKeyword(const char* word) {
        if (!CheckKeyword(word)) {
            return false;
        }
        position_ += 1;
        return true;
    }

    bool ParseBlock(std::vector<NodePtr>& out, bool stopAtUntil) {
        while (true) {
            if (Peek().kind == Token::Kind::kEnd) {
                break;
            }
            if (CheckKeyword("end") || CheckKeyword("else") || CheckKeyword("elseif") ||
                CheckKeyword("until")) {
                break;
            }
            if (stopAtUntil && CheckKeyword("until")) {
                break;
            }
            if (!ParseStatement(out)) {
                return false;
            }
        }
        return true;
    }

    bool ParseStatement(std::vector<NodePtr>& out) {
        if (AcceptSymbol(";")) {
            return true;
        }
        if (stats_ != nullptr) {
            stats_->statements += 1;
        }

        if (CheckKeyword("local")) {
            position_ += 1;
            NodePtr node = Make(Node::Kind::kLocal);
            if (AcceptKeyword("function")) {
                if (Peek().kind != Token::Kind::kName) {
                    *error_ = "local function 缺少名字";
                    return false;
                }
                node->text = Peek().text;
                node->names.push_back(Peek().text);
                position_ += 1;
                NodePtr function = Make(Node::Kind::kFunction);
                if (!ParseFunctionBody(*function)) {
                    return false;
                }
                node->list.push_back(function);
                out.push_back(node);
                return true;
            }
            while (true) {
                if (Peek().kind != Token::Kind::kName) {
                    *error_ = "local 后必须是名字";
                    return false;
                }
                node->names.push_back(Peek().text);
                position_ += 1;
                if (!AcceptSymbol(",")) {
                    break;
                }
            }
            if (AcceptSymbol("=")) {
                if (!ParseExpressionList(node->list)) {
                    return false;
                }
            }
            out.push_back(node);
            return true;
        }

        if (CheckKeyword("function")) {
            position_ += 1;
            NodePtr node = Make(Node::Kind::kFunction);
            if (Peek().kind != Token::Kind::kName) {
                *error_ = "function 缺少名字";
                return false;
            }
            node->text = Peek().text;
            position_ += 1;
            /* a.b.c 与 a:b 两种写法: 记成目标表达式链. */
            NodePtr target = Make(Node::Kind::kName);
            target->text = node->text;
            bool methodStyle = false;
            while (CheckSymbol(".") || CheckSymbol(":")) {
                methodStyle = CheckSymbol(":");
                position_ += 1;
                if (Peek().kind != Token::Kind::kName) {
                    *error_ = "函数名后缀必须是名字";
                    return false;
                }
                NodePtr key = Make(Node::Kind::kString);
                key->text = Peek().text;
                position_ += 1;
                NodePtr index = Make(Node::Kind::kIndex);
                index->left = target;
                index->right = key;
                target = index;
            }
            if (!ParseFunctionBody(*node, methodStyle)) {
                return false;
            }
            node->left = target;
            out.push_back(node);
            return true;
        }

        if (CheckKeyword("if")) {
            position_ += 1;
            NodePtr node = Make(Node::Kind::kIf);
            while (true) {
                NodePtr condition = ParseExpression();
                if (condition == nullptr || !AcceptKeyword("then")) {
                    *error_ = "if 缺少 then";
                    return false;
                }
                node->list.push_back(condition);
                std::vector<NodePtr> block;
                if (!ParseBlock(block, false)) {
                    return false;
                }
                node->blocks.push_back(block);
                if (AcceptKeyword("elseif")) {
                    continue;
                }
                if (AcceptKeyword("else")) {
                    std::vector<NodePtr> elseBlock;
                    if (!ParseBlock(elseBlock, false)) {
                        return false;
                    }
                    node->blocks.push_back(elseBlock);
                }
                if (!AcceptKeyword("end")) {
                    *error_ = "if 缺少 end";
                    return false;
                }
                out.push_back(node);
                return true;
            }
        }

        if (CheckKeyword("while")) {
            position_ += 1;
            NodePtr node = Make(Node::Kind::kWhile);
            NodePtr condition = ParseExpression();
            if (condition == nullptr || !AcceptKeyword("do")) {
                *error_ = "while 缺少 do";
                return false;
            }
            node->list.push_back(condition);
            std::vector<NodePtr> block;
            if (!ParseBlock(block, false)) {
                return false;
            }
            if (!AcceptKeyword("end")) {
                *error_ = "while 缺少 end";
                return false;
            }
            node->blocks.push_back(block);
            out.push_back(node);
            return true;
        }

        if (CheckKeyword("repeat")) {
            /* repeat ... until: 先执行后判断, 用 while 形态承载, 标记 op 区分. */
            position_ += 1;
            NodePtr node = Make(Node::Kind::kWhile);
            node->op = "repeat";
            std::vector<NodePtr> block;
            if (!ParseBlock(block, true)) {
                return false;
            }
            if (!AcceptKeyword("until")) {
                *error_ = "repeat 缺少 until";
                return false;
            }
            NodePtr condition = ParseExpression();
            if (condition == nullptr) {
                return false;
            }
            node->list.push_back(condition);
            node->blocks.push_back(block);
            out.push_back(node);
            return true;
        }

        if (CheckKeyword("for")) {
            position_ += 1;
            if (Peek().kind != Token::Kind::kName) {
                *error_ = "for 缺少循环变量";
                return false;
            }
            std::vector<std::string> names;
            names.push_back(Peek().text);
            position_ += 1;
            while (AcceptSymbol(",")) {
                if (Peek().kind != Token::Kind::kName) {
                    *error_ = "for 变量表不合法";
                    return false;
                }
                names.push_back(Peek().text);
                position_ += 1;
            }
            if (AcceptKeyword("in")) {
                /* 泛型 for: 表达式列表 + do ... end, 用 kDo 承载以区分. */
                NodePtr node = Make(Node::Kind::kDo);
                node->op = "forin";
                node->names = names;
                if (!ParseExpressionList(node->list)) {
                    return false;
                }
                if (!AcceptKeyword("do")) {
                    *error_ = "for 缺少 do";
                    return false;
                }
                std::vector<NodePtr> block;
                if (!ParseBlock(block, false)) {
                    return false;
                }
                if (!AcceptKeyword("end")) {
                    *error_ = "for 缺少 end";
                    return false;
                }
                node->blocks.push_back(block);
                out.push_back(node);
                return true;
            }
            NodePtr node = Make(Node::Kind::kNumericFor);
            node->names = names;
            if (!AcceptSymbol("=")) {
                *error_ = "for 缺少等号";
                return false;
            }
            NodePtr start = ParseExpression();
            if (start == nullptr || !AcceptSymbol(",")) {
                *error_ = "for 起始值不合法";
                return false;
            }
            NodePtr stop = ParseExpression();
            if (stop == nullptr) {
                return false;
            }
            node->list.push_back(start);
            node->list.push_back(stop);
            if (AcceptSymbol(",")) {
                NodePtr step = ParseExpression();
                if (step == nullptr) {
                    return false;
                }
                node->list.push_back(step);
            }
            if (!AcceptKeyword("do")) {
                *error_ = "for 缺少 do";
                return false;
            }
            std::vector<NodePtr> block;
            if (!ParseBlock(block, false)) {
                return false;
            }
            if (!AcceptKeyword("end")) {
                *error_ = "for 缺少 end";
                return false;
            }
            node->blocks.push_back(block);
            out.push_back(node);
            return true;
        }

        if (CheckKeyword("do")) {
            position_ += 1;
            NodePtr node = Make(Node::Kind::kDo);
            node->op = "block";
            std::vector<NodePtr> block;
            if (!ParseBlock(block, false)) {
                return false;
            }
            if (!AcceptKeyword("end")) {
                *error_ = "do 缺少 end";
                return false;
            }
            node->blocks.push_back(block);
            out.push_back(node);
            return true;
        }

        if (CheckKeyword("return")) {
            position_ += 1;
            NodePtr node = Make(Node::Kind::kReturn);
            if (!CheckKeyword("end") && !CheckKeyword("else") && !CheckKeyword("elseif") &&
                !CheckKeyword("until") && Peek().kind != Token::Kind::kEnd &&
                !CheckSymbol(";")) {
                if (!ParseExpressionList(node->list)) {
                    return false;
                }
            }
            AcceptSymbol(";");
            out.push_back(node);
            return true;
        }

        /* 标签 ::name:: 与 goto name: 界面脚本里偶有使用, 解析阶段直接跳过. */
        if (AcceptSymbol("::")) {
            if (Peek().kind != Token::Kind::kName) {
                *error_ = "标签缺少名字";
                return false;
            }
            position_ += 1;
            if (!AcceptSymbol("::")) {
                *error_ = "标签未闭合";
                return false;
            }
            return true;
        }
        if (CheckKeyword("goto")) {
            position_ += 1;
            if (Peek().kind != Token::Kind::kName) {
                *error_ = "goto 缺少标签";
                return false;
            }
            position_ += 1;
            return true;
        }

        if (CheckKeyword("break")) {
            position_ += 1;
            out.push_back(Make(Node::Kind::kBreak));
            return true;
        }

        /* 赋值或调用语句. */
        NodePtr first = ParseSuffixedExpression();
        if (first == nullptr) {
            *error_ = "无法解析的语句";
            return false;
        }
        if (CheckSymbol("=") || CheckSymbol(",")) {
            NodePtr node = Make(Node::Kind::kAssign);
            node->list.push_back(first);
            while (AcceptSymbol(",")) {
                NodePtr target = ParseSuffixedExpression();
                if (target == nullptr) {
                    return false;
                }
                node->list.push_back(target);
            }
            if (!AcceptSymbol("=")) {
                *error_ = "赋值缺少等号";
                return false;
            }
            std::vector<NodePtr> values;
            if (!ParseExpressionList(values)) {
                return false;
            }
            node->blocks.push_back(values);
            out.push_back(node);
            return true;
        }
        NodePtr node = Make(Node::Kind::kCallStmt);
        node->list.push_back(first);
        out.push_back(node);
        return true;
    }

    /* 表达式列表: 逗号分隔, 结果追加到 out. */
    bool ParseExpressionList(std::vector<NodePtr>& out) {
        while (true) {
            NodePtr value = ParseExpression();
            if (value == nullptr) {
                return false;
            }
            out.push_back(value);
            if (!AcceptSymbol(",")) {
                break;
            }
        }
        return true;
    }

    bool ParseFunctionBody(Node& node, bool methodStyle = false) {
        if (stats_ != nullptr) {
            stats_->functions += 1;
        }
        if (!AcceptSymbol("(")) {
            *error_ = "函数缺少参数表";
            return false;
        }
        if (methodStyle) {
            node.names.push_back("self");
        }
        while (!CheckSymbol(")")) {
            /* 可变参数 ... 按名字 "..." 记入参数表, 解析阶段不区分求值方式. */
            if (AcceptSymbol("..")) {
                /* ".." 与 "..." 在词法上会被切成 ".." + ".", 这里兼容两种切分. */
                AcceptSymbol(".");
                node.names.push_back("...");
            } else if (AcceptSymbol("...")) {
                node.names.push_back("...");
            } else {
                if (Peek().kind != Token::Kind::kName) {
                    *error_ = "函数参数必须是名字";
                    return false;
                }
                node.names.push_back(Peek().text);
                position_ += 1;
            }
            if (!AcceptSymbol(",")) {
                break;
            }
        }
        if (!AcceptSymbol(")")) {
            *error_ = "函数参数表未闭合";
            return false;
        }
        std::vector<NodePtr> body;
        if (!ParseBlock(body, false)) {
            return false;
        }
        if (!AcceptKeyword("end")) {
            *error_ = "函数缺少 end";
            return false;
        }
        node.blocks.push_back(body);
        return true;
    }

    NodePtr ParseExpression() { return ParseBinary(1); }

    NodePtr ParseBinary(int32_t minPrecedence) {
        NodePtr left = ParseUnary();
        if (left == nullptr) {
            return nullptr;
        }
        while (true) {
            const Token& token = Peek();
            if (token.kind != Token::Kind::kSymbol && token.kind != Token::Kind::kKeyword) {
                break;
            }
            const int32_t precedence = PrecedenceOf(token.text);
            if (precedence == 0 || precedence < minPrecedence) {
                break;
            }
            const std::string op = token.text;
            position_ += 1;
            NodePtr right = ParseBinary(precedence + 1);
            if (right == nullptr) {
                *error_ = "运算符右侧缺少表达式";
                return nullptr;
            }
            NodePtr node = Make(Node::Kind::kBinary);
            node->op = op;
            node->left = left;
            node->right = right;
            left = node;
            if (stats_ != nullptr) {
                stats_->expressions += 1;
            }
        }
        return left;
    }

    NodePtr ParseUnary() {
        if (CheckKeyword("not") || CheckSymbol("-") || CheckSymbol("#")) {
            const std::string op = Peek().text;
            position_ += 1;
            NodePtr operand = ParseUnary();
            if (operand == nullptr) {
                return nullptr;
            }
            NodePtr node = Make(Node::Kind::kUnary);
            node->op = op;
            node->left = operand;
            return node;
        }
        return ParseSuffixedExpression();
    }

    NodePtr ParseSuffixedExpression() {
        NodePtr base = ParsePrimary();
        if (base == nullptr) {
            return nullptr;
        }
        return ParseSuffixes(base);
    }

    NodePtr ParsePrimary() {
        /* 可变参数表达式: 出现在实参与表构造里. */
        if (AcceptSymbol("..")) {
            AcceptSymbol(".");
            NodePtr node = Make(Node::Kind::kName);
            node->text = "...";
            return node;
        }
        const Token token = Peek();
        if (token.kind == Token::Kind::kNumber) {
            position_ += 1;
            NodePtr node = Make(Node::Kind::kNumber);
            node->number = token.number;
            return node;
        }
        if (token.kind == Token::Kind::kString) {
            position_ += 1;
            NodePtr node = Make(Node::Kind::kString);
            node->text = token.text;
            return node;
        }
        if (token.kind == Token::Kind::kKeyword) {
            if (token.text == "nil") {
                position_ += 1;
                return Make(Node::Kind::kNil);
            }
            if (token.text == "true" || token.text == "false") {
                position_ += 1;
                NodePtr node = Make(Node::Kind::kBool);
                node->boolean = token.text == "true";
                return node;
            }
            if (token.text == "function") {
                position_ += 1;
                NodePtr node = Make(Node::Kind::kFunction);
                if (!ParseFunctionBody(*node)) {
                    return nullptr;
                }
                return node;
            }
            return nullptr;
        }
        if (token.kind == Token::Kind::kSymbol) {
            if (token.text == "(") {
                position_ += 1;
                NodePtr inner = ParseExpression();
                if (inner == nullptr || !AcceptSymbol(")")) {
                    return nullptr;
                }
                return inner;
            }
            if (token.text == "{") {
                return ParseTable();
            }
            return nullptr;
        }
        if (token.kind == Token::Kind::kName) {
            position_ += 1;
            NodePtr node = Make(Node::Kind::kName);
            node->text = token.text;
            return node;
        }
        return nullptr;
    }

    NodePtr ParseTable() {
        if (stats_ != nullptr) {
            stats_->tables += 1;
        }
        position_ += 1;  /* { */
        NodePtr node = Make(Node::Kind::kTable);
        while (!CheckSymbol("}")) {
            if (Peek().kind == Token::Kind::kEnd) {
                *error_ = "表构造未闭合";
                return nullptr;
            }
            if (CheckSymbol("[")) {
                position_ += 1;
                NodePtr key = ParseExpression();
                if (key == nullptr || !AcceptSymbol("]") || !AcceptSymbol("=")) {
                    *error_ = "表的键写法不合法";
                    return nullptr;
                }
                NodePtr value = ParseExpression();
                if (value == nullptr) {
                    return nullptr;
                }
                node->fields.emplace_back(key, value);
            } else if (Peek().kind == Token::Kind::kName && Peek(1).kind == Token::Kind::kSymbol &&
                       Peek(1).text == "=") {
                NodePtr key = Make(Node::Kind::kString);
                key->text = Peek().text;
                position_ += 2;
                NodePtr value = ParseExpression();
                if (value == nullptr) {
                    return nullptr;
                }
                node->fields.emplace_back(key, value);
            } else {
                NodePtr value = ParseExpression();
                if (value == nullptr) {
                    *error_ = "表的数组项不合法";
                    return nullptr;
                }
                node->list.push_back(value);
            }
            if (!AcceptSymbol(",") && !AcceptSymbol(";")) {
                break;
            }
        }
        if (!AcceptSymbol("}")) {
            *error_ = "表构造未闭合";
            return nullptr;
        }
        return node;
    }

    NodePtr ParseSuffixes(NodePtr base) {
        NodePtr current = base;
        while (true) {
            if (CheckSymbol(".")) {
                position_ += 1;
                if (Peek().kind != Token::Kind::kName) {
                    *error_ = "点号后必须是名字";
                    return nullptr;
                }
                NodePtr key = Make(Node::Kind::kString);
                key->text = Peek().text;
                position_ += 1;
                NodePtr node = Make(Node::Kind::kIndex);
                node->left = current;
                node->right = key;
                current = node;
                continue;
            }
            if (CheckSymbol("[")) {
                position_ += 1;
                NodePtr key = ParseExpression();
                if (key == nullptr || !AcceptSymbol("]")) {
                    return nullptr;
                }
                NodePtr node = Make(Node::Kind::kIndex);
                node->left = current;
                node->right = key;
                current = node;
                continue;
            }
            if (CheckSymbol(":")) {
                position_ += 1;
                if (Peek().kind != Token::Kind::kName) {
                    *error_ = "冒号后必须是方法名";
                    return nullptr;
                }
                NodePtr node = Make(Node::Kind::kMethodCall);
                node->text = Peek().text;
                node->left = current;
                position_ += 1;
                /* 方法调用也允许省略括号: obj:method "text" 与 obj:method { ... }. */
                if (CheckSymbol("(")) {
                    if (!ParseCallArguments(node->list)) {
                        return nullptr;
                    }
                } else if (Peek().kind == Token::Kind::kString) {
                    NodePtr arg = Make(Node::Kind::kString);
                    arg->text = Peek().text;
                    position_ += 1;
                    node->list.push_back(arg);
                } else if (CheckSymbol("{")) {
                    NodePtr arg = ParseTable();
                    if (arg == nullptr) {
                        return nullptr;
                    }
                    node->list.push_back(arg);
                } else {
                    *error_ = "方法调用缺少实参";
                    return nullptr;
                }
                current = node;
                continue;
            }
            if (CheckSymbol("(")) {
                NodePtr node = Make(Node::Kind::kCall);
                node->left = current;
                if (!ParseCallArguments(node->list)) {
                    return nullptr;
                }
                current = node;
                continue;
            }
            if (Peek().kind == Token::Kind::kString) {
                /* 单字符串实参的调用写法: f "text". */
                NodePtr node = Make(Node::Kind::kCall);
                node->left = current;
                NodePtr arg = Make(Node::Kind::kString);
                arg->text = Peek().text;
                position_ += 1;
                node->list.push_back(arg);
                current = node;
                continue;
            }
            if (CheckSymbol("{")) {
                /* 单表实参的调用写法: f { ... }. */
                NodePtr node = Make(Node::Kind::kCall);
                node->left = current;
                NodePtr arg = ParseTable();
                if (arg == nullptr) {
                    return nullptr;
                }
                node->list.push_back(arg);
                current = node;
                continue;
            }
            break;
        }
        return current;
    }

    bool ParseCallArguments(std::vector<NodePtr>& out) {
        if (!AcceptSymbol("(")) {
            *error_ = "调用缺少参数表";
            return false;
        }
        while (!CheckSymbol(")")) {
            NodePtr arg = ParseExpression();
            if (arg == nullptr) {
                return false;
            }
            out.push_back(arg);
            if (!AcceptSymbol(",")) {
                break;
            }
        }
        if (!AcceptSymbol(")")) {
            *error_ = "调用参数表未闭合";
            return false;
        }
        return true;
    }

    std::vector<Token> tokens_;
    size_t position_ = 0;
    ParseStats* stats_ = nullptr;
    std::string* error_ = nullptr;
};

}  /* namespace */

std::vector<NodePtr> ParseScript(const char* source, size_t length, ParseStats* stats,
                                 std::string* error) {
    std::vector<NodePtr> empty;
    std::string localError;
    std::string* sink = error != nullptr ? error : &localError;
    sink->clear();
    std::vector<Token> tokens;
    if (!Tokenize(source, length, tokens, sink)) {
        return empty;
    }
    Parser parser(std::move(tokens), stats);
    std::vector<NodePtr> out;
    if (!parser.Run(out, sink)) {
        return empty;
    }
    return out;
}

}  /* namespace immirror::ui::script */
