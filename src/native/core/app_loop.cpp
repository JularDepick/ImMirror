#include "core/app_loop.h"

namespace immirror::core {
namespace {

/* 非活动状态下的事件泵等待时长, 兼顾响应速度与待机能耗. */
constexpr int32_t kIdlePollMillis = 16;

}  /* namespace */

AppLoop::AppLoop(const LoopConfig& config) : config_(config) {}

void AppLoop::Run(LoopHost& host) {
    while (!host.ShouldExit()) {
        const int32_t pollMillis = host.IsActive() ? 0 : kIdlePollMillis;
        if (!host.PumpEvents(pollMillis)) {
            break;
        }
        if (host.ShouldExit()) {
            break;
        }

        const double frameDelta = clock_.Advance();
        if (!host.IsActive()) {
            accumulator_ = 0.0;
            host.Render(frameDelta);
            continue;
        }

        accumulator_ += frameDelta;
        int32_t executed = 0;
        while (accumulator_ >= config_.fixedStepSeconds && executed < config_.maxTicksPerFrame) {
            host.Tick(config_.fixedStepSeconds);
            accumulator_ -= config_.fixedStepSeconds;
            ++executed;
            ++ticks_;
        }
        if (executed >= config_.maxTicksPerFrame) {
            /* 逻辑步数触顶说明本帧严重超时, 丢弃积压避免后续连跳. */
            accumulator_ = 0.0;
        }
        host.Render(frameDelta);
    }
}

int64_t AppLoop::Ticks() const {
    return ticks_;
}

}  /* namespace immirror::core */
