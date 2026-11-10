/*
 * 设置单例实现. 读取路径只碰原子标量, 写入路径负责裁剪与置脏,
 * 落盘与读盘由互斥量串行化, 二者都不进入逐帧路径.
 */
#include "settings/settings.h"

#include <cstring>

#include "core/log.h"
#include "settings/settings_tuning.h"

namespace immirror::settings {
namespace {

constexpr const char* kTag = "ImMirrorSettings";

/* 按落盘键名取下标, 未知键返回 -1. */
int32_t IndexOfName(const char* name) {
    const KeyInfo* info = FindKey(name);
    return info == nullptr ? -1 : static_cast<int32_t>(info->key);
}

}  /* namespace */

Settings& Settings::Instance() {
    /* 函数内静态对象由编译器保证一次性构造, 首次访问即完成默认值填充. */
    static Settings instance;
    return instance;
}

Settings::Settings() {
    ApplyDefaults();
}

void Settings::ApplyDefaults() {
    for (int32_t index = 0; index < kSettingKeyCount; ++index) {
        values_[index].store(Info(static_cast<Key>(index)).defaultValue);
    }
    /* 默认表里 classical 为 0, rocker 为 1, 即默认浮动摇杆, 与原版一致. */
}

void Settings::ApplyEntries(const std::vector<SettingsStore::Entry>& entries) {
    for (const SettingsStore::Entry& entry : entries) {
        const KeyInfo* info = FindKey(entry.name.c_str());
        if (info == nullptr) {
            IM_LOGW(kTag, "unknown setting key dropped, key=%s", entry.name.c_str());
            continue;
        }
        const int32_t applied = NormalizeValue(info->key, entry.value);
        if (applied != entry.value) {
            IM_LOGW(kTag, "setting clamped, key=%s file=%d applied=%d", info->name,
                    static_cast<int>(entry.value), static_cast<int>(applied));
        }
        values_[static_cast<int32_t>(info->key)].store(applied);
        if (info->key == Key::kViewDistance) {
            viewDistanceFromFile_.store(true);
        }
    }
}

bool Settings::Initialize(const char* dataPath) {
    std::lock_guard<std::mutex> guard(mutex_);
    if (!store_.SetDirectory(dataPath)) {
        IM_LOGW(kTag, "settings directory not provided, defaults only");
        return false;
    }
    const std::string path = store_.Path();
    if (loaded_ && path == loadedPath_) {
        return true;
    }
    loaded_ = true;
    loadedPath_ = path;
    ApplyDefaults();

    std::vector<SettingsStore::Entry> entries;
    const bool fromFile = store_.Load(&entries);
    if (fromFile) {
        ApplyEntries(entries);
    }
    dirty_.store(false);
    IM_LOGI(kTag, "settings %s, path=%s entries=%d sensitivity=%d view_distance=%d volume=%d",
            fromFile ? "loaded" : "defaults", path.c_str(), static_cast<int>(entries.size()),
            static_cast<int>(values_[static_cast<int32_t>(Key::kSensitivity)].load()),
            static_cast<int>(values_[static_cast<int32_t>(Key::kViewDistance)].load()),
            static_cast<int>(values_[static_cast<int32_t>(Key::kVolume)].load()));
    return true;
}

std::string Settings::StoragePath() const {
    std::lock_guard<std::mutex> guard(mutex_);
    return store_.Path();
}

int32_t Settings::Get(Key key) const {
    const int32_t index = static_cast<int32_t>(key);
    if (index < 0 || index >= kSettingKeyCount) {
        return 0;
    }
    return values_[index].load();
}

int32_t Settings::GetByName(const char* name, int32_t fallback) const {
    const int32_t index = IndexOfName(name);
    return index < 0 ? fallback : values_[index].load();
}

int32_t Settings::Set(Key key, int32_t value) {
    const int32_t index = static_cast<int32_t>(key);
    if (index < 0 || index >= kSettingKeyCount) {
        return -1;
    }
    const int32_t applied = NormalizeValue(key, value);
    values_[index].store(applied);

    /*
     * 经典模式与摇杆模式在原版是互斥勾选, 数据层保证两者不会同时为 1:
     * 打开其中一个立即关掉另一个, 设置界面读到的状态因此始终一致.
     */
    if (key == Key::kClassical) {
        values_[static_cast<int32_t>(Key::kRocker)].store(applied != 0 ? 0 : 1);
    } else if (key == Key::kRocker) {
        values_[static_cast<int32_t>(Key::kClassical)].store(applied != 0 ? 0 : 1);
    }
    if (key == Key::kViewDistance) {
        viewDistanceFromFile_.store(true);
    }
    dirty_.store(true);
    IM_LOGD(kTag, "setting set, key=%s value=%d", Info(key).name, static_cast<int>(applied));
    return applied;
}

int32_t Settings::SetByName(const char* name, int32_t value) {
    const KeyInfo* info = FindKey(name);
    if (info == nullptr) {
        IM_LOGW(kTag, "unknown setting key write ignored, key=%s", name == nullptr ? "" : name);
        return -1;
    }
    return Set(info->key, value);
}

void Settings::Reset(Key key) {
    Set(key, Info(key).defaultValue);
    /* 恢复默认视距不算用户覆盖, 允许后续按设备分级重新播种. */
    if (key == Key::kViewDistance) {
        viewDistanceFromFile_.store(false);
    }
}

void Settings::ResetAll() {
    for (int32_t index = 0; index < kSettingKeyCount; ++index) {
        values_[index].store(Info(static_cast<Key>(index)).defaultValue);
    }
    viewDistanceFromFile_.store(false);
    dirty_.store(true);
    IM_LOGI(kTag, "all settings reset to defaults");
}

bool Settings::Save() {
    std::lock_guard<std::mutex> guard(mutex_);
    if (store_.Path().empty()) {
        return false;
    }
    std::vector<SettingsStore::Entry> entries;
    entries.reserve(static_cast<size_t>(kSettingKeyCount));
    for (int32_t index = 0; index < kSettingKeyCount; ++index) {
        entries.push_back(SettingsStore::Entry{Info(static_cast<Key>(index)).name,
                                               values_[index].load()});
    }
    const bool saved = store_.Save(entries);
    if (saved) {
        dirty_.store(false);
    }
    IM_LOGI(kTag, "settings saved=%d path=%s", saved ? 1 : 0, store_.Path().c_str());
    return saved;
}

bool Settings::FlushIfDirty() {
    if (!dirty_.load()) {
        return false;
    }
    return Save();
}

int32_t Settings::SeedViewDistanceFromGrade(const char* grade) {
    int32_t seeded = tuning::kViewDistanceMediumGrade;
    if (grade != nullptr) {
        if (std::strcmp(grade, "low") == 0) {
            seeded = tuning::kViewDistanceLowGrade;
        } else if (std::strcmp(grade, "mid") == 0 || std::strcmp(grade, "medium") == 0) {
            seeded = tuning::kViewDistanceMediumGrade;
        } else if (std::strcmp(grade, "high") == 0) {
            seeded = tuning::kViewDistanceHighGrade;
        }
    }
    {
        std::lock_guard<std::mutex> guard(mutex_);
        if (!viewDistanceFromFile_.load()) {
            values_[static_cast<int32_t>(Key::kViewDistance)].store(
                NormalizeValue(Key::kViewDistance, seeded));
            viewDistanceFromFile_.store(true);
            dirty_.store(true);
            IM_LOGI(kTag, "view distance seeded by grade, grade=%s level=%d",
                    grade == nullptr ? "unknown" : grade, static_cast<int>(seeded));
        }
    }
    return Get(Key::kViewDistance);
}

std::string Settings::Export() const {
    std::string text;
    text.reserve(static_cast<size_t>(kSettingKeyCount) * 24u);
    for (int32_t index = 0; index < kSettingKeyCount; ++index) {
        const KeyInfo& info = Info(static_cast<Key>(index));
        text += info.name;
        text += '=';
        text += std::to_string(values_[index].load());
        text += '\n';
    }
    return text;
}

std::string Settings::Describe() const {
    std::string text;
    text.reserve(static_cast<size_t>(kSettingKeyCount) * 32u);
    for (int32_t index = 0; index < kSettingKeyCount; ++index) {
        const KeyInfo& info = Info(static_cast<Key>(index));
        text += info.name;
        text += '|';
        text += std::to_string(info.defaultValue);
        text += '|';
        text += std::to_string(info.minValue);
        text += '|';
        text += std::to_string(info.maxValue);
        text += '\n';
    }
    return text;
}

float Settings::LookFactor() const {
    const float value = static_cast<float>(Get(Key::kSensitivity));
    return tuning::kLookFactorAtZeroSensitivity +
           tuning::kLookFactorSpan * (value / tuning::kSensitivityFull);
}

float Settings::LookDegreesPerScreenWidth() const {
    return tuning::kLookDegreesPerScreenWidthAtDefault * LookFactor();
}

float Settings::LookDegreesPerScreenHeight() const {
    return tuning::kLookDegreesPerScreenHeightAtDefault * LookFactor();
}

float Settings::MasterVolume() const {
    return static_cast<float>(Get(Key::kVolume)) / tuning::kVolumeFull;
}

bool Settings::SoundEnabled() const {
    return Get(Key::kSoundOpen) != 0;
}

bool Settings::MusicEnabled() const {
    return Get(Key::kMusicOpen) != 0;
}

bool Settings::VoiceEnabled() const {
    return Get(Key::kVoiceOpen) != 0;
}

bool Settings::AutoJump() const {
    return Get(Key::kAutoJump) != 0;
}

bool Settings::CameraShake() const {
    return Get(Key::kCameraShake) != 0;
}

bool Settings::ReversalY() const {
    return Get(Key::kReversalY) != 0;
}

bool Settings::LeftHanded() const {
    return Get(Key::kLeftHanded) != 0;
}

bool Settings::HideUi() const {
    return Get(Key::kHideUi) != 0;
}

bool Settings::ClassicalMoveMode() const {
    return Get(Key::kClassical) != 0;
}

int32_t Settings::ViewDistanceChunks() const {
    return Get(Key::kViewDistance);
}

}  /* namespace immirror::settings */
