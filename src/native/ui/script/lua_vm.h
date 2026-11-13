#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ui/script/lua_value.h"

namespace immirror::ui::script {

/*
 * 界面脚本求值器.
 *
 * 在解析结果上执行: 表达式求值与语句执行, 作用域按块创建, 函数按闭包调用.
 * 宿主能力通过全局函数注册进入脚本, 因此界面控件动作不需要写死在求值器里.
 */
class Interpreter {
public:
    /* 建好全局作用域并注册基础库. */
    Interpreter();

    /* 执行一段脚本; 失败返回 false 并给出带行号的错误. */
    bool Run(const char* source, size_t length);

    /* 注册一个宿主函数到全局作用域. */
    void RegisterNative(const std::string& name, Closure::NativeFn fn);

    /* 给全局变量赋值 / 取值. */
    void SetGlobal(const std::string& name, const Value& value);
    Value GetGlobal(const std::string& name) const;

    /* 调用全局函数, 返回第一个结果. */
    Value CallGlobal(const std::string& name, const std::vector<Value>& args);

    /* 全局作用域里定义的函数名, 供按名字批量调用入口函数. */
    std::vector<std::string> GlobalFunctionNames() const;

    const std::string& Error() const { return error_; }
    /* 已执行的语句条数, 用于统计与限流. */
    int64_t StepCount() const { return steps_; }

private:
    /* 语句执行: 返回 return 语句的值, returned 标记是否提前返回. */
    std::vector<Value> ExecBlock(const std::vector<NodePtr>& block, std::shared_ptr<Scope> scope,
                                 bool& returned);
    std::vector<Value> ExecStmt(const Node& node, std::shared_ptr<Scope> scope, bool& returned);

    Value Eval(const Node& node, std::shared_ptr<Scope> scope);
    std::vector<Value> EvalList(const std::vector<NodePtr>& list, std::shared_ptr<Scope> scope);
    void AssignTo(const Node& target, const Value& value, std::shared_ptr<Scope> scope);
    std::vector<Value> Call(const Value& callee, const std::vector<Value>& args);

    /* 统一的错误出口: 带行号. */
    bool Fail(const char* message);

    std::shared_ptr<Scope> global_;
    std::string error_;
    int64_t steps_ = 0;
    /* 递归深度与总步数上限, 防止脚本死循环把界面卡住. */
    int32_t depth_ = 0;
    int64_t stepLimit_ = 2000000;
};

}  /* namespace immirror::ui::script */
