#include "progress/progress_types.h"

#include <ctime>

namespace immirror::progress {
namespace {

/* 统计键名表, 下标即 StatKind. 键名同时用于落盘与服务端上报. */
constexpr const char* kStatKeys[kStatCount] = {
    "break_block",
    "place_block",
    "pick_item",
    "walk_distance",
    "online",
    "checkin",
    "online_days",
};

/* 统计中文名表, 下标即 StatKind. */
constexpr const char* kStatTitles[kStatCount] = {
    "破坏方块",
    "放置方块",
    "采集物品",
    "行走距离",
    "在线时长",
    "签到",
    "在线天数",
};

/* 动作类别映射表, 下标即 StatKind; 签到与在线天数不推进计数类成就. */
constexpr GoalAction kStatActions[kStatCount] = {
    GoalAction::kCount,
    GoalAction::kCount,
    GoalAction::kCount,
    GoalAction::kCount,
    GoalAction::kNone,
    GoalAction::kNone,
    GoalAction::kOnlineDay,
};

}  /* namespace */

const char* StatKey(StatKind kind) {
    const int32_t index = StatIndex(kind);
    return index >= 0 ? kStatKeys[index] : "";
}

const char* StatTitle(StatKind kind) {
    const int32_t index = StatIndex(kind);
    return index >= 0 ? kStatTitles[index] : "";
}

int32_t StatIndex(StatKind kind) {
    const int32_t index = static_cast<int32_t>(kind);
    return (index >= 0 && index < kStatCount) ? index : 0;
}

StatKind StatKindByKey(const std::string& key) {
    for (int32_t index = 0; index < kStatCount; ++index) {
        if (key == kStatKeys[index]) {
            return static_cast<StatKind>(index);
        }
    }
    return StatKind::kCount;
}

GoalAction ActionOfStat(StatKind kind) {
    const int32_t index = static_cast<int32_t>(kind);
    if (index < 0 || index >= kStatCount) {
        return GoalAction::kNone;
    }
    return kStatActions[index];
}

std::string LocalDateKeyOf(int64_t unixSeconds) {
    /* 跨日判定只用本地时区的年月日, 与设备显示的日期保持一致. */
    const std::time_t stamp = static_cast<std::time_t>(unixSeconds);
    std::tm parts {};
    if (::localtime_r(&stamp, &parts) == nullptr) {
        return std::string();
    }
    char buffer[16];
    const int written = std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d",
                                      parts.tm_year + 1900, parts.tm_mon + 1, parts.tm_mday);
    if (written <= 0) {
        return std::string();
    }
    return std::string(buffer);
}

std::string LocalDateKey() {
    return LocalDateKeyOf(static_cast<int64_t>(std::time(nullptr)));
}

uint32_t Fnv1a32(const std::string& text) {
    uint32_t hash = 2166136261u;
    for (const char raw : text) {
        hash ^= static_cast<uint32_t>(static_cast<unsigned char>(raw));
        hash *= 16777619u;
    }
    return hash;
}

}  /* namespace immirror::progress */
