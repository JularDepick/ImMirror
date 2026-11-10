#pragma once

#include <cstdint>
#include <string>

#include "common.h"

namespace immirror::script {

/*
 * 运行时错误码. 虚拟机内部不抛异常, 一律通过该枚举与消息文本回传,
 * 宿主据此决定是忽略, 记录日志还是中止脚本.
 */
enum class VmError : uint16_t {
    None = 0,
    InstructionBudget,  /* 指令预算耗尽 */
    Timeout,            /* 执行超时 */
    MemoryQuota,        /* 内存配额超限 */
    CallDepth,          /* 调用深度超限 */
    StackOverflow,      /* 栈空间不足 */
    RegisterRange,      /* 寄存器下标越界 */
    InvalidOpcode,      /* 非法指令 */
    ConstantRange,      /* 常量下标越界 */
    TypeMismatch,       /* 操作数类型不符 */
    DivisorZero,        /* 除数为零 */
    NotCallable,        /* 调用了非函数值 */
    NotIndexable,       /* 对非表值取下标 */
    ArityMismatch,      /* 实参个数不符 */
    BindingRejected,    /* binding 拒绝调用 */
    BindingType,        /* binding 参数类型不符 */
    NativeFailure,      /* 原生函数内部失败 */
    Internal,           /* 内部状态异常 */
};

const char* VmErrorName(VmError error);

/*
 * 沙盒限制. 指令预算按"单次执行"计, 每次 Execute 重新计数;
 * 内存配额统计区域分配器与表内部存储的总字节;
 * 超时以毫秒计, 按指令间隔抽样检测, 避免每条指令读时钟.
 */
struct SandboxLimits {
    uint64_t instructionBudget = 2000000ull;
    uint64_t memoryQuotaBytes = 4ull * 1024ull * 1024ull;
    uint32_t timeoutMillis = 500;
    uint32_t maxCallDepth = 64;
    uint32_t maxStackSlots = 4096;
    uint32_t timeoutCheckInterval = 256;

    /* 一组宽松限制, 供基准测试使用 */
    static SandboxLimits Benchmark();
};

/* 一次执行的统计信息, 供宿主上报与调试 */
struct SandboxStats {
    uint64_t instructions = 0;
    uint64_t memoryUsedBytes = 0;
    uint64_t memoryPeakBytes = 0;
    uint32_t maxCallDepth = 0;
    uint32_t tablesCreated = 0;
    uint32_t stringsCreated = 0;
    uint64_t elapsedMicros = 0;
    bool timedOut = false;
};

/*
 * 沙盒运行期状态. 由虚拟机持有, 负责预算, 超时与配额的记账,
 * 自身不做任何系统调用以外的副作用.
 */
class Sandbox {
public:
    void Reset(const SandboxLimits& limits);

    const SandboxLimits& Limits() const { return limits_; }
    SandboxStats& Stats() { return stats_; }
    const SandboxStats& Stats() const { return stats_; }

    /* 指令计数自增, 超出预算返回 false */
    bool ConsumeInstruction();

    /* 周期检测: 返回 false 表示超时 */
    bool CheckTimeout();

    /* 内存记账, 超出配额返回 false 并标记 */
    bool AccountMemory(int64_t deltaBytes);

    bool MemoryExceeded() const { return memoryExceeded_; }
    uint64_t MemoryUsed() const { return memoryUsed_; }

private:
    SandboxLimits limits_;
    SandboxStats stats_;
    uint64_t startMillis_ = 0;
    uint64_t memoryUsed_ = 0;
    bool memoryExceeded_ = false;
    bool started_ = false;
};

/* 单调毫秒时钟, 便于在无标准库时钟的平台上替换 */
uint64_t MonotonicMillis();

/* 单调微秒时钟 */
uint64_t MonotonicMicros();

}  /* namespace immirror::script */
