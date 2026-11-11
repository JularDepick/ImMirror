#include <jni.h>

#include <string>

#include "core/log.h"
#include "progress/progress_hub.h"

/*
 * 成就与任务模块的 JNI 入口.
 *
 * 界面需要的进度全部来自原生侧的 ProgressHub: 成就页打开时取一次快照文本,
 * 签到成功后回报一次事件, 离开页面时落盘. 全部是低频调用,
 * 逐帧路径仍然只走 C++ 主循环, 不经过这里.
 *
 * 符号名对应 com.julardepick.immirror.progress.NativeProgress.
 */

namespace {

constexpr const char* kTag = "ImMirrorProgressJni";

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

}  /* namespace */

extern "C" {

/* 指定应用内部存储目录并载入进度; 重复调用幂等. */
JNIEXPORT jboolean JNICALL
Java_com_julardepick_immirror_progress_NativeProgress_nativeInit(JNIEnv* env, jclass clazz,
                                                                 jstring dataPath) {
    (void)clazz;
    const std::string path = ToStdString(env, dataPath);
    const bool ready = immirror::progress::ProgressHub::Instance().Initialize(path.c_str());
    IM_LOGI(kTag, "nativeInit ready=%d bytes=%zu", ready ? 1 : 0, path.size());
    return ready ? JNI_TRUE : JNI_FALSE;
}

/* 签到成功后调用, 推进签到类每日任务. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_progress_NativeProgress_nativeCheckin(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    immirror::progress::ProgressHub::Instance().OnCheckin();
}

/* 立即落盘, 界面离开前调用. */
JNIEXPORT jboolean JNICALL
Java_com_julardepick_immirror_progress_NativeProgress_nativeFlush(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    immirror::progress::ProgressHub::Instance().Flush();
    return JNI_TRUE;
}

/* 导出界面快照文本, 制表符分隔; 未初始化时返回空串. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_progress_NativeProgress_nativeSnapshot(JNIEnv* env, jclass clazz) {
    (void)clazz;
    const std::string snapshot = immirror::progress::ProgressHub::Instance().ExportSnapshot();
    return env->NewStringUTF(snapshot.c_str());
}

}  /* extern "C" */
