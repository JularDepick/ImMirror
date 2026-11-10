#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "common.h"

namespace immirror::script {

/* 词法单元类型. 关键字与运算符各自独立, 语法分析器直接按类型匹配 */
enum class TokenType : uint8_t {
    EndOfFile = 0,
    Identifier,
    Number,
    String,

    KwVar,
    KwConst,
    KwFn,
    KwReturn,
    KwIf,
    KwElse,
    KwWhile,
    KwFor,
    KwBreak,
    KwContinue,
    KwTrue,
    KwFalse,
    KwNil,
    KwAnd,
    KwOr,
    KwNot,

    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Comma,
    Semicolon,
    Dot,
    Colon,

    Assign,
    PlusAssign,
    MinusAssign,
    StarAssign,
    SlashAssign,
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    AndAnd,
    OrOr,
    Bang,
};

/* 词法单元名称, 用于错误提示 */
const char* TokenTypeName(TokenType type);

/* 单个词法单元, 数字同时保留整数与浮点解释 */
struct Token {
    TokenType type = TokenType::EndOfFile;
    uint32_t line = 1;
    uint32_t column = 1;
    std::string text;
    int64_t intValue = 0;
    double floatValue = 0.0;
    bool isFloat = false;
};

/*
 * 词法分析器. 一次性把整段源码切成词法单元序列,
 * 失败时通过 error 输出首个错误位置, 不使用异常.
 */
class Lexer {
public:
    explicit Lexer(const std::string& source);

    bool Tokenize(std::vector<Token>& out, Diagnostic& error);

private:
    char Peek(size_t offset) const;
    char Advance();
    bool Match(char expected);
    void SkipTrivia();
    bool LexNumber(Token& token, Diagnostic& error);
    bool LexString(Token& token, Diagnostic& error);
    bool LexIdentifierOrKeyword(Token& token);

    /* 按值保存源码: 调用方常以字面量构造临时 std::string, 引用会悬垂 */
    std::string source_;
    size_t index_ = 0;
    uint32_t line_ = 1;
    uint32_t column_ = 1;
};

}  /* namespace immirror::script */
