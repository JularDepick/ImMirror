#include "core/frame_stats.h"

namespace immirror::core {

FrameStats::FrameStats(int32_t windowFrames) : windowFrames_(windowFrames > 0 ? windowFrames : 60) {}

void FrameStats::AddFrame(double deltaSeconds) {
    if (deltaSeconds <= 0.0) {
        return;
    }
    ++totalFrames_;
    ++windowCount_;
    windowSeconds_ += deltaSeconds;
    if (deltaSeconds > maxFrame_) {
        maxFrame_ = deltaSeconds;
    }
}

bool FrameStats::ShouldReport() const {
    return windowCount_ >= windowFrames_;
}

FrameReport FrameStats::Snapshot() {
    FrameReport report;
    report.frames = totalFrames_;
    report.windowFrames = windowCount_;
    report.windowSeconds = windowSeconds_;
    if (windowCount_ > 0) {
        report.averageMs = windowSeconds_ * 1000.0 / static_cast<double>(windowCount_);
        if (report.averageMs > 0.0) {
            report.averageFps = 1000.0 / report.averageMs;
        }
    }
    report.maxMs = maxFrame_ * 1000.0;
    ResetWindow();
    return report;
}

int64_t FrameStats::TotalFrames() const {
    return totalFrames_;
}

void FrameStats::ResetWindow() {
    windowCount_ = 0;
    windowSeconds_ = 0.0;
    maxFrame_ = 0.0;
}

}  /* namespace immirror::core */
