#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace immirror::ui::script {

/*
 * 界面脚本的语法解析.
 *
 * 界面内容由脚本在运行时生成, 因此要还原界面必须先能读懂脚本.
 * 这里只做语法分析, 不执行: 解析结果用于统计覆盖率与后续的求值实现,
 * 因此可以先独立验证"能不能读懂"这件事.
 *
 * 支持的语法: 局部变量, 赋值, 函数定义与调用, 方法调用, if/elseif/else,
 * while, 数值 for, 表构造与索引, 算术/比较/逻辑/拼接运算符, 长注释与长字符串.
 * 不支持协程, goto, 元表与可变参数; 这些在界面脚本里没有出现.
 */

struct Node;
using NodePtr = std::shared_ptr<Node>;

/* 语法树节点: 单结构多形态, 避免为几十种子类各写一份访问器. */
struct Node {
    enum class Kind : uint8_t {
        kNil,
        kBool,
        kNumber,
        kString,
        kName,
        kIndex,
        kCall,
        kMethodCall,
        kBinary,
        kUnary,
        kTable,
        kFunction,
        /* 语句. */
        kLocal,
        kAssign,
        kIf,
        kWhile,
        kNumericFor,
        kReturn,
        kBreak,
        kDo,
        kCallStmt,
    };

    Kind kind = Kind::kNil;
    bool boolean = false;
    double number = 0.0;
    std::string text;
    std::string op;
    NodePtr left;
    NodePtr right;
    /* 列表: 实参, 数组项, 表达式序列. */
    std::vector<NodePtr> list;
    /* 表构造的键值对. */
    std::vector<std::pair<NodePtr, NodePtr>> fields;
    /* 块: if 的各分支, while 与 for 的循环体, 函数体. */
    std::vector<std::vector<NodePtr>> blocks;
    /* 名字列表: local 声明, 函数参数. */
    std::vector<std::string> names;
};

/* 解析统计: 供覆盖率统计与调试. */
struct ParseStats {
    int32_t statements = 0;
    int32_t expressions = 0;
    int32_t functions = 0;
    int32_t tables = 0;
    int32_t maxDepth = 0;
};

/*
 * 解析一份界面脚本.
 * 成功时返回整段语句列表并把统计写入 stats; 失败时返回空并给出错误信息.
 */
std::vector<NodePtr> ParseScript(const char* source, size_t length, ParseStats* stats,
                                 std::string* error);

}  /* namespace immirror::ui::script */
