#pragma once

#include <string>
#include <vector>

#include "ast.h"
#include "common.h"
#include "lexer.h"

namespace immirror::script {

/*
 * 递归下降语法分析器. 表达式优先级从低到高依次为
 * 逻辑或, 逻辑与, 相等, 关系, 加减, 乘除模, 一元, 后缀.
 * 语句层采用恐慌模式恢复, 单条语句出错不影响后续语句诊断.
 */
class Parser {
public:
    Parser(std::vector<Token> tokens, std::string chunkName);

    /* 解析整个模块, errors 非空时表示失败 */
    bool ParseModule(ast::Module& module, std::vector<Diagnostic>& errors);

private:
    const Token& Peek(size_t offset = 0) const;
    const Token& Current() const;
    bool Check(TokenType type) const;
    bool Match(TokenType type);
    bool Expect(TokenType type, const char* what);
    /* 读取标识符并前进, 名字通过 out 返回 */
    bool ExpectIdentifier(std::string& out, const char* what);
    void Advance();
    void ReportError(uint32_t line, uint32_t column, const std::string& message);
    void Synchronize();

    bool ParseStatement(ast::StmtPtr& out);
    bool ParseBlock(std::vector<ast::StmtPtr>& out);
    bool ParseVarDecl(ast::StmtPtr& out, bool expectSemicolon);
    bool ParseFunctionDecl(ast::StmtPtr& out);
    bool ParseIf(ast::StmtPtr& out);
    bool ParseWhile(ast::StmtPtr& out);
    bool ParseFor(ast::StmtPtr& out);
    bool ParseReturn(ast::StmtPtr& out);
    bool ParseExprStatement(ast::StmtPtr& out, bool expectSemicolon);

    bool ParseExpressionList(std::vector<ast::ExprPtr>& out);
    bool ParseExpression(ast::ExprPtr& out);
    bool ParseOr(ast::ExprPtr& out);
    bool ParseAnd(ast::ExprPtr& out);
    bool ParseEquality(ast::ExprPtr& out);
    bool ParseRelational(ast::ExprPtr& out);
    bool ParseAdditive(ast::ExprPtr& out);
    bool ParseMultiplicative(ast::ExprPtr& out);
    bool ParseUnary(ast::ExprPtr& out);
    bool ParsePostfix(ast::ExprPtr& out);
    bool ParsePrimary(ast::ExprPtr& out);
    bool ParseTable(ast::ExprPtr& out);
    bool ParseFunctionExpression(ast::ExprPtr& out);

    std::string chunkName_;
    std::vector<Token> tokens_;
    size_t index_ = 0;
    std::vector<Diagnostic> errors_;
};

}  /* namespace immirror::script */
