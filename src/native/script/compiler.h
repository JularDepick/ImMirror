#pragma once

#include <string>
#include <vector>

#include "ast.h"
#include "bytecode.h"
#include "common.h"

namespace immirror::script {

/* 编译选项 */
struct CompileOptions {
    /* 丢弃行号映射, 生成更瘦的字节码, 发布包可按需开启 */
    bool stripLineInfo = false;
};

/* 编译结果. diagnostics 非空时 program 不可用 */
struct CompileResult {
    bool ok = false;
    Program program;
    std::vector<Diagnostic> diagnostics;
};

/* 编译一个已经解析完成的模块 */
CompileResult CompileModule(const ast::Module& module, const std::string& chunkName,
                            const CompileOptions& options = CompileOptions());

/* 一步完成词法分析, 语法分析与编译 */
CompileResult CompileSource(const std::string& source, const std::string& chunkName,
                            const CompileOptions& options = CompileOptions());

}  /* namespace immirror::script */
