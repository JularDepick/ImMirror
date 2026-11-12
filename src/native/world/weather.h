#pragma once

#include <cstdint>

namespace immirror::world {

/*
 * 天气状态机.
 *
 * biomedef 每行给出能否下雨/下雪与晴天/雨天/雷暴三档权重,
 * 这里按权重周期性重抽, 强度在两种状态之间线性过渡, 避免天气瞬间切换.
 * 模块不读时间也不读地形: 调用方按玩家所在群系填参数, 再逐帧 Tick.
 */
enum class WeatherKind : uint8_t {
    kClear = 0,
    kRain = 1,
    kSnow = 2,
    kThunder = 3,
};

class Weather {
public:
    /* 按群系参数重新配置: 能否下雨/下雪与三档权重 (权重相对值, 0 表示该档不出现). */
    void Configure(bool canRain, bool canSnow, uint8_t clearWeight, uint8_t rainWeight,
                   uint8_t thunderWeight);

    /* 推进天气: 到期按权重重抽, 强度向目标线性过渡. */
    void Tick(float deltaSeconds);

    WeatherKind Kind() const { return kind_; }

    /* 当前强度 0 到 1, 渲染侧据此折算降水密度与天色压暗. */
    float Intensity() const { return intensity_; }

    /* 本轮配置下是否允许出现降水 (雨天/雪天/雷暴). */
    bool PrecipitationAllowed() const;

    /* 调试用名称. */
    const char* KindName() const;

private:
    /* 按权重重抽一次天气, 只在允许的档位里选. */
    void Reevaluate();

    float NextUnit();

    bool canRain_ = true;
    bool canSnow_ = false;
    uint8_t clearWeight_ = 100;
    uint8_t rainWeight_ = 30;
    uint8_t thunderWeight_ = 5;

    WeatherKind kind_ = WeatherKind::kClear;
    float intensity_ = 0.0f;
    float timer_ = 0.0f;
    /* 下一次重抽的间隔, 取 90 到 240 秒之间的随机值, 避免全服同时变天. */
    float nextChangeSeconds_ = 120.0f;
    uint32_t rng_ = 0x9e3779b9u;
};

}  /* namespace immirror::world */
