#pragma once

#include <cstdint>

namespace immirror::world {

/*
 * 世界时间. 二十分钟现实时间等于一个游戏日 (1200 秒, 日长无文档,
 * 此值取多数沙盒游戏的常用节奏并集中在本文件), 0 秒对应游戏日 0 点整.
 * 昼夜分界与playmain.lua 的语义一致: 6 点到 20 点为昼, 其余为夜.
 */
class WorldTime {
public:
    /* 一个游戏日的现实秒数. */
    static constexpr float kDayLengthSeconds = 1200.0f;

    /* 昼夜分界小时数: [6, 20) 为昼. */
    static constexpr float kDayBeginHour = 6.0f;
    static constexpr float kDayEndHour = 20.0f;

    /* 空时间构造: 从正午开始, 便于新建世界落在光照明亮的时段. */
    WorldTime() = default;

    /* 推进世界时间, dt 为本帧现实秒数, 累计进游戏日内秒数. */
    void Tick(float dt);

    /* 直接设置世界已过的总秒数, 供读档恢复使用, 负值按 0 处理. */
    void SetTotalSeconds(float seconds);

    /* 世界已过的总秒数, 存档按这个值落盘. */
    float TotalSeconds() const { return totalSeconds_; }

    /* 游戏日内已过的秒数, 0 到 1200. */
    float DaySeconds() const { return totalSeconds_ - DayCount() * kDayLengthSeconds; }

    /* 游戏日计数, 从世界创建起累计. */
    uint32_t DayCount() const { return static_cast<uint32_t>(totalSeconds_ / kDayLengthSeconds); }

    /* 当前小时数, 0 到 23 的浮点. */
    float GetHour() const {
        return (totalSeconds_ - DayCount() * kDayLengthSeconds) * 24.0f / kDayLengthSeconds;
    }

    /* 是否为白天: 6 点含到 20 点不含. */
    bool IsDay() const { return GetHour() >= kDayBeginHour && GetHour() < kDayEndHour; }

    /*
     * 天光混合因子 0 到 1: 深夜 0, 白天 1, 分界前后各留 1 小时线性过渡.
     * 渲染侧按 mix(0.27, 1.0, factor) 折算顶点明暗, 月夜的 27% 下限
     * 由渲染端承担, 时间层只负责昼夜比例.
     */
    float SkyFactor() const;

private:
    /* 世界已过的总秒数, 读写档与日计数都由它折算. */
    float totalSeconds_ = kDayLengthSeconds * 0.5f;
};

}  /* namespace immirror::world */
