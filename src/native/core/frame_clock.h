#pragma once

#include <cstdint>

namespace immirror::core {

/*
 * 单调帧计时器. 使用 steady_clock, 不受系统时间调整影响;
 * Advance 返回距上次调用的秒数, 并按上限截断, 防止应用挂起后
 * 单帧增量过大导致主循环爆发式补帧.
 */
class FrameClock {
public:
    FrameClock();

    /* 推进到当前时刻并返回本帧增量秒数. */
    double Advance();

    /* 累计活动时间, 只统计被截断后的增量. */
    double ElapsedSeconds() const;

    double MaxFrameSeconds() const;
    void SetMaxFrameSeconds(double seconds);

private:
    int64_t lastTicks_ = 0;
    double maxFrameSeconds_ = 0.25;
    double elapsedSeconds_ = 0.0;
};

}  /* namespace immirror::core */
