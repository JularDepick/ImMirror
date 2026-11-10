/*
 * 设置键元数据的唯一实现. 键表以 X 宏形式定义在 settings_keys.h,
 * 这里把宏展开成可直接按下标访问的常量表, 枚举与元数据不会出现两处维护.
 */
#include "settings/settings_keys.h"

#include <cstddef>
#include <cstring>

#include "settings/settings_tuning.h"

namespace immirror::settings {
namespace {

const KeyInfo kKeyInfos[] = {
#define IMMIRROR_SETTING_KEY_ROW(enumName, keyName, defValue, minRange, maxRange, labelText) \
    KeyInfo{Key::enumName, keyName, defValue, minRange, maxRange, labelText},
    IMMIRROR_SETTING_KEY_TABLE(IMMIRROR_SETTING_KEY_ROW)
#undef IMMIRROR_SETTING_KEY_ROW
};

/* 键表行数必须与枚举数量一致, 否则数组下标会错位. */
static_assert(sizeof(kKeyInfos) / sizeof(kKeyInfos[0]) ==
                  static_cast<std::size_t>(kSettingKeyCount),
              "setting key table size mismatch");

int32_t ClampInt(int32_t value, int32_t low, int32_t high) {
    if (value < low) {
        return low;
    }
    return value > high ? high : value;
}

/* 视野距离只有 1 2 3 5 8 五档, 其余取值吸附到数值最近的合法档位. */
int32_t SnapViewDistance(int32_t value) {
    int32_t best = tuning::kViewDistanceLevels[0];
    int32_t bestGap = -1;
    for (int32_t index = 0; index < tuning::kViewDistanceLevelCount; ++index) {
        const int32_t level = tuning::kViewDistanceLevels[index];
        const int32_t gap = value > level ? value - level : level - value;
        if (bestGap < 0 || gap < bestGap) {
            best = level;
            bestGap = gap;
        }
    }
    return best;
}

}  /* namespace */

const KeyInfo& Info(Key key) {
    const int32_t index = static_cast<int32_t>(key);
    if (index < 0 || index >= kSettingKeyCount) {
        return kKeyInfos[0];
    }
    return kKeyInfos[index];
}

const KeyInfo* FindKey(const char* name) {
    if (name == nullptr) {
        return nullptr;
    }
    for (const KeyInfo& info : kKeyInfos) {
        if (std::strcmp(info.name, name) == 0) {
            return &info;
        }
    }
    return nullptr;
}

int32_t NormalizeValue(Key key, int32_t value) {
    if (key == Key::kViewDistance) {
        return SnapViewDistance(value);
    }
    const KeyInfo& info = Info(key);
    return ClampInt(value, info.minValue, info.maxValue);
}

}  /* namespace immirror::settings */
