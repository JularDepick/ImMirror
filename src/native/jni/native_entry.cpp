#include <jni.h>

#include <string>

#include <android_native_app_glue.h>

#include "core/log.h"
#include "audio/audio_engine.h"
#include "jni/engine_host.h"
#include "platform/android_runtime.h"

namespace {

constexpr const char* kTag = "ImMirrorJni";

/*
 * 安全地把 Java 字符串转成本地副本.
 * 空引用与编码失败都退化为空串, 保证不会有异常跨过 JNI 边界.
 */
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

/*
 * Java 侧低频入口: 进入游戏前下发设备画像文本.
 * 该调用发生在 Java 主线程, 只做一次拷贝, 不触碰渲染状态.
 */
JNIEXPORT void JNICALL Java_com_julardepick_immirror_bridge_NativeBridge_nativeOnDeviceProfile(
    JNIEnv* env, jclass clazz, jstring payload) {
    (void)clazz;
    const std::string text = ToStdString(env, payload);
    immirror::jni::EngineHost::Instance().SetDeviceProfile(text);
    IM_LOGI(kTag, "nativeOnDeviceProfile bytes=%zu", text.size());
}

/* Java 侧低频入口: 请求原生主循环进入游戏状态. */
JNIEXPORT void JNICALL Java_com_julardepick_immirror_bridge_NativeBridge_nativeStart(JNIEnv* env,
                                                                                    jclass clazz) {
    (void)env;
    (void)clazz;
    immirror::jni::EngineHost::Instance().RequestStart();
}

/* Java 侧低频入口: 请求原生主循环退出, 由胶水层完成线程回收. */
JNIEXPORT void JNICALL Java_com_julardepick_immirror_bridge_NativeBridge_nativeStop(JNIEnv* env,
                                                                                   jclass clazz) {
    (void)env;
    (void)clazz;
    immirror::jni::EngineHost::Instance().RequestStop();
}

/* Java 侧低频入口: 把音效池与音效句柄推给原生侧. */
JNIEXPORT void JNICALL Java_com_julardepick_immirror_bridge_NativeBridge_nativeAttachAudio(
    JNIEnv* env, jclass clazz, jobject soundPool, jintArray soundIds) {
    (void)clazz;
    immirror::audio::AudioEngine::Instance().Attach(env, soundPool, soundIds);
}

/* Java 侧低频入口: Activity 销毁前解除音效引用. */
JNIEXPORT void JNICALL Java_com_julardepick_immirror_bridge_NativeBridge_nativeDetachAudio(
    JNIEnv* env, jclass clazz) {
    (void)clazz;
    immirror::audio::AudioEngine::Instance().Detach(env);
}

}  /* extern "C" */

/*
 * 胶水层要求的应用入口. 由 android_native_app_glue 在独立线程上调用,
 * 每次 Activity 重建都会重新进入一次.
 */
void android_main(struct android_app* app) {
    immirror::platform::RunApplication(app);
}