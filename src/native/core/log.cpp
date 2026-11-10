#include "core/log.h"

#include <android/log.h>

namespace immirror::core {
namespace {

/* 本工程日志级别到平台日志优先级的映射, 顺序与 LogLevel 声明保持一致. */
constexpr int kPlatformPriority[] = {
    ANDROID_LOG_VERBOSE,
    ANDROID_LOG_DEBUG,
    ANDROID_LOG_INFO,
    ANDROID_LOG_WARN,
    ANDROID_LOG_ERROR,
};

constexpr int kPriorityCount = static_cast<int>(sizeof(kPlatformPriority) / sizeof(kPlatformPriority[0]));

/* 越界级别回退到 Info, 保证异常输入也不会让日志调用失败. */
int PriorityOf(LogLevel level) {
    const int index = static_cast<int>(level);
    if (index < 0 || index >= kPriorityCount) {
        return ANDROID_LOG_INFO;
    }
    return kPlatformPriority[index];
}

}  /* namespace */

void LogWriteValue(LogLevel level, const char* tag, const char* format, va_list args) {
    if (format == nullptr) {
        return;
    }
    __android_log_vprint(PriorityOf(level), tag != nullptr ? tag : "ImMirror", format, args);
}

void LogWrite(LogLevel level, const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    LogWriteValue(level, tag, format, args);
    va_end(args);
}

}  /* namespace immirror::core */
