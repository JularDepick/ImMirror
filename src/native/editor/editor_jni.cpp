#include <jni.h>

#include <string>
#include <vector>

#include "core/log.h"
#include "editor/editor_session.h"
#include "editor/editor_types.h"
#include "editor/project_format.h"
#include "editor/trigger_model.h"

namespace {

constexpr const char* kTag = "ImMirrorEditorJni";

/* Java 侧 Activity 的全局引用与虚拟机句柄, 用于原生线程回调 Java. */
struct ActivityLink {
    JavaVM* vm = nullptr;
    jobject activity = nullptr;
    jmethodID exitMethod = nullptr;
};

ActivityLink g_activity;

/* 本地字符串转 jstring, 失败时返回空串而不是抛异常越过 JNI 边界. */
jstring ToJString(JNIEnv* env, const std::string& text) {
    return env->NewStringUTF(text.c_str());
}

std::string ToStdString(JNIEnv* env, jstring value) {
    if (value == nullptr) {
        return std::string();
    }
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (chars == nullptr) {
        return std::string();
    }
    std::string result(chars);
    env->ReleaseStringUTFChars(value, chars);
    return result;
}

/* 名称里的分隔符与换行替换掉, 保证 Java 侧按行与竖线切分不会错位. */
std::string SanitizeField(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        if (c == '|' || c == '\n' || c == '\r') {
            out.push_back('/');
        } else {
            out.push_back(c);
        }
    }
    return out;
}

/*
 * 原生线程回调 Java: 附加上线程, 调用 Activity 的退出方法, 再按需解绑.
 * 编辑器线程不是 Java 创建的, 因此必须先 GetEnv 判断是否需要附加.
 */
void ExitToJavaThunk(void* user) {
    (void)user;
    if (g_activity.vm == nullptr || g_activity.activity == nullptr ||
        g_activity.exitMethod == nullptr) {
        IM_LOGW(kTag, "exit callback missing activity link");
        return;
    }
    JNIEnv* env = nullptr;
    bool attached = false;
    const jint state = g_activity.vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (state == JNI_EDETACHED) {
        if (g_activity.vm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            IM_LOGE(kTag, "attach current thread failed");
            return;
        }
        attached = true;
    } else if (state != JNI_OK || env == nullptr) {
        IM_LOGE(kTag, "get jni env failed");
        return;
    }
    env->CallVoidMethod(g_activity.activity, g_activity.exitMethod);
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    if (attached) {
        g_activity.vm->DetachCurrentThread();
    }
    IM_LOGI(kTag, "exit callback delivered to java");
}

}  /* namespace */

extern "C" {

/*
 * 绑定当前 Activity: 保存虚拟机与 Activity 的全局引用, 并注册原生侧的退出回调.
 * 必须在进入编辑器之前调用一次, Activity 销毁时再解除.
 */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_editor_EditorBridge_nativeAttachActivity(JNIEnv* env, jclass clazz,
                                                                      jobject activity) {
    (void)clazz;
    if (g_activity.activity != nullptr) {
        env->DeleteGlobalRef(g_activity.activity);
        g_activity.activity = nullptr;
    }
    env->GetJavaVM(&g_activity.vm);
    if (activity != nullptr) {
        g_activity.activity = env->NewGlobalRef(activity);
        jclass activityClass = env->GetObjectClass(activity);
        if (activityClass != nullptr) {
            g_activity.exitMethod =
                env->GetMethodID(activityClass, "onEditorExitRequested", "()V");
            env->DeleteLocalRef(activityClass);
        }
    }
    immirror::editor::SetExitHandler(&ExitToJavaThunk, nullptr);
    IM_LOGI(kTag, "activity attached, exit method=%p",
            reinterpret_cast<void*>(g_activity.exitMethod));
}

/* 解除 Activity 绑定, 编辑器退出时调用. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_editor_EditorBridge_nativeDetachActivity(JNIEnv* env, jclass clazz) {
    (void)clazz;
    immirror::editor::ClearExitHandler();
    if (g_activity.activity != nullptr) {
        env->DeleteGlobalRef(g_activity.activity);
        g_activity.activity = nullptr;
    }
    g_activity.exitMethod = nullptr;
    IM_LOGI(kTag, "activity detached");
}

/* 登记待打开的工程; 真正加载发生在原生主循环表面就绪之后. */
JNIEXPORT jboolean JNICALL
Java_com_julardepick_immirror_editor_EditorBridge_nativeRequestProject(JNIEnv* env, jclass clazz,
                                                                     jstring projectDir,
                                                                     jstring dataPath) {
    (void)clazz;
    const std::string directory = ToStdString(env, projectDir);
    if (directory.empty()) {
        IM_LOGE(kTag, "request project without directory");
        return JNI_FALSE;
    }
    immirror::editor::RequestSession(directory, ToStdString(env, dataPath));
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_julardepick_immirror_editor_EditorBridge_nativeCancelProject(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    immirror::editor::CancelSession();
}

/* 工程根目录: <dataPath>/editor. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_editor_EditorBridge_nativeProjectRoot(JNIEnv* env, jclass clazz,
                                                                  jstring dataPath) {
    (void)clazz;
    return ToJString(env, immirror::editor::ProjectRoot(ToStdString(env, dataPath)));
}

/*
 * 工程列表, 每行一个工程: 目录|名称|种子|区块数|触发器数|更新时间.
 * 返回空串表示没有工程; 读取失败只记录日志, 不向 Java 抛异常.
 */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_editor_EditorBridge_nativeListProjects(JNIEnv* env, jclass clazz,
                                                                   jstring dataPath) {
    (void)clazz;
    const std::string root = immirror::editor::ProjectRoot(ToStdString(env, dataPath));
    std::vector<immirror::editor::ProjectSummary> projects;
    std::string error;
    if (!immirror::editor::ListProjects(root, projects, error)) {
        IM_LOGW(kTag, "list projects failed: %s", error.c_str());
        return ToJString(env, std::string());
    }
    std::string text;
    for (const auto& project : projects) {
        text += SanitizeField(project.directory);
        text += "|";
        text += SanitizeField(project.name);
        text += "|";
        text += std::to_string(project.seed);
        text += "|";
        text += std::to_string(project.chunkCount);
        text += "|";
        text += std::to_string(project.triggerCount);
        text += "|";
        text += SanitizeField(project.updatedAt);
        text += "\n";
    }
    return ToJString(env, text);
}

/* 新建工程, 返回工程目录; 失败返回空串. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_editor_EditorBridge_nativeCreateProject(JNIEnv* env, jclass clazz,
                                                                    jstring dataPath,
                                                                    jstring name, jint seed) {
    (void)clazz;
    const std::string root = immirror::editor::ProjectRoot(ToStdString(env, dataPath));
    std::string directory;
    std::string error;
    if (!immirror::editor::CreateProject(root, ToStdString(env, name),
                                         static_cast<uint32_t>(seed), directory, error)) {
        IM_LOGE(kTag, "create project failed: %s", error.c_str());
        return ToJString(env, std::string());
    }
    return ToJString(env, directory);
}

/* 删除工程目录; 只允许删除带 project.json 的目录. */
JNIEXPORT jboolean JNICALL
Java_com_julardepick_immirror_editor_EditorBridge_nativeDeleteProject(JNIEnv* env, jclass clazz,
                                                                    jstring directory) {
    (void)clazz;
    std::string error;
    const bool ok =
        immirror::editor::DeleteProject(ToStdString(env, directory), error);
    if (!ok) {
        IM_LOGW(kTag, "delete project failed: %s", error.c_str());
    }
    return ok ? JNI_TRUE : JNI_FALSE;
}

/*
 * 把工程导出成 IMWF 可玩世界.
 * 返回空串表示成功, 否则是错误文本, 供界面直接展示.
 */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_editor_EditorBridge_nativeExportProject(JNIEnv* env, jclass clazz,
                                                                    jstring directory,
                                                                    jstring targetPath) {
    (void)clazz;
    immirror::editor::Project project;
    std::string error;
    if (!project.Load(ToStdString(env, directory), error)) {
        return ToJString(env, error);
    }
    std::string target = ToStdString(env, targetPath);
    if (target.empty()) {
        target = immirror::editor::GameWorldPath(std::string(),
                                                 project.Name().empty() ? "project"
                                                                        : project.Name());
    }
    if (!project.ExportWorld(target, error)) {
        return ToJString(env, error);
    }
    return ToJString(env, std::string());
}

/* 工程详情: 首行 META, 之后每条触发器一行, 字段用竖线分隔. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_editor_EditorBridge_nativeDescribeProject(JNIEnv* env, jclass clazz,
                                                                      jstring directory) {
    (void)clazz;
    immirror::editor::Project project;
    std::string error;
    if (!project.Load(ToStdString(env, directory), error)) {
        IM_LOGW(kTag, "describe project failed: %s", error.c_str());
        return ToJString(env, std::string());
    }
    std::string text;
    text += "META|";
    text += SanitizeField(project.Name());
    text += "|";
    text += std::to_string(project.Seed());
    text += "|";
    text += std::to_string(project.ChunkCount());
    text += "|";
    text += std::to_string(static_cast<int32_t>(project.Triggers().size()));
    text += "|";
    text += SanitizeField(project.Meta().updatedAt);
    text += "\n";
    for (const auto& trigger : project.Triggers()) {
        text += "TRIGGER|";
        text += std::to_string(trigger.id);
        text += "|";
        text += SanitizeField(trigger.name);
        text += "|";
        text += immirror::editor::TriggerEventNameZh(trigger.eventType);
        text += "|";
        text += std::to_string(static_cast<int32_t>(trigger.actions.size()));
        text += "|";
        text += trigger.enabled ? "1" : "0";
        text += "\n";
    }
    return ToJString(env, text);
}


/* 命令状态: dataPath, 当前工程目录, 工具, 方块键, 光标. 进程内单会话, 只在编辑器线程访问. */
namespace {

std::string gEditorDataPath;
std::string gEditorProjectDir;
std::string gEditorStatusExtra;

}  /* namespace */

/*
 * 统一命令入口. Java 侧 EditorBridge.command() 把全部操作折叠成一行文本,
 * 这里按动词分派到上面的细粒度逻辑, 返回 EditorReply 兼容的 key=value 行.
 * 未知动词返回 ok=false 带原因, 不抛异常不崩溃.
 */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_editor_EditorBridge_nativeEditorCommand(JNIEnv* env, jclass clazz,
                                                                      jstring command) {
    (void)clazz;
    const std::string line = ToStdString(env, command);
    const size_t space = line.find(' ');
    const std::string verb = space == std::string::npos ? line : line.substr(0, space);
    const std::string argument = space == std::string::npos ? std::string() : line.substr(space + 1);
    std::string reply = "ok=true\n";

    if (verb == "init") {
        gEditorDataPath = argument;
        gEditorStatusExtra = "dataPath set";
    } else if (verb == "status") {
        reply = "ok=true\nproject=" + (gEditorProjectDir.empty() ? std::string("(none)")
                                                                 : gEditorProjectDir) + "\n";
        reply += "data=" + (gEditorDataPath.empty() ? std::string("(none)") : gEditorDataPath) + "\n";
        if (!gEditorStatusExtra.empty()) {
            reply += "extra=" + gEditorStatusExtra + "\n";
        }
    } else if (verb == "projects") {
        const std::string root = immirror::editor::ProjectRoot(gEditorDataPath);
        std::vector<immirror::editor::ProjectSummary> projects;
        std::string error;
        if (!immirror::editor::ListProjects(root, projects, error)) {
            reply = "ok=false\nerror=" + SanitizeField(error) + "\n";
        } else {
            reply = "ok=true\ncount=" + std::to_string(projects.size()) + "\n";
            for (const auto& project : projects) {
                reply += "project|" + SanitizeField(project.directory) + "|" + SanitizeField(project.name)
                       + "|" + std::to_string(project.triggerCount) + "\n";
            }
        }
    } else if (verb == "new") {
        std::string directory;
        std::string error;
        if (!immirror::editor::CreateProject(gEditorDataPath, argument, 0u, directory, error)) {
            reply = "ok=false\nerror=" + SanitizeField(error) + "\n";
        } else {
            gEditorProjectDir = directory;
            reply = "ok=true\ndirectory=" + SanitizeField(directory) + "\nname=" + SanitizeField(argument) + "\n";
        }
    } else if (verb == "open") {
        immirror::editor::Project project;
        std::string error;
        if (!project.Load(argument, error)) {
            reply = "ok=false\nerror=" + SanitizeField(error) + "\n";
        } else {
            gEditorProjectDir = argument;
            reply = "ok=true\nname=" + SanitizeField(project.Name()) + "\nseed="
                  + std::to_string(project.Seed()) + "\nchunks=" + std::to_string(project.ChunkCount())
                  + "\ntriggers=" + std::to_string(project.Triggers().size()) + "\n";
        }
    } else if (verb == "save") {
        if (gEditorProjectDir.empty()) {
            reply = "ok=false\nerror=no project\n";
        } else {
            immirror::editor::Project project;
            std::string error;
            if (!project.Load(gEditorProjectDir, error) || !project.Save(error)) {
                reply = "ok=false\nerror=" + SanitizeField(error) + "\n";
            }
        }
    } else if (verb == "tool" || verb == "block" || verb == "cursor" || verb == "apply"
               || verb == "sel" || verb == "fill") {
        /* 工具与画布操作: 世界侧暂未接入, 只记录参数供后续接线, 返回成功. */
        gEditorStatusExtra = verb + " " + argument;
    } else {
        reply = "ok=false\nerror=unknown command " + SanitizeField(verb) + "\n";
    }
    return ToJString(env, reply);
}

/* 导出到游戏存档目录时使用的路径, 供 Java 侧显示. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_editor_EditorBridge_nativeGameWorldPath(JNIEnv* env, jclass clazz,
                                                                    jstring dataPath,
                                                                    jstring projectName) {
    (void)clazz;
    return ToJString(env, immirror::editor::GameWorldPath(ToStdString(env, dataPath),
                                                          ToStdString(env, projectName)));
}

}  /* extern "C" */