#include "world/world_time.h"

#include <cmath>

namespace immirror::world {

namespace {

/* 晨昏过渡时长: 分界小时前后各留 1 小时线性渐变. */
constexpr float kTwilightHours = 1.0f;

}  /* namespace */

void WorldTime::Tick(float dt) {
    if (dt > 0.0f) {
        totalSeconds_ += dt;
    }
}

void WorldTime::SetTotalSeconds(float seconds) {
    totalSeconds_ = seconds > 0.0f ? seconds : 0.0f;
}

float WorldTime::SkyFactor() const {
    const float hour = GetHour();
    if (hour >= kDayEndHour + kTwilightHours || hour < kDayBeginHour - kTwilightHours) {
        return 0.0f;
    }
    if (hour >= kDayBeginHour - kTwilightHours && hour < kDayBeginHour + kTwilightHours) {
        /* 拂晓: 5 到 7 点从夜过渡到昼. */
        const float t = (hour - (kDayBeginHour - kTwilightHours)) / (2.0f * kTwilightHours);
        return t;
    }
    if (hour >= kDayEndHour - kTwilightHours && hour < kDayEndHour + kTwilightHours) {
        /* 黄昏: 19 到 21 点从昼过渡到夜. */
        const float t = (hour - (kDayEndHour - kTwilightHours)) / (2.0f * kTwilightHours);
        return 1.0f - t;
    }
    return 1.0f;
}

}  /* namespace immirror::world */
