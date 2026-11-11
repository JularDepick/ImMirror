#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "progress/progress_stats.h"
#include "progress/progress_store.h"
#include "progress/progress_types.h"

namespace immirror::progress {

/* 里程碑定义: 累计统计跨过阈值即达成, 只做展示, 不发放货币. */
struct MilestoneDef {
    const char* key;  /* 稳定键名, 同时作为落盘键 */
    StatKind stat;    /* 关联的累计统计 */
    int32_t target;   /* 达成阈值 */
    const char* title;  /* 中文标题 */
};

constexpr int32_t kMilestoneCount = 10;

const MilestoneDef* Milestones();

/*
 * 里程碑进度. 达成状态只增不减, 进度直接取当前累计统计,
 * 因此不需要单独维护进度值, 只需记录达成的键名.
 */
class MilestoneBoard {
public:
    struct Item {
        const MilestoneDef* def = nullptr;
        int64_t progress = 0;
        bool done = false;
    };

    MilestoneBoard();

    void Load(const std::vector<ProgressStore::Entry>& entries);
    void CollectEntries(std::vector<ProgressStore::Entry>* out) const;

    /* 按当前统计刷新进度, 返回本次新达成的条数. */
    int32_t Refresh(const ProgressStats& stats);

    const std::vector<Item>& Items() const { return items_; }

private:
    std::vector<Item> items_;
};

}  /* namespace immirror::progress */
