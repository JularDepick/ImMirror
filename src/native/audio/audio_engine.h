#pragma once

#include <jni.h>

namespace immirror::audio {

/* 方块交互音效. 顺序与 Java 侧加载顺序一致. */
enum class SoundId {
    kDigGrass = 0,
    kDigStone,
    kDigSand,
    kPlaceGrass,
    kPlaceStone,
    kPlaceSand,
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

    void Play(SoundId id);

private:
    /* 播放需要 JNIEnv, 原生线程首次使用时附加到虚拟机. */
    JNIEnv* CurrentEnv();

    JavaVM* vm_ = nullptr;
    jobject soundPool_ = nullptr;
    jint soundIds_[static_cast<int>(SoundId::kCount)] = {0};
    jmethodID playMethod_ = nullptr;
    int32_t attachedCount_ = 0;
};

}  /* namespace immirror::audio */
