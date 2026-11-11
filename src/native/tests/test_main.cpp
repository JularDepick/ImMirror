#include "tests/test_framework.h"

#include <cstdio>
#include <string>
#include <vector>

namespace {

/*
 * 测试主入口: 用例通过静态注册表登记, 全部跑完后按失败数决定退出码,
 * 与 script/tests 的极简框架同一套约定.
 */
void RunAll() {
    for (const auto& testCase : immirror::editor::test::Registry()) {
        std::printf("[ RUN  ] %s\n", testCase.name);
        const int before = immirror::editor::test::FailureCount();
        testCase.function();
        const bool ok = immirror::editor::test::FailureCount() == before;
        std::printf(ok ? "[   OK ] %s\n" : "[ FAIL ] %s\n", testCase.name);
    }
}

}  /* namespace */

int main() {
    RunAll();
    const int checks = immirror::editor::test::CheckCount();
    const int failures = immirror::editor::test::FailureCount();
    std::printf("\nchecks=%d failures=%d\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
