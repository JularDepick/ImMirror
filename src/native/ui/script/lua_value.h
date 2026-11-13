#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ui/script/lua_parser.h"

namespace immirror::ui::script {

class Table;
class Closure;
class Scope;
struct Value;

/*
 * 脚本值: 界面脚本用到的六种.
 * 数值统一双精度, 字符串与表用共享指针, 函数值指向一段已解析的函数体.
 */
struct Value {
    enum class Kind : uint8_t { kNil, kBool, kNumber, kString, kTable, kFunction };

    Kind kind = Kind::kNil;
    bool boolean = false;
    double number = 0.0;
    std::string text;
    std::shared_ptr<Table> table;
    std::shared_ptr<Closure> closure;

    static Value Nil();
    static Value Bool(bool value);
    static Value Number(double value);
    static Value Str(const std::string& value);
    static Value Of(std::shared_ptr<Table> value);
    static Value Of(std::shared_ptr<Closure> value);

    bool IsNil() const { return kind == Kind::kNil; }
    bool Truthy() const { return !(kind == Kind::kNil || (kind == Kind::kBool && !boolean)); }
    std::string ToText() const;
};

/*
 * 表: 字符串键与数值键分开存.
 * 界面脚本同时使用 {"name"=...} 与 {1,2,3} 两种写法, 混在一个容器里会让
 * 顺序遍历与长度计算都变复杂, 因此分成两张表.
 */
class Table {
public:
    Value Get(const Value& key) const;
    void Set(const Value& key, const Value& value);

    /* 连续数值键的长度, 从 1 数到第一个空洞. */
    int32_t Length() const;

    const std::map<std::string, Value>& Fields() const { return fields_; }
    const std::vector<Value>& Items() const { return items_; }

private:
    std::map<std::string, Value> fields_;
    /* 下标从 0 开始存, 对应脚本里的 1..n. */
    std::vector<Value> items_;
};

/*
 * 闭包: 函数体语法树, 形参表与定义处的作用域.
 * native 非空时表示宿主函数, 不再执行语法树.
 */
class Closure {
public:
    using NativeFn = std::function<std::vector<Value>(const std::vector<Value>&)>;

    /* 函数体语句列表: 解析结果里函数体就是一组语句. */
    std::vector<NodePtr> body;
    std::vector<std::string> params;
    std::shared_ptr<Scope> scope;
    NativeFn native;
};

/* 作用域链: 每个块一层, 变量按名字查找. */
class Scope {
public:
    explicit Scope(std::shared_ptr<Scope> parent) : parent_(std::move(parent)) {}

    Value Get(const std::string& name) const;
    void Define(const std::string& name, const Value& value);
    /* 赋值: 找到已有的那层就改它, 都没有则写在当前层. */
    void Assign(const std::string& name, const Value& value);

    /* 当前层定义的变量名, 供调试与按名字批量调用使用. */
    std::vector<std::string> Names() const {
        std::vector<std::string> out;
        out.reserve(values_.size());
        for (const auto& entry : values_) {
            out.push_back(entry.first);
        }
        return out;
    }

private:
    std::shared_ptr<Scope> parent_;
    std::map<std::string, Value> values_;
};

}  /* namespace immirror::ui::script */
