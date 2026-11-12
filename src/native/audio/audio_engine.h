#pragma once

#include <jni.h>

#include <cstdint>
#include <mutex>

namespace immirror::audio {

/* 方块交互音效, 按材质分族. 顺序与 Java 侧加载顺序一致. */
enum class SoundId {
    kDigGrass = 0,
    kDigStone,
    kDigSand,
    kDigWood,
    kDigSnow,
    kDigGravel,
    kDigCloth,
    kDigGlass,
    kDigJar,
    kPlaceGrass,
    kPlaceStone,
    kPlaceSand,
    kPlaceWood,
    kPlaceSnow,
    kPlaceGravel,
    kPlaceCloth,
    kWalkGrass,
    kWalkStone,
    kWalkSand,
    kWalkWood,
    kWalkSnow,
    kWalkGravel,
    kWalkCloth,
    kWalkLadder,
    kCount,
};

/*
 * 每族音效的变体上限: Java 侧为 dig 与 walk 族各加载多个变体,
 * 原生侧按族保存句柄数组, 播放时随机取一个.
 */
constexpr int kSoundVariants = 4;

/*
 * 音效播放. 音频输出交给平台层的 SoundPool, 由 Java 侧在 Activity 创建时
 * 加载安装包内的 ogg 资源并通过 JNI 把句柄推给引擎. 播放属于事件触发的低频调用,
 * 不进入逐帧路径.
 */
class AudioEngine {
public:
    /* 进程内唯一实例, 供 JNI 入口与游戏层共用. */
    static AudioEngine& Instance();

    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    /*
     * Java 侧推送 SoundPool 与音效句柄; 内部保存全局引用.
     * soundIds 按族顺序平铺, 每族最多 kSoundVariants 个变体;
     * familyCounts 给出各族实际拥有的变体数, 计数缺省或为 0 时按单文件族处理.
     */
    void Attach(JNIEnv* env, jobject soundPool, jintArray soundIds, jintArray familyCounts);

    /* Activity 销毁前释放全局引用. */
    void Detach(JNIEnv* env);

    bool Ready() const;

    /*
     * 音量与开关由设置层下发. 音量取 0 到 1 的线性系数,
     * 音效开关关闭时 Play 直接返回, 不再进入 JNI 调用.
     * 音乐与语音通道当前还没有音源, 开关先保存下来供后续接入.
     */
    void SetMasterVolume(float volume);
    void SetSoundEnabled(bool enabled);
    void SetMusicEnabled(bool enabled);
    void SetVoiceEnabled(bool enabled);

    float MasterVolume() const { return volume_; }
    bool SoundEnabled() const { return soundEnabled_; }
    bool MusicEnabled() const { return musicEnabled_; }
    bool VoiceEnabled() const { return voiceEnabled_; }

    void Play(SoundId id);

    /*
     * 生物音效: 句柄由 Java 侧的句柄银行按资源路径懒加载, 这里只保存银行的
     * 全局引用与取句柄的方法, 播放走与方块音效相同的 SoundPool 通道.
     * 生物音效种类多 (每种生物四类动作), 因此不预加载, 逐次按路径取句柄.
     */
    void AttachCreatureBank(JNIEnv* env, jobject bank, jobject soundPool);
    void DetachCreatureBank(JNIEnv* env);

    /* 按资源路径播放一次生物音效; 句柄为 0 或路径为空时静默跳过. */
    void PlayCreatureAsset(const char* assetPath);

private:
    /* 播放需要 JNIEnv, 原生线程首次使用时附加到虚拟机. */
    JNIEnv* CurrentEnv();

    /*
     * 挂接与解除在 Java 主线程, 播放与就绪判定在原生线程:
     * 两边同时发生会读到已经释放的全局引用, 因此共用同一把锁.
     */
    mutable std::mutex mutex_;

    JavaVM* vm_ = nullptr;
    jobject soundPool_ = nullptr;
    /* 同族多变体的句柄表: 第一维是 SoundId, 第二维是族内变体, counts 记录实际数量. */
    jint soundIds_[static_cast<int>(SoundId::kCount)][kSoundVariants] = {{0}};
    int32_t variantCounts_[static_cast<int>(SoundId::kCount)] = {0};
    jmethodID playMethod_ = nullptr;
    /* 生物音效句柄银行: Java 侧按资源路径懒加载, 原生只调它的取句柄方法. */
    /* 静态方法 handleFor 的宿主是类本身, 因此保存的是 jclass 而不是实例. */
    jclass creatureBankClass_ = nullptr;
    jmethodID creatureHandleMethod_ = nullptr;
    int32_t attachedCount_ = 0;
    /* 播放选变体用, 只在持锁的播放与挂接路径触碰. */
    uint32_t randomState_ = 0x20261101u;
    float volume_ = 1.0f;         /* 主音量, 0 到 1 */
    bool soundEnabled_ = true;    /* 音效开关 */
    bool musicEnabled_ = true;    /* 音乐开关, 暂无音源 */
    bool voiceEnabled_ = true;    /* 语音开关, 暂无音源 */
};

}  /* namespace immirror::audio */