#include "progress/achievement_tracker.h"

#include <algorithm>
#include <cstdlib>

namespace immirror::progress {

namespace {

/* 表按ID 升序排列, 这里的查找与表顺序无关, 直接线性扫描. */
const gen::AchievementData* FindAchievementById(int32_t id) {
    for (int32_t index = 0; index < gen::kAchievementDataCount; ++index) {
        if (static_cast<int32_t>(gen::kAchievementData[index].id) == id) {
            return &gen::kAchievementData[index];
        }
    }
    return nullptr;
}

}  /* namespace */


AchievementTracker::AchievementTracker() {
    items_.reserve(static_cast<size_t>(gen::kAchievementDataCount));
    for (int32_t index = 0; index < gen::kAchievementDataCount; ++index) {
        Item item;
        item.def = &gen::kAchievementData[index];
        item.progress = 0;
        item.done = false;
        items_.push_back(item);
        if (item.def->eventKind != static_cast<int32_t>(GoalAction::kNone)) {
            trackable_.push_back(index);
        }
    }
    trackableCount_ = static_cast<int32_t>(trackable_.size());
}

void AchievementTracker::Reset() {
    for (Item& item : items_) {
        item.progress = 0;
        item.done = false;
    }
    completed_ = 0;
}

void AchievementTracker::Load(const std::vector<ProgressStore::Entry>& entries) {
    constexpr const char* kPrefix = "ach.";
    for (const ProgressStore::Entry& entry : entries) {
        if (entry.name.compare(0, 4, kPrefix) != 0) {
            continue;
        }
        const int32_t id = static_cast<int32_t>(std::strtol(entry.name.c_str() + 4, nullptr, 10));
        const gen::AchievementData* def = FindAchievementById(id);
        if (def == nullptr) {
            continue;
        }
        const int32_t index = static_cast<int32_t>(def - gen::kAchievementData);
        if (index < 0 || index >= static_cast<int32_t>(items_.size())) {
            continue;
        }
        Item& item = items_[static_cast<size_t>(index)];
        const int32_t value = static_cast<int32_t>(entry.value);
        ApplyProgress(item, value);
    }
}

void AchievementTracker::CollectEntries(std::vector<ProgressStore::Entry>* out) const {
    if (out == nullptr) {
        return;
    }
    for (const Item& item : items_) {
        if (item.progress <= 0 || item.def == nullptr) {
            continue;
        }
        ProgressStore::Entry entry;
        entry.name = std::string("ach.") + std::to_string(item.def->id);
        entry.value = item.progress;
        out->push_back(entry);
    }
}

int32_t AchievementTracker::Advance(StatKind stat, int32_t clientBlockId, int64_t amount) {
    if (amount <= 0) {
        return 0;
    }
    const GoalAction action = ActionOfStat(stat);
    if (action == GoalAction::kNone) {
        return 0;
    }
    int32_t finished = 0;
    for (const int32_t index : trackable_) {
        Item& item = items_[static_cast<size_t>(index)];
        if (!Matches(*item.def, action, clientBlockId)) {
            continue;
        }
        if (item.done) {
            continue;
        }
        const int64_t raw = static_cast<int64_t>(item.progress) + amount;
        const int64_t capped = std::min<int64_t>(raw, item.def->target);
        ApplyProgress(item, static_cast<int32_t>(capped));
        if (item.done) {
            ++finished;
        }
    }
    return finished;
}

int32_t AchievementTracker::AdvanceOnlineDays(int32_t onlineDays) {
    if (onlineDays <= 0) {
        return 0;
    }
    int32_t finished = 0;
    for (const int32_t index : trackable_) {
        Item& item = items_[static_cast<size_t>(index)];
        if (item.def->eventKind != static_cast<int32_t>(GoalAction::kOnlineDay) || item.done) {
            continue;
        }
        const int32_t capped = std::min<int32_t>(onlineDays, item.def->target);
        ApplyProgress(item, capped);
        if (item.done) {
            ++finished;
        }
    }
    return finished;
}

bool AchievementTracker::Matches(const gen::AchievementData& def, GoalAction action,
                                 int32_t clientBlockId) const {
    if (def.eventKind != static_cast<int32_t>(action)) {
        return false;
    }
    /*
     * 当前成就表只保存对象 ID, 尚未折算到客户端方块 ID, 因此按同类动作
     * 任意方块计数. 折算表落地后在这里加一层对象到方块 id 的映射即可.
     */
    (void)def;
    (void)clientBlockId;
    return true;
}

void AchievementTracker::ApplyProgress(Item& item, int32_t value) {
    if (item.def == nullptr) {
        return;
    }
    const int32_t clamped = value < 0 ? 0 : std::min(value, item.def->target);
    if (clamped <= item.progress) {
        return;
    }
    item.progress = clamped;
    if (!item.done && item.progress >= item.def->target) {
        item.done = true;
        ++completed_;
    }
}

}  /* namespace immirror::progress */
