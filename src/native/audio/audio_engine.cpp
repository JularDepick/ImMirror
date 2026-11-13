#include "audio/audio_engine.h"

#include <android/log.h>

#include <cstring>

namespace immirror::audio {
namespace {

constexpr const char* kTag = "ImMirrorAudio";
constexpr int32_t kSoundCount = static_cast<int32_t>(SoundId::kCount);
constexpr int32_t kVariantsPerFamily = static_cast<int32_t>(kSoundVariants);

}  /* namespace */

AudioEngine& AudioEngine::Instance() {
    static AudioEngine instance;
    return instance;
}

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() = default;

void AudioEngine::Attach(JNIEnv* env, jobject soundPool, jintArray soundIds,
                         jintArray familyCounts) {
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

    /* 先展开族计数: Java 侧按 [族1变体1, 族1变体2, ..., 族2变体1, ...] 平铺推送,
     * familyCounts 给出各族实际变体数, 缺省按 1 处理, 保持单文件族的兼容语义. */
    jint counts[kSoundCount] = {0};
    jsize familyTotal = 0;
    if (familyCounts != nullptr) {
        const jsize countLength = env->GetArrayLength(familyCounts);
        const jsize copyCounts = countLength < kSoundCount ? countLength : kSoundCount;
        env->GetIntArrayRegion(familyCounts, 0, copyCounts, counts);
        for (int32_t i = 0; i < copyCounts; ++i) {
            if (counts[i] <= 0) {
                counts[i] = 1;
            }
            if (counts[i] > kVariantsPerFamily) {
                counts[i] = kVariantsPerFamily;
            }
            familyTotal += counts[i];
        }
    }
    if (familyTotal <= 0) {
        /* 旧式单文件推送: 每族一个句柄. */
        const jsize length = env->GetArrayLength(soundIds);
        for (int32_t i = 0; i < kSoundCount && i < length; ++i) {
            counts[i] = 1;
        }
        familyTotal = length < kSoundCount ? length : kSoundCount;
    }

    for (int32_t i = 0; i < kSoundCount; ++i) {
        for (int32_t v = 0; v < kVariantsPerFamily; ++v) {
            soundIds_[i][v] = 0;
        }
        variantCounts_[i] = 0;
    }

    /* 平铺句柄按族计数切开: 超出该族上限的变体被丢弃, 句柄总量以数组长度封顶. */
    const jsize length = env->GetArrayLength(soundIds);
    jint flat[kSoundCount * kVariantsPerFamily] = {0};
    const jsize expected = familyTotal;
    const jsize copyTotal = length < expected ? length : expected;
    env->GetIntArrayRegion(soundIds, 0, copyTotal, flat);

    int32_t cursor = 0;
    for (int32_t i = 0; i < kSoundCount; ++i) {
        const int32_t count = counts[i];
        for (int32_t v = 0; v < count && cursor < copyTotal; ++v, ++cursor) {
            soundIds_[i][v] = flat[cursor];
        }
        /* 按实际拿到的有效句柄数收口, 全族加载失败时保持 0, 播放时静默跳过. */
        int32_t valid = 0;
        for (int32_t v = 0; v < count; ++v) {
            if (soundIds_[i][v] > 0) {
                valid = v + 1;
            }
        }
        variantCounts_[i] = valid;
    }
    __android_log_print(ANDROID_LOG_INFO, kTag, "audio attached, families=%d handles=%d",
                        static_cast<int>(kSoundCount), static_cast<int>(copyTotal));
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
    for (int32_t i = 0; i < kSoundCount; ++i) {
        for (int32_t v = 0; v < kVariantsPerFamily; ++v) {
            soundIds_[i][v] = 0;
        }
        variantCounts_[i] = 0;
    }
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

void AudioEngine::AttachCreatureBank(JNIEnv* env, jobject bank, jobject soundPool) {
    if (env == nullptr || bank == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> guard(mutex_);
    if (creatureBankClass_ != nullptr) {
        env->DeleteGlobalRef(creatureBankClass_);
        creatureBankClass_ = nullptr;
    }
    /* 入参是 Class 对象: 静态方法必须挂在类引用上, 实例引用调不到. */
    jclass localClass = static_cast<jclass>(bank);
    creatureBankClass_ = static_cast<jclass>(env->NewGlobalRef(localClass));
    /* CreatureSoundBank.handleFor(String) 返回句柄, 0 表示不可用. */
    creatureHandleMethod_ = env->GetStaticMethodID(creatureBankClass_, "handleFor", "(Ljava/lang/String;)I");
    if (creatureHandleMethod_ == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "CreatureSoundBank.handleFor not found");
        env->DeleteGlobalRef(creatureBankClass_);
        creatureBankClass_ = nullptr;
        return;
    }
    /*
     * 生物音效与方块音效共用一个 SoundPool 时, 播放方法已经从方块音效挂接里拿到;
     * 若尚未挂接, 这里用同一套签名补取一次, 保证两条通道都能播放.
     */
    if (playMethod_ == nullptr && soundPool != nullptr) {
        if (soundPool_ == nullptr) {
            soundPool_ = env->NewGlobalRef(soundPool);
        }
        jclass poolClass = env->GetObjectClass(soundPool);
        playMethod_ = env->GetMethodID(poolClass, "play", "(IFFIIF)I");
        env->DeleteLocalRef(poolClass);
    }
    __android_log_print(ANDROID_LOG_INFO, kTag, "creature sound bank attached");
}

void AudioEngine::DetachCreatureBank(JNIEnv* env) {
    std::lock_guard<std::mutex> guard(mutex_);
    if (env != nullptr && creatureBankClass_ != nullptr) {
        env->DeleteGlobalRef(creatureBankClass_);
    }
    creatureBankClass_ = nullptr;
    creatureHandleMethod_ = nullptr;
}

void AudioEngine::PlayCreatureAsset(const char* assetPath, float scale, float pan) {
    if (assetPath == nullptr || assetPath[0] == '\0') {
        return;
    }
    if (!soundEnabled_ || volume_ <= 0.0f) {
        return;
    }
    std::lock_guard<std::mutex> guard(mutex_);
    if (creatureBankClass_ == nullptr || creatureHandleMethod_ == nullptr || soundPool_ == nullptr ||
        playMethod_ == nullptr) {
        return;
    }
    JNIEnv* env = CurrentEnv();
    if (env == nullptr) {
        return;
    }
    jstring path = env->NewStringUTF(assetPath);
    if (path == nullptr) {
        return;
    }
    const jint handle = env->CallStaticIntMethod(creatureBankClass_, creatureHandleMethod_, path);
    env->DeleteLocalRef(path);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return;
    }
    if (handle <= 0) {
        return;
    }
    /* 距离衰减: 系数与总音量相乘, 钳到 0 到 1, 远处生物自然变轻. */
    float attenuated = volume_ * (scale < 0.0f ? 0.0f : (scale > 1.0f ? 1.0f : scale));
    if (attenuated <= 0.001f) {
        return;
    }
    /*
     * 左右声像: 把居中音量按声像拆成两路, 声像越偏向一侧,
     * 另一侧衰减越多; 极端值退化为单侧发声.
     */
    const float clampedPan = pan < -1.0f ? -1.0f : (pan > 1.0f ? 1.0f : pan);
    const float leftVolume = attenuated * (clampedPan > 0.0f ? 1.0f - clampedPan : 1.0f);
    const float rightVolume = attenuated * (clampedPan < 0.0f ? 1.0f + clampedPan : 1.0f);
    const jint stream = env->CallIntMethod(soundPool_, playMethod_, handle, leftVolume, rightVolume,
                                           1, 0, 1.0f);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return;
    }
    /* 播放成功记一行: 生物音效按需加载, 出问题时需要区分"没触发"与"没加载". */
    __android_log_print(ANDROID_LOG_DEBUG, kTag, "creature sound path=%s handle=%d stream=%d", assetPath,
                        static_cast<int>(handle), static_cast<int>(stream));
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
    if (index < 0 || index >= kSoundCount) {
        return;
    }
    /* 参数依次为: 音效句柄, 左声道, 右声道, 优先级, 循环次数, 播放速率.
     * 左右声道音量取设置层下发的主音量, 音量 0 时直接返回, 不产生无效流. */
    if (volume_ <= 0.0f) {
        return;
    }
    /* 同族多句柄时随机挑一个变体, 用 xorshift 生成轻量伪随机, 避免每次踩同一样音色. */
    const int32_t count = variantCounts_[index];
    if (count <= 0 || soundIds_[index][0] <= 0) {
        return;
    }
    int32_t variant = 0;
    if (count > 1) {
        randomState_ ^= randomState_ << 13;
        randomState_ ^= randomState_ >> 17;
        randomState_ ^= randomState_ << 5;
        variant = static_cast<int32_t>(randomState_ % static_cast<uint32_t>(count));
    }
    const jint handle = soundIds_[index][variant];
    if (handle <= 0) {
        return;
    }
    JNIEnv* env = CurrentEnv();
    if (env == nullptr) {
        return;
    }
    env->CallIntMethod(soundPool_, playMethod_, handle, volume_, volume_, 1, 0, 1.0f);
}

}  /* namespace immirror::audio */
