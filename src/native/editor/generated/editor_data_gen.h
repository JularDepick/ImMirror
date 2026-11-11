#pragma once

#include <cstdint>

/*
 * 由 scripts/gen_editor_data.py 从 assets 仓库的 csvdef 生成, 请勿手改.
 * 数据源: assets/project/csvdef 下的 blockdef.csv, triggeritem.csv 与 triggerparam.csv.
 * 本表只承载只读属性, 工程数据与运行时状态不写入这里.
 */
namespace immirror::editor::gen {

/* 方块调色板条目: 客户端可放置方块与原版方块表属性的关联. */
struct BlockEntry {
    uint16_t clientId;      /* 客户端方块 id, 与 voxel::block 命名空间一致 */
    int32_t legacyId;       /* 原版 blockdef 的 ID, 便于回溯 */
    const char* key;        /* 客户端注册表英文键, ASCII */
    const char* nameZh;     /* 原版中文名, UTF-8 */
    float hardness;         /* 原版硬度, -1 表示不可破坏 */
    int32_t mineTool;       /* 原版采集工具: 1 斧 2 镐 3 铲 5 剪刀, 0 徒手 */
    int32_t terrainEditor;  /* 地形编辑器开放位, 0 表示原版不开放 */
    int32_t triggerType;    /* 触发器开放位, 0 表示原版不开放 */
};

/* 触发器控件: 编辑器支持的事件与动作. */
struct TriggerItemEntry {
    int32_t itemId;         /* 原版 triggeritem ID */
    const char* key;        /* ASCII 键, 工程 JSON 与界面共用 */
    const char* nameEn;     /* ASCII 显示名, 点阵字只收录 ASCII */
    const char* nameZh;     /* 原版中文名, UTF-8 */
    int32_t category;       /* 1 事件, 3 动作 */
    int32_t childType;      /* 原版子类序号 */
    int32_t paramId;        /* 第一参数的 triggerparam ID, 0 表示无 */
    int32_t paramTwoId;     /* 第二参数的 triggerparam ID, 0 表示无 */
};

/* 触发器参数描述. */
struct TriggerParamEntry {
    int32_t id;             /* 原版 triggerparam ID */
    const char* key;        /* ASCII 键 */
    const char* nameZh;     /* 中文描述, UTF-8 */
};

/* 原版方块表行数: 1191 */
/* 原版地形编辑器开放方块数: 539 */
/* 原版触发器开放方块数: 673 */

extern const BlockEntry kBlockEntries[];
extern const int32_t kBlockEntryCount;

extern const TriggerItemEntry kTriggerItems[];
extern const int32_t kTriggerItemCount;

extern const TriggerParamEntry kTriggerParams[];
extern const int32_t kTriggerParamCount;

/* 原版方块表行数与地形编辑器/触发器开放方块数, 供自检与界面展示. */
constexpr int32_t kLegacyBlockRows = 1191;
constexpr int32_t kLegacyTerrainEditorBlocks = 539;
constexpr int32_t kLegacyTriggerOpenBlocks = 673;

/* 按客户端方块 id 取条目, 越界返回空气条目. */
const BlockEntry& BlockEntryOf(uint16_t clientId);

/* 按 ASCII 键取触发器控件, 未收录返回空指针. */
const TriggerItemEntry* TriggerItemOfKey(const char* key);

/* 按原版 ID 取触发器控件, 未收录返回空指针. */
const TriggerItemEntry* TriggerItemOfId(int32_t itemId);

}  /* namespace immirror::editor::gen */
