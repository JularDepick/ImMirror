#include "parser.h"

#include <utility>

namespace immirror::script {

namespace {

/* 单次解析允许累计的错误上限, 避免错误恢复死循环时刷屏 */
const size_t kMaxErrors = 16;

ast::ExprPtr MakeExpr(ast::ExprKind kind, uint32_t line) {
    ast::ExprPtr expr(new ast::Expr());
    expr->kind = kind;
    expr->line = line;
    return expr;
}

ast::StmtPtr MakeStmt(ast::StmtKind kind, uint32_t line) {
    ast::StmtPtr stmt(new ast::Stmt());
    stmt->kind = kind;
    stmt->line = line;
    return stmt;
}

}  /* namespace */

Parser::Parser(std::vector<Token> tokens, std::string chunkName)
    : chunkName_(std::move(chunkName)), tokens_(std::move(tokens)) {}

const Token& Parser::Peek(size_t offset) const {
    const size_t position = index_ + offset;
    if (position >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[position];
}

const Token& Parser::Current() const { return Peek(0); }

bool Parser::Check(TokenType type) const { return Current().type == type; }

bool Parser::Match(TokenType type) {
    if (!Check(type)) {
        return false;
    }
    ++index_;
    return true;
}

void Parser::Advance() {
    if (index_ + 1 < tokens_.size()) {
        ++index_;
    }
}

void Parser::ReportError(uint32_t line, uint32_t column, const std::string& message) {
    if (errors_.size() >= kMaxErrors) {
        return;
    }
    Diagnostic diagnostic;
    diagnostic.line = line;
    diagnostic.column = column;
    diagnostic.message = message;
    errors_.push_back(std::move(diagnostic));
}

bool Parser::Expect(TokenType type, const char* what) {
    if (Match(type)) {
        return true;
    }
    const Token& token = Current();
    std::string message = "期望 ";
    message += what;
    message += ", 实际得到 ";
    message += TokenTypeName(token.type);
    if (token.type == TokenType::Identifier || token.type == TokenType::Number) {
        message += "(" + token.text + ")";
    }
    ReportError(token.line, token.column, message);
    return false;
}

bool Parser::ExpectIdentifier(std::string& out, const char* what) {
    if (!Check(TokenType::Identifier)) {
        const Token& token = Current();
        std::string message = "期望 ";
        message += what;
        message += ", 实际得到 ";
        message += TokenTypeName(token.type);
        ReportError(token.line, token.column, message);
        return false;
    }
    out = Current().text;
    Advance();
    return true;
}

/*
 * 错误恢复: 跳过当前语句的剩余部分, 停在分号或右花括号之后,
 * 使后续语句仍能被解析.
 */
void Parser::Synchronize() {
    while (!Check(TokenType::EndOfFile)) {
        if (Check(TokenType::Semicolon)) {
            Advance();
            return;
        }
        if (Check(TokenType::RBrace)) {
            return;
        }
        if (Check(TokenType::KwVar) || Check(TokenType::KwFn) || Check(TokenType::KwIf) ||
            Check(TokenType::KwWhile) || Check(TokenType::KwFor) || Check(TokenType::KwReturn) ||
            Check(TokenType::KwBreak) || Check(TokenType::KwContinue) ||
            Check(TokenType::KwElse) || Check(TokenType::KwConst)) {
            return;
        }
        Advance();
    }
}

bool Parser::ParseModule(ast::Module& module, std::vector<Diagnostic>& errors) {
    module.statements.clear();
    while (!Check(TokenType::EndOfFile)) {
        if (Match(TokenType::Semicolon)) {
            continue;
        }
        const size_t startIndex = index_;
        ast::StmtPtr statement;
        if (!ParseStatement(statement)) {
            Synchronize();
            /* 恢复过程没有任何前进时强制跳过当前单元, 避免死循环 */
            if (index_ == startIndex) {
                Advance();
            }
            continue;
        }
        if (statement) {
            module.statements.push_back(std::move(statement));
        }
    }
    errors = errors_;
    module.ok = errors_.empty();
    return module.ok;
}

bool Parser::ParseStatement(ast::StmtPtr& out) {
    const Token& token = Current();
    switch (token.type) {
        case TokenType::LBrace: {
            ast::StmtPtr stmt = MakeStmt(ast::StmtKind::Block, token.line);
            if (!ParseBlock(stmt->body)) {
                return false;
            }
            out = std::move(stmt);
            return true;
        }
        case TokenType::KwVar:
        case TokenType::KwConst:
            return ParseVarDecl(out, true);
        case TokenType::KwFn:
            return ParseFunctionDecl(out);
        case TokenType::KwIf:
            return ParseIf(out);
        case TokenType::KwWhile:
            return ParseWhile(out);
        case TokenType::KwFor:
            return ParseFor(out);
        case TokenType::KwReturn:
            return ParseReturn(out);
        case TokenType::KwBreak: {
            ast::StmtPtr stmt = MakeStmt(ast::StmtKind::Break, token.line);
            Advance();
            Match(TokenType::Semicolon);
            out = std::move(stmt);
            return true;
        }
        case TokenType::KwContinue: {
            ast::StmtPtr stmt = MakeStmt(ast::StmtKind::Continue, token.line);
            Advance();
            Match(TokenType::Semicolon);
            out = std::move(stmt);
            return true;
        }
        case TokenType::KwElse:
            ReportError(token.line, token.column, "else 没有匹配的 if");
            Advance();
            return false;
        default:
            return ParseExprStatement(out, true);
    }
}

bool Parser::ParseBlock(std::vector<ast::StmtPtr>& out) {
    if (!Expect(TokenType::LBrace, "{")) {
        return false;
    }
    while (!Check(TokenType::RBrace) && !Check(TokenType::EndOfFile)) {
        if (Match(TokenType::Semicolon)) {
            continue;
        }
        const size_t startIndex = index_;
        ast::StmtPtr statement;
        if (!ParseStatement(statement)) {
            Synchronize();
            if (index_ == startIndex) {
                Advance();
            }
            continue;
        }
        if (statement) {
            out.push_back(std::move(statement));
        }
    }
    return Expect(TokenType::RBrace, "}");
}

bool Parser::ParseVarDecl(ast::StmtPtr& out, bool expectSemicolon) {
    const Token& keyword = Current();
    const bool isConst = keyword.type == TokenType::KwConst;
    Advance();

    ast::StmtPtr stmt = MakeStmt(ast::StmtKind::VarDecl, keyword.line);
    stmt->isConst = isConst;

    std::string name;
    if (!ExpectIdentifier(name, "变量名")) {
        return false;
    }
    stmt->names.push_back(name);
    while (Match(TokenType::Comma)) {
        if (!ExpectIdentifier(name, "变量名")) {
            return false;
        }
        stmt->names.push_back(name);
    }

    if (Match(TokenType::Assign)) {
        if (!ParseExpressionList(stmt->exprs)) {
            return false;
        }
    }
    if (expectSemicolon) {
        Match(TokenType::Semicolon);
    }
    out = std::move(stmt);
    return true;
}

bool Parser::ParseFunctionDecl(ast::StmtPtr& out) {
    const Token& keyword = Current();
    Advance();

    std::string name;
    if (!ExpectIdentifier(name, "函数名")) {
        return false;
    }
    ast::StmtPtr stmt = MakeStmt(ast::StmtKind::FunctionDecl, keyword.line);
    stmt->func.reset(new ast::FunctionDef());
    stmt->func->name = name;
    stmt->func->line = keyword.line;
    stmt->names.push_back(name);

    if (!Expect(TokenType::LParen, "(")) {
        return false;
    }
    if (!Check(TokenType::RParen)) {
        for (;;) {
            std::string param;
            if (!ExpectIdentifier(param, "参数名")) {
                return false;
            }
            stmt->func->params.push_back(param);
            if (!Match(TokenType::Comma)) {
                break;
            }
        }
    }
    if (!Expect(TokenType::RParen, ")")) {
        return false;
    }
    if (!ParseBlock(stmt->func->body)) {
        return false;
    }
    out = std::move(stmt);
    return true;
}

bool Parser::ParseIf(ast::StmtPtr& out) {
    const Token& keyword = Current();
    Advance();
    ast::StmtPtr stmt = MakeStmt(ast::StmtKind::If, keyword.line);

    for (;;) {
        ast::IfBranch branch;
        if (!Expect(TokenType::LParen, "(")) {
            return false;
        }
        if (!ParseExpression(branch.cond)) {
            return false;
        }
        if (!Expect(TokenType::RParen, ")")) {
            return false;
        }
        if (!ParseBlock(branch.body)) {
            return false;
        }
        stmt->branches.push_back(std::move(branch));

        if (!Match(TokenType::KwElse)) {
            break;
        }
        if (!Match(TokenType::KwIf)) {
            if (!ParseBlock(stmt->elseBody)) {
                return false;
            }
            break;
        }
    }
    out = std::move(stmt);
    return true;
}

bool Parser::ParseWhile(ast::StmtPtr& out) {
    const Token& keyword = Current();
    Advance();
    ast::StmtPtr stmt = MakeStmt(ast::StmtKind::While, keyword.line);
    if (!Expect(TokenType::LParen, "(")) {
        return false;
    }
    if (!ParseExpression(stmt->cond)) {
        return false;
    }
    if (!Expect(TokenType::RParen, ")")) {
        return false;
    }
    if (!ParseBlock(stmt->body)) {
        return false;
    }
    out = std::move(stmt);
    return true;
}

bool Parser::ParseFor(ast::StmtPtr& out) {
    const Token& keyword = Current();
    Advance();
    ast::StmtPtr stmt = MakeStmt(ast::StmtKind::For, keyword.line);
    if (!Expect(TokenType::LParen, "(")) {
        return false;
    }
    if (!Check(TokenType::Semicolon)) {
        if (Check(TokenType::KwVar) || Check(TokenType::KwConst)) {
            ast::StmtPtr init;
            if (!ParseVarDecl(init, false)) {
                return false;
            }
            stmt->forInit = std::move(init);
        } else {
            ast::StmtPtr init;
            if (!ParseExprStatement(init, false)) {
                return false;
            }
            stmt->forInit = std::move(init);
        }
    }
    if (!Expect(TokenType::Semicolon, ";")) {
        return false;
    }
    if (!Check(TokenType::Semicolon)) {
        if (!ParseExpression(stmt->forCond)) {
            return false;
        }
    }
    if (!Expect(TokenType::Semicolon, ";")) {
        return false;
    }
    if (!Check(TokenType::RParen)) {
        ast::StmtPtr step;
        if (!ParseExprStatement(step, false)) {
            return false;
        }
        stmt->forStep = std::move(step);
    }
    if (!Expect(TokenType::RParen, ")")) {
        return false;
    }
    if (!ParseBlock(stmt->body)) {
        return false;
    }
    out = std::move(stmt);
    return true;
}

bool Parser::ParseReturn(ast::StmtPtr& out) {
    const Token& keyword = Current();
    Advance();
    ast::StmtPtr stmt = MakeStmt(ast::StmtKind::Return, keyword.line);
    if (!Check(TokenType::Semicolon) && !Check(TokenType::RBrace) && !Check(TokenType::EndOfFile)) {
        if (!ParseExpressionList(stmt->exprs)) {
            return false;
        }
    }
    Match(TokenType::Semicolon);
    out = std::move(stmt);
    return true;
}

/*
 * 表达式语句与赋值语句共用入口: 先解析表达式列表,
 * 若后面紧跟赋值符号则整体改写为赋值语句.
 */
bool Parser::ParseExprStatement(ast::StmtPtr& out, bool expectSemicolon) {
    const uint32_t line = Current().line;
    std::vector<ast::ExprPtr> exprs;
    if (!ParseExpressionList(exprs)) {
        return false;
    }

    TokenType assignType = TokenType::EndOfFile;
    if (Check(TokenType::Assign) || Check(TokenType::PlusAssign) ||
        Check(TokenType::MinusAssign) || Check(TokenType::StarAssign) ||
        Check(TokenType::SlashAssign)) {
        assignType = Current().type;
        Advance();
    }

    if (assignType == TokenType::EndOfFile) {
        if (exprs.size() > 1) {
            ReportError(line, 1, "表达式列表只能出现在赋值或函数调用中");
            return false;
        }
        ast::StmtPtr stmt = MakeStmt(ast::StmtKind::Expr, line);
        stmt->exprs.push_back(std::move(exprs[0]));
        if (expectSemicolon) {
            Match(TokenType::Semicolon);
        }
        out = std::move(stmt);
        return true;
    }

    std::vector<ast::ExprPtr> values;
    if (!ParseExpressionList(values)) {
        return false;
    }
    if (expectSemicolon) {
        Match(TokenType::Semicolon);
    }

    ast::StmtPtr stmt = MakeStmt(ast::StmtKind::Assign, line);
    stmt->targets = std::move(exprs);

    if (assignType == TokenType::Assign) {
        if (values.size() != 1 && values.size() != stmt->targets.size()) {
            ReportError(line, 1, "赋值左右两边的数量不一致");
            return false;
        }
        stmt->exprs = std::move(values);
        out = std::move(stmt);
        return true;
    }

    /* 复合赋值只在单目标单值时有效, 内部改写为 target = target op value */
    if (stmt->targets.size() != 1 || values.size() != 1) {
        ReportError(line, 1, "复合赋值只支持单个目标与单个右值");
        return false;
    }
    ast::BinaryOp op = ast::BinaryOp::Add;
    switch (assignType) {
        case TokenType::PlusAssign: op = ast::BinaryOp::Add; break;
        case TokenType::MinusAssign: op = ast::BinaryOp::Sub; break;
        case TokenType::StarAssign: op = ast::BinaryOp::Mul; break;
        case TokenType::SlashAssign: op = ast::BinaryOp::Div; break;
        default: break;
    }
    ast::ExprPtr read = ast::CloneExpr(*stmt->targets[0]);
    ast::ExprPtr combined = MakeExpr(ast::ExprKind::Binary, line);
    combined->binaryOp = op;
    combined->a = std::move(read);
    combined->b = std::move(values[0]);
    stmt->exprs.push_back(std::move(combined));
    out = std::move(stmt);
    return true;
}

bool Parser::ParseExpressionList(std::vector<ast::ExprPtr>& out) {
    for (;;) {
        ast::ExprPtr expr;
        if (!ParseExpression(expr)) {
            return false;
        }
        out.push_back(std::move(expr));
        if (!Match(TokenType::Comma)) {
            break;
        }
    }
    return true;
}

bool Parser::ParseExpression(ast::ExprPtr& out) { return ParseOr(out); }

bool Parser::ParseOr(ast::ExprPtr& out) {
    if (!ParseAnd(out)) {
        return false;
    }
    while (Check(TokenType::OrOr) || Check(TokenType::KwOr)) {
        const uint32_t line = Current().line;
        Advance();
        ast::ExprPtr right;
        if (!ParseAnd(right)) {
            return false;
        }
        ast::ExprPtr binary = MakeExpr(ast::ExprKind::Binary, line);
        binary->binaryOp = ast::BinaryOp::Or;
        binary->a = std::move(out);
        binary->b = std::move(right);
        out = std::move(binary);
    }
    return true;
}

bool Parser::ParseAnd(ast::ExprPtr& out) {
    if (!ParseEquality(out)) {
        return false;
    }
    while (Check(TokenType::AndAnd) || Check(TokenType::KwAnd)) {
        const uint32_t line = Current().line;
        Advance();
        ast::ExprPtr right;
        if (!ParseEquality(right)) {
            return false;
        }
        ast::ExprPtr binary = MakeExpr(ast::ExprKind::Binary, line);
        binary->binaryOp = ast::BinaryOp::And;
        binary->a = std::move(out);
        binary->b = std::move(right);
        out = std::move(binary);
    }
    return true;
}

bool Parser::ParseEquality(ast::ExprPtr& out) {
    if (!ParseRelational(out)) {
        return false;
    }
    while (Check(TokenType::Eq) || Check(TokenType::Ne)) {
        const TokenType type = Current().type;
        const uint32_t line = Current().line;
        Advance();
        ast::ExprPtr right;
        if (!ParseRelational(right)) {
            return false;
        }
        ast::ExprPtr binary = MakeExpr(ast::ExprKind::Binary, line);
        binary->binaryOp = (type == TokenType::Eq) ? ast::BinaryOp::Eq : ast::BinaryOp::Ne;
        binary->a = std::move(out);
        binary->b = std::move(right);
        out = std::move(binary);
    }
    return true;
}

bool Parser::ParseRelational(ast::ExprPtr& out) {
    if (!ParseAdditive(out)) {
        return false;
    }
    for (;;) {
        ast::BinaryOp op;
        if (Check(TokenType::Lt)) {
            op = ast::BinaryOp::Lt;
        } else if (Check(TokenType::Le)) {
            op = ast::BinaryOp::Le;
        } else if (Check(TokenType::Gt)) {
            op = ast::BinaryOp::Gt;
        } else if (Check(TokenType::Ge)) {
            op = ast::BinaryOp::Ge;
        } else {
            return true;
        }
        const uint32_t line = Current().line;
        Advance();
        ast::ExprPtr right;
        if (!ParseAdditive(right)) {
            return false;
        }
        ast::ExprPtr binary = MakeExpr(ast::ExprKind::Binary, line);
        binary->binaryOp = op;
        binary->a = std::move(out);
        binary->b = std::move(right);
        out = std::move(binary);
    }
}

bool Parser::ParseAdditive(ast::ExprPtr& out) {
    if (!ParseMultiplicative(out)) {
        return false;
    }
    while (Check(TokenType::Plus) || Check(TokenType::Minus)) {
        const TokenType type = Current().type;
        const uint32_t line = Current().line;
        Advance();
        ast::ExprPtr right;
        if (!ParseMultiplicative(right)) {
            return false;
        }
        ast::ExprPtr binary = MakeExpr(ast::ExprKind::Binary, line);
        binary->binaryOp = (type == TokenType::Plus) ? ast::BinaryOp::Add : ast::BinaryOp::Sub;
        binary->a = std::move(out);
        binary->b = std::move(right);
        out = std::move(binary);
    }
    return true;
}

bool Parser::ParseMultiplicative(ast::ExprPtr& out) {
    if (!ParseUnary(out)) {
        return false;
    }
    for (;;) {
        ast::BinaryOp op;
        if (Check(TokenType::Star)) {
            op = ast::BinaryOp::Mul;
        } else if (Check(TokenType::Slash)) {
            op = ast::BinaryOp::Div;
        } else if (Check(TokenType::Percent)) {
            op = ast::BinaryOp::Mod;
        } else {
            return true;
        }
        const uint32_t line = Current().line;
        Advance();
        ast::ExprPtr right;
        if (!ParseUnary(right)) {
            return false;
        }
        ast::ExprPtr binary = MakeExpr(ast::ExprKind::Binary, line);
        binary->binaryOp = op;
        binary->a = std::move(out);
        binary->b = std::move(right);
        out = std::move(binary);
    }
}

bool Parser::ParseUnary(ast::ExprPtr& out) {
    const Token& token = Current();
    if (Check(TokenType::Minus)) {
        Advance();
        ast::ExprPtr operand;
        if (!ParseUnary(operand)) {
            return false;
        }
        /* 负数常量在语法层直接折叠, 省去一条运行期指令 */
        if (operand->kind == ast::ExprKind::Int) {
            operand->intValue = -operand->intValue;
            out = std::move(operand);
            return true;
        }
        if (operand->kind == ast::ExprKind::Float) {
            operand->floatValue = -operand->floatValue;
            out = std::move(operand);
            return true;
        }
        ast::ExprPtr unary = MakeExpr(ast::ExprKind::Unary, token.line);
        unary->unaryOp = ast::UnaryOp::Neg;
        unary->a = std::move(operand);
        out = std::move(unary);
        return true;
    }
    if (Check(TokenType::Bang) || Check(TokenType::KwNot)) {
        Advance();
        ast::ExprPtr operand;
        if (!ParseUnary(operand)) {
            return false;
        }
        ast::ExprPtr unary = MakeExpr(ast::ExprKind::Unary, token.line);
        unary->unaryOp = ast::UnaryOp::Not;
        unary->a = std::move(operand);
        out = std::move(unary);
        return true;
    }
    return ParsePostfix(out);
}

bool Parser::ParsePostfix(ast::ExprPtr& out) {
    if (!ParsePrimary(out)) {
        return false;
    }
    for (;;) {
        if (Match(TokenType::Dot)) {
            const uint32_t fieldLine = Current().line;
            std::string fieldName;
            if (!ExpectIdentifier(fieldName, "字段名")) {
                return false;
            }
            ast::ExprPtr field = MakeExpr(ast::ExprKind::Field, fieldLine);
            field->a = std::move(out);
            field->stringValue = fieldName;
            out = std::move(field);
            continue;
        }
        if (Match(TokenType::LBracket)) {
            ast::ExprPtr index = MakeExpr(ast::ExprKind::Index, Current().line);
            ast::ExprPtr key;
            if (!ParseExpression(key)) {
                return false;
            }
            if (!Expect(TokenType::RBracket, "]")) {
                return false;
            }
            index->a = std::move(out);
            index->b = std::move(key);
            out = std::move(index);
            continue;
        }
        if (Check(TokenType::LParen)) {
            ast::ExprPtr call = MakeExpr(ast::ExprKind::Call, Current().line);
            Advance();
            if (!Check(TokenType::RParen)) {
                if (!ParseExpressionList(call->args)) {
                    return false;
                }
            }
            if (!Expect(TokenType::RParen, ")")) {
                return false;
            }
            call->a = std::move(out);
            out = std::move(call);
            continue;
        }
        return true;
    }
}

bool Parser::ParsePrimary(ast::ExprPtr& out) {
    const Token& token = Current();
    switch (token.type) {
        case TokenType::KwNil:
            out = MakeExpr(ast::ExprKind::Nil, token.line);
            Advance();
            return true;
        case TokenType::KwTrue:
        case TokenType::KwFalse: {
            ast::ExprPtr expr = MakeExpr(ast::ExprKind::Bool, token.line);
            expr->boolValue = token.type == TokenType::KwTrue;
            Advance();
            out = std::move(expr);
            return true;
        }
        case TokenType::Number: {
            ast::ExprPtr expr = MakeExpr(token.isFloat ? ast::ExprKind::Float : ast::ExprKind::Int,
                                        token.line);
            expr->intValue = token.intValue;
            expr->floatValue = token.floatValue;
            Advance();
            out = std::move(expr);
            return true;
        }
        case TokenType::String: {
            ast::ExprPtr expr = MakeExpr(ast::ExprKind::String, token.line);
            expr->stringValue = token.text;
            Advance();
            out = std::move(expr);
            return true;
        }
        case TokenType::Identifier: {
            ast::ExprPtr expr = MakeExpr(ast::ExprKind::Name, token.line);
            expr->stringValue = token.text;
            Advance();
            out = std::move(expr);
            return true;
        }
        case TokenType::LParen: {
            Advance();
            if (!ParseExpression(out)) {
                return false;
            }
            return Expect(TokenType::RParen, ")");
        }
        case TokenType::LBrace:
            return ParseTable(out);
        case TokenType::KwFn:
            return ParseFunctionExpression(out);
        default:
            ReportError(token.line, token.column,
                        std::string("期望表达式, 实际得到 ") + TokenTypeName(token.type));
            return false;
    }
}

bool Parser::ParseTable(ast::ExprPtr& out) {
    const Token& token = Current();
    ast::ExprPtr table = MakeExpr(ast::ExprKind::Table, token.line);
    Advance();
    if (Match(TokenType::RBrace)) {
        out = std::move(table);
        return true;
    }
    for (;;) {
        ast::TableField field;
        if (Check(TokenType::Identifier) && Peek(1).type == TokenType::Assign) {
            field.kind = ast::TableField::Kind::Named;
            field.name = Current().text;
            Advance();
            Advance();
            if (!ParseExpression(field.value)) {
                return false;
            }
        } else if (Check(TokenType::LBracket)) {
            field.kind = ast::TableField::Kind::Computed;
            Advance();
            if (!ParseExpression(field.key)) {
                return false;
            }
            if (!Expect(TokenType::RBracket, "]")) {
                return false;
            }
            if (!Expect(TokenType::Assign, "=")) {
                return false;
            }
            if (!ParseExpression(field.value)) {
                return false;
            }
        } else {
            field.kind = ast::TableField::Kind::Positional;
            if (!ParseExpression(field.value)) {
                return false;
            }
        }
        table->fields.push_back(std::move(field));
        if (Match(TokenType::Comma) || Match(TokenType::Semicolon)) {
            if (Check(TokenType::RBrace)) {
                break;
            }
            continue;
        }
        break;
    }
    return Expect(TokenType::RBrace, "}");
}

bool Parser::ParseFunctionExpression(ast::ExprPtr& out) {
    const Token& token = Current();
    Advance();
    ast::ExprPtr expr = MakeExpr(ast::ExprKind::Function, token.line);
    expr->func.reset(new ast::FunctionDef());
    expr->func->line = token.line;
    expr->func->name = "<匿名>";

    if (!Expect(TokenType::LParen, "(")) {
        return false;
    }
    if (!Check(TokenType::RParen)) {
        for (;;) {
            std::string param;
            if (!ExpectIdentifier(param, "参数名")) {
                return false;
            }
            expr->func->params.push_back(param);
            if (!Match(TokenType::Comma)) {
                break;
            }
        }
    }
    if (!Expect(TokenType::RParen, ")")) {
        return false;
    }
    if (!ParseBlock(expr->func->body)) {
        return false;
    }
    out = std::move(expr);
    return true;
}

}  /* namespace immirror::script */
