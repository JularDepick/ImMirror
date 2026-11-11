#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "progress/progress_store.h"
#include "progress/progress_types.h"

namespace immirror::progress {

/* 每日任务条目定义. 同一份目录在客户端与服务端各存一份, 键名与目标值必须一致. */
struct DailyTaskDef {
    StatKind stat;    /* 推进该任务的统计种类 */
    int32_t target;   /* 达成所需累计值 */
    int32_t coins;    /* 服务端发放的镜币数, 客户端只用于展示 */
    const char* title;  /* 中文标题 */
};

constexpr int32_t kDailyCatalogCount = 6;
constexpr int32_t kDailyTaskCount = 3;

/* 任务目录, 由原版成就条件归纳而来: 破坏, 放置, 采集, 行走, 在线, 签到. */
const DailyTaskDef* DailyCatalog();

/*
 * 每日任务进度.
 *
 * 当日任务从目录里按日期确定性地选三条, 客户端与服务端用同一套算法,
 * 因此两端选出的任务天然一致; 服务端另外负责发放奖励与防重复领取.
 * 跨自然日时进度归零, 任务按新日期重新选择.
 */
class DailyTaskBoard {
public:
    struct Task {
        const DailyTaskDef* def = nullptr;
        int32_t progress = 0;
        bool done = false;
    };

    /* 从键值存档恢复: 认 "task.date" 与 "task.<槽位>.<统计键>" 两种键. */
    void Load(const std::vector<ProgressStore::Entry>& entries);

    /* 日期变化时按新日期重建任务并清零进度, 返回是否发生了重建. */
    bool Refresh(const std::string& dateKey);

    /* 导出槽位进度, 用于落盘. */
    void CollectEntries(std::vector<ProgressStore::Entry>* out) const;

    /* 统计事件推进当日任务, 返回本次因此完成的条数. */
    int32_t OnEvent(StatKind stat, int64_t amount);

    const std::vector<Task>& Tasks() const { return tasks_; }
    const std::string& DateKey() const { return dateKey_; }

    /*
     * 按日期键挑选当日任务的槽位序号. 客户端与服务端必须完全一致:
     * 以日期键做 FNV-1a 散列, 再用线性同余发生器从剩余目录里逐个抽取, 取满三条.
     */
    static const std::vector<int32_t>& SelectSlots(const std::string& dateKey);

private:
    void Rebuild(const std::string& dateKey);

    std::vector<Task> tasks_;
    std::string dateKey_;

    /* 载入时的日期与槽位进度, 用于把存档进度套到重建后的任务上. */
    std::string loadedDate_;
    int32_t loadedProgress_[kDailyTaskCount] = {};
};

}  /* namespace immirror::progress */
