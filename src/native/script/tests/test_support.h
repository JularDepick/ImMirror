#pragma once

#include <string>
#include <utility>
#include <vector>

#include "binding.h"
#include "compiler.h"
#include "sandbox.h"
#include "vm.h"

/*
 * 测试辅助: 一段源码从编译到执行的全流程封装,
 * 以及可供断言使用的结果读取接口.
 */
namespace immirror::script::test {

/* 假宿主, 记录 binding 的调用参数与日志 */
class TestWorld : public HostWorld {
public:
    int32_t blockId = 7;
    int32_t lastX = 0;
    int32_t lastY = 0;
    int32_t lastZ = 0;
    int32_t lastPlaced = 0;
    std::vector<std::string> logs;

    int32_t GetBlock(int32_t x, int32_t y, int32_t z) override {
        lastX = x;
        lastY = y;
        lastZ = z;
        return blockId;
    }

    bool SetBlock(int32_t x, int32_t y, int32_t z, int32_t block) override {
        lastX = x;
        lastY = y;
        lastZ = z;
        lastPlaced = block;
        return block != 0;
    }

    bool PlayerPosition(double& x, double& y, double& z) override {
        x = 1.5;
        y = 2.5;
        z = 3.5;
        return true;
    }

    void Log(const char* message) override { logs.push_back(message); }
};

inline BindingRegistry& SharedRegistry() {
    static BindingRegistry registry = []() {
        BindingRegistry value;
        RegisterDefaultBindings(value);
        return value;
    }();
    return registry;
}

/* 一次执行的完整结果 */
struct Outcome {
    bool compiled = false;
    bool ok = false;
    VmError error = VmError::None;
    std::string message;
    uint32_t line = 0;
    std::vector<ResultValue> results;
    SandboxStats stats;

    size_t Count() const { return results.size(); }

    int64_t Int(size_t index) const {
        if (index >= results.size()) {
            return -999999;
        }
        const ResultValue& value = results[index];
        if (value.type == ValueType::Int) {
            return value.intValue;
        }
        if (value.type == ValueType::Double) {
            return static_cast<int64_t>(value.doubleValue);
        }
        return -999999;
    }

    double Real(size_t index) const {
        if (index >= results.size()) {
            return -999999.0;
        }
        double out = 0.0;
        results[index].AsDouble(out);
        return out;
    }

    std::string Text(size_t index) const {
        if (index >= results.size()) {
            return "<越界>";
        }
        return results[index].ToDisplayString();
    }

    bool Bool(size_t index) const {
        if (index >= results.size()) {
            return false;
        }
        return results[index].type == ValueType::Bool && results[index].boolValue;
    }
};

inline void ExecuteProgram(const Program& program, const SandboxLimits& limits, TestWorld* world,
                           Outcome& outcome) {
    Vm vm;
    vm.SetBindingRegistry(&SharedRegistry());
    vm.SetHost(world);
    const VmResult result = vm.Execute(program, limits);
    outcome.ok = result.ok();
    outcome.error = result.error;
    outcome.message = result.message;
    outcome.line = result.line;
    outcome.results = result.results;
    outcome.stats = result.stats;
}

/* 编译并执行一段源码 */
inline Outcome Run(const std::string& source, const SandboxLimits& limits = SandboxLimits()) {
    Outcome outcome;
    const CompileResult compiled = CompileSource(source, "test", CompileOptions());
    outcome.compiled = compiled.ok;
    if (!compiled.ok) {
        for (const Diagnostic& diagnostic : compiled.diagnostics) {
            outcome.message += FormatDiagnostic("test", diagnostic);
            outcome.message += ';';
        }
        return outcome;
    }
    TestWorld world;
    ExecuteProgram(compiled.program, limits, &world, outcome);
    return outcome;
}

/* 编译并执行, 同时把宿主交给调用方检查 */
inline Outcome RunWithWorld(const std::string& source, TestWorld& world,
                            const SandboxLimits& limits = SandboxLimits()) {
    Outcome outcome;
    const CompileResult compiled = CompileSource(source, "test", CompileOptions());
    outcome.compiled = compiled.ok;
    if (!compiled.ok) {
        for (const Diagnostic& diagnostic : compiled.diagnostics) {
            outcome.message += FormatDiagnostic("test", diagnostic);
            outcome.message += ';';
        }
        return outcome;
    }
    ExecuteProgram(compiled.program, limits, &world, outcome);
    return outcome;
}

/* 只编译, 用于检查编译期错误 */
inline CompileResult CompileOnly(const std::string& source) {
    return CompileSource(source, "test", CompileOptions());
}

}  /* namespace immirror::script::test */
