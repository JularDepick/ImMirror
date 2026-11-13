#include "ui/script/lua_vm.h"

#include <cmath>
#include <cstdio>

namespace immirror::ui::script {
namespace {

constexpr int32_t kMaxDepth = 64;
constexpr int64_t kMaxLoop = 200000;

double ToNumber(const Value& value) {
    if (value.kind == Value::Kind::kNumber) {
        return value.number;
    }
    if (value.kind == Value::Kind::kString) {
        return std::atof(value.text.c_str());
    }
    if (value.kind == Value::Kind::kBool) {
        return value.boolean ? 1.0 : 0.0;
    }
    return 0.0;
}

bool Equal(const Value& left, const Value& right) {
    if (left.kind != right.kind) {
        return false;
    }
    switch (left.kind) {
        case Value::Kind::kNil:
            return true;
        case Value::Kind::kBool:
            return left.boolean == right.boolean;
        case Value::Kind::kNumber:
            return left.number == right.number;
        case Value::Kind::kString:
            return left.text == right.text;
        case Value::Kind::kTable:
            return left.table == right.table;
        case Value::Kind::kFunction:
            return left.closure == right.closure;
    }
    return false;
}

}  /* namespace */

Interpreter::Interpreter() {
    global_ = std::make_shared<Scope>(nullptr);
    /* 基础库: 只放界面脚本实际用到的几个. */
    RegisterNative("print", [](const std::vector<Value>& args) {
        std::string line;
        for (size_t index = 0; index < args.size(); ++index) {
            if (index > 0) {
                line += "\t";
            }
            line += args[index].ToText();
        }
        std::printf("%s\n", line.c_str());
        return std::vector<Value>();
    });
    RegisterNative("tostring", [](const std::vector<Value>& args) {
        return std::vector<Value>{Value::Str(args.empty() ? "nil" : args[0].ToText())};
    });
    RegisterNative("tonumber", [](const std::vector<Value>& args) {
        return std::vector<Value>{Value::Number(args.empty() ? 0.0 : ToNumber(args[0]))};
    });
    RegisterNative("type", [](const std::vector<Value>& args) {
        static const char* const kNames[] = {"nil", "boolean", "number", "string", "table",
                                             "function"};
        const int32_t index = args.empty() ? 0 : static_cast<int32_t>(args[0].kind);
        return std::vector<Value>{Value::Str(kNames[index < 0 || index > 5 ? 0 : index])};
    });
}

bool Interpreter::Fail(const char* message) {
    error_ = message;
    return false;
}

bool Interpreter::Run(const char* source, size_t length) {
    error_.clear();
    steps_ = 0;
    depth_ = 0;
    ParseStats stats;
    std::string parseError;
    const std::vector<NodePtr> program = ParseScript(source, length, &stats, &parseError);
    if (program.empty() && !parseError.empty()) {
        error_ = parseError;
        return false;
    }
    bool returned = false;
    ExecBlock(program, global_, returned);
    return error_.empty();
}

void Interpreter::RegisterNative(const std::string& name, Closure::NativeFn fn) {
    std::shared_ptr<Closure> closure(new Closure());
    closure->native = std::move(fn);
    global_->Define(name, Value::Of(closure));
}

void Interpreter::SetGlobal(const std::string& name, const Value& value) {
    global_->Define(name, value);
}

Value Interpreter::GetGlobal(const std::string& name) const {
    return global_->Get(name);
}

std::vector<std::string> Interpreter::GlobalFunctionNames() const {
    std::vector<std::string> out;
    for (const std::string& name : global_->Names()) {
        if (global_->Get(name).kind == Value::Kind::kFunction) {
            out.push_back(name);
        }
    }
    return out;
}

Value Interpreter::CallGlobal(const std::string& name, const std::vector<Value>& args) {
    const Value callee = global_->Get(name);
    if (callee.kind != Value::Kind::kFunction) {
        return Value();
    }
    const std::vector<Value> results = Call(callee, args);
    return results.empty() ? Value() : results[0];
}

std::vector<Value> Interpreter::EvalList(const std::vector<NodePtr>& list,
                                         std::shared_ptr<Scope> scope) {
    std::vector<Value> out;
    out.reserve(list.size());
    for (const NodePtr& node : list) {
        if (node == nullptr) {
            out.push_back(Value());
            continue;
        }
        out.push_back(Eval(*node, scope));
    }
    return out;
}

void Interpreter::AssignTo(const Node& target, const Value& value,
                           std::shared_ptr<Scope> scope) {
    if (target.kind == Node::Kind::kName) {
        scope->Assign(target.text, value);
        return;
    }
    if (target.kind == Node::Kind::kIndex && target.left != nullptr && target.right != nullptr) {
        const Value container = Eval(*target.left, scope);
        if (container.kind == Value::Kind::kTable && container.table != nullptr) {
            container.table->Set(Eval(*target.right, scope), value);
        }
    }
}

std::vector<Value> Interpreter::ExecBlock(const std::vector<NodePtr>& block,
                                          std::shared_ptr<Scope> scope, bool& returned) {
    for (const NodePtr& node : block) {
        if (node == nullptr) {
            continue;
        }
        std::vector<Value> results = ExecStmt(*node, scope, returned);
        if (returned || !error_.empty()) {
            return results;
        }
    }
    return std::vector<Value>();
}

std::vector<Value> Interpreter::ExecStmt(const Node& node, std::shared_ptr<Scope> scope,
                                         bool& returned) {
    steps_ += 1;
    if (steps_ > stepLimit_) {
        Fail("脚本执行步数超过上限");
        return std::vector<Value>();
    }
    switch (node.kind) {
        case Node::Kind::kLocal: {
            const std::vector<Value> values = EvalList(node.list, scope);
            for (size_t index = 0; index < node.names.size(); ++index) {
                scope->Define(node.names[index], index < values.size() ? values[index] : Value());
            }
            return std::vector<Value>();
        }
        case Node::Kind::kAssign: {
            /* 右值存在 blocks[0], 左值在 list. */
            const std::vector<Value> values =
                node.blocks.empty() ? std::vector<Value>() : EvalList(node.blocks[0], scope);
            for (size_t index = 0; index < node.list.size(); ++index) {
                if (node.list[index] == nullptr) {
                    continue;
                }
                AssignTo(*node.list[index], index < values.size() ? values[index] : Value(), scope);
            }
            return std::vector<Value>();
        }
        case Node::Kind::kCallStmt:
            EvalList(node.list, scope);
            return std::vector<Value>();
        case Node::Kind::kFunction: {
            std::shared_ptr<Closure> closure(new Closure());
            closure->params = node.names;
            closure->scope = scope;
            closure->body = node.blocks.empty() ? std::vector<NodePtr>() : node.blocks[0];
            const Value value = Value::Of(closure);
            if (node.left != nullptr) {
                AssignTo(*node.left, value, scope);
            }
            if (!node.text.empty()) {
                scope->Define(node.text, value);
            }
            return std::vector<Value>();
        }
        case Node::Kind::kIf: {
            for (size_t index = 0; index < node.blocks.size(); ++index) {
                const bool hasCondition = index < node.list.size() && node.list[index] != nullptr;
                if (hasCondition && !Eval(*node.list[index], scope).Truthy()) {
                    continue;
                }
                auto inner = std::make_shared<Scope>(scope);
                return ExecBlock(node.blocks[index], inner, returned);
            }
            return std::vector<Value>();
        }
        case Node::Kind::kWhile: {
            if (node.list.empty() || node.blocks.empty()) {
                return std::vector<Value>();
            }
            const bool isRepeat = node.op == "repeat";
            int64_t iterations = 0;
            while (true) {
                if (!isRepeat && !Eval(*node.list[0], scope).Truthy()) {
                    break;
                }
                if (++iterations > kMaxLoop) {
                    Fail("循环次数超过上限");
                    return std::vector<Value>();
                }
                auto inner = std::make_shared<Scope>(scope);
                bool innerReturned = false;
                std::vector<Value> results = ExecBlock(node.blocks[0], inner, innerReturned);
                if (innerReturned) {
                    returned = true;
                    return results;
                }
                if (!error_.empty()) {
                    return std::vector<Value>();
                }
                /* repeat 的条件在体执行之后判断, 为真则退出. */
                if (isRepeat && Eval(*node.list[0], scope).Truthy()) {
                    break;
                }
            }
            return std::vector<Value>();
        }
        case Node::Kind::kNumericFor: {
            if (node.list.size() < 2 || node.blocks.empty() || node.names.empty()) {
                return std::vector<Value>();
            }
            const double from = ToNumber(Eval(*node.list[0], scope));
            const double to = ToNumber(Eval(*node.list[1], scope));
            const double step = node.list.size() > 2 ? ToNumber(Eval(*node.list[2], scope)) : 1.0;
            if (step == 0.0) {
                Fail("for 步长不能为零");
                return std::vector<Value>();
            }
            int64_t iterations = 0;
            for (double value = from; (step > 0.0 ? value <= to : value >= to); value += step) {
                if (++iterations > kMaxLoop) {
                    Fail("循环次数超过上限");
                    return std::vector<Value>();
                }
                auto inner = std::make_shared<Scope>(scope);
                inner->Define(node.names[0], Value::Number(value));
                bool innerReturned = false;
                std::vector<Value> results = ExecBlock(node.blocks[0], inner, innerReturned);
                if (innerReturned) {
                    returned = true;
                    return results;
                }
                if (!error_.empty()) {
                    return std::vector<Value>();
                }
            }
            return std::vector<Value>();
        }
        case Node::Kind::kDo: {
            /* 泛型 for 与 do 块共用该形态; 泛型 for 暂按 do 块执行一次. */
            if (node.blocks.empty()) {
                return std::vector<Value>();
            }
            auto inner = std::make_shared<Scope>(scope);
            return ExecBlock(node.blocks[0], inner, returned);
        }
        case Node::Kind::kReturn:
            returned = true;
            return EvalList(node.list, scope);
        case Node::Kind::kBreak:
            returned = true;
            return std::vector<Value>();
        default:
            break;
    }
    return std::vector<Value>();
}

Value Interpreter::Eval(const Node& node, std::shared_ptr<Scope> scope) {
    switch (node.kind) {
        case Node::Kind::kNil:
            return Value();
        case Node::Kind::kBool:
            return Value::Bool(node.boolean);
        case Node::Kind::kNumber:
            return Value::Number(node.number);
        case Node::Kind::kString:
            return Value::Str(node.text);
        case Node::Kind::kName:
            return scope->Get(node.text);
        case Node::Kind::kIndex: {
            const Value container = Eval(*node.left, scope);
            if (container.kind != Value::Kind::kTable || container.table == nullptr) {
                return Value();
            }
            return container.table->Get(Eval(*node.right, scope));
        }
        case Node::Kind::kCall: {
            const Value callee = Eval(*node.left, scope);
            const std::vector<Value> results = Call(callee, EvalList(node.list, scope));
            return results.empty() ? Value() : results[0];
        }
        case Node::Kind::kMethodCall: {
            const Value container = Eval(*node.left, scope);
            std::vector<Value> args;
            args.push_back(container);
            const std::vector<Value> extra = EvalList(node.list, scope);
            args.insert(args.end(), extra.begin(), extra.end());
            if (container.kind == Value::Kind::kTable && container.table != nullptr) {
                const Value method = container.table->Get(Value::Str(node.text));
                if (method.kind == Value::Kind::kFunction) {
                    const std::vector<Value> results = Call(method, args);
                    return results.empty() ? Value() : results[0];
                }
            }
            return Value();
        }
        case Node::Kind::kUnary: {
            const Value operand = Eval(*node.left, scope);
            if (node.op == "not") {
                return Value::Bool(!operand.Truthy());
            }
            if (node.op == "-") {
                return Value::Number(-ToNumber(operand));
            }
            if (node.op == "#") {
                if (operand.kind == Value::Kind::kString) {
                    return Value::Number(static_cast<double>(operand.text.size()));
                }
                if (operand.kind == Value::Kind::kTable && operand.table != nullptr) {
                    return Value::Number(static_cast<double>(operand.table->Length()));
                }
                return Value::Number(0.0);
            }
            return Value();
        }
        case Node::Kind::kBinary: {
            if (node.op == "and") {
                const Value left = Eval(*node.left, scope);
                return left.Truthy() ? Eval(*node.right, scope) : left;
            }
            if (node.op == "or") {
                const Value left = Eval(*node.left, scope);
                return left.Truthy() ? left : Eval(*node.right, scope);
            }
            const Value left = Eval(*node.left, scope);
            const Value right = Eval(*node.right, scope);
            if (node.op == "==") {
                return Value::Bool(Equal(left, right));
            }
            if (node.op == "~=") {
                return Value::Bool(!Equal(left, right));
            }
            if (node.op == "..") {
                return Value::Str(left.ToText() + right.ToText());
            }
            const double a = ToNumber(left);
            const double b = ToNumber(right);
            if (node.op == "+") {
                return Value::Number(a + b);
            }
            if (node.op == "-") {
                return Value::Number(a - b);
            }
            if (node.op == "*") {
                return Value::Number(a * b);
            }
            if (node.op == "/") {
                return Value::Number(b == 0.0 ? 0.0 : a / b);
            }
            if (node.op == "%") {
                return Value::Number(b == 0.0 ? 0.0 : std::fmod(a, b));
            }
            if (node.op == "^") {
                return Value::Number(std::pow(a, b));
            }
            if (node.op == "<") {
                return Value::Bool(a < b);
            }
            if (node.op == "<=") {
                return Value::Bool(a <= b);
            }
            if (node.op == ">") {
                return Value::Bool(a > b);
            }
            if (node.op == ">=") {
                return Value::Bool(a >= b);
            }
            return Value();
        }
        case Node::Kind::kTable: {
            std::shared_ptr<Table> table(new Table());
            int32_t position = 1;
            for (const NodePtr& item : node.list) {
                if (item == nullptr) {
                    continue;
                }
                table->Set(Value::Number(static_cast<double>(position)), Eval(*item, scope));
                position += 1;
            }
            for (const auto& field : node.fields) {
                if (field.first == nullptr || field.second == nullptr) {
                    continue;
                }
                table->Set(Eval(*field.first, scope), Eval(*field.second, scope));
            }
            return Value::Of(table);
        }
        case Node::Kind::kFunction: {
            std::shared_ptr<Closure> closure(new Closure());
            closure->params = node.names;
            closure->scope = scope;
            closure->body = node.blocks.empty() ? std::vector<NodePtr>() : node.blocks[0];
            return Value::Of(closure);
        }
        default:
            break;
    }
    return Value();
}

std::vector<Value> Interpreter::Call(const Value& callee, const std::vector<Value>& args) {
    if (callee.kind != Value::Kind::kFunction || callee.closure == nullptr) {
        return std::vector<Value>();
    }
    const Closure& closure = *callee.closure;
    if (closure.native) {
        return closure.native(args);
    }
    if (depth_ >= kMaxDepth) {
        Fail("调用层级超过上限");
        return std::vector<Value>();
    }
    depth_ += 1;
    auto scope = std::make_shared<Scope>(closure.scope);
    for (size_t index = 0; index < closure.params.size(); ++index) {
        scope->Define(closure.params[index], index < args.size() ? args[index] : Value());
    }
    bool returned = false;
    std::vector<Value> results = ExecBlock(closure.body, scope, returned);
    depth_ -= 1;
    return results;
}

}  /* namespace immirror::ui::script */
