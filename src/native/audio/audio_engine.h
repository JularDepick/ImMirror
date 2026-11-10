#pragma once

#include <jni.h>

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
    kPlaceGrass,
    kPlaceStone,
    kPlaceSand,
    kPlaceWood,
    kPlaceSnow,
    kPlaceGravel,
    kCount,
};

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

    /* Java 侧推送 SoundPool 与音效句柄; 内部保存全局引用. */
    void Attach(JNIEnv* env, jobject soundPool, jintArray soundIds);

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
    jint soundIds_[static_cast<int>(SoundId::kCount)] = {0};
    jmethodID playMethod_ = nullptr;
    int32_t attachedCount_ = 0;
    float volume_ = 1.0f;         /* 主音量, 0 到 1 */
    bool soundEnabled_ = true;    /* 音效开关 */
    bool musicEnabled_ = true;    /* 音乐开关, 暂无音源 */
    bool voiceEnabled_ = true;    /* 语音开关, 暂无音源 */
};

}  /* namespace immirror::audio */
