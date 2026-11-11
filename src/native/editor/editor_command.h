#pragma once

#include <string>

namespace immirror::editor {

/*
 * 编辑器统一命令入口: Java 侧的低频调用全部走这一条命令串.
 * 命令串以首个空白分隔动词与参数, 动词不区分大小写; 返回多行
 * "key=value" 文本, 首行恒为 code=..., 格式与 Java 侧 EditorReply.parse
 * 的消费方式一致. 全部命令与字段见 editor/COMMANDS.md.
 *
 * 状态存储: 编辑器状态保存在本翻译单元的进程内单例 (见 editor_command.cpp),
 * 工程对象由命令层持有, 与编辑器外壳 (EditorShell) 的世界内编辑互不共享.
 * 全部命令由 Java 主线程串行调用, 因此单例不加锁.
 */
std::string HandleEditorCommand(const std::string& command);

/* 测试辅助: 清空命令层全部状态, 回到未初始化的样子; 只在测试里使用. */
void ResetEditorCommandStateForTest();

}  /* namespace immirror::editor */
