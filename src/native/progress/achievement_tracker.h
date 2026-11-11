#pragma once

#include <cstdint>
#include <vector>

#include "progress/generated/achievement_data_gen.h"
#include "progress/progress_store.h"
#include "progress/progress_types.h"

namespace immirror::progress {

/*
 * 成就进度.
 *
 * 一条定义对应一条进度记录, 进度只增不减, 达到目标即记为完成.
 * 事件按统计种类分发: 破坏, 放置与采集各自推进对应目标类型的成就,
 * 在线天数单独推进存活天数类成就. 表的条目按 ID 升序, 因此进度记录的
 * 下标与定义表下标一一对应, 落盘时只写非零项.
 */
class AchievementTracker {
public:
    struct Item {
        const gen::AchievementData* def = nullptr;
        int32_t progress = 0;
        bool done = false;
    };

    AchievementTracker();

    void Reset();

    /* 从键值存档恢复: 只认 "ach.<成就ID>" 形式的键, 越界与非法 ID 忽略. */
    void Load(const std::vector<ProgressStore::Entry>& entries);

    /* 导出非零进度, 用于落盘. */
    void CollectEntries(std::vector<ProgressStore::Entry>* out) const;

    /*
     * 计数类事件: clientBlockId 为客户端方块 id, 负值表示不区分方块.
     * 返回本次因此完成的成就条数.
     */
    int32_t Advance(StatKind stat, int32_t clientBlockId, int64_t amount);

    /* 在线天数事件: 直接用当前累计天数推进存活天数类成就, 返回本次完成数. */
    int32_t AdvanceOnlineDays(int32_t onlineDays);

    const std::vector<Item>& Items() const { return items_; }

    int32_t CompletedCount() const { return completed_; }
    int32_t TotalCount() const { return static_cast<int32_t>(items_.size()); }
    int32_t TrackableCount() const { return trackableCount_; }

private:
    /* 定义与事件是否匹配: 动作类别一致, 且方块约束满足. */
    bool Matches(const gen::AchievementData& def, GoalAction action, int32_t clientBlockId) const;

    void ApplyProgress(Item& item, int32_t value);

    std::vector<Item> items_;

    /* 可推进条目的下标表, 事件分发只遍历这张表. */
    std::vector<int32_t> trackable_;

    int32_t trackableCount_ = 0;
    int32_t completed_ = 0;
};

}  /* namespace immirror::progress */
