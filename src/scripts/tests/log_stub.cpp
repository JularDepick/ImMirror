/* 宿主断言用的日志桩: 安卓日志头在主机上不可用. */
#include <cstdarg>
#include <cstdio>

#include "core/log.h"

namespace immirror::core {

void LogWrite(LogLevel level, const char* tag, const char* format, ...) {
    (void)level;
    va_list args;
    va_start(args, format);
    std::printf("[%s] ", tag == nullptr ? "" : tag);
    std::vprintf(format, args);
    std::printf("\n");
    va_end(args);
}

void LogWriteValue(LogLevel level, const char* tag, const char* format, va_list args) {
    (void)level;
    std::printf("[%s] ", tag == nullptr ? "" : tag);
    std::vprintf(format, args);
    std::printf("\n");
}

}  /* namespace immirror::core */
