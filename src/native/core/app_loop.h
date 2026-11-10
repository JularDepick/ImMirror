#pragma once

#include "core/frame_clock.h"

namespace immirror::core {

/* 主循环配置: 逻辑步长固定, 单帧最多补若干逻辑步, 防止慢帧雪崩. */
struct LoopConfig {
    double fixedStepSeconds = 1.0 / 60.0;
    int32_t maxTicksPerFrame = 4;
};

/*
 * 主循环宿主接口. 引擎侧实现它来接管事件泵, 逻辑推进与渲染,
 * 主循环本身只负责节奏, 不感知任何平台细节.
 */
class LoopHost {
public:
    virtual ~LoopHost() = default;

    /* 排空事件队列; timeoutMillis 为 0 表示不等待. 返回 false 表示应当退出. */
    virtual bool PumpEvents(int32_t timeoutMillis) = 0;

    /* 固定步长逻辑推进. */
    virtual void Tick(double fixedDeltaSeconds) = 0;

    /* 渲染一帧; 非活动状态下同样调用, 由宿主自行决定是否提交绘制. */
    virtual void Render(double frameDeltaSeconds) = 0;

    /* 是否存在活动画面, 决定事件泵是否阻塞等待以节省电量. */
    virtual bool IsActive() const = 0;

    /* 是否应当退出主循环. */
    virtual bool ShouldExit() const = 0;
};

/*
 * 固定步长逻辑 + 可变步长渲染的主循环.
 * Run 会阻塞当前线程直到宿主声明退出, 退出后的资源回收由宿主负责.
 */
class AppLoop {
public:
    explicit AppLoop(const LoopConfig& config);

    void Run(LoopHost& host);

    int64_t Ticks() const;

private:
    LoopConfig config_;
    FrameClock clock_;
    double accumulator_ = 0.0;
    int64_t ticks_ = 0;
};

}  /* namespace immirror::core */
