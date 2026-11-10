#pragma once

#include <cstdarg>
#include <cstdlib>

namespace immirror::core {

/*
 * 日志级别. 取值顺序同时决定过滤阈值, 数值越大越严重,
 * 后续接入运行期日志开关时只需比较该枚举即可.
 */
enum class LogLevel : int {
    Verbose = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
};

/*
 * 日志写出入口. 统一走 C 变参, 避免逐帧构造 std::string,
 * 内部不抛异常, 单条日志失败只丢弃本条内容.
 */
void LogWrite(LogLevel level, const char* tag, const char* format, ...);

/* 已经持有 va_list 的调用点使用该重载, 避免二次展开变参. */
void LogWriteValue(LogLevel level, const char* tag, const char* format, va_list args);

}  /* namespace immirror::core */

#define IM_LOGV(tag, ...) \
    ::immirror::core::LogWrite(::immirror::core::LogLevel::Verbose, tag, __VA_ARGS__)
#define IM_LOGD(tag, ...) \
    ::immirror::core::LogWrite(::immirror::core::LogLevel::Debug, tag, __VA_ARGS__)
#define IM_LOGI(tag, ...) \
    ::immirror::core::LogWrite(::immirror::core::LogLevel::Info, tag, __VA_ARGS__)
#define IM_LOGW(tag, ...) \
    ::immirror::core::LogWrite(::immirror::core::LogLevel::Warn, tag, __VA_ARGS__)
#define IM_LOGE(tag, ...) \
    ::immirror::core::LogWrite(::immirror::core::LogLevel::Error, tag, __VA_ARGS__)

/*
 * 断言宏. 条件不成立时按错误级别记录并附带源码位置;
 * 调试构建直接中断进程以便尽早暴露问题, 发布构建只记日志避免线上闪退.
 */
#if defined(NDEBUG)
#define IM_CHECK(condition, tag, ...)                                                 \
    do {                                                                              \
        if (!(condition)) {                                                           \
            IM_LOGE(tag, "check failed: %s", #condition);                             \
            IM_LOGE(tag, __VA_ARGS__);                                                \
        }                                                                             \
    } while (false)
#else
#define IM_CHECK(condition, tag, ...)                                                 \
    do {                                                                              \
        if (!(condition)) {                                                           \
            IM_LOGE(tag, "check failed: %s (%s:%d)", #condition, __FILE__, __LINE__); \
            IM_LOGE(tag, __VA_ARGS__);                                                \
            abort();                                                                  \
        }                                                                             \
    } while (false)
#endif
