#pragma once

/* 主机单元测试用的 android/log.h 桩: 把日志调用吞掉, 不依赖 NDK. */

#define ANDROID_LOG_VERBOSE 0
#define ANDROID_LOG_DEBUG   1
#define ANDROID_LOG_INFO    2
#define ANDROID_LOG_WARN    3
#define ANDROID_LOG_ERROR   4

#ifdef __cplusplus
extern "C" {
#endif

int __android_log_vprint(int priority, const char* tag, const char* format, __builtin_va_list args);
int __android_log_print(int priority, const char* tag, const char* format, ...);

#ifdef __cplusplus
}
#endif
