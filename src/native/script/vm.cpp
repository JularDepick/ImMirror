#include "vm.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>

namespace immirror::script {

namespace {

bool AddOverflow(int64_t left, int64_t right, int64_t& out) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_add_overflow(left, right, &out);
#else
    if ((right > 0 && left > std::numeric_limits<int64_t>::max() - right) ||
        (right < 0 && left < std::numeric_limits<int64_t>::min() - right)) {
        return true;
    }
    out = left + right;
    return false;
#endif
}

bool SubOverflow(int64_t left, int64_t right, int64_t& out) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_sub_overflow(left, right, &out);
#else
    if ((right < 0 && left > std::numeric_limits<int64_t>::max() + right) ||
        (right > 0 && left < std::numeric_limits<int64_t>::min() + right)) {
        return true;
    }
    out = left - right;
    return false;
#endif
}

bool MulOverflow(int64_t left, int64_t right, int64_t& out) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_mul_overflow(left, right, &out);
#else
    if (left == 0 || right == 0) {
        out = 0;
        return false;
    }
    const int64_t result = left * right;
    if (result / right != left) {
        return true;
    }
    out = result;
    return false;
#endif
}

bool ToDouble(const Value& value, double& out) {
    if (value.type == ValueType::Int) {
        out = static_cast<double>(value.intValue);
        return true;
    }
    if (value.type == ValueType::Double) {
        out = value.doubleValue;
        return true;
    }
    return false;
}

/* 整数与浮点的精确比较, 返回 -1/0/1, 返回 2 表示无法比较(NaN) */
int CompareIntDouble(int64_t integer, double real) {
    if (std::isnan(real)) {
        return 2;
    }
    if (real >= 9223372036854775808.0) {
        return -1;
    }
    if (real < -9223372036854775808.0) {
        return 1;
    }
    const int64_t truncated = static_cast<int64_t>(real);
    if (integer < truncated) {
        return -1;
    }
    if (integer > truncated) {
        return 1;
    }
    const double fraction = real - static_cast<double>(truncated);
    if (fraction > 0.0) {
        return -1;
    }
    if (fraction < 0.0) {
        return 1;
    }
    return 0;
}

bool AppendCell(const Value& value, std::string& out) {
    switch (value.type) {
        case ValueType::String:
            out.append(value.string->data, value.string->length);
            return true;
        case ValueType::Int:
            out += FormatInt(value.intValue);
            return true;
        case ValueType::Double:
            out += FormatDouble(value.doubleValue);
            return true;
        case ValueType::Bool:
            out += value.boolValue ? "true" : "false";
            return true;
        default:
            return false;
    }
}

/* 算术指令到浮点运算的映射 */
double ApplyDouble(Op op, double left, double right) {
    switch (op) {
        case Op::Add: return left + right;
        case Op::Sub: return left - right;
        case Op::Mul: return left * right;
        case Op::Div: return left / right;
        case Op::Mod: return std::fmod(left, right);
        default: return 0.0;
    }
}

}  /* namespace */

std::string ResultValue::ToDisplayString() const {
    switch (type) {
        case ValueType::Nil: return "nil";
        case ValueType::Bool: return boolValue ? "true" : "false";
        case ValueType::Int: return FormatInt(intValue);
        case ValueType::Double: return FormatDouble(doubleValue);
        case ValueType::String: return text;
        case ValueType::Table: return text;
        case ValueType::Closure: return text;
    }
    return "?";
}

bool ResultValue::AsDouble(double& out) const {
    if (type == ValueType::Int) {
        out = static_cast<double>(intValue);
        return true;
    }
    if (type == ValueType::Double) {
        out = doubleValue;
        return true;
    }
    return false;
}

ResultValue MakeResultValue(const Value& value) {
    ResultValue result;
    result.type = value.type;
    switch (value.type) {
        case ValueType::Nil:
            break;
        case ValueType::Bool:
            result.boolValue = value.boolValue;
            break;
        case ValueType::Int:
            result.intValue = value.intValue;
            break;
        case ValueType::Double:
            result.doubleValue = value.doubleValue;
            break;
        case ValueType::String:
            result.text.assign(value.string->data, value.string->length);
            break;
        case ValueType::Table:
        case ValueType::Closure:
            result.text = ValueToDisplayString(value);
            break;
    }
    return result;
}

Vm::Vm() = default;

Vm::~Vm() = default;

template <typename T>
T* Vm::NewArenaObject() {
    const size_t before = arena_.UsedBytes();
    T* object = arena_.New<T>();
    if (object == nullptr) {
        error_ = VmError::MemoryQuota;
        errorMessage_ = "区域分配失败, 内存不足";
        return nullptr;
    }
    const size_t now = arena_.UsedBytes();
    if (now > before) {
        sandbox_.AccountMemory(static_cast<int64_t>(now - before));
    }
    return object;
}

StringObject* Vm::InternString(const char* data, size_t length) {
    const size_t before = arena_.UsedBytes();
    StringObject* object = strings_.Intern(arena_, data, length);
    if (object == nullptr) {
        error_ = VmError::MemoryQuota;
        errorMessage_ = "字符串分配失败, 内存不足";
        return nullptr;
    }
    const size_t now = arena_.UsedBytes();
    if (now > before) {
        sandbox_.AccountMemory(static_cast<int64_t>(now - before));
    }
    return object;
}

StringObject* Vm::MakeStringThunk(void* context, const char* data, size_t length) {
    auto* vm = static_cast<Vm*>(context);
    return vm->InternString(data, length);
}

bool Vm::TableMemoryHook(void* context, int64_t deltaBytes) {
    auto* vm = static_cast<Vm*>(context);
    return vm->sandbox_.AccountMemory(deltaBytes);
}

void Vm::SetError(VmError error, const std::string& message) {
    if (error_ == VmError::None) {
        error_ = error;
        errorMessage_ = message;
    }
}

void Vm::InstallBindings() {
    if (registry_ == nullptr) {
        return;
    }
    for (const BindingDef& def : registry_->All()) {
        Closure* closure = NewArenaObject<Closure>();
        if (closure == nullptr) {
            return;
        }
        closure->kind = Closure::Kind::Native;
        closure->binding = &def;
        closure->name = def.name.c_str();
        StringObject* key = InternString(def.name.data(), def.name.size());
        if (key == nullptr) {
            return;
        }
        globals_->Set(MakeString(key), MakeClosure(closure));
    }
}

bool Vm::PushFrame(const Proto* proto, uint32_t protoIndex, uint32_t base, uint32_t resultBase,
                   uint32_t resultCount) {
    if (frames_.size() >= sandbox_.Limits().maxCallDepth) {
        error_ = VmError::CallDepth;
        errorMessage_ = "调用深度超过上限";
        return false;
    }
    if (base + proto->numRegs > stack_.size()) {
        error_ = VmError::StackOverflow;
        errorMessage_ = "栈空间不足, 请缩小调用深度或增大 maxStackSlots";
        return false;
    }
    CallFrame frame;
    frame.proto = proto;
    frame.protoIndex = protoIndex;
    frame.base = base;
    frame.pc = 0;
    frame.top = base;
    frame.resultBase = resultBase;
    frame.resultCount = resultCount;
    frames_.push_back(frame);
    if (frames_.size() > sandbox_.Stats().maxCallDepth) {
        sandbox_.Stats().maxCallDepth = static_cast<uint32_t>(frames_.size());
    }
    return true;
}

Value Vm::ConstantValue(uint32_t protoIndex, const Proto& proto, uint32_t index) const {
    if (index >= proto.constants.size()) {
        return NilValue();
    }
    const Constant& constant = proto.constants[index];
    switch (constant.kind) {
        case ConstantKind::Nil:
            return NilValue();
        case ConstantKind::Bool:
            return MakeBool(constant.boolValue);
        case ConstantKind::Int:
            return MakeInt(constant.intValue);
        case ConstantKind::Double:
            return MakeDouble(constant.doubleValue);
        case ConstantKind::String:
            return MakeString(stringConstants_[protoIndex][index]);
        case ConstantKind::Proto:
            return NilValue();
    }
    return NilValue();
}

bool Vm::CheckBindingArgs(const BindingDef& def, const Value* args, uint32_t argc) {
    if (argc < def.minArgs || argc > def.maxArgs) {
        SetError(VmError::ArityMismatch,
                 "函数 " + def.name + " 需要 " + std::to_string(def.minArgs) + " 到 " +
                     std::to_string(def.maxArgs) + " 个参数, 实际 " + std::to_string(argc));
        return false;
    }
    for (uint32_t i = 0; i < def.params.size() && i < argc; ++i) {
        const ValueType expected = def.params[i];
        const ValueType actual = args[i].type;
        if (expected == actual) {
            continue;
        }
        /* 数值参数允许整数与浮点互通, 其余类型严格匹配 */
        const bool numericPair = (expected == ValueType::Double || expected == ValueType::Int) &&
                                 (actual == ValueType::Double || actual == ValueType::Int);
        if (numericPair) {
            continue;
        }
        SetError(VmError::BindingType,
                 "函数 " + def.name + " 的第 " + std::to_string(i + 1) + " 个参数需要 " +
                     ValueTypeName(expected) + ", 实际 " + ValueTypeName(actual));
        return false;
    }
    return true;
}

bool Vm::PlaceResults(uint32_t destAbs, uint32_t wanted, const Value* values, uint32_t produced) {
    const uint32_t total = wanted > 0 ? (wanted > produced ? wanted : produced) : produced;
    if (total > 0 && destAbs + total > stack_.size()) {
        SetError(VmError::StackOverflow, "返回值数量超出栈空间");
        return false;
    }
    if (wanted > 0) {
        for (uint32_t i = 0; i < wanted; ++i) {
            stack_[destAbs + i] = i < produced ? values[i] : NilValue();
        }
        return true;
    }
    for (uint32_t i = 0; i < produced; ++i) {
        stack_[destAbs + i] = values[i];
    }
    return true;
}

bool Vm::DoReturn(uint32_t firstAbs, uint32_t count) {
    CallFrame frame = frames_.back();
    const uint32_t dest = frame.resultBase;
    const uint32_t wanted = frame.resultCount;
    uint32_t produced = count;
    if (wanted > 0 && produced > wanted) {
        produced = wanted;
    }
    if (dest + (wanted > 0 ? wanted : produced) > stack_.size()) {
        SetError(VmError::StackOverflow, "返回值数量超出栈空间");
        return true;
    }
    /* 目标寄存器一定位于源寄存器下方, 正向拷贝不会互相覆盖 */
    for (uint32_t i = 0; i < produced; ++i) {
        stack_[dest + i] = stack_[firstAbs + i];
    }
    if (wanted > 0) {
        for (uint32_t i = produced; i < wanted; ++i) {
            stack_[dest + i] = NilValue();
        }
    }
    frames_.pop_back();
    if (frames_.empty()) {
        returned_.clear();
        for (uint32_t i = 0; i < produced; ++i) {
            returned_.push_back(stack_[dest + i]);
        }
        finished_ = true;
        return true;
    }
    if (wanted == 0) {
        frames_.back().top = dest + produced;
    }
    return false;
}

VmError Vm::Concatenate(const Value& left, const Value& right, Value& out) {
    std::string text;
    if (!AppendCell(left, text) || !AppendCell(right, text)) {
        return VmError::TypeMismatch;
    }
    StringObject* object = InternString(text.data(), text.size());
    if (object == nullptr) {
        return VmError::MemoryQuota;
    }
    out = MakeString(object);
    return VmError::None;
}

VmError Vm::Arithmetic(Op op, const Value& left, const Value& right, Value& out) {
    if (op == Op::Add && (left.type == ValueType::String || right.type == ValueType::String)) {
        return Concatenate(left, right, out);
    }
    if (left.type == ValueType::Int && right.type == ValueType::Int) {
        const int64_t lhs = left.intValue;
        const int64_t rhs = right.intValue;
        int64_t result = 0;
        switch (op) {
            case Op::Add:
                if (!AddOverflow(lhs, rhs, result)) {
                    out = MakeInt(result);
                    return VmError::None;
                }
                break;
            case Op::Sub:
                if (!SubOverflow(lhs, rhs, result)) {
                    out = MakeInt(result);
                    return VmError::None;
                }
                break;
            case Op::Mul:
                if (!MulOverflow(lhs, rhs, result)) {
                    out = MakeInt(result);
                    return VmError::None;
                }
                break;
            case Op::Div:
                if (rhs == 0) {
                    return VmError::DivisorZero;
                }
                if (!(lhs == std::numeric_limits<int64_t>::min() && rhs == -1)) {
                    out = MakeInt(lhs / rhs);
                    return VmError::None;
                }
                break;
            case Op::Mod:
                if (rhs == 0) {
                    return VmError::DivisorZero;
                }
                if (lhs == std::numeric_limits<int64_t>::min() && rhs == -1) {
                    out = MakeInt(0);
                    return VmError::None;
                }
                out = MakeInt(lhs % rhs);
                return VmError::None;
            default:
                break;
        }
        /* 溢出时退化为浮点, 与整数溢出不崩溃的约束一致 */
        out = MakeDouble(ApplyDouble(op, static_cast<double>(lhs), static_cast<double>(rhs)));
        return VmError::None;
    }

    double lhs = 0.0;
    double rhs = 0.0;
    if (!ToDouble(left, lhs) || !ToDouble(right, rhs)) {
        return VmError::TypeMismatch;
    }
    if ((op == Op::Div || op == Op::Mod) && rhs == 0.0) {
        return VmError::DivisorZero;
    }
    out = MakeDouble(ApplyDouble(op, lhs, rhs));
    return VmError::None;
}

VmError Vm::OrderCompare(const Value& left, const Value& right, int& comparison) {
    if (left.type == ValueType::Int && right.type == ValueType::Int) {
        comparison = left.intValue < right.intValue ? -1 : (left.intValue > right.intValue ? 1 : 0);
        return VmError::None;
    }
    const bool leftNumeric =
        left.type == ValueType::Int || left.type == ValueType::Double;
    const bool rightNumeric =
        right.type == ValueType::Int || right.type == ValueType::Double;
    if (leftNumeric && rightNumeric) {
        if (left.type == ValueType::Double && right.type == ValueType::Double) {
            comparison = left.doubleValue < right.doubleValue ? -1
                         : (left.doubleValue > right.doubleValue ? 1 : 0);
            return VmError::None;
        }
        if (left.type == ValueType::Int) {
            comparison = CompareIntDouble(left.intValue, right.doubleValue);
            return VmError::None;
        }
        const int reversed = CompareIntDouble(right.intValue, left.doubleValue);
        comparison = reversed == 2 ? 2 : -reversed;
        return VmError::None;
    }
    if (left.type == ValueType::String && right.type == ValueType::String) {
        const uint32_t length = left.string->length < right.string->length ? left.string->length
                                                                           : right.string->length;
        const int order = length == 0 ? 0 : std::memcmp(left.string->data, right.string->data, length);
        if (order < 0) {
            comparison = -1;
        } else if (order > 0) {
            comparison = 1;
        } else {
            comparison = left.string->length < right.string->length ? -1
                         : (left.string->length > right.string->length ? 1 : 0);
        }
        return VmError::None;
    }
    return VmError::TypeMismatch;
}

VmError Vm::IndexValue(const Value& object, const Value& key, Value& out) {
    if (object.type != ValueType::Table) {
        return VmError::NotIndexable;
    }
    if (key.type == ValueType::Nil) {
        return VmError::TypeMismatch;
    }
    out = object.table->Get(key);
    return VmError::None;
}

bool Vm::Prepare(const Program& program, const SandboxLimits& limits, VmResult& result) {
    BytecodeStatus status;
    if (!VerifyProgram(program, status)) {
        result.error = VmError::InvalidOpcode;
        result.message = "字节码未通过静态校验: " + status.message;
        result.line = status.offset;
        return false;
    }
    if (program.entry >= program.protos.size()) {
        result.error = VmError::ConstantRange;
        result.message = "入口原型下标越界";
        return false;
    }

    error_ = VmError::None;
    errorMessage_.clear();
    errorLine_ = 0;
    finished_ = false;
    returned_.clear();
    frames_.clear();
    frames_.reserve(limits.maxCallDepth + 1);

    arena_.Reset();
    strings_.Clear();
    sandbox_.Reset(limits);
    program_ = &program;
    globals_ = nullptr;

    const uint32_t stackSlots = limits.maxStackSlots < 32 ? 32 : limits.maxStackSlots;
    stack_.assign(stackSlots, NilValue());

    stringConstants_.clear();
    stringConstants_.resize(program.protos.size());
    for (size_t index = 0; index < program.protos.size(); ++index) {
        const Proto& proto = program.protos[index];
        stringConstants_[index].assign(proto.constants.size(), nullptr);
        for (size_t k = 0; k < proto.constants.size(); ++k) {
            if (proto.constants[k].kind != ConstantKind::String) {
                continue;
            }
            StringObject* object = InternString(proto.constants[k].stringValue.data(),
                                               proto.constants[k].stringValue.size());
            if (object == nullptr) {
                result.error = VmError::MemoryQuota;
                result.message = "常量字符串驻留失败, 内存配额不足";
                return false;
            }
            stringConstants_[index][k] = object;
        }
    }

    globals_ = NewArenaObject<Table>();
    if (globals_ == nullptr) {
        result.error = VmError::MemoryQuota;
        result.message = "全局表分配失败";
        return false;
    }
    globals_->SetMemoryHook(&Vm::TableMemoryHook, this);
    InstallBindings();
    if (error_ != VmError::None) {
        result.error = error_;
        result.message = errorMessage_;
        return false;
    }

    const Proto* entry = &program.protos[program.entry];
    if (!PushFrame(entry, program.entry, 0, 0, 0)) {
        result.error = error_;
        result.message = errorMessage_;
        return false;
    }
    frames_.back().top = 0;
    return true;
}

void Vm::Run() {
    CallFrame* frame = &frames_.back();
    const Proto* proto = frame->proto;
    uint32_t protoIndex = frame->protoIndex;
    Value* regs = stack_.data() + frame->base;
    uint32_t pc = 0;
    const uint32_t timeoutInterval = sandbox_.Limits().timeoutCheckInterval;

    auto reloadFrame = [&]() {
        frame = &frames_.back();
        proto = frame->proto;
        protoIndex = frame->protoIndex;
        regs = stack_.data() + frame->base;
        pc = frame->pc;
    };

    for (;;) {
        if (!sandbox_.ConsumeInstruction()) {
            error_ = VmError::InstructionBudget;
            errorMessage_ = "指令预算耗尽";
            break;
        }
        if (timeoutInterval != 0 && (sandbox_.Stats().instructions % timeoutInterval) == 0) {
            if (!sandbox_.CheckTimeout()) {
                error_ = VmError::Timeout;
                errorMessage_ = "执行超时";
                break;
            }
        }
        if (pc >= proto->code.size()) {
            error_ = VmError::Internal;
            errorMessage_ = "指令指针越过函数体末尾";
            break;
        }

        const uint32_t word = proto->code[pc++];
        const Op op = InstructionOp(word);
        const uint32_t a = OperandA(word);
        const uint32_t b = OperandB(word);
        const uint32_t c = OperandC(word);
        const uint32_t bx = OperandBx(word);
        const int32_t sbx = OperandSBx(word);

        switch (op) {
            case Op::Nop:
                break;
            case Op::LoadNil:
                regs[a] = NilValue();
                break;
            case Op::LoadBool:
                regs[a] = MakeBool(b != 0);
                break;
            case Op::LoadInt:
                regs[a] = MakeInt(sbx);
                break;
            case Op::LoadK:
                regs[a] = ConstantValue(protoIndex, *proto, bx);
                break;
            case Op::Move:
                regs[a] = regs[b];
                break;
            case Op::GetGlobal: {
                const StringObject* key = stringConstants_[protoIndex][bx];
                regs[a] = globals_->Get(MakeString(const_cast<StringObject*>(key)));
                break;
            }
            case Op::SetGlobal: {
                const StringObject* key = stringConstants_[protoIndex][bx];
                globals_->Set(MakeString(const_cast<StringObject*>(key)), regs[a]);
                if (sandbox_.MemoryExceeded()) {
                    error_ = VmError::MemoryQuota;
                    errorMessage_ = "内存配额超限";
                    break;
                }
                break;
            }
            case Op::NewTable: {
                Table* table = NewArenaObject<Table>();
                if (table == nullptr) {
                    break;
                }
                table->SetMemoryHook(&Vm::TableMemoryHook, this);
                if (b > 0 || c > 0) {
                    table->Reserve(b, c);
                }
                regs[a] = MakeTable(table);
                ++sandbox_.Stats().tablesCreated;
                if (sandbox_.MemoryExceeded()) {
                    error_ = VmError::MemoryQuota;
                    errorMessage_ = "内存配额超限";
                    break;
                }
                break;
            }
            case Op::GetTable: {
                Value value;
                const VmError status = IndexValue(regs[b], regs[c], value);
                if (status != VmError::None) {
                    error_ = status;
                    errorMessage_ = std::string("下标访问失败: ") + VmErrorName(status);
                    break;
                }
                regs[a] = value;
                break;
            }
            case Op::SetTable: {
                if (regs[a].type != ValueType::Table) {
                    error_ = VmError::NotIndexable;
                    errorMessage_ = "对非表值写入字段";
                    break;
                }
                if (regs[b].type == ValueType::Nil) {
                    error_ = VmError::TypeMismatch;
                    errorMessage_ = "表的键不能是 nil";
                    break;
                }
                regs[a].table->Set(regs[b], regs[c]);
                if (sandbox_.MemoryExceeded()) {
                    error_ = VmError::MemoryQuota;
                    errorMessage_ = "内存配额超限";
                    break;
                }
                break;
            }
            case Op::GetField: {
                Value value;
                const StringObject* key = stringConstants_[protoIndex][c];
                const VmError status =
                    IndexValue(regs[b], MakeString(const_cast<StringObject*>(key)), value);
                if (status != VmError::None) {
                    error_ = status;
                    errorMessage_ = std::string("字段访问失败: ") + VmErrorName(status);
                    break;
                }
                regs[a] = value;
                break;
            }
            case Op::SetField: {
                if (regs[a].type != ValueType::Table) {
                    error_ = VmError::NotIndexable;
                    errorMessage_ = "对非表值写入字段";
                    break;
                }
                const StringObject* key = stringConstants_[protoIndex][c];
                regs[a].table->Set(MakeString(const_cast<StringObject*>(key)), regs[b]);
                if (sandbox_.MemoryExceeded()) {
                    error_ = VmError::MemoryQuota;
                    errorMessage_ = "内存配额超限";
                    break;
                }
                break;
            }
            case Op::Len: {
                if (regs[b].type == ValueType::String) {
                    regs[a] = MakeInt(static_cast<int64_t>(regs[b].string->length));
                } else if (regs[b].type == ValueType::Table) {
                    regs[a] = MakeInt(static_cast<int64_t>(regs[b].table->Length()));
                } else {
                    error_ = VmError::TypeMismatch;
                    errorMessage_ = "长度运算只支持字符串与表";
                }
                break;
            }
            case Op::Add:
            case Op::Sub:
            case Op::Mul:
            case Op::Div:
            case Op::Mod: {
                Value value;
                const VmError status = Arithmetic(op, regs[b], regs[c], value);
                if (status != VmError::None) {
                    error_ = status;
                    errorMessage_ = std::string("算术运算失败: ") + VmErrorName(status);
                    break;
                }
                regs[a] = value;
                if (sandbox_.MemoryExceeded()) {
                    error_ = VmError::MemoryQuota;
                    errorMessage_ = "内存配额超限";
                    break;
                }
                break;
            }
            case Op::Unm: {
                if (regs[b].type == ValueType::Int) {
                    const int64_t value = regs[b].intValue;
                    if (value == std::numeric_limits<int64_t>::min()) {
                        regs[a] = MakeDouble(-static_cast<double>(value));
                    } else {
                        regs[a] = MakeInt(-value);
                    }
                } else if (regs[b].type == ValueType::Double) {
                    regs[a] = MakeDouble(-regs[b].doubleValue);
                } else {
                    error_ = VmError::TypeMismatch;
                    errorMessage_ = "取负只支持数值";
                }
                break;
            }
            case Op::Not:
                regs[a] = MakeBool(!IsTruthy(regs[b]));
                break;
            case Op::Eq:
                regs[a] = MakeBool(ValuesEqual(regs[b], regs[c]));
                break;
            case Op::Ne:
                regs[a] = MakeBool(!ValuesEqual(regs[b], regs[c]));
                break;
            case Op::Lt:
            case Op::Le:
            case Op::Gt:
            case Op::Ge: {
                int comparison = 0;
                const VmError status = OrderCompare(regs[b], regs[c], comparison);
                if (status != VmError::None) {
                    error_ = status;
                    errorMessage_ = std::string("比较运算失败: ") + VmErrorName(status);
                    break;
                }
                bool value = false;
                if (comparison != 2) {
                    switch (op) {
                        case Op::Lt: value = comparison < 0; break;
                        case Op::Le: value = comparison <= 0; break;
                        case Op::Gt: value = comparison > 0; break;
                        default: value = comparison >= 0; break;
                    }
                }
                regs[a] = MakeBool(value);
                break;
            }
            case Op::Jmp:
                pc = static_cast<uint32_t>(static_cast<int64_t>(pc) + sbx);
                break;
            case Op::JmpIf:
                if (IsTruthy(regs[a])) {
                    pc = static_cast<uint32_t>(static_cast<int64_t>(pc) + sbx);
                }
                break;
            case Op::JmpIfNot:
                if (!IsTruthy(regs[a])) {
                    pc = static_cast<uint32_t>(static_cast<int64_t>(pc) + sbx);
                }
                break;
            case Op::Call: {
                if (!IsCallable(regs[a])) {
                    error_ = VmError::NotCallable;
                    errorMessage_ = "调用目标不是函数";
                    break;
                }
                Closure* closure = regs[a].closure;
                const uint32_t resultAbs = frame->base + a;
                if (closure->kind == Closure::Kind::Native) {
                    const BindingDef* def = closure->binding;
                    if (def == nullptr || def->function == nullptr) {
                        error_ = VmError::Internal;
                        errorMessage_ = "原生函数缺少绑定定义";
                        break;
                    }
                    if (!CheckBindingArgs(*def, &regs[a + 1], b)) {
                        break;
                    }
                    Value produced[kMaxNativeResults];
                    NativeCall call;
                    call.host = host_;
                    call.args = &regs[a + 1];
                    call.argc = b;
                    call.results = produced;
                    call.maxResults = kMaxNativeResults;
                    call.makeString = &Vm::MakeStringThunk;
                    call.stringContext = this;
                    const int32_t count = def->function(call);
                    if (count < 0) {
                        error_ = static_cast<VmError>(-count);
                        errorMessage_ = call.errorMessage != nullptr ? call.errorMessage
                                                                     : "原生函数返回错误";
                        break;
                    }
                    if (static_cast<uint32_t>(count) > kMaxNativeResults) {
                        error_ = VmError::Internal;
                        errorMessage_ = "原生函数返回值数量超过上限";
                        break;
                    }
                    if (!PlaceResults(resultAbs, c, produced, static_cast<uint32_t>(count))) {
                        break;
                    }
                    if (error_ != VmError::None) {
                        break;
                    }
                    if (sandbox_.MemoryExceeded()) {
                        error_ = VmError::MemoryQuota;
                        errorMessage_ = "内存配额超限";
                        break;
                    }
                    if (c == 0) {
                        frame->top = resultAbs + static_cast<uint32_t>(count);
                    }
                    break;
                }

                const Proto* calleeProto = closure->proto;
                if (calleeProto == nullptr || closure->protoIndex >= program_->protos.size()) {
                    error_ = VmError::Internal;
                    errorMessage_ = "闭包缺少有效的函数原型";
                    break;
                }
                const uint32_t newBase = frame->base + a + 1;
                frame->pc = pc;
                if (!PushFrame(calleeProto, closure->protoIndex, newBase, resultAbs, c)) {
                    break;
                }
                for (uint32_t i = b; i < calleeProto->numRegs; ++i) {
                    stack_[newBase + i] = NilValue();
                }
                frame = &frames_.back();
                proto = calleeProto;
                protoIndex = closure->protoIndex;
                regs = stack_.data() + newBase;
                pc = 0;
                continue;
            }
            case Op::Return: {
                if (DoReturn(frame->base + a, b)) {
                    break;
                }
                reloadFrame();
                break;
            }
            case Op::ReturnZero: {
                if (DoReturn(frame->base, 0)) {
                    break;
                }
                reloadFrame();
                break;
            }
            case Op::ReturnMulti: {
                const uint32_t first = frame->base + a;
                const uint32_t count = frame->top > first ? frame->top - first : 0;
                if (DoReturn(first, count)) {
                    break;
                }
                reloadFrame();
                break;
            }
            case Op::Closure: {
                const Constant& constant = proto->constants[bx];
                if (constant.protoIndex >= program_->protos.size()) {
                    error_ = VmError::ConstantRange;
                    errorMessage_ = "闭包引用的原型下标越界";
                    break;
                }
                Closure* closure = NewArenaObject<Closure>();
                if (closure == nullptr) {
                    break;
                }
                closure->kind = Closure::Kind::Proto;
                closure->proto = &program_->protos[constant.protoIndex];
                closure->protoIndex = constant.protoIndex;
                closure->name = program_->protos[constant.protoIndex].name.c_str();
                regs[a] = MakeClosure(closure);
                break;
            }
            case Op::Count:
                error_ = VmError::InvalidOpcode;
                errorMessage_ = "遇到保留操作码";
                break;
        }

        if (error_ != VmError::None) {
            break;
        }
        if (finished_) {
            break;
        }
    }

    if (error_ != VmError::None && pc > 0 && pc - 1 < proto->lines.size()) {
        errorLine_ = proto->lines[pc - 1];
    }
}

VmResult Vm::Execute(const Program& program, const SandboxLimits& limits) {
    VmResult result;
    const uint64_t startMicros = MonotonicMicros();
    if (!Prepare(program, limits, result)) {
        result.stats = sandbox_.Stats();
        return result;
    }
    Run();

    result.error = error_;
    result.message = errorMessage_;
    result.line = errorLine_;
    if (error_ == VmError::None) {
        for (const Value& value : returned_) {
            result.results.push_back(MakeResultValue(value));
        }
    }
    result.stats = sandbox_.Stats();
    result.stats.memoryUsedBytes = sandbox_.MemoryUsed();
    result.stats.elapsedMicros = MonotonicMicros() - startMicros;

    /* 结果已深拷贝, 释放本次执行的全部堆对象 */
    returned_.clear();
    frames_.clear();
    globals_ = nullptr;
    strings_.Clear();
    arena_.Reset();
    return result;
}

}  /* namespace immirror::script */
