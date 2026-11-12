#include "progress/daily_tasks.h"

#include <algorithm>
#include <cstdlib>

namespace immirror::progress {
namespace {

/*
 * 任务目录. 目标值取整便于展示与上报, 奖励与服务端 services/achievement.ts
 * 的 DAILY_TASK_CATALOG 保持一致, 改动时两侧同时改.
 */
constexpr DailyTaskDef kDailyCatalog[kDailyCatalogCount] = {
    {StatKind::kBlockBreak, 20, 40, "开山取石"},
    {StatKind::kBlockBreak, 60, 80, "挖掘达人"},
    {StatKind::kBlockPlace, 10, 40, "开工建造"},
    {StatKind::kBlockPlace, 30, 80, "施工队长"},
    {StatKind::kItemPick, 10, 40, "采集新手"},
    {StatKind::kItemPick, 30, 80, "采集行家"},
    {StatKind::kWalkDistance, 100, 30, "四处走走"},
    {StatKind::kWalkDistance, 300, 60, "长途跋涉"},
    {StatKind::kOnlineMinutes, 15, 40, "上线报到"},
    {StatKind::kOnlineMinutes, 30, 80, "冒险常客"},
    {StatKind::kCheckin, 1, 30, "每日签到"},
    {StatKind::kCraft, 10, 40, "巧手匠心"},
    {StatKind::kSmelt, 5, 40, "炉火纯青"},
    {StatKind::kKill, 3, 40, "初试锋芒"},
};

/* 线性同余发生器的常量, 与服务端保持一致. */
constexpr uint32_t kLcgMultiplier = 1664525u;
constexpr uint32_t kLcgIncrement = 1013904223u;

}  /* namespace */

const DailyTaskDef* DailyCatalog() {
    return kDailyCatalog;
}

const std::vector<int32_t>& DailyTaskBoard::SelectSlots(const std::string& dateKey) {
    static thread_local std::vector<int32_t> slots;
    slots.clear();
    uint32_t seed = Fnv1a32(dateKey);
    std::vector<int32_t> pool;
    pool.reserve(kDailyCatalogCount);
    for (int32_t index = 0; index < kDailyCatalogCount; ++index) {
        pool.push_back(index);
    }
    while (static_cast<int32_t>(slots.size()) < kDailyTaskCount && !pool.empty()) {
        seed = seed * kLcgMultiplier + kLcgIncrement;
        const size_t pick = static_cast<size_t>(seed % static_cast<uint32_t>(pool.size()));
        slots.push_back(pool[pick]);
        pool.erase(pool.begin() + static_cast<long>(pick));
    }
    return slots;
}

void DailyTaskBoard::Load(const std::vector<ProgressStore::Entry>& entries) {
    loadedDate_.clear();
    for (int32_t slot = 0; slot < kDailyTaskCount; ++slot) {
        loadedProgress_[slot] = 0;
    }
    for (const ProgressStore::Entry& entry : entries) {
        if (entry.name == "task.date") {
            loadedDate_ = entry.text;
            continue;
        }
        if (entry.name.compare(0, 5, "task.") != 0) {
            continue;
        }
        const char* rest = entry.name.c_str() + 5;
        char* end = nullptr;
        const long slot = std::strtol(rest, &end, 10);
        if (end == nullptr || *end != '.' || slot < 0 || slot >= kDailyTaskCount) {
            continue;
        }
        loadedProgress_[slot] = static_cast<int32_t>(entry.value);
    }
    /* 日期不是进度, 而是判定当日任务是否还有效的依据, 存在时按存档日期重建. */
    if (!loadedDate_.empty()) {
        Rebuild(loadedDate_);
    }
}

bool DailyTaskBoard::Refresh(const std::string& dateKey) {
    if (dateKey.empty() || dateKey == dateKey_) {
        return false;
    }
    Rebuild(dateKey);
    return true;
}

void DailyTaskBoard::CollectEntries(std::vector<ProgressStore::Entry>* out) const {
    if (out == nullptr) {
        return;
    }
    /* 日期由导出方单独写入 task.date, 这里只导出各槽位的进度. */
    for (size_t slot = 0; slot < tasks_.size(); ++slot) {
        const Task& task = tasks_[slot];
        if (task.progress <= 0 || task.def == nullptr) {
            continue;
        }
        ProgressStore::Entry entry;
        entry.name = std::string("task.") + std::to_string(slot) + "." + StatKey(task.def->stat);
        entry.value = task.progress;
        out->push_back(entry);
    }
}

int32_t DailyTaskBoard::OnEvent(StatKind stat, int64_t amount) {
    if (amount <= 0 || tasks_.empty()) {
        return 0;
    }
    int32_t finished = 0;
    for (Task& task : tasks_) {
        if (task.def == nullptr || task.def->stat != stat || task.done) {
            continue;
        }
        const int64_t raw = static_cast<int64_t>(task.progress) + amount;
        const int64_t capped = std::min<int64_t>(raw, task.def->target);
        task.progress = static_cast<int32_t>(capped);
        if (task.progress >= task.def->target) {
            task.progress = task.def->target;
            task.done = true;
            ++finished;
        }
    }
    return finished;
}

void DailyTaskBoard::Rebuild(const std::string& dateKey) {
    dateKey_ = dateKey;
    tasks_.clear();
    const std::vector<int32_t> slots = SelectSlots(dateKey);
    const bool useLoaded = dateKey == loadedDate_;
    for (size_t index = 0; index < slots.size(); ++index) {
        const int32_t catalogIndex = slots[index];
        if (catalogIndex < 0 || catalogIndex >= kDailyCatalogCount) {
            continue;
        }
        Task task;
        task.def = &kDailyCatalog[catalogIndex];
        task.progress = 0;
        task.done = false;
        if (useLoaded && index < static_cast<size_t>(kDailyTaskCount)) {
            const int32_t stored = loadedProgress_[index];
            task.progress = std::min(stored, task.def->target);
            task.done = task.progress >= task.def->target;
        }
        tasks_.push_back(task);
    }
}

}  /* namespace immirror::progress */
