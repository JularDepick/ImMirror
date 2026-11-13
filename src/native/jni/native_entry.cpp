#include <jni.h>

#include <string>

#include <android_native_app_glue.h>

#include "core/log.h"
#include "audio/audio_engine.h"
#include "game/game_shell.h"
#include "world/world.h"
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

/* Java 侧低频入口: 试玩模式选择世界存档; 名字对应编辑器导出的文件. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeSelectWorld(JNIEnv* env, jclass clazz,
                                                                    jstring projectName) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr || projectName == nullptr) {
        return;
    }
    const char* chars = env->GetStringUTFChars(projectName, nullptr);
    const std::string name = chars != nullptr ? chars : "";
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(projectName, chars);
    }
    if (!name.empty()) {
        shell->LoadNamedWorld(name);
    }
}

/* Java 侧低频入口: 世界列表, 扫描内部存储 worlds 目录下的全部存档头. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeListWorlds(JNIEnv* env, jclass clazz,
                                                                   jstring directory) {
    (void)clazz;
    if (directory == nullptr) {
        return env->NewStringUTF("[]");
    }
    const char* chars = env->GetStringUTFChars(directory, nullptr);
    const std::string dir = chars != nullptr ? chars : "";
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(directory, chars);
    }
    /*
     * 列表读取只碰文件头, 不依赖游戏运行时: 大厅与冷启动时原生主循环尚未接管,
     * 走 Shell 实例会拿不到对象而返回空列表, 因此直接调用世界模块的静态实现.
     */
    const std::string listed = immirror::world::ListWorldsJson(dir.c_str());
    return env->NewStringUTF(listed.c_str());
}

/*
 * Java 侧低频入口: 选择游玩的世界 (多世界语义, 与编辑器试玩的 nativeSelectWorld 区分):
 * 切换存档路径并载入; 文件不存在时按新世界处理.
 */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeChooseWorld(JNIEnv* env, jclass clazz,
                                                                    jstring worldName) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr || worldName == nullptr) {
        return;
    }
    const char* chars = env->GetStringUTFChars(worldName, nullptr);
    const std::string name = chars != nullptr ? chars : "";
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(worldName, chars);
    }
    if (!name.empty()) {
        shell->SelectWorld(name);
    }
}

/*
 * Java 侧低频入口: 按指定种子选择世界, 供联机房间让同一房间的各方生成同一份地形.
 * 种子为 0 时退回按世界名派生, 与 nativeChooseWorld 行为一致.
 */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeChooseWorldSeed(JNIEnv* env, jclass clazz,
                                                                       jstring worldName,
                                                                       jint seed) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr || worldName == nullptr) {
        return;
    }
    const char* chars = env->GetStringUTFChars(worldName, nullptr);
    const std::string name = chars != nullptr ? chars : "";
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(worldName, chars);
    }
    if (!name.empty()) {
        shell->SelectWorld(name, static_cast<uint32_t>(seed));
    }
}

/* Java 侧低频入口: 取当前世界时间总秒数, 供房主同步给房间. */
JNIEXPORT jfloat JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeWorldTimeSeconds(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return 0.0f;
    }
    return static_cast<jfloat>(shell->WorldTimeSeconds());
}

/* Java 侧低频入口: 按房主时间对齐昼夜, 客人侧使用. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeSetWorldTimeSeconds(JNIEnv* env,
                                                                          jclass clazz,
                                                                          jfloat seconds) {
    (void)env;
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return;
    }
    shell->SetWorldTimeSeconds(static_cast<float>(seconds));
}

/* Java 侧低频入口: 取当前天气编号, 供房主同步给房间. */
JNIEXPORT jint JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeWeatherKind(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return 0;
    }
    return static_cast<jint>(shell->WeatherKindCode());
}

/* Java 侧低频入口: 取当前天气强度 0 到 1. */
JNIEXPORT jfloat JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeWeatherIntensity(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return 0.0f;
    }
    return static_cast<jfloat>(shell->WeatherIntensity());
}

/* Java 侧低频入口: 按房主的天气设定本地天气, 客人侧使用. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeSetWeatherState(JNIEnv* env, jclass clazz,
                                                                       jint kind,
                                                                       jfloat intensity) {
    (void)env;
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return;
    }
    shell->SetWeatherState(static_cast<int32_t>(kind), static_cast<float>(intensity));
}

/* Java 侧低频入口: 取走客人侧的拾取申报, 逗号分隔的掉落物标识. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeTakePickupRequests(JNIEnv* env,
                                                                         jclass clazz) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return env->NewStringUTF("");
    }
    return env->NewStringUTF(shell->TakePickupRequests().c_str());
}

/* Java 侧低频入口: 服务端裁定后把结果交回原生. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeApplyPickupResult(JNIEnv* env, jclass clazz,
                                                                        jint dropId,
                                                                        jint blockId) {
    (void)env;
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return;
    }
    shell->ApplyPickupResult(static_cast<int32_t>(dropId), static_cast<int32_t>(blockId));
}

/* Java 侧低频入口: 当前是否潜水, 供环境音压减. */
JNIEXPORT jboolean JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeIsUnderwater(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return JNI_FALSE;
    }
    return shell->IsUnderwater() ? JNI_TRUE : JNI_FALSE;
}

/* Java 侧低频入口: 由界面层给一条 HUD 提示, 文案为拉丁字母. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeShowTip(JNIEnv* env, jclass clazz,
                                                               jstring text) {
    (void)clazz;
    if (text == nullptr) {
        return;
    }
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return;
    }
    const char* utf = env->GetStringUTFChars(text, nullptr);
    if (utf == nullptr) {
        return;
    }
    shell->ShowTipFromUi(utf);
    env->ReleaseStringUTFChars(text, utf);
}

/* Java 侧低频入口: 设置房间客人模式, 关闭本地自然生成. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeSetGuestMode(JNIEnv* env, jclass clazz,
                                                                    jboolean guest) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return;
    }
    shell->SetGuestMode(guest == JNI_TRUE);
}

/* Java 侧低频入口: 切换暂停菜单, 由返回键触发. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeTogglePauseMenu(JNIEnv* env,
                                                                       jclass clazz) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return;
    }
    shell->TogglePauseMenu();
}

/* Java 侧低频入口: 取走原生界面事件队列, 编码用逗号分隔. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeTakeUiEvents(JNIEnv* env, jclass clazz) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return env->NewStringUTF("");
    }
    return env->NewStringUTF(shell->TakeUiEvents().c_str());
}

/* Java 侧低频入口: 导出当前熔炉内容快照, 供房主上行到房间. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeTakeFurnaceSnapshot(JNIEnv* env,
                                                                           jclass clazz) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return env->NewStringUTF("[]");
    }
    return env->NewStringUTF(shell->TakeFurnaceSnapshot().c_str());
}

/* Java 侧低频入口: 应用房主下发的熔炉内容. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeApplyRemoteFurnaces(JNIEnv* env,
                                                                          jclass clazz,
                                                                          jstring json) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr || json == nullptr) {
        return;
    }
    const char* chars = env->GetStringUTFChars(json, nullptr);
    const std::string text = chars != nullptr ? chars : "[]";
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(json, chars);
    }
    shell->ApplyRemoteFurnaces(text);
}

/* Java 侧低频入口: 导出当前储物箱内容快照, 供房主上行到房间. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeTakeChestSnapshot(JNIEnv* env,
                                                                         jclass clazz) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return env->NewStringUTF("[]");
    }
    return env->NewStringUTF(shell->TakeChestSnapshot().c_str());
}

/* Java 侧低频入口: 应用房主下发的储物箱内容. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeApplyRemoteChests(JNIEnv* env, jclass clazz,
                                                                        jstring json) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr || json == nullptr) {
        return;
    }
    const char* chars = env->GetStringUTFChars(json, nullptr);
    const std::string text = chars != nullptr ? chars : "[]";
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(json, chars);
    }
    shell->ApplyRemoteChests(text);
}

/* Java 侧低频入口: 导出当前地面掉落物快照, 供房主上行到房间. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeTakeDropSnapshot(JNIEnv* env,
                                                                        jclass clazz) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return env->NewStringUTF("[]");
    }
    return env->NewStringUTF(shell->TakeDropSnapshot().c_str());
}

/* Java 侧低频入口: 应用房主下发的掉落物快照. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeApplyRemoteDrops(JNIEnv* env, jclass clazz,
                                                                       jstring json) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr || json == nullptr) {
        return;
    }
    const char* chars = env->GetStringUTFChars(json, nullptr);
    const std::string text = chars != nullptr ? chars : "[]";
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(json, chars);
    }
    shell->ApplyRemoteDrops(text);
}

/* Java 侧低频入口: 导出当前生物快照, 供房主上行到房间. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeTakeCreatureSnapshot(JNIEnv* env,
                                                                           jclass clazz) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return env->NewStringUTF("[]");
    }
    return env->NewStringUTF(shell->TakeCreatureSnapshot().c_str());
}

/* Java 侧低频入口: 应用房主下发的远端生物快照. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeApplyRemoteCreatures(JNIEnv* env,
                                                                          jclass clazz,
                                                                          jstring json) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr || json == nullptr) {
        return;
    }
    const char* chars = env->GetStringUTFChars(json, nullptr);
    const std::string text = chars != nullptr ? chars : "[]";
    if (chars != nullptr) {
        env->ReleaseStringUTFChars(json, chars);
    }
    shell->ApplyRemoteCreatures(text);
}

/*
 * Java 侧低频入口: 查询当前是否为游戏内白天.
 * 环境音按日相切换曲目, 由 PlayActivity 的轮询周期顺带拉取;
 * Shell 未装配时回落 true, 避免误切到夜曲.
 */
JNIEXPORT jboolean JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeGetDayPhase(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return JNI_TRUE;
    }
    return shell->IsDay() ? JNI_TRUE : JNI_FALSE;
}

/*
 * Java 侧低频入口: 查询环境音场景 (0 通用, 1 雨, 2 雷暴, 3 水下, 4 岩浆).
 * 与日相同一批轮询, 由环境音管理器按场景切分组.
 */
JNIEXPORT jint JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeGetAmbientContext(JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return 0;
    }
    return static_cast<jint>(shell->AmbientContext());
}

/* Java 侧低频入口: 取玩家姿态 JSON, 供联机 presence 轮询. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeTakePlayerPose(JNIEnv* env, jclass clazz) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return env->NewStringUTF("{}");
    }
    return env->NewStringUTF(shell->FormatPlayerPose().c_str());
}

/* Java 侧低频入口: 取走本地方块改动 JSON, 供联机同步上报. */
JNIEXPORT jstring JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeTakeBlockEdits(JNIEnv* env, jclass clazz) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr) {
        return env->NewStringUTF("[]");
    }
    return env->NewStringUTF(shell->TakeBlockEditsJson().c_str());
}

/* Java 侧低频入口: 应用远端方块改动 JSON. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeApplyBlockEdits(JNIEnv* env, jclass clazz,
                                                                       jstring json) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr || json == nullptr) {
        return;
    }
    const char* chars = env->GetStringUTFChars(json, nullptr);
    if (chars != nullptr) {
        shell->ApplyRemoteBlockEdits(chars);
        env->ReleaseStringUTFChars(json, chars);
    }
}

/* Java 侧低频入口: 接收其他玩家快照 JSON. */
JNIEXPORT void JNICALL
Java_com_julardepick_immirror_bridge_NativeBridge_nativeApplyRemotePlayers(JNIEnv* env, jclass clazz,
                                                                          jstring json) {
    (void)clazz;
    auto* shell = immirror::platform::RuntimeHostShell();
    if (shell == nullptr || json == nullptr) {
        return;
    }
    const char* chars = env->GetStringUTFChars(json, nullptr);
    if (chars != nullptr) {
        shell->ApplyRemotePlayers(chars);
        env->ReleaseStringUTFChars(json, chars);
    }
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
             + ",\"pick_item\":" + std::to_string(counters.pickedItems)
             + ",\"craft_item\":" + std::to_string(counters.crafts)
             + ",\"smelt_item\":" + std::to_string(counters.smelted)
             + ",\"kill_creature\":" + std::to_string(counters.kills) + "}";
    }
    return env->NewStringUTF(json.c_str());
}

/* Java 侧低频入口: 把音效池与分族变体句柄推给原生侧. */
JNIEXPORT void JNICALL Java_com_julardepick_immirror_bridge_NativeBridge_nativeAttachAudioEx(
    JNIEnv* env, jclass clazz, jobject soundPool, jintArray soundIds, jintArray familyCounts) {
    (void)clazz;
    immirror::audio::AudioEngine::Instance().Attach(env, soundPool, soundIds, familyCounts);
}

/* Java 侧低频入口: 把生物音效句柄银行推给原生侧. */
JNIEXPORT void JNICALL Java_com_julardepick_immirror_bridge_NativeBridge_nativeAttachCreatureSound(
    JNIEnv* env, jclass clazz, jobject bank, jobject soundPool) {
    (void)clazz;
    immirror::audio::AudioEngine::Instance().AttachCreatureBank(env, bank, soundPool);
}

/* Java 侧低频入口: Activity 销毁前解除音效引用. */
JNIEXPORT void JNICALL Java_com_julardepick_immirror_bridge_NativeBridge_nativeDetachAudio(
    JNIEnv* env, jclass clazz) {
    (void)clazz;
    immirror::audio::AudioEngine::Instance().DetachCreatureBank(env);
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