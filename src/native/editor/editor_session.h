#pragma once

#include <string>

namespace immirror::editor {

/*
 * Java 侧与原生主线程之间的编辑会话请求.
 * Java 在启动原生主循环之前登记工程目录, 原生侧表面就绪时取走并接管;
 * 取走即清空, 因此每次进入编辑器都是一次新的会话.
 */
struct SessionRequest {
    std::string projectDirectory; /* 要打开的工程目录 */
    std::string dataPath;         /* 应用内部存储目录 */
    bool valid = false;
};

/* 登记一条待处理的会话请求, 后登记的覆盖先登记的. */
void RequestSession(const std::string& projectDirectory, const std::string& dataPath);

/* 取走请求并清空; 没有请求时返回 false. */
bool TakeSession(SessionRequest& out);

/* 放弃待处理请求, 用于 Java 侧取消进入编辑器. */
void CancelSession();

/* 是否存在待处理请求. */
bool HasPendingSession();

/* 工程根目录: <dataPath>/editor, 每个工程是其中一子目录. */
std::string ProjectRoot(const std::string& dataPath);

/* 导出到游戏存档目录时可用的路径: <dataPath>/worlds/<stem>.world. */
std::string GameWorldPath(const std::string& dataPath, const std::string& projectName);

/*
 * 原生侧请求回到 Java 界面时使用的回调.
 * JNI 层在进入编辑器时注册, 编辑器自身不直接触碰 JNI 与 Activity.
 */
using ExitHandler = void (*)(void* user);

void SetExitHandler(ExitHandler handler, void* user);
void ClearExitHandler();

/* 请求 Java 侧结束当前编辑界面. */
void RequestExitToJava();

}  /* namespace immirror::editor */
