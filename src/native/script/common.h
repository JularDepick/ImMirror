#pragma once

#include <cstdint>
#include <string>

namespace immirror::script {

/*
 * 编译期诊断结构. 词法、语法与语义错误统一用它上报,
 * 行号与列号均从 1 开始, 0 表示未知位置.
 */
struct Diagnostic {
    uint32_t line = 0;
    uint32_t column = 0;
    std::string message;
};

/* 把诊断格式化为 "chunk:行:列: 文本", 供 CLI 与日志直接打印 */
std::string FormatDiagnostic(const char* chunkName, const Diagnostic& diagnostic);

/* 脚本运行时的版本标识, 写入字节码头部并参与加载校验 */
const char* ScriptVersion();

}  /* namespace immirror::script */
