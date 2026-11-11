#pragma once

/*
 * 主机 MinGW 编译兼容层: 仅单元测试构建使用, Android NDK 构建不包含本文件.
 * MinGW 的 mkdir 只有单参版本, 工程格式层按 POSIX 两参调用,
 * 这里把两参调用转发到 CRT 的 _mkdir, 避免改动工程源码.
 */
#if defined(__MINGW32__) || defined(__MINGW64__)
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif
