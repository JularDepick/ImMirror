#include "audio/audio_engine.h"

#include <android/log.h>

#include <cstring>

namespace immirror::audio {
namespace {

constexpr const char* kTag = "ImMirrorAudio";
constexpr int32_t kSoundCount = static_cast<int32_t>(SoundId::kCount);

}  /* namespace */

AudioEngine& AudioEngine::Instance() {
    static AudioEngine instance;
    return instance;
}

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() = default;

void AudioEngine::Attach(JNIEnv* env, jobject soundPool, jintArray soundIds) {
    if (env == nullptr || soundPool == nullptr || soundIds == nullptr) {
        return;
    }
    /* 挂接发生在 Java 主线程, 播放可能在原生线程, 引用读写共用同一把锁. */
    std::lock_guard<std::mutex> guard(mutex_);
    env->GetJavaVM(&vm_);
    if (soundPool_ != nullptr) {
        env->DeleteGlobalRef(soundPool_);
        soundPool_ = nullptr;
    }
    soundPool_ = env->NewGlobalRef(soundPool);
    jclass poolClass = env->GetObjectClass(soundPool);
    /* SoundPool.play(int, float, float, int, int, float) 返回 int 类型的流编号. */
    playMethod_ = env->GetMethodID(poolClass, "play", "(IFFIIF)I");
    env->DeleteLocalRef(poolClass);
    if (playMethod_ == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "SoundPool.play not found");
        env->DeleteGlobalRef(soundPool_);
        soundPool_ = nullptr;
        return;
    }

    const jsize length = env->GetArrayLength(soundIds);
    const jsize copy = length < kSoundCount ? length : kSoundCount;
    env->GetIntArrayRegion(soundIds, 0, copy, soundIds_);
    __android_log_print(ANDROID_LOG_INFO, kTag, "audio attached, sounds=%d", static_cast<int>(copy));
}

void AudioEngine::Detach(JNIEnv* env) {
    /*
     * 解除引用时原生线程可能仍在处理最后一帧的音效请求,
     * 加锁保证播放方要么看到完整的引用, 要么直接走未就绪分支.
     */
    std::lock_guard<std::mutex> guard(mutex_);
    const bool hadPool = soundPool_ != nullptr;
    if (env != nullptr && soundPool_ != nullptr) {
        env->DeleteGlobalRef(soundPool_);
    }
    soundPool_ = nullptr;
    playMethod_ = nullptr;
    if (hadPool) {
        __android_log_print(ANDROID_LOG_INFO, kTag, "audio detached");
    }
}

bool AudioEngine::Ready() const {
    std::lock_guard<std::mutex> guard(mutex_);
    return soundPool_ != nullptr && playMethod_ != nullptr;
}

JNIEnv* AudioEngine::CurrentEnv() {
    if (vm_ == nullptr) {
        return nullptr;
    }
    JNIEnv* env = nullptr;
    const jint status = vm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (status == JNI_OK) {
        return env;
    }
    if (status == JNI_EDETACHED) {
        if (vm_->AttachCurrentThread(&env, nullptr) != JNI_OK) {
            return nullptr;
        }
        attachedCount_ += 1;
        return env;
    }
    return nullptr;
}

void AudioEngine::SetMasterVolume(float volume) {
    if (!(volume > 0.0f)) {
        volume_ = 0.0f;
        return;
    }
    volume_ = volume > 1.0f ? 1.0f : volume;
}

void AudioEngine::SetSoundEnabled(bool enabled) {
    soundEnabled_ = enabled;
}

void AudioEngine::SetMusicEnabled(bool enabled) {
    musicEnabled_ = enabled;
}

void AudioEngine::SetVoiceEnabled(bool enabled) {
    voiceEnabled_ = enabled;
}

void AudioEngine::Play(SoundId id) {
    if (!soundEnabled_) {
        return;
    }
    /*
     * 播放全程持锁: Activity 销毁时的 Detach 会删除全局引用,
     * 两边交错会让这里的 CallIntMethod 落到已释放的引用上.
     */
    std::lock_guard<std::mutex> guard(mutex_);
    if (soundPool_ == nullptr || playMethod_ == nullptr) {
        return;
    }
    const int32_t index = static_cast<int32_t>(id);
    if (index < 0 || index >= kSoundCount || soundIds_[index] <= 0) {
        return;
    }
    /* 参数依次为: 音效句柄, 左声道, 右声道, 优先级, 循环次数, 播放速率.
     * 左右声道音量取设置层下发的主音量, 音量 0 时直接返回, 不产生无效流. */
    if (volume_ <= 0.0f) {
        return;
    }
    JNIEnv* env = CurrentEnv();
    if (env == nullptr) {
        return;
    }
    env->CallIntMethod(soundPool_, playMethod_, soundIds_[index], volume_, volume_, 1, 0, 1.0f);
}

}  /* namespace immirror::audio */
