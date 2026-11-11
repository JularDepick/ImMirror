#include "progress/achievement_tracker.h"

#include <algorithm>
#include <cstdlib>

namespace immirror::progress {

AchievementTracker::AchievementTracker() {
    items_.reserve(static_cast<size_t>(gen::kAchievementCount));
    for (int32_t index = 0; index < gen::kAchievementCount; ++index) {
        Item item;
        item.def = &gen::kAchievements[index];
        item.progress = 0;
        item.done = false;
        items_.push_back(item);
        if (item.def->action != static_cast<int32_t>(GoalAction::kNone)) {
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
        const gen::AchievementDef* def = gen::FindAchievement(id);
        if (def == nullptr) {
            continue;
        }
        const int32_t index = static_cast<int32_t>(def - gen::kAchievements);
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
        const int64_t capped = std::min<int64_t>(raw, item.def->goalTarget);
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
        if (item.def->action != static_cast<int32_t>(GoalAction::kOnlineDay) || item.done) {
            continue;
        }
        const int32_t capped = std::min<int32_t>(onlineDays, item.def->goalTarget);
        ApplyProgress(item, capped);
        if (item.done) {
            ++finished;
        }
    }
    return finished;
}

bool AchievementTracker::Matches(const gen::AchievementDef& def, GoalAction action,
                                 int32_t clientBlockId) const {
    if (def.action != static_cast<int32_t>(action)) {
        return false;
    }
    /*
     * matchBlock 为负表示原版对象 ID 折算不到客户端方块, 此时按同类动作任意方块计数;
     * 非负时必须命中同一种方块, 与原版"指定对象"的语义一致.
     */
    if (def.matchBlock < 0) {
        return true;
    }
    return def.matchBlock == clientBlockId;
}

void AchievementTracker::ApplyProgress(Item& item, int32_t value) {
    if (item.def == nullptr) {
        return;
    }
    const int32_t clamped = value < 0 ? 0 : std::min(value, item.def->goalTarget);
    if (clamped <= item.progress) {
        return;
    }
    item.progress = clamped;
    if (!item.done && item.progress >= item.def->goalTarget) {
        item.done = true;
        ++completed_;
    }
}

}  /* namespace immirror::progress */
