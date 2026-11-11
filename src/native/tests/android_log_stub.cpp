/* 主机单元测试的日志桩: 日志一律丢弃, 只保证链接符号存在. */
#include <cstdarg>
#include <cstdio>

#include "android/log.h"

namespace {

void DiscardV(int priority, const char* tag, const char* format, __builtin_va_list args) {
    (void)priority;
    (void)tag;
    (void)format;
    (void)args;
}

}  /* namespace */

int __android_log_vprint(int priority, const char* tag, const char* format, __builtin_va_list args) {
    DiscardV(priority, tag, format, args);
    return 0;
}

int __android_log_print(int priority, const char* tag, const char* format, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, format);
    DiscardV(priority, tag, format, args);
    __builtin_va_end(args);
    return 0;
}
