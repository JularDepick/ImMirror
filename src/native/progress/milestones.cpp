#include "progress/milestones.h"

#include <algorithm>

namespace immirror::progress {
namespace {

/* 里程碑表: 覆盖破坏, 放置, 采集, 行走与在线五条主线, 阈值按数量级递进. */
constexpr MilestoneDef kMilestones[kMilestoneCount] = {
    {"break_block_50", StatKind::kBlockBreak, 50, "累计破坏 50 个方块"},
    {"break_block_500", StatKind::kBlockBreak, 500, "累计破坏 500 个方块"},
    {"break_block_5000", StatKind::kBlockBreak, 5000, "累计破坏 5000 个方块"},
    {"place_block_100", StatKind::kBlockPlace, 100, "累计放置 100 个方块"},
    {"place_block_1000", StatKind::kBlockPlace, 1000, "累计放置 1000 个方块"},
    {"pick_item_200", StatKind::kItemPick, 200, "累计采集 200 个物品"},
    {"pick_item_2000", StatKind::kItemPick, 2000, "累计采集 2000 个物品"},
    {"walk_distance_1000", StatKind::kWalkDistance, 1000, "累计行走 1000 米"},
    {"walk_distance_10000", StatKind::kWalkDistance, 10000, "累计行走 10000 米"},
    {"online_600", StatKind::kOnlineMinutes, 600, "累计在线 600 分钟"},
};

constexpr const char* kPrefix = "milestone.";

}  /* namespace */

const MilestoneDef* Milestones() {
    return kMilestones;
}

MilestoneBoard::MilestoneBoard() {
    items_.reserve(kMilestoneCount);
    for (int32_t index = 0; index < kMilestoneCount; ++index) {
        Item item;
        item.def = &kMilestones[index];
        item.progress = 0;
        item.done = false;
        items_.push_back(item);
    }
}

void MilestoneBoard::Load(const std::vector<ProgressStore::Entry>& entries) {
    for (const ProgressStore::Entry& entry : entries) {
        if (entry.value == 0 || entry.name.compare(0, 10, kPrefix) != 0) {
            continue;
        }
        const std::string key = entry.name.substr(10);
        for (Item& item : items_) {
            if (item.def != nullptr && key == item.def->key) {
                item.done = true;
                break;
            }
        }
    }
}

void MilestoneBoard::CollectEntries(std::vector<ProgressStore::Entry>* out) const {
    if (out == nullptr) {
        return;
    }
    for (const Item& item : items_) {
        if (!item.done || item.def == nullptr) {
            continue;
        }
        ProgressStore::Entry entry;
        entry.name = std::string(kPrefix) + item.def->key;
        entry.value = 1;
        out->push_back(entry);
    }
}

int32_t MilestoneBoard::Refresh(const ProgressStats& stats) {
    int32_t added = 0;
    for (Item& item : items_) {
        if (item.def == nullptr) {
            continue;
        }
        item.progress = stats.Get(item.def->stat);
        if (!item.done && item.progress >= item.def->target) {
            item.done = true;
            ++added;
        }
    }
    return added;
}

}  /* namespace immirror::progress */
