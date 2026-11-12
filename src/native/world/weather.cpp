#include "world/weather.h"

namespace immirror::world {
namespace {

/* 强度过渡速度: 每秒 0.2, 五秒内从晴到满强度. */
constexpr float kIntensityRate = 0.2f;
/* 重抽间隔下限与上限, 单位秒. */
constexpr float kChangeMinSeconds = 90.0f;
constexpr float kChangeMaxSeconds = 240.0f;

}  /* namespace */

void Weather::Configure(bool canRain, bool canSnow, uint8_t clearWeight, uint8_t rainWeight,
                        uint8_t thunderWeight) {
    if (canRain_ == canRain && canSnow_ == canSnow && clearWeight_ == clearWeight &&
        rainWeight_ == rainWeight && thunderWeight_ == thunderWeight) {
        return;
    }
    canRain_ = canRain;
    canSnow_ = canSnow;
    clearWeight_ = clearWeight > 0 ? clearWeight : 1;
    rainWeight_ = rainWeight;
    thunderWeight_ = thunderWeight;
    /* 群系变化后当前天气可能不再合法, 下一次 Tick 立即重抽. */
    timer_ = nextChangeSeconds_;
}

bool Weather::PrecipitationAllowed() const {
    return (canRain_ && rainWeight_ > 0) || (canSnow_ && rainWeight_ > 0) ||
           (rainWeight_ > 0 && thunderWeight_ > 0);
}

const char* Weather::KindName() const {
    switch (kind_) {
        case WeatherKind::kRain:
            return "rain";
        case WeatherKind::kSnow:
            return "snow";
        case WeatherKind::kThunder:
            return "thunder";
        case WeatherKind::kClear:
        default:
            return "clear";
    }
}

float Weather::NextUnit() {
    /* xorshift32: 天气只需要可复现的伪随机, 不引入标准库随机设备. */
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    return static_cast<float>(rng_ & 0x00ffffffu) / static_cast<float>(0x01000000u);
}

void Weather::Reevaluate() {
    /*
     * 权重按档位累加后抽签: 雪只在允许下雪的群系出现, 其他情况按雨/雷暴落,
     * 两档都为 0 时只能回落晴天.
     */
    const uint32_t clearWeight = clearWeight_;
    const uint32_t rainWeight = canRain_ ? rainWeight_ : 0u;
    const uint32_t snowWeight = canSnow_ ? rainWeight_ : 0u;
    const uint32_t thunderWeight = thunderWeight_ > 0 && canRain_ ? thunderWeight_ : 0u;
    const uint32_t total = clearWeight + rainWeight + snowWeight + thunderWeight;
    if (total == 0) {
        kind_ = WeatherKind::kClear;
        return;
    }
    const uint32_t pick = static_cast<uint32_t>(NextUnit() * static_cast<float>(total));
    if (pick < clearWeight) {
        kind_ = WeatherKind::kClear;
    } else if (pick < clearWeight + rainWeight) {
        kind_ = WeatherKind::kRain;
    } else if (pick < clearWeight + rainWeight + snowWeight) {
        kind_ = WeatherKind::kSnow;
    } else {
        kind_ = WeatherKind::kThunder;
    }
    nextChangeSeconds_ = kChangeMinSeconds + NextUnit() * (kChangeMaxSeconds - kChangeMinSeconds);
}

void Weather::Tick(float deltaSeconds) {
    if (!(deltaSeconds > 0.0f)) {
        return;
    }
    timer_ += deltaSeconds;
    if (timer_ >= nextChangeSeconds_) {
        timer_ = 0.0f;
        Reevaluate();
    }
    const float target = kind_ == WeatherKind::kClear ? 0.0f : 1.0f;
    if (intensity_ < target) {
        intensity_ += kIntensityRate * deltaSeconds;
        if (intensity_ > target) {
            intensity_ = target;
        }
    } else if (intensity_ > target) {
        intensity_ -= kIntensityRate * deltaSeconds;
        if (intensity_ < target) {
            intensity_ = target;
        }
    }
}

}  /* namespace immirror::world */
