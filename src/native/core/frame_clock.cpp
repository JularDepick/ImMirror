#include "core/frame_clock.h"

#include <chrono>

namespace immirror::core {
namespace {

/* 统一使用纳秒精度的单调时钟读数. */
int64_t NowTicks() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

constexpr double kNanosecondsPerSecond = 1000000000.0;

}  /* namespace */

FrameClock::FrameClock() : lastTicks_(NowTicks()) {}

double FrameClock::Advance() {
    const int64_t now = NowTicks();
    double delta = static_cast<double>(now - lastTicks_) / kNanosecondsPerSecond;
    lastTicks_ = now;
    if (delta < 0.0) {
        delta = 0.0;
    }
    if (delta > maxFrameSeconds_) {
        delta = maxFrameSeconds_;
    }
    elapsedSeconds_ += delta;
    return delta;
}

double FrameClock::ElapsedSeconds() const {
    return elapsedSeconds_;
}

double FrameClock::MaxFrameSeconds() const {
    return maxFrameSeconds_;
}

void FrameClock::SetMaxFrameSeconds(double seconds) {
    if (seconds > 0.0) {
        maxFrameSeconds_ = seconds;
    }
}

}  /* namespace immirror::core */
