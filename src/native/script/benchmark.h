#pragma once

#include <cstdint>

namespace immirror::script {

/*
 * 主机侧性能基准入口: 斐波那契递归与表读写各跑一轮,
 * 输出每百万条指令的耗时. 实现位于 tests/bench.cpp,
 * 只编进主机工具与测试可执行文件, 不进入 Android 库.
 */
int RunBenchmarks();

/* 基准用的源码文本, 测试与工具共用 */
const char* BenchmarkFibonacciSource();
const char* BenchmarkTableSource();

}  /* namespace immirror::script */
