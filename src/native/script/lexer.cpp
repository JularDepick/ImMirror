#include "lexer.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

namespace immirror::script {

namespace {

/* 关键字表, 顺序无要求, 查找走线性扫描, 关键字总量很小 */
struct KeywordEntry {
    const char* name;
    TokenType type;
};

const KeywordEntry kKeywords[] = {
    {"var", TokenType::KwVar},
    {"const", TokenType::KwConst},
    {"fn", TokenType::KwFn},
    {"return", TokenType::KwReturn},
    {"if", TokenType::KwIf},
    {"else", TokenType::KwElse},
    {"while", TokenType::KwWhile},
    {"for", TokenType::KwFor},
    {"break", TokenType::KwBreak},
    {"continue", TokenType::KwContinue},
    {"true", TokenType::KwTrue},
    {"false", TokenType::KwFalse},
    {"nil", TokenType::KwNil},
    {"and", TokenType::KwAnd},
    {"or", TokenType::KwOr},
    {"not", TokenType::KwNot},
};

bool IsDigit(char c) { return c >= '0' && c <= '9'; }

bool IsHexDigit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool IsIdentifierStart(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool IsIdentifierBody(char c) { return IsIdentifierStart(c) || IsDigit(c); }

int HexValue(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return c - 'A' + 10;
}

/* 把 Unicode 码点写入 UTF-8 字节串, 仅支持 BMP 范围 */
void AppendUtf8(std::string& out, uint32_t code) {
    if (code < 0x80u) {
        out.push_back(static_cast<char>(code));
    } else if (code < 0x800u) {
        out.push_back(static_cast<char>(0xC0u | (code >> 6)));
        out.push_back(static_cast<char>(0x80u | (code & 0x3Fu)));
    } else {
        out.push_back(static_cast<char>(0xE0u | (code >> 12)));
        out.push_back(static_cast<char>(0x80u | ((code >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (code & 0x3Fu)));
    }
}

}  /* namespace */

const char* TokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::EndOfFile: return "<文件结束>";
        case TokenType::Identifier: return "标识符";
        case TokenType::Number: return "数字";
        case TokenType::String: return "字符串";
        case TokenType::KwVar: return "var";
        case TokenType::KwConst: return "const";
        case TokenType::KwFn: return "fn";
        case TokenType::KwReturn: return "return";
        case TokenType::KwIf: return "if";
        case TokenType::KwElse: return "else";
        case TokenType::KwWhile: return "while";
        case TokenType::KwFor: return "for";
        case TokenType::KwBreak: return "break";
        case TokenType::KwContinue: return "continue";
        case TokenType::KwTrue: return "true";
        case TokenType::KwFalse: return "false";
        case TokenType::KwNil: return "nil";
        case TokenType::KwAnd: return "and";
        case TokenType::KwOr: return "or";
        case TokenType::KwNot: return "not";
        case TokenType::LParen: return "(";
        case TokenType::RParen: return ")";
        case TokenType::LBrace: return "{";
        case TokenType::RBrace: return "}";
        case TokenType::LBracket: return "[";
        case TokenType::RBracket: return "]";
        case TokenType::Comma: return ",";
        case TokenType::Semicolon: return ";";
        case TokenType::Dot: return ".";
        case TokenType::Colon: return ":";
        case TokenType::Assign: return "=";
        case TokenType::PlusAssign: return "+=";
        case TokenType::MinusAssign: return "-=";
        case TokenType::StarAssign: return "*=";
        case TokenType::SlashAssign: return "/=";
        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
        case TokenType::Star: return "*";
        case TokenType::Slash: return "/";
        case TokenType::Percent: return "%";
        case TokenType::Eq: return "==";
        case TokenType::Ne: return "!=";
        case TokenType::Lt: return "<";
        case TokenType::Le: return "<=";
        case TokenType::Gt: return ">";
        case TokenType::Ge: return ">=";
        case TokenType::AndAnd: return "&&";
        case TokenType::OrOr: return "||";
        case TokenType::Bang: return "!";
    }
    return "<未知>";
}

Lexer::Lexer(const std::string& source) : source_(source) {}

char Lexer::Peek(size_t offset) const {
    const size_t position = index_ + offset;
    if (position >= source_.size()) {
        return '\0';
    }
    return source_[position];
}

char Lexer::Advance() {
    if (index_ >= source_.size()) {
        return '\0';
    }
    const char c = source_[index_++];
    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return c;
}

bool Lexer::Match(char expected) {
    if (Peek(0) != expected) {
        return false;
    }
    Advance();
    return true;
}

/*
 * 跳过空白与注释. 行注释 //, 块注释 与 C 语言一致,
 * 块注释不嵌套, 未闭合时留到词法单元阶段报错.
 */
void Lexer::SkipTrivia() {
    for (;;) {
        const char c = Peek(0);
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            Advance();
            continue;
        }
        if (c == '/' && Peek(1) == '/') {
            while (Peek(0) != '\n' && Peek(0) != '\0') {
                Advance();
            }
            continue;
        }
        if (c == '/' && Peek(1) == '*') {
            Advance();
            Advance();
            while (Peek(0) != '\0' && !(Peek(0) == '*' && Peek(1) == '/')) {
                Advance();
            }
            if (Peek(0) == '*') {
                Advance();
                Advance();
            }
            continue;
        }
        break;
    }
}

bool Lexer::LexNumber(Token& token, Diagnostic& error) {
    const size_t start = index_;
    bool isFloat = false;

    if (Peek(0) == '0' && (Peek(1) == 'x' || Peek(1) == 'X')) {
        Advance();
        Advance();
        const size_t digitsStart = index_;
        while (IsHexDigit(Peek(0))) {
            Advance();
        }
        if (index_ == digitsStart) {
            error.line = token.line;
            error.column = token.column;
            error.message = "十六进制字面量缺少数字";
            return false;
        }
        const std::string text = source_.substr(digitsStart, index_ - digitsStart);
        unsigned long long value = 0;
        for (char c : text) {
            value = value * 16ull + static_cast<unsigned long long>(HexValue(c));
        }
        token.type = TokenType::Number;
        token.intValue = static_cast<int64_t>(value);
        token.isFloat = false;
        return true;
    }

    while (IsDigit(Peek(0))) {
        Advance();
    }
    if (Peek(0) == '.' && IsDigit(Peek(1))) {
        isFloat = true;
        Advance();
        while (IsDigit(Peek(0))) {
            Advance();
        }
    }
    if (Peek(0) == 'e' || Peek(0) == 'E') {
        const size_t save = index_;
        Advance();
        if (Peek(0) == '+' || Peek(0) == '-') {
            Advance();
        }
        if (IsDigit(Peek(0))) {
            isFloat = true;
            while (IsDigit(Peek(0))) {
                Advance();
            }
        } else {
            index_ = save;
        }
    }

    const std::string text = source_.substr(start, index_ - start);
    token.type = TokenType::Number;
    token.text = text;
    token.isFloat = isFloat;
    if (isFloat) {
        token.floatValue = std::strtod(text.c_str(), nullptr);
    } else {
        errno = 0;
        char* end = nullptr;
        const long long value = std::strtoll(text.c_str(), &end, 10);
        if (errno == ERANGE) {
            error.line = token.line;
            error.column = token.column;
            error.message = "整数字面量超出 int64 范围: " + text;
            return false;
        }
        token.intValue = static_cast<int64_t>(value);
        token.floatValue = static_cast<double>(token.intValue);
    }
    return true;
}

bool Lexer::LexString(Token& token, Diagnostic& error) {
    const char quote = Advance();
    std::string out;
    for (;;) {
        const char c = Peek(0);
        if (c == '\0') {
            error.line = token.line;
            error.column = token.column;
            error.message = "字符串字面量未闭合";
            return false;
        }
        Advance();
        if (c == quote) {
            break;
        }
        if (c == '\n') {
            error.line = token.line;
            error.column = token.column;
            error.message = "字符串字面量内不允许直接换行";
            return false;
        }
        if (c != '\\') {
            out.push_back(c);
            continue;
        }
        const char escape = Advance();
        switch (escape) {
            case 'n': out.push_back('\n'); break;
            case 't': out.push_back('\t'); break;
            case 'r': out.push_back('\r'); break;
            case '\0': out.push_back('\0'); break;
            case '\\': out.push_back('\\'); break;
            case '\'': out.push_back('\''); break;
            case '"': out.push_back('"'); break;
            case 'x': {
                uint32_t value = 0;
                for (int i = 0; i < 2; ++i) {
                    if (!IsHexDigit(Peek(0))) {
                        error.line = token.line;
                        error.column = token.column;
                        error.message = "\\x 转义需要两位十六进制数字";
                        return false;
                    }
                    value = value * 16u + static_cast<uint32_t>(HexValue(Advance()));
                }
                out.push_back(static_cast<char>(value & 0xFFu));
                break;
            }
            case 'u': {
                uint32_t value = 0;
                for (int i = 0; i < 4; ++i) {
                    if (!IsHexDigit(Peek(0))) {
                        error.line = token.line;
                        error.column = token.column;
                        error.message = "\\u 转义需要四位十六进制数字";
                        return false;
                    }
                    value = value * 16u + static_cast<uint32_t>(HexValue(Advance()));
                }
                AppendUtf8(out, value);
                break;
            }
            default:
                error.line = token.line;
                error.column = token.column;
                error.message = std::string("不支持的转义字符: \\") + escape;
                return false;
        }
    }
    token.type = TokenType::String;
    token.text = out;
    return true;
}

bool Lexer::LexIdentifierOrKeyword(Token& token) {
    const size_t start = index_;
    while (IsIdentifierBody(Peek(0))) {
        Advance();
    }
    token.text = source_.substr(start, index_ - start);
    token.type = TokenType::Identifier;
    for (const KeywordEntry& entry : kKeywords) {
        if (token.text == entry.name) {
            token.type = entry.type;
            break;
        }
    }
    return true;
}

bool Lexer::Tokenize(std::vector<Token>& out, Diagnostic& error) {
    out.clear();
    for (;;) {
        SkipTrivia();
        Token token;
        token.line = line_;
        token.column = column_;
        const char c = Peek(0);
        if (c == '\0') {
            token.type = TokenType::EndOfFile;
            out.push_back(std::move(token));
            return true;
        }
        if (IsDigit(c)) {
            if (!LexNumber(token, error)) {
                return false;
            }
            out.push_back(std::move(token));
            continue;
        }
        if (IsIdentifierStart(c)) {
            LexIdentifierOrKeyword(token);
            out.push_back(std::move(token));
            continue;
        }
        if (c == '"' || c == '\'') {
            if (!LexString(token, error)) {
                return false;
            }
            out.push_back(std::move(token));
            continue;
        }

        Advance();
        switch (c) {
            case '(': token.type = TokenType::LParen; break;
            case ')': token.type = TokenType::RParen; break;
            case '{': token.type = TokenType::LBrace; break;
            case '}': token.type = TokenType::RBrace; break;
            case '[': token.type = TokenType::LBracket; break;
            case ']': token.type = TokenType::RBracket; break;
            case ',': token.type = TokenType::Comma; break;
            case ';': token.type = TokenType::Semicolon; break;
            case '.': token.type = TokenType::Dot; break;
            case ':': token.type = TokenType::Colon; break;
            case '+':
                token.type = Match('=') ? TokenType::PlusAssign : TokenType::Plus;
                break;
            case '-':
                token.type = Match('=') ? TokenType::MinusAssign : TokenType::Minus;
                break;
            case '*':
                token.type = Match('=') ? TokenType::StarAssign : TokenType::Star;
                break;
            case '%': token.type = TokenType::Percent; break;
            case '/':
                token.type = Match('=') ? TokenType::SlashAssign : TokenType::Slash;
                break;
            case '=':
                token.type = Match('=') ? TokenType::Eq : TokenType::Assign;
                break;
            case '!':
                token.type = Match('=') ? TokenType::Ne : TokenType::Bang;
                break;
            case '<':
                token.type = Match('=') ? TokenType::Le : TokenType::Lt;
                break;
            case '>':
                token.type = Match('=') ? TokenType::Ge : TokenType::Gt;
                break;
            case '&':
                if (!Match('&')) {
                    error.line = token.line;
                    error.column = token.column;
                    error.message = "单个 & 不是合法运算符, 逻辑与请使用 && 或 and";
                    return false;
                }
                token.type = TokenType::AndAnd;
                break;
            case '|':
                if (!Match('|')) {
                    error.line = token.line;
                    error.column = token.column;
                    error.message = "单个 | 不是合法运算符, 逻辑或请使用 || 或 or";
                    return false;
                }
                token.type = TokenType::OrOr;
                break;
            default:
                error.line = token.line;
                error.column = token.column;
                error.message = std::string("无法识别的字符: ") + c;
                return false;
        }
        out.push_back(std::move(token));
    }
}

}  /* namespace immirror::script */
