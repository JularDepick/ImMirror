#pragma once

#include <cstdint>
#include <mutex>
#include <string>

#include "progress/achievement_tracker.h"
#include "progress/daily_tasks.h"
#include "progress/milestones.h"
#include "progress/progress_stats.h"
#include "progress/progress_store.h"

namespace immirror::progress {

/* 成就点数折算镜币的比例, 服务端 services/achievement.ts 使用同一个值. */
constexpr int32_t kCoinsPerAchievementPoint = 100;

/*
 * 成就与任务的对外总装.
 *
 * 游戏侧只向这里发事件: 破坏方块, 放置方块, 采集物品, 每帧的在线与行走,
 * 以及签到. 事件先落到累计统计, 再由统计派生成就进度, 每日任务与里程碑.
 * 全部状态在进程内只有一份, 由互斥量保护: 原生主线程发事件,
 * Java 线程通过 JNI 取快照与写入签到事件, 两侧不会读到半更新状态.
 *
 * 落盘策略: 有改动时每 kFlushSeconds 秒写一次键值存档, 每 kSnapshotSeconds 秒
 * 写一次界面快照; 离开游戏时由 Flush 立即写盘.
 */
class ProgressHub {
public:
    static ProgressHub& Instance();

    ProgressHub(const ProgressHub&) = delete;
    ProgressHub& operator=(const ProgressHub&) = delete;

    /*
     * 指定应用内部存储目录, 载入进度. 重复调用幂等: 目录未变化时不重复读盘;
     * 目录变化时以新目录为准重新载入, 便于开发期切换数据目录.
     */
    bool Initialize(const char* dataPath);

    bool Ready() const;

    /* 破坏方块: 推进开采类成就, 破坏计数与破坏类每日任务. */
    void OnBlockBroken(uint32_t clientBlockId);

    /* 放置方块: 推进使用类成就, 放置计数与放置类每日任务. */
    void OnBlockPlaced(uint32_t clientBlockId);

    /* 采集物品: 推进拾取类成就, 采集计数与采集类每日任务. */
    void OnItemPicked(uint32_t clientBlockId, int32_t count);

    /* 签到成功: 推进签到类每日任务. */
    void OnCheckin();

    /* 每帧调用: 累计在线时长与行走距离, 处理跨日与低频落盘. */
    void Tick(double deltaSeconds, float distanceMeters);

    /* 立即落盘, 离开游戏前调用. */
    void Flush();

    /* 导出界面快照文本. 未初始化时返回只有版本头的空快照. */
    std::string ExportSnapshot();

private:
    ProgressHub() = default;

    /* 事件分发: 累计统计 -> 成就 -> 每日任务 -> 里程碑, 调用方必须持锁. */
    void DispatchLocked(StatKind stat, int32_t clientBlockId, int64_t amount);

    /* 跨日处理: 记录新的自然日, 刷新在线天数与当日任务. */
    void EnsureDayLocked();

    void AddOnlineLocked(double deltaSeconds);
    void AddWalkLocked(double distanceMeters);

    void LoadLocked();
    void SaveLocked(bool includeSnapshot);
    std::string BuildSnapshotLocked() const;

    mutable std::mutex mutex_;

    ProgressStore store_;
    ProgressStats stats_;
    AchievementTracker tracker_;
    DailyTaskBoard daily_;
    MilestoneBoard milestones_;

    std::string today_;
    double onlineSeconds_ = 0.0;
    double walkMeters_ = 0.0;
    double flushAccumulator_ = 0.0;
    double snapshotAccumulator_ = 0.0;
    bool ready_ = false;
    bool dirty_ = false;
};

}  /* namespace immirror::progress */
