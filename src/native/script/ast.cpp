#include "ast.h"

namespace immirror::script::ast {

namespace {

ExprPtr CloneExprImpl(const Expr& expr) {
    ExprPtr copy(new Expr());
    copy->kind = expr.kind;
    copy->line = expr.line;
    copy->boolValue = expr.boolValue;
    copy->intValue = expr.intValue;
    copy->floatValue = expr.floatValue;
    copy->stringValue = expr.stringValue;
    copy->unaryOp = expr.unaryOp;
    copy->binaryOp = expr.binaryOp;
    if (expr.a) {
        copy->a = CloneExprImpl(*expr.a);
    }
    if (expr.b) {
        copy->b = CloneExprImpl(*expr.b);
    }
    for (const ExprPtr& arg : expr.args) {
        copy->args.push_back(arg ? CloneExprImpl(*arg) : ExprPtr());
    }
    for (const TableField& field : expr.fields) {
        TableField cloned;
        cloned.kind = field.kind;
        cloned.name = field.name;
        if (field.key) {
            cloned.key = CloneExprImpl(*field.key);
        }
        if (field.value) {
            cloned.value = CloneExprImpl(*field.value);
        }
        copy->fields.push_back(std::move(cloned));
    }
    if (expr.func) {
        copy->func.reset(new FunctionDef());
        copy->func->name = expr.func->name;
        copy->func->params = expr.func->params;
        copy->func->line = expr.func->line;
        for (const StmtPtr& statement : expr.func->body) {
            copy->func->body.push_back(statement ? CloneStmt(*statement) : StmtPtr());
        }
    }
    return copy;
}

}  /* namespace */

ExprPtr CloneExpr(const Expr& expr) { return CloneExprImpl(expr); }

StmtPtr CloneStmt(const Stmt& stmt) {
    StmtPtr copy(new Stmt());
    copy->kind = stmt.kind;
    copy->line = stmt.line;
    copy->isConst = stmt.isConst;
    copy->names = stmt.names;
    for (const ExprPtr& expr : stmt.exprs) {
        copy->exprs.push_back(expr ? CloneExprImpl(*expr) : ExprPtr());
    }
    for (const ExprPtr& target : stmt.targets) {
        copy->targets.push_back(target ? CloneExprImpl(*target) : ExprPtr());
    }
    if (stmt.func) {
        copy->func.reset(new FunctionDef());
        copy->func->name = stmt.func->name;
        copy->func->params = stmt.func->params;
        copy->func->line = stmt.func->line;
        for (const StmtPtr& statement : stmt.func->body) {
            copy->func->body.push_back(statement ? CloneStmt(*statement) : StmtPtr());
        }
    }
    for (const IfBranch& branch : stmt.branches) {
        IfBranch cloned;
        if (branch.cond) {
            cloned.cond = CloneExprImpl(*branch.cond);
        }
        for (const StmtPtr& statement : branch.body) {
            cloned.body.push_back(statement ? CloneStmt(*statement) : StmtPtr());
        }
        copy->branches.push_back(std::move(cloned));
    }
    for (const StmtPtr& statement : stmt.elseBody) {
        copy->elseBody.push_back(statement ? CloneStmt(*statement) : StmtPtr());
    }
    if (stmt.cond) {
        copy->cond = CloneExprImpl(*stmt.cond);
    }
    for (const StmtPtr& statement : stmt.body) {
        copy->body.push_back(statement ? CloneStmt(*statement) : StmtPtr());
    }
    if (stmt.forInit) {
        copy->forInit = CloneStmt(*stmt.forInit);
    }
    if (stmt.forCond) {
        copy->forCond = CloneExprImpl(*stmt.forCond);
    }
    if (stmt.forStep) {
        copy->forStep = CloneStmt(*stmt.forStep);
    }
    return copy;
}

}  /* namespace immirror::script::ast */
