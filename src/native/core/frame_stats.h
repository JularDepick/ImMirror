#pragma once

#include <cstdint>

namespace immirror::core {

/* 一个统计窗口的汇总结果, 由 FrameStats::Snapshot 产出. */
struct FrameReport {
    int64_t frames = 0;        /* 自启动以来的累计帧数 */
    int32_t windowFrames = 0;  /* 本窗口内的帧数 */
    double windowSeconds = 0.0;
    double averageFps = 0.0;
    double averageMs = 0.0;
    double maxMs = 0.0;
};

/*
 * 帧率统计器. 按帧数窗口聚合, 默认每 60 帧产出一次报告,
 * 报告同时给出窗口内平均帧耗时与最差帧耗时, 便于定位卡顿.
 */
class FrameStats {
public:
    explicit FrameStats(int32_t windowFrames = 60);

    void AddFrame(double deltaSeconds);

    bool ShouldReport() const;

    /* 取走当前窗口报告并开启下一个窗口. */
    FrameReport Snapshot();

    int64_t TotalFrames() const;

    /* 只清空窗口内计数, 保留累计帧数, 用于窗口重建后重新统计. */
    void ResetWindow();

private:
    int32_t windowFrames_ = 60;
    int32_t windowCount_ = 0;
    double windowSeconds_ = 0.0;
    double maxFrame_ = 0.0;
    int64_t totalFrames_ = 0;
};

}  /* namespace immirror::core */
