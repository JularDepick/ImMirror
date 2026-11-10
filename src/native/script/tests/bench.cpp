#include "benchmark.h"

#include <cstdio>
#include <string>

#include "binding.h"
#include "compiler.h"
#include "sandbox.h"
#include "vm.h"

namespace immirror::script {

namespace {

/* 朴素递归斐波那契, 调用密集, 反映函数调用与算术的综合开销 */
const char* const kFibonacciSource = R"SRC(
fn fib(n) {
    if (n < 2) { return n; }
    return fib(n - 1) + fib(n - 2);
}
var total = 0;
for (var i = 0; i < 24; i = i + 1) { total = total + fib(i); }
return total;
)SRC";

/* 表读写: 填充 1000 个数组元素, 再整体遍历 200 轮 */
const char* const kTableSource = R"SRC(
var t = {};
for (var i = 1; i <= 1000; i = i + 1) { t[i] = i * 2; }
var sum = 0;
for (var round = 0; round < 200; round = round + 1) {
    for (var i = 1; i <= 1000; i = i + 1) { sum = sum + t[i]; }
}
return sum;
)SRC";

struct BenchItem {
    const char* name;
    const char* source;
    int64_t expected;
};

/* 计算 sum(fib(i), i=0..23) = fib(25) - 1 */
const int64_t kExpectedFibonacci = 75024;
const int64_t kExpectedTable = 200200000;

int RunOne(const BenchItem& item, const BindingRegistry& registry) {
    const CompileResult compiled = CompileSource(item.source, "bench", CompileOptions());
    if (!compiled.ok) {
        std::printf("基准 %s 编译失败:\n", item.name);
        for (const Diagnostic& diagnostic : compiled.diagnostics) {
            std::printf("  %s\n", FormatDiagnostic("bench", diagnostic).c_str());
        }
        return 1;
    }

    const SandboxLimits limits = SandboxLimits::Benchmark();
    double bestMicros = 0.0;
    uint64_t bestInstructions = 0;
    uint64_t memoryPeak = 0;
    int64_t observed = 0;

    for (int run = 0; run < 3; ++run) {
        Vm vm;
        vm.SetBindingRegistry(&registry);
        const VmResult result = vm.Execute(compiled.program, limits);
        if (!result.ok()) {
            std::printf("基准 %s 执行失败: %s (%s)\n", item.name, VmErrorName(result.error),
                        result.message.c_str());
            return 1;
        }
        observed = result.results.empty() ? 0 : result.results[0].intValue;
        const double micros = static_cast<double>(result.stats.elapsedMicros);
        if (bestMicros == 0.0 || micros < bestMicros) {
            bestMicros = micros;
            bestInstructions = result.stats.instructions;
            memoryPeak = result.stats.memoryPeakBytes;
        }
    }

    if (observed != item.expected) {
        std::printf("基准 %s 结果不符: %lld, 期望 %lld\n", item.name,
                    static_cast<long long>(observed), static_cast<long long>(item.expected));
        return 1;
    }

    const double millions = static_cast<double>(bestInstructions) / 1000000.0;
    const double microsPerMillion = millions > 0.0 ? bestMicros / millions : 0.0;
    const double mips = bestMicros > 0.0 ? millions / (bestMicros / 1000000.0) : 0.0;
    std::printf("基准 %s\n", item.name);
    std::printf("  指令 %llu 条, 耗时 %.3f ms, 每百万指令 %.3f ms, 吞吐 %.1f MIPS\n",
                static_cast<unsigned long long>(bestInstructions), bestMicros / 1000.0,
                microsPerMillion / 1000.0, mips);
    std::printf("  内存峰值 %llu 字节, 字节码 %zu 字节\n",
                static_cast<unsigned long long>(memoryPeak), compiled.program.ByteSize());
    return 0;
}

}  /* namespace */

const char* BenchmarkFibonacciSource() { return kFibonacciSource; }

const char* BenchmarkTableSource() { return kTableSource; }

int RunBenchmarks() {
    BindingRegistry registry;
    RegisterDefaultBindings(registry);

    const BenchItem items[] = {
        {"斐波那契 fib(24) 累计和", kFibonacciSource, kExpectedFibonacci},
        {"表读写 1000 项 x 200 轮", kTableSource, kExpectedTable},
    };
    int failures = 0;
    for (const BenchItem& item : items) {
        failures += RunOne(item, registry);
    }
    return failures == 0 ? 0 : 1;
}

}  /* namespace immirror::script */
