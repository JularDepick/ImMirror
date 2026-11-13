#include "progress/progress_hub.h"

#include <cstring>

#include <algorithm>
#include <cstdio>

#include "core/log.h"

namespace immirror::progress {
namespace {

constexpr const char* kTag = "ImMirrorProgress";

/* 落盘节奏: 键值存档五秒一次, 界面快照十五秒一次, 减少内部存储的写入量. */
constexpr double kFlushSeconds = 5.0;
constexpr double kSnapshotSeconds = 15.0;

/* 快照版本与文件头, 界面按版本号判断能否解析. */
constexpr int32_t kSnapshotVersion = 1;

std::vector<std::string> SplitComma(const std::string& text) {
    std::vector<std::string> parts;
    size_t begin = 0;
    while (begin <= text.size()) {
        const size_t comma = text.find(',', begin);
        const size_t end = comma == std::string::npos ? text.size() : comma;
        if (end > begin) {
            parts.push_back(text.substr(begin, end - begin));
        }
        if (comma == std::string::npos) {
            break;
        }
        begin = comma + 1;
    }
    return parts;
}

std::string JoinComma(const std::vector<std::string>& parts) {
    std::string text;
    for (size_t index = 0; index < parts.size(); ++index) {
        if (index > 0) {
            text += ',';
        }
        text += parts[index];
    }
    return text;
}

/* 快照是制表符分隔的行文本, 字段里的制表符与换行必须换掉, 否则界面解析会错列. */
std::string Sanitize(const char* text) {
    if (text == nullptr) {
        return std::string();
    }
    std::string result;
    for (const char ch : std::string(text)) {
        if (ch == '\t' || ch == '\n' || ch == '\r') {
            result += ' ';
        } else {
            result += ch;
        }
    }
    return result;
}

}  /* namespace */

ProgressHub& ProgressHub::Instance() {
    static ProgressHub hub;
    return hub;
}

bool ProgressHub::Initialize(const char* dataPath) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (dataPath == nullptr || dataPath[0] == '\0') {
        return false;
    }
    if (ready_ && store_.Directory() == std::string(dataPath) + "/progress") {
        return true;
    }
    if (!store_.SetDirectory(dataPath)) {
        return false;
    }
    LoadLocked();
    ready_ = true;
    EnsureDayLocked();
    milestones_.Refresh(stats_);
    /* 首次进入就把快照写出来, 界面在游戏未启动时也能看到进度. */
    SaveLocked(true);
    IM_LOGI(kTag, "progress ready dir=%s achievements=%d trackable=%d completed=%d days=%d",
            store_.Directory().c_str(), tracker_.TotalCount(), tracker_.TrackableCount(),
            tracker_.CompletedCount(), static_cast<int32_t>(stats_.Get(StatKind::kOnlineDays)));
    return true;
}

bool ProgressHub::Ready() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ready_;
}

void ProgressHub::OnBlockBroken(uint32_t clientBlockId) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ready_) {
        return;
    }
    EnsureDayLocked();
    DispatchLocked(StatKind::kBlockBreak, static_cast<int32_t>(clientBlockId), 1);
}

void ProgressHub::OnBlockPlaced(uint32_t clientBlockId) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ready_) {
        return;
    }
    EnsureDayLocked();
    DispatchLocked(StatKind::kBlockPlace, static_cast<int32_t>(clientBlockId), 1);
}

void ProgressHub::OnItemPicked(uint32_t clientBlockId, int32_t count) {
    if (count <= 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ready_) {
        return;
    }
    EnsureDayLocked();
    DispatchLocked(StatKind::kItemPick, static_cast<int32_t>(clientBlockId), count);
}

void ProgressHub::OnCraft(int32_t amount) {
    if (amount <= 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ready_) {
        return;
    }
    EnsureDayLocked();
    DispatchLocked(StatKind::kCraft, -1, amount);
}

void ProgressHub::OnSmelt(int32_t amount) {
    if (amount <= 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ready_) {
        return;
    }
    EnsureDayLocked();
    DispatchLocked(StatKind::kSmelt, -1, amount);
}

void ProgressHub::OnKill(int32_t amount) {
    if (amount <= 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ready_) {
        return;
    }
    EnsureDayLocked();
    DispatchLocked(StatKind::kKill, -1, amount);
}

void ProgressHub::OnCheckin() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ready_) {
        return;
    }
    EnsureDayLocked();
    DispatchLocked(StatKind::kCheckin, -1, 1);
}

void ProgressHub::Tick(double deltaSeconds, float distanceMeters) {
    if (deltaSeconds <= 0.0) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ready_) {
        return;
    }
    EnsureDayLocked();
    AddOnlineLocked(deltaSeconds);
    if (distanceMeters > 0.0f) {
        AddWalkLocked(static_cast<double>(distanceMeters));
    }

    flushAccumulator_ += deltaSeconds;
    snapshotAccumulator_ += deltaSeconds;
    if (!dirty_) {
        return;
    }
    if (flushAccumulator_ >= kFlushSeconds) {
        flushAccumulator_ = 0.0;
        const bool withSnapshot = snapshotAccumulator_ >= kSnapshotSeconds;
        if (withSnapshot) {
            snapshotAccumulator_ = 0.0;
        }
        SaveLocked(withSnapshot);
    }
}

void ProgressHub::Flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ready_) {
        return;
    }
    flushAccumulator_ = 0.0;
    snapshotAccumulator_ = 0.0;
    SaveLocked(true);
}

std::string ProgressHub::ExportSnapshot() {
    std::lock_guard<std::mutex> lock(mutex_);
    return BuildSnapshotLocked();
}

void ProgressHub::DispatchLocked(StatKind stat, int32_t clientBlockId, int64_t amount) {
    if (amount <= 0) {
        return;
    }
    stats_.Add(stat, amount);
    const int32_t finished = tracker_.Advance(stat, clientBlockId, amount);
    daily_.OnEvent(stat, amount);
    milestones_.Refresh(stats_);
    dirty_ = true;
    if (finished > 0) {
        IM_LOGI(kTag, "achievement completed stat=%s block=%d total=%d", StatKey(stat),
                clientBlockId, tracker_.CompletedCount());
    }
}

void ProgressHub::EnsureDayLocked() {
    const std::string today = LocalDateKey();
    if (today.empty() || today == today_) {
        return;
    }
    today_ = today;
    if (stats_.RecordDayKey(today)) {
        const int32_t days = static_cast<int32_t>(stats_.Get(StatKind::kOnlineDays));
        const int32_t finished = tracker_.AdvanceOnlineDays(days);
        IM_LOGI(kTag, "new play day=%s online_days=%d completed=%d", today.c_str(), days, finished);
    }
    if (daily_.Refresh(today)) {
        IM_LOGI(kTag, "daily tasks refreshed date=%s", today.c_str());
    }
    dirty_ = true;
}

int64_t ProgressHub::StatTotal(const char* key) const {
    if (key == nullptr) {
        return 0;
    }
    /* 名字表与统计键一一对应, 界面用名字读取, 避免暴露枚举序号. */
    struct Entry {
        const char* name;
        StatKind kind;
    };
    static const Entry kEntries[] = {
        {"breakBlocks", StatKind::kBlockBreak},   {"placeBlocks", StatKind::kBlockPlace},
        {"pickItems", StatKind::kItemPick},       {"walkDistance", StatKind::kWalkDistance},
        {"onlineSeconds", StatKind::kOnlineSeconds}, {"checkin", StatKind::kCheckin},
        {"onlineDays", StatKind::kOnlineDays},    {"craft", StatKind::kCraft},
        {"smelt", StatKind::kSmelt},              {"kill", StatKind::kKill},
    };
    for (const Entry& entry : kEntries) {
        if (std::strcmp(entry.name, key) == 0) {
            return stats_.Get(entry.kind);
        }
    }
    return 0;
}

void ProgressHub::AddOnlineLocked(double deltaSeconds) {
    onlineSeconds_ += deltaSeconds;
    const int64_t minutes = static_cast<int64_t>(onlineSeconds_ / 60.0);
    if (minutes <= 0) {
        return;
    }
    onlineSeconds_ -= static_cast<double>(minutes) * 60.0;
    DispatchLocked(StatKind::kOnlineMinutes, -1, minutes);
}

void ProgressHub::AddWalkLocked(double distanceMeters) {
    walkMeters_ += distanceMeters;
    const int64_t meters = static_cast<int64_t>(walkMeters_);
    if (meters <= 0) {
        return;
    }
    walkMeters_ -= static_cast<double>(meters);
    DispatchLocked(StatKind::kWalkDistance, -1, meters);
}

void ProgressHub::LoadLocked() {
    std::vector<ProgressStore::Entry> entries;
    store_.LoadState(&entries);

    stats_.Reset();
    tracker_.Reset();
    for (const ProgressStore::Entry& entry : entries) {
        if (entry.name.compare(0, 5, "stat.") != 0) {
            continue;
        }
        const StatKind kind = StatKindByKey(entry.name.substr(5));
        if (kind != StatKind::kCount) {
            stats_.Set(kind, entry.value);
        }
    }
    /* 天数键表在校验完整后再套用, 它会覆盖 stat.online_days. */
    for (const ProgressStore::Entry& entry : entries) {
        if (entry.name == "daykeys" && !entry.text.empty()) {
            stats_.SetDayKeys(SplitComma(entry.text));
        }
        if (entry.name == "date") {
            today_ = entry.text;
        }
    }
    tracker_.Load(entries);
    daily_.Load(entries);
    milestones_.Load(entries);
    milestones_.Refresh(stats_);
}

void ProgressHub::SaveLocked(bool includeSnapshot) {
    if (!ready_) {
        return;
    }
    std::vector<ProgressStore::Entry> entries;
    entries.reserve(static_cast<size_t>(kStatCount + tracker_.TotalCount() + kMilestoneCount + 8));

    ProgressStore::Entry dateEntry;
    dateEntry.name = "date";
    dateEntry.text = today_;
    entries.push_back(dateEntry);

    ProgressStore::Entry dayEntry;
    dayEntry.name = "daykeys";
    dayEntry.text = JoinComma(stats_.DayKeys());
    entries.push_back(dayEntry);

    ProgressStore::Entry taskDate;
    taskDate.name = "task.date";
    taskDate.text = daily_.DateKey();
    entries.push_back(taskDate);

    for (int32_t index = 0; index < kStatCount; ++index) {
        const StatKind kind = static_cast<StatKind>(index);
        if (kind == StatKind::kOnlineDays) {
            /* 在线天数由天数键表推导, 不重复落盘. */
            continue;
        }
        const int64_t value = stats_.Get(kind);
        if (value <= 0) {
            continue;
        }
        ProgressStore::Entry entry;
        entry.name = std::string("stat.") + StatKey(kind);
        entry.value = value;
        entries.push_back(entry);
    }

    tracker_.CollectEntries(&entries);
    daily_.CollectEntries(&entries);
    milestones_.CollectEntries(&entries);

    const bool saved = store_.SaveState(entries);
    if (includeSnapshot) {
        store_.SaveSnapshot(BuildSnapshotLocked());
    }
    dirty_ = false;
    IM_LOGD(kTag, "progress saved entries=%zu snapshot=%d state=%d", entries.size(),
            includeSnapshot ? 1 : 0, saved ? 1 : 0);
}

std::string ProgressHub::BuildSnapshotLocked() const {
    std::string out;
    out.reserve(32 * 1024);
    char buffer[256];

    out += "# ImMirror progress snapshot v";
    out += std::to_string(kSnapshotVersion);
    out += "\n";
    out += "# 由原生 progress 模块生成, 界面只读; 字段以制表符分隔\n";

    std::snprintf(buffer, sizeof(buffer), "meta\tversion\t%d\n", kSnapshotVersion);
    out += buffer;
    std::snprintf(buffer, sizeof(buffer), "meta\tdate\t%s\n", today_.c_str());
    out += buffer;
    std::snprintf(buffer, sizeof(buffer), "meta\tonlineDays\t%d\n",
                  static_cast<int32_t>(stats_.Get(StatKind::kOnlineDays)));
    out += buffer;
    std::snprintf(buffer, sizeof(buffer), "meta\tachievements\t%d\t%d\t%d\n",
                  tracker_.TotalCount(), tracker_.TrackableCount(), tracker_.CompletedCount());
    out += buffer;
    std::snprintf(buffer, sizeof(buffer), "meta\ttaskDate\t%s\n", daily_.DateKey().c_str());
    out += buffer;
    std::snprintf(buffer, sizeof(buffer), "meta\tcoinsPerPoint\t%d\n", kCoinsPerAchievementPoint);
    out += buffer;

    for (int32_t index = 0; index < kStatCount; ++index) {
        const StatKind kind = static_cast<StatKind>(index);
        std::snprintf(buffer, sizeof(buffer), "stat\t%s\t%lld\n", StatKey(kind),
                      static_cast<long long>(stats_.Get(kind)));
        out += buffer;
    }

    for (const AchievementTracker::Item& item : tracker_.Items()) {
        if (item.def == nullptr) {
            continue;
        }
        std::snprintf(buffer, sizeof(buffer), "achievement\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t",
                      item.def->id, item.progress, item.def->target, item.done ? 1 : 0,
                      item.def->eventKind != static_cast<int32_t>(GoalAction::kNone) ? 1 : 0,
                      item.def->point, item.def->point * kCoinsPerAchievementPoint, item.def->category);
        out += buffer;
        out += Sanitize(item.def->name);
        out += '\t';
        out += Sanitize(item.def->description);
        out += '\n';
    }

    const std::vector<DailyTaskBoard::Task>& tasks = daily_.Tasks();
    for (size_t slot = 0; slot < tasks.size(); ++slot) {
        const DailyTaskBoard::Task& task = tasks[slot];
        if (task.def == nullptr) {
            continue;
        }
        std::snprintf(buffer, sizeof(buffer), "task\t%zu\t%s\t%d\t%d\t%d\t%d\t",
                      slot, StatKey(task.def->stat), task.progress, task.def->target,
                      task.done ? 1 : 0, task.def->coins);
        out += buffer;
        out += Sanitize(task.def->title);
        out += '\n';
    }

    for (const MilestoneBoard::Item& item : milestones_.Items()) {
        if (item.def == nullptr) {
            continue;
        }
        std::snprintf(buffer, sizeof(buffer), "milestone\t%s\t%lld\t%d\t%d\t", item.def->key,
                      static_cast<long long>(item.progress), item.def->target, item.done ? 1 : 0);
        out += buffer;
        out += Sanitize(item.def->title);
        out += '\n';
    }
    return out;
}

}  /* namespace immirror::progress */
