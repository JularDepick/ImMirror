#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace immirror::script::ast {

/* 一元运算符 */
enum class UnaryOp : uint8_t {
    Neg = 0,
    Not = 1,
};

/* 二元运算符, 优先级由语法分析器保证, 此处只记录语义 */
enum class BinaryOp : uint8_t {
    Add = 0,
    Sub,
    Mul,
    Div,
    Mod,
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    And,
    Or,
};

enum class ExprKind : uint8_t {
    Nil = 0,
    Bool,
    Int,
    Float,
    String,
    Name,
    Unary,
    Binary,
    Index,     /* a[b] */
    Field,     /* a.b */
    Call,
    Table,
    Function,  /* 匿名函数表达式 */
};

enum class StmtKind : uint8_t {
    Expr = 0,       /* 表达式语句 */
    VarDecl,        /* var 声明, 支持多变量与多初始化表达式 */
    Assign,         /* 赋值语句, 支持多目标 */
    FunctionDecl,   /* fn 具名函数声明 */
    If,
    While,
    For,            /* C 风格数值循环 */
    Break,
    Continue,
    Return,
    Block,          /* 独立花括号块 */
};

struct Expr;
struct Stmt;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

/*
 * 函数定义. 具名声明与匿名表达式共用该结构,
 * name 为空时表示匿名函数.
 */
struct FunctionDef {
    std::string name;
    std::vector<std::string> params;
    std::vector<StmtPtr> body;
    uint32_t line = 0;
};

/* 表构造中的单个字段 */
struct TableField {
    enum class Kind : uint8_t {
        Positional = 0,  /* { 1, 2, 3 } */
        Named,           /* { x = 1 } */
        Computed,        /* { ["k"] = 1 } 或 { [expr] = 1 } */
    };
    Kind kind = Kind::Positional;
    std::string name;
    ExprPtr key;
    ExprPtr value;
};

/*
 * 表达式节点. 为控制代码体量采用扁平结构,
 * 未使用的成员保持默认值.
 */
struct Expr {
    ExprKind kind = ExprKind::Nil;
    uint32_t line = 0;

    bool boolValue = false;
    int64_t intValue = 0;
    double floatValue = 0.0;
    std::string stringValue;  /* 字符串字面量或标识符 */

    UnaryOp unaryOp = UnaryOp::Neg;
    BinaryOp binaryOp = BinaryOp::Add;

    ExprPtr a;
    ExprPtr b;

    std::vector<ExprPtr> args;        /* Call 实参 */
    std::vector<TableField> fields;   /* Table 字段 */
    std::unique_ptr<FunctionDef> func;
};

/* if / else if 的单个分支 */
struct IfBranch {
    ExprPtr cond;
    std::vector<StmtPtr> body;
};

/* 语句节点, 同样采用扁平结构 */
struct Stmt {
    StmtKind kind = StmtKind::Expr;
    uint32_t line = 0;

    std::vector<ExprPtr> exprs;      /* 表达式语句, return 值, 赋值右值 */
    std::vector<ExprPtr> targets;    /* 赋值左值 */
    std::vector<std::string> names;  /* var 声明的名字列表 */
    bool isConst = false;

    std::unique_ptr<FunctionDef> func;

    std::vector<IfBranch> branches;
    std::vector<StmtPtr> elseBody;

    ExprPtr cond;
    std::vector<StmtPtr> body;

    StmtPtr forInit;
    ExprPtr forCond;
    StmtPtr forStep;
};

/* 语法分析结果, errors 非空时 statements 不可用 */
struct Module {
    std::vector<StmtPtr> statements;
    bool ok = false;
};

/*
 * 深拷贝表达式树. 复合赋值需要把左值读引用复制一份,
 * 节点内部持有 unique_ptr, 不能依赖默认拷贝构造.
 */
ExprPtr CloneExpr(const Expr& expr);

/* 深拷贝语句节点, 供语法层改写使用 */
StmtPtr CloneStmt(const Stmt& stmt);

}  /* namespace immirror::script::ast */
