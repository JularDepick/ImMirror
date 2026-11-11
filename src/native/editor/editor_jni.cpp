#include <jni.h>

#include <string>
#include <vector>

#include "core/log.h"
#include "editor/editor_command.h"
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
 * 编辑器统一命令入口: Java 侧的低频调用全部走这一条命令串.
 * 命令的解释与状态保存在 editor_command.cpp, 这里只做字符串搬运,
 * 任何情况下都不向 Java 抛异常, 失败以 code 与 message 字段回传.
 */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_editor_EditorBridge_nativeEditorCommand(JNIEnv* env, jclass clazz,
                                                                     jstring command) {
    (void)clazz;
    const std::string text = ToStdString(env, command);
    const std::string reply = immirror::editor::HandleEditorCommand(text);
    IM_LOGD(kTag, "editor command: %s", text.c_str());
    return ToJString(env, reply);
}

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