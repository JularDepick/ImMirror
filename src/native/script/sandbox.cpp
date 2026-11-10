#include "sandbox.h"

#include <chrono>

namespace immirror::script {

namespace {

uint64_t NowMicros() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(now).count());
}

}  /* namespace */

const char* VmErrorName(VmError error) {
    switch (error) {
        case VmError::None: return "无错误";
        case VmError::InstructionBudget: return "指令预算耗尽";
        case VmError::Timeout: return "执行超时";
        case VmError::MemoryQuota: return "内存配额超限";
        case VmError::CallDepth: return "调用深度超限";
        case VmError::StackOverflow: return "栈空间不足";
        case VmError::RegisterRange: return "寄存器下标越界";
        case VmError::InvalidOpcode: return "非法指令";
        case VmError::ConstantRange: return "常量下标越界";
        case VmError::TypeMismatch: return "操作数类型不符";
        case VmError::DivisorZero: return "除数为零";
        case VmError::NotCallable: return "调用了非函数值";
        case VmError::NotIndexable: return "对非表值取下标";
        case VmError::ArityMismatch: return "实参个数不符";
        case VmError::BindingRejected: return "binding 拒绝调用";
        case VmError::BindingType: return "binding 参数类型不符";
        case VmError::NativeFailure: return "原生函数内部失败";
        case VmError::Internal: return "内部状态异常";
    }
    return "未知错误";
}

SandboxLimits SandboxLimits::Benchmark() {
    SandboxLimits limits;
    limits.instructionBudget = 400000000ull;
    limits.memoryQuotaBytes = 256ull * 1024ull * 1024ull;
    limits.timeoutMillis = 60000;
    limits.maxCallDepth = 512;
    limits.maxStackSlots = 8192;
    limits.timeoutCheckInterval = 4096;
    return limits;
}

void Sandbox::Reset(const SandboxLimits& limits) {
    limits_ = limits;
    stats_ = SandboxStats();
    memoryUsed_ = 0;
    memoryExceeded_ = false;
    startMillis_ = MonotonicMillis();
    started_ = true;
}

bool Sandbox::ConsumeInstruction() {
    ++stats_.instructions;
    return stats_.instructions <= limits_.instructionBudget;
}

bool Sandbox::CheckTimeout() {
    if (limits_.timeoutCheckInterval == 0) {
        return true;
    }
    if (limits_.timeoutMillis == 0) {
        return true;
    }
    const uint64_t elapsed = MonotonicMillis() - startMillis_;
    if (elapsed > limits_.timeoutMillis) {
        stats_.timedOut = true;
        return false;
    }
    return true;
}

bool Sandbox::AccountMemory(int64_t deltaBytes) {
    if (deltaBytes > 0) {
        memoryUsed_ += static_cast<uint64_t>(deltaBytes);
    } else if (memoryUsed_ >= static_cast<uint64_t>(-deltaBytes)) {
        memoryUsed_ -= static_cast<uint64_t>(-deltaBytes);
    } else {
        memoryUsed_ = 0;
    }
    stats_.memoryUsedBytes = memoryUsed_;
    if (memoryUsed_ > stats_.memoryPeakBytes) {
        stats_.memoryPeakBytes = memoryUsed_;
    }
    if (memoryUsed_ > limits_.memoryQuotaBytes) {
        memoryExceeded_ = true;
        return false;
    }
    return true;
}

uint64_t MonotonicMillis() { return NowMicros() / 1000ull; }

uint64_t MonotonicMicros() { return NowMicros(); }

}  /* namespace immirror::script */
