#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "game/furnace.h"
#include "game/item/item_def.h"

namespace immirror::game {

using item::ItemId;
using item::ItemStack;

/*
 * 储物箱实例. 27 格储物空间, 按 9 列 3 行排布, 与原版大箱子的格数一致.
 * chestdef.csv 的战利品表本期不消费: 世界里放置的箱子一律开成空箱,
 * 战利品随机掉落等接入 chestdef 时再补.
 */
class ChestState {
public:
    /* 格数, 与 tuning::kChestSlots 一致, 编译期对照. */
    static constexpr int32_t kSlots = tuning::kChestSlots;

    /* 越界索引回落到首末格, 与物品栏的取值约定一致. */
    const ItemStack& Slot(int32_t index) const;
    ItemStack& MutableSlot(int32_t index);

private:
    static int32_t ClampIndex(int32_t index);

    ItemStack slots_[kSlots];
};

/* 世界里全部储物箱的容器: 键是世界方块坐标编码. */
using ChestMap = std::unordered_map<uint64_t, ChestState>;

/*
 * 容器存档. 熔炉与箱子状态写入独立于世界存档的容器文件:
 * 世界存档格式由 world 模块持有, 本期不改动; 容器文件自带版本头,
 * 与世界存档配套读写, 文件缺失或版本不符时按空容器处理, 向后兼容.
 * 返回 false 表示文件存在但无法解析, 调用方记日志即可, 不阻断进世界.
 */
bool SaveContainersToFile(const char* path, const FurnaceMap& furnaces, const ChestMap& chests);
bool LoadContainersFromFile(const char* path, FurnaceMap& furnaces, ChestMap& chests);

}  /* namespace immirror::game */
