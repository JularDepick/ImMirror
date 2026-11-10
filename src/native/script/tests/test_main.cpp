#include <cstdio>
#include <cstring>

#include "benchmark.h"
#include "test_framework.h"

int main(int argc, char** argv) {
    /* 关闭输出缓冲, 便于定位卡住的用例 */
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    bool runBenchmarks = true;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--no-bench") == 0) {
            runBenchmarks = false;
        }
    }

    int failedCases = 0;
    const auto& registry = immirror::script::test::Registry();
    for (const auto& testCase : registry) {
        const int before = immirror::script::test::FailureCount();
        std::printf("[开始] %s\n", testCase.name);
        testCase.function();
        if (immirror::script::test::FailureCount() != before) {
            ++failedCases;
            std::printf("[失败] %s\n", testCase.name);
        } else {
            std::printf("[通过] %s\n", testCase.name);
        }
    }

    const int checks = immirror::script::test::CheckCount();
    const int failures = immirror::script::test::FailureCount();
    std::printf("\n单元测试汇总: %zu 个用例, %d 个断言, %d 处断言失败, %d 个用例失败\n",
                registry.size(), checks, failures, failedCases);

    if (runBenchmarks) {
        std::printf("\n");
        immirror::script::RunBenchmarks();
    }
    return failedCases == 0 ? 0 : 1;
}
