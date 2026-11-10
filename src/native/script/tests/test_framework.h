#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

/*
 * 极简单元测试框架. 不引入第三方依赖, 用例通过静态注册表登记,
 * main 按登记顺序逐个执行并统计断言数量.
 */
namespace immirror::script::test {

struct TestCase {
    const char* name;
    void (*function)();
};

inline std::vector<TestCase>& Registry() {
    static std::vector<TestCase> registry;
    return registry;
}

inline int& FailureCount() {
    static int value = 0;
    return value;
}

inline int& CheckCount() {
    static int value = 0;
    return value;
}

struct Registrar {
    Registrar(const char* name, void (*function)()) {
        Registry().push_back(TestCase{name, function});
    }
};

inline void Fail(const char* file, int line, const std::string& message) {
    ++FailureCount();
    std::printf("    %s:%d: %s\n", file, line, message.c_str());
}

}  /* namespace immirror::script::test */

#define TEST_CASE(name)                                                                     \
    static void name();                                                                     \
    static ::immirror::script::test::Registrar test_registrar_##name(#name, &name);          \
    static void name()

#define CHECK(condition)                                                                    \
    do {                                                                                    \
        ++::immirror::script::test::CheckCount();                                           \
        if (!(condition)) {                                                                 \
            ::immirror::script::test::Fail(__FILE__, __LINE__, #condition);                  \
        }                                                                                   \
    } while (false)

#define CHECK_INT(actual, expected)                                                         \
    do {                                                                                    \
        ++::immirror::script::test::CheckCount();                                           \
        const long long actualValue_ = static_cast<long long>(actual);                       \
        const long long expectedValue_ = static_cast<long long>(expected);                   \
        if (actualValue_ != expectedValue_) {                                               \
            ::immirror::script::test::Fail(                                                 \
                __FILE__, __LINE__, std::string(#actual) + " = " +                          \
                                        std::to_string(actualValue_) + ", 期望 " +           \
                                        std::to_string(expectedValue_));                    \
        }                                                                                   \
    } while (false)

#define CHECK_REAL(actual, expected)                                                        \
    do {                                                                                    \
        ++::immirror::script::test::CheckCount();                                           \
        const double actualValue_ = (actual);                                               \
        const double expectedValue_ = (expected);                                           \
        const double diff_ = actualValue_ > expectedValue_ ? actualValue_ - expectedValue_   \
                                                          : expectedValue_ - actualValue_;  \
        if (!(diff_ <= 1e-9)) {                                                             \
            ::immirror::script::test::Fail(__FILE__, __LINE__,                              \
                                           std::string(#actual) + " = " +                   \
                                               std::to_string(actualValue_) + ", 期望 " +   \
                                               std::to_string(expectedValue_));             \
        }                                                                                   \
    } while (false)

#define CHECK_TEXT(actual, expected)                                                        \
    do {                                                                                    \
        ++::immirror::script::test::CheckCount();                                           \
        const std::string actualValue_ = (actual);                                          \
        const std::string expectedValue_ = (expected);                                      \
        if (actualValue_ != expectedValue_) {                                               \
            ::immirror::script::test::Fail(__FILE__, __LINE__,                              \
                                           std::string(#actual) + " = \"" + actualValue_ +  \
                                               "\", 期望 \"" + expectedValue_ + "\"");   \
        }                                                                                   \
    } while (false)
