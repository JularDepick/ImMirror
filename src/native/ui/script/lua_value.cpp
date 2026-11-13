#include "ui/script/lua_value.h"

#include <cmath>
#include <cstdio>

namespace immirror::ui::script {

Value Value::Nil() {
    return Value();
}

Value Value::Bool(bool value) {
    Value out;
    out.kind = Kind::kBool;
    out.boolean = value;
    return out;
}

Value Value::Number(double value) {
    Value out;
    out.kind = Kind::kNumber;
    out.number = value;
    return out;
}

Value Value::Str(const std::string& value) {
    Value out;
    out.kind = Kind::kString;
    out.text = value;
    return out;
}

Value Value::Of(std::shared_ptr<Table> value) {
    Value out;
    if (value != nullptr) {
        out.kind = Kind::kTable;
        out.table = std::move(value);
    }
    return out;
}

Value Value::Of(std::shared_ptr<Closure> value) {
    Value out;
    if (value != nullptr) {
        out.kind = Kind::kFunction;
        out.closure = std::move(value);
    }
    return out;
}

std::string Value::ToText() const {
    switch (kind) {
        case Kind::kNil:
            return "nil";
        case Kind::kBool:
            return boolean ? "true" : "false";
        case Kind::kNumber: {
            /* 整数值不带小数部分, 其余按六位有效数字. */
            if (std::fabs(number - std::floor(number)) < 1.0e-9 && std::fabs(number) < 1.0e15) {
                char buffer[32];
                std::snprintf(buffer, sizeof(buffer), "%lld",
                              static_cast<long long>(std::llround(number)));
                return buffer;
            }
            char buffer[40];
            std::snprintf(buffer, sizeof(buffer), "%g", number);
            return buffer;
        }
        case Kind::kString:
            return text;
        case Kind::kTable:
            return "table";
        case Kind::kFunction:
            return "function";
    }
    return "nil";
}

namespace {

/* 数值键只支持正整数, 非整数按字符串键处理. */
bool KeyIndex(const Value& key, int32_t& out) {
    if (key.kind != Value::Kind::kNumber) {
        return false;
    }
    const double rounded = std::floor(key.number);
    if (std::fabs(key.number - rounded) > 1.0e-9 || rounded < 1.0 || rounded > 1.0e6) {
        return false;
    }
    out = static_cast<int32_t>(rounded);
    return true;
}

}  /* namespace */

Value Table::Get(const Value& key) const {
    int32_t index = 0;
    if (KeyIndex(key, index)) {
        const size_t position = static_cast<size_t>(index - 1);
        return position < items_.size() ? items_[position] : Value();
    }
    const std::string name = key.kind == Value::Kind::kString ? key.text : key.ToText();
    const auto found = fields_.find(name);
    return found == fields_.end() ? Value() : found->second;
}

void Table::Set(const Value& key, const Value& value) {
    int32_t index = 0;
    if (KeyIndex(key, index)) {
        const size_t position = static_cast<size_t>(index - 1);
        if (items_.size() <= position) {
            items_.resize(position + 1);
        }
        items_[position] = value;
        return;
    }
    fields_[key.kind == Value::Kind::kString ? key.text : key.ToText()] = value;
}

int32_t Table::Length() const {
    int32_t count = 0;
    for (const Value& item : items_) {
        if (item.IsNil()) {
            break;
        }
        count += 1;
    }
    return count;
}

Value Scope::Get(const std::string& name) const {
    for (const Scope* scope = this; scope != nullptr; scope = scope->parent_.get()) {
        const auto found = scope->values_.find(name);
        if (found != scope->values_.end()) {
            return found->second;
        }
    }
    return Value();
}

void Scope::Define(const std::string& name, const Value& value) {
    values_[name] = value;
}

void Scope::Assign(const std::string& name, const Value& value) {
    /*
     * 先在作用域链里找已声明的变量; 都没找到时写到最外层 (全局).
     * 写成"落到当前块"会让块内首次赋值的名字在块结束时消失,
     * 与脚本语言里"未声明赋值即全局"的语义不符.
     */
    Scope* outermost = this;
    for (Scope* scope = this; scope != nullptr; scope = scope->parent_.get()) {
        const auto found = scope->values_.find(name);
        if (found != scope->values_.end()) {
            found->second = value;
            return;
        }
        outermost = scope;
    }
    outermost->values_[name] = value;
}

}  /* namespace immirror::ui::script */
