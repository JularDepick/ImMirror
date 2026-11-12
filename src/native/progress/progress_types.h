#pragma once

#include <cstdint>
#include <string>

namespace immirror::progress {

/*
 * 累计统计种类. 事件驱动: 每次破坏方块, 放置方块, 采集物品, 行走与在线都累加一次,
 * 成就, 每日任务与里程碑都从这张表派生进度. 顺序固定, 落盘键名与顺序无关,
 * 因此可以在中间插入新种类而不影响旧存档.
 */
enum class StatKind : int32_t {
    kBlockBreak = 0,
    kBlockPlace = 1,
    kItemPick = 2,
    kWalkDistance = 3,
    kOnlineMinutes = 4,
    kCheckin = 5,
    kOnlineDays = 6,
    kCraft = 7,
    kSmelt = 8,
    kKill = 9,
    kCount = 10,
};

constexpr int32_t kStatCount = static_cast<int32_t>(StatKind::kCount);

/* 落盘与上报用的稳定键名, 服务端按同名键接收. 越界返回空串. */
const char* StatKey(StatKind kind);

/* 界面展示用的中文名. 越界返回空串. */
const char* StatTitle(StatKind kind);

/* 统计种类到下标, 越界时返回 0. */
int32_t StatIndex(StatKind kind);

/* 按键名反查统计种类, 未知键返回 kCount. */
StatKind StatKindByKey(const std::string& key);

/*
 * 成就推进动作类别, 与 generated/achievement_data_gen.h 的 action 列一一对应.
 * 生成表里不可推进的条目 action 为 kNone, 只做展示.
 */
enum class GoalAction : int32_t {
    kNone = 0,
    kCount = 1,
    kOnlineDay = 2,
};

/* 统计种类对应的成就动作类别: 破坏对应开采, 放置对应使用, 采集对应拾取. */
GoalAction ActionOfStat(StatKind kind);

/* 自然日键, 形如 2026-11-11; 按设备本地时区计算. */
std::string LocalDateKey();

/* 从 Unix 秒换算自然日键, 供测试与跨日换算使用. */
std::string LocalDateKeyOf(int64_t unixSeconds);

/* 32 位 FNV-1a 散列. */
uint32_t Fnv1a32(const std::string& text);

}  /* namespace immirror::progress */
