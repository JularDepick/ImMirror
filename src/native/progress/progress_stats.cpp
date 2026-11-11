#include "progress/progress_stats.h"

#include <algorithm>

namespace immirror::progress {

void ProgressStats::Reset() {
    for (int32_t index = 0; index < kStatCount; ++index) {
        values_[index] = 0;
    }
    dayKeys_.clear();
}

void ProgressStats::Add(StatKind kind, int64_t amount) {
    const int32_t index = StatIndex(kind);
    if (kind == StatKind::kCount || amount == 0) {
        return;
    }
    values_[index] += amount;
    if (values_[index] < 0) {
        values_[index] = 0;
    }
}

void ProgressStats::Set(StatKind kind, int64_t value) {
    if (kind == StatKind::kCount) {
        return;
    }
    values_[StatIndex(kind)] = value < 0 ? 0 : value;
}

int64_t ProgressStats::Get(StatKind kind) const {
    if (kind == StatKind::kCount) {
        return 0;
    }
    return values_[StatIndex(kind)];
}

bool ProgressStats::RecordDayKey(const std::string& key) {
    if (key.empty()) {
        return false;
    }
    if (std::find(dayKeys_.begin(), dayKeys_.end(), key) != dayKeys_.end()) {
        return false;
    }
    dayKeys_.push_back(key);
    if (dayKeys_.size() > kMaxDayKeys) {
        dayKeys_.erase(dayKeys_.begin());
    }
    /* 在线天数同时作为统计暴露给成就与界面, 保持一致. */
    values_[StatIndex(StatKind::kOnlineDays)] = static_cast<int64_t>(dayKeys_.size());
    return true;
}

void ProgressStats::SetDayKeys(const std::vector<std::string>& keys) {
    dayKeys_ = keys;
    if (dayKeys_.size() > kMaxDayKeys) {
        dayKeys_.erase(dayKeys_.begin(), dayKeys_.end() - static_cast<long>(kMaxDayKeys));
    }
    values_[StatIndex(StatKind::kOnlineDays)] = static_cast<int64_t>(dayKeys_.size());
}

}  /* namespace immirror::progress */
