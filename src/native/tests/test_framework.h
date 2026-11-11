#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace immirror::editor::test {

/*
 * 极简单元测试框架. 不引入第三方依赖, 用例通过静态注册表登记,
 * main 按登记顺序逐个执行并统计断言数量, 与 script/tests 同一约定.
 */
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


#define TEST_CASE(name)                                                                        \
    static void name();                                                                        \
    static ::immirror::editor::test::Registrar test_registrar_##name(#name, &name);            \
    static void name()

#define CHECK(condition)                                                                       \
    do {                                                                                       \
        ++::immirror::editor::test::CheckCount();                                              \
        if (!(condition)) {                                                                    \
            ::immirror::editor::test::Fail(__FILE__, __LINE__, #condition);                    \
        }                                                                                      \
    } while (false)

#define CHECK_INT(actual, expected)                                                            \
    do {                                                                                       \
        ++::immirror::editor::test::CheckCount();                                              \
        const long long actualValue_ = static_cast<long long>(actual);                         \
        const long long expectedValue_ = static_cast<long long>(expected);                     \
        if (actualValue_ != expectedValue_) {                                                  \
            ::immirror::editor::test::Fail(                                                    \
                __FILE__, __LINE__, std::string(#actual) + " = " +                             \
                                        std::to_string(actualValue_) + ", 期望 " +             \
                                        std::to_string(expectedValue_));                       \
        }                                                                                      \
    } while (false)

/* 按 key 取首行值的辅助, 与 Java 侧 EditorReply.first 的语义一致. */
inline std::string ReplyFirst(const std::string& reply, const std::string& key) {
    size_t cursor = 0;
    while (cursor < reply.size()) {
        size_t end = reply.find("\n", cursor);
        if (end == std::string::npos) {
            end = reply.size();
        }
        const std::string line = reply.substr(cursor, end - cursor);
        const size_t split = line.find("=");
        if (split != std::string::npos && line.substr(0, split) == key) {
            return line.substr(split + 1);
        }
        cursor = end + 1;
    }
    return std::string();
}

/* 按 key 取全部行值的辅助, 与 Java 侧 EditorReply.all 的语义一致. */
inline std::vector<std::string> ReplyAll(const std::string& reply, const std::string& key) {
    std::vector<std::string> values;
    size_t cursor = 0;
    while (cursor < reply.size()) {
        size_t end = reply.find("\n", cursor);
        if (end == std::string::npos) {
            end = reply.size();
        }
        const std::string line = reply.substr(cursor, end - cursor);
        const size_t split = line.find("=");
        if (split != std::string::npos && line.substr(0, split) == key) {
            values.push_back(line.substr(split + 1));
        }
        cursor = end + 1;
    }
    return values;
}
}  /* namespace immirror::editor::test */
