#pragma once

#include <string>
#include <vector>

#include "binding.h"
#include "bytecode.h"
#include "sandbox.h"
#include "value.h"

namespace immirror::script {

/*
 * 返回值的外部表示. 虚拟机每次执行结束都会把结果深拷贝到该结构,
 * 区域分配器随后重置, 因此宿主可以长期持有这些值.
 */
struct ResultValue {
    ValueType type = ValueType::Nil;
    bool boolValue = false;
    int64_t intValue = 0;
    double doubleValue = 0.0;
    std::string text;

    bool IsNumber() const { return type == ValueType::Int || type == ValueType::Double; }
    bool AsDouble(double& out) const;
    std::string ToDisplayString() const;
};

/* 一次执行的结果与统计 */
struct VmResult {
    VmError error = VmError::None;
    std::string message;
    uint32_t line = 0;
    std::vector<ResultValue> results;
    SandboxStats stats;

    bool ok() const { return error == VmError::None; }
};

/*
 * 寄存器式虚拟机. 生命周期内可重复执行多个脚本,
 * 每次 Execute 都会重置区域分配器与全局环境,
 * 因此脚本内部状态不跨执行保留, 跨执行状态由宿主通过 binding 承载.
 */
class Vm {
public:
    Vm();
    ~Vm();

    Vm(const Vm&) = delete;
    Vm& operator=(const Vm&) = delete;

    /* 安装白名单表, 为空时不注册任何原生函数 */
    void SetBindingRegistry(const BindingRegistry* registry) { registry_ = registry; }

    /* 安装宿主世界句柄, 原样传给 NativeCall::host */
    void SetHost(void* host) { host_ = host; }

    /* 执行入口原型, 返回主函数的全部返回值 */
    VmResult Execute(const Program& program, const SandboxLimits& limits);

    /* 最近一次执行的沙盒状态, 便于宿主读取累计用量 */
    const Sandbox& GetSandbox() const { return sandbox_; }

private:
    struct CallFrame {
        const Proto* proto = nullptr;
        uint32_t protoIndex = 0;
        uint32_t base = 0;        /* 该帧第一个寄存器在栈数组中的下标 */
        uint32_t pc = 0;          /* 返回地址, 即调用者的下一条指令 */
        uint32_t top = 0;         /* 多返回值调用的结果上界 */
        uint32_t resultBase = 0;  /* 调用者帧中的结果目标寄存器 */
        uint32_t resultCount = 0; /* 期望返回值个数, 0 表示全部 */
    };

    void SetError(VmError error, const std::string& message);

    template <typename T>
    T* NewArenaObject();

    StringObject* InternString(const char* data, size_t length);
    static StringObject* MakeStringThunk(void* context, const char* data, size_t length);
    static bool TableMemoryHook(void* context, int64_t deltaBytes);

    bool Prepare(const Program& program, const SandboxLimits& limits, VmResult& result);
    void InstallBindings();
    bool PushFrame(const Proto* proto, uint32_t protoIndex, uint32_t base, uint32_t resultBase,
                   uint32_t resultCount);
    void Run();

    Value ConstantValue(uint32_t protoIndex, const Proto& proto, uint32_t index) const;
    bool CheckBindingArgs(const BindingDef& def, const Value* args, uint32_t argc);
    bool PlaceResults(uint32_t destAbs, uint32_t wanted, const Value* values, uint32_t produced);
    bool DoReturn(uint32_t firstAbs, uint32_t count);

    VmError Arithmetic(Op op, const Value& left, const Value& right, Value& out);
    VmError OrderCompare(const Value& left, const Value& right, int& comparison);
    VmError Concatenate(const Value& left, const Value& right, Value& out);
    VmError IndexValue(const Value& object, const Value& key, Value& out);

    ResultValue ToResultValue(const Value& value) const;

    const BindingRegistry* registry_ = nullptr;
    void* host_ = nullptr;
    Arena arena_;
    StringInterner strings_;
    Sandbox sandbox_;
    Table* globals_ = nullptr;
    std::vector<Value> stack_;
    std::vector<CallFrame> frames_;
    std::vector<std::vector<StringObject*>> stringConstants_;
    std::vector<Value> returned_;
    const Program* program_ = nullptr;
    VmError error_ = VmError::None;
    std::string errorMessage_;
    uint32_t errorLine_ = 0;
    bool finished_ = false;
};

/* 把值深拷贝为外部表示 */
ResultValue MakeResultValue(const Value& value);

}  /* namespace immirror::script */
