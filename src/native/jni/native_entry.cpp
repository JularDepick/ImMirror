#include <jni.h>

#include <string>

#include <android_native_app_glue.h>

#include "core/log.h"
#include "audio/audio_engine.h"
#include "game/game_shell.h"
#include "jni/engine_host.h"
#include "platform/android_runtime.h"
#include "settings/settings.h"

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

/* 把本地字符串回传给 Java; 只用于设置读取这类低频调用, 不进入逐帧路径. */
jstring ToJString(JNIEnv* env, const std::string& text) {
    return env->NewStringUTF(text.c_str());
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

/* Java 侧低频入口: 取走成就统计增量并清零. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeTakeProgressCounters(JNIEnv* env, jclass clazz) {
    (void)clazz;
    std::string json = "{}";
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell != nullptr) {
        const auto counters = shell->TakeProgressCounters();
        json = "{\"break_block\":" + std::to_string(counters.breakBlocks)
             + ",\"place_block\":" + std::to_string(counters.placeBlocks)
             + ",\"pick_item\":" + std::to_string(counters.pickedItems) + "}";
    }
    return env->NewStringUTF(json.c_str());
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

/*
 * 设置系统入口. 全部是低频调用: 设置界面打开时批量读取, 用户改动时逐项写入,
 * 逐帧路径只读原生侧的原子快照, 不经过这里.
 * 返回 -1 表示键名未知, 正常取值都不小于 0.
 */

/* 指定应用内部存储目录并载入设置; 重复调用幂等. */
JNIEXPORT jboolean JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeSettingsInit(JNIEnv* env, jclass clazz,
                                                                    jstring dataPath) {
    (void)clazz;
    const std::string path = ToStdString(env, dataPath);
    const bool ok = immirror::settings::Settings::Instance().Initialize(path.c_str());
    IM_LOGI(kTag, "nativeSettingsInit ok=%d bytes=%zu", ok ? 1 : 0, path.size());
    return ok ? JNI_TRUE : JNI_FALSE;
}

/* 立即落盘, 供 Activity 进入后台时调用. */
JNIEXPORT jboolean JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeSettingsFlush(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    return immirror::settings::Settings::Instance().Save() ? JNI_TRUE : JNI_FALSE;
}

/* 按落盘键名读取单项设置. */
JNIEXPORT jint JNICALL Java_com_julardepick_immirror_bridge_NativeBridge_nativeSettingsGetInt(
    JNIEnv* env, jclass clazz, jstring key) {
    (void)clazz;
    const std::string name = ToStdString(env, key);
    return immirror::settings::Settings::Instance().GetByName(name.c_str(), -1);
}

/* 写入单项设置, 返回裁剪后的落定值. */
JNIEXPORT jint JNICALL Java_com_julardepick_immirror_bridge_NativeBridge_nativeSettingsSetInt(
    JNIEnv* env, jclass clazz, jstring key, jint value) {
    (void)clazz;
    const std::string name = ToStdString(env, key);
    return immirror::settings::Settings::Instance().SetByName(name.c_str(), value);
}

/* 全量导出 "键=值" 多行文本, 设置界面一次读完所有项. */
JNIEXPORT jstring JNICALL Java_com_julardepick_immirror_bridge_NativeBridge_nativeSettingsExport(
    JNIEnv* env, jclass clazz) {
    (void)clazz;
    return ToJString(env, immirror::settings::Settings::Instance().Export());
}

/* 导出键表元数据, 每行 "键|默认值|最小值|最大值", 供设置界面生成控件. */
JNIEXPORT jstring JNICALL Java_com_julardepick_immirror_bridge_NativeBridge_nativeSettingsDescribe(
    JNIEnv* env, jclass clazz) {
    (void)clazz;
    return ToJString(env, immirror::settings::Settings::Instance().Describe());
}

/* 恢复单项默认值; 键名为空串时恢复全部. */
JNIEXPORT jboolean JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeSettingsReset(JNIEnv* env, jclass clazz,
                                                                      jstring key) {
    (void)clazz;
    const std::string name = ToStdString(env, key);
    auto& settings = immirror::settings::Settings::Instance();
    if (name.empty()) {
        settings.ResetAll();
        return JNI_TRUE;
    }
    const immirror::settings::KeyInfo* info = immirror::settings::FindKey(name.c_str());
    if (info == nullptr) {
        IM_LOGW(kTag, "settings reset ignored, unknown key=%s", name.c_str());
        return JNI_FALSE;
    }
    settings.Reset(info->key);
    return JNI_TRUE;
}

}  /* extern "C" */

/*
 * 胶水层要求的应用入口. 由 android_native_app_glue 在独立线程上调用,
 * 每次 Activity 重建都会重新进入一次.
 */
void android_main(struct android_app* app) {
    immirror::platform::RunApplication(app);
}