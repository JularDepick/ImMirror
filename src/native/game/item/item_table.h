#pragma once

#include <cstdint>

#include "game/item/item_def.h"
#include "voxel/voxel_types.h"

namespace immirror::game::item {

/*
 * 物品表. 数据来自 scripts/gen_item_data.py 生成的 item_data_gen.h,
 * 进程内唯一且只读, 物品栏, 合成与界面都从这里取名称, 堆叠上限与关联方块.
 * 越界 id 与未知键一律回落到空物品, 调用方不必判空.
 */
class ItemTable {
public:
    static const ItemTable& Instance();

    /* 按物品 id 取属性, 越界 id 回落到空物品条目. */
    const ItemInfo& Info(ItemId id) const;

    /* 已登记条目数量, 也是合法 id 的上界. */
    int32_t Count() const;

    /* 按英文键查物品 id, 未收录返回 kNone. */
    ItemId FindByKey(const char* key) const;

    /* 客户端方块 id 对应的物品 id, 该方块没有物品时返回 kNone. */
    ItemId OfBlock(voxel::BlockId block) const;

    /* 物品放置出的客户端方块 id, 不可放置时返回 block::Air. */
    voxel::BlockId BlockOf(ItemId id) const;

    /*
     * 破坏方块后掉落的物品 id. 掉落规则沿用方块数据表的 ToolMineDrop1 折算结果,
     * 方块本身没有掉落物时返回 kNone.
     */
    ItemId DropOfBlock(voxel::BlockId block) const;

    /* 单格最大堆叠, 越界按 64 处理. */
    int32_t MaxStack(ItemId id) const;

    /*
     * 物品图标使用的方块图集格: 关联方块的物品取方块顶面, 其余物品返回 -1,
     * 界面据此改用首字母色块占位.
     */
    int32_t AtlasCellOf(ItemId id) const;

    /* 界面占位用的首字母, 取英文键首字符并转大写, 空键返回问号. */
    char Initial(ItemId id) const;

private:
    /* 只允许 Instance() 构造, 避免出现第二份表. */
    ItemTable() = default;
};

}  /* namespace immirror::game::item */
