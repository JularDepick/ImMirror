/*
 * 主机单元测试的日志桩: core/log.cpp 依赖 android/log.h,
 * 测试构建用同一签名在主机输出到 stderr (或直接丢弃).
 */
#include "core/log.h"

#include <cstdarg>
#include <cstdio>

namespace immirror::core {

void LogWriteValue(LogLevel level, const char* tag, const char* format, va_list args) {
    (void)level;
    (void)tag;
    (void)format;
    (void)args;
}

void LogWrite(LogLevel level, const char* tag, const char* format, ...) {
    (void)level;
    (void)tag;
    (void)format;
}

}  /* namespace immirror::core */
