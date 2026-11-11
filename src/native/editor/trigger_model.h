#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "editor/editor_json.h"
#include "editor/editor_types.h"
#include "voxel/voxel_types.h"

namespace immirror::editor {

/*
 * 触发器事件类型. 三种事件都在编辑器与试玩模式下驱动,
 * 与原版 triggeritem 表里的"事件"控件一一对应, 具体 ID 由生成的参数表给出.
 */
enum class TriggerEventType {
    kEnterRegion = 0, /* 玩家进入区域 */
    kBlockBroken = 1, /* 任意方块被破坏 */
    kTimer = 2,       /* 定时器到期 */
};

/* 触发器动作类型, 与原版 triggeritem 表里的"动作"控件对应. */
enum class TriggerActionType {
    kLog = 0,      /* 输出日志 */
    kSetBlock = 1, /* 在指定位置放置方块 */
    kTeleport = 2, /* 把玩家传送到指定位置 */
};

/* 单条动作. 字段按类型使用, 未使用的字段保持默认值. */
struct TriggerAction {
    TriggerActionType type = TriggerActionType::kLog;
    int32_t itemId = 0;      /* 原版 triggeritem ID, 写盘与回溯用 */
    std::string text;        /* log: 输出文本 */
    int32_t x = 0;           /* set_block: 方块坐标 */
    int32_t y = 0;
    int32_t z = 0;
    int32_t block = 0;       /* set_block: 客户端方块 id */
    float fx = 0.0f;         /* teleport: 世界坐标 */
    float fy = 0.0f;
    float fz = 0.0f;

    /* 该动作在脚本 DSL 里对应的一行源码, 不带分号. */
    std::string ToScriptCall() const;

    /* 界面显示用的单行摘要, ASCII. */
    std::string DescribeAscii() const;
};

/*
 * 一条触发器.
 * actions 是编辑期的结构化模型, script 是真正交给脚本运行时执行的源码:
 * 属性面板改动动作后调用 RebuildScript 同步, 直接编辑源码时以 script 为准.
 */
struct Trigger {
    int32_t id = 0;
    std::string name;
    bool enabled = true;
    TriggerEventType eventType = TriggerEventType::kEnterRegion;
    Region region;                          /* 进入区域事件的区域 */
    voxel::BlockId brokenBlock = 0;         /* 破坏方块事件的目标方块, 0 表示任意 */
    float intervalSeconds = 2.0f;           /* 定时事件的间隔秒数 */
    bool fireImmediately = false;           /* 定时事件是否在创建后立刻触发一次 */
    std::vector<TriggerAction> actions;
    std::string script;

    /* 用 actions 重新生成脚本源码. */
    void RebuildScript();

    /* 事件为定时器且已启用时是否参与计时. */
    bool IsTimed() const { return enabled && eventType == TriggerEventType::kTimer; }
};

/* 字符串键与显示名, 工程 JSON 与界面共用同一套键. */
const char* TriggerEventKey(TriggerEventType type);
const char* TriggerEventNameEn(TriggerEventType type);
const char* TriggerEventNameZh(TriggerEventType type);
const char* TriggerActionKey(TriggerActionType type);
const char* TriggerActionNameEn(TriggerActionType type);
const char* TriggerActionNameZh(TriggerActionType type);

bool ParseTriggerEventKey(const std::string& key, TriggerEventType& out);
bool ParseTriggerActionKey(const std::string& key, TriggerActionType& out);

/* 原版 triggeritem ID, 取自生成的参数表; 表里没有时返回 0. */
int32_t TriggerEventItemId(TriggerEventType type);
int32_t TriggerActionItemId(TriggerActionType type);

/* 事件与动作的可选项个数, 属性面板按序号循环切换. */
constexpr int32_t kEventTypeCount = 3;
constexpr int32_t kActionTypeCount = 3;

TriggerEventType TriggerEventAt(int32_t index);
TriggerActionType TriggerActionAt(int32_t index);

/* 触发器与 JSON 的互转. 解析失败时 error 给出字段级原因. */
JsonValue TriggerToJson(const Trigger& trigger);
bool TriggerFromJson(const JsonValue& value, Trigger& out, std::string& error);

}  /* namespace immirror::editor */
