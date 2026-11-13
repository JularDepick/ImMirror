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

    /* 方块到物品的反查: 用于"直接掉落方块本体"一类的采集效果. */
    ItemId ItemOfBlock(voxel::BlockId block) const;

    /*
     * 工具属性查询: 该物品是挖掘类工具时返回条目, 否则返回空指针.
     * 工具表由 scripts/gen_tool_data.py 按名称把工具定义挂到物品上,
     * 挂不上的工具不返回条目, 挖掘时按徒手处理.
     */
    struct ToolInfo {
        uint8_t harvest = 0;     /* 采集工具档位, 与 voxel::HarvestTool 对齐 */
        uint8_t level = 0;       /* 工具等级, 1 起 */
        uint16_t efficiency = 0; /* 效率百分比, 徒手按 100 */
        uint16_t durability = 0; /* 满耐久 */
    };
    bool ToolOf(ItemId id, ToolInfo& out) const;

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

    /*
     * 图标图集格序号: 非方块物品用它取图标; 没有图标返回 -1.
     * 与 AtlasCellOf 分开是因为两者取自不同的图集.
     */
    int32_t IconCellOf(ItemId id) const;

    /* 物品图标代表色: 掉落物这类只需要一个颜色的场合用它; 没有图标返回中灰. */
    void IconColorOf(ItemId id, float* red, float* green, float* blue) const;

    /* 界面占位用的首字母, 取英文键首字符并转大写, 空键返回问号. */
    char Initial(ItemId id) const;

private:
    /* 只允许 Instance() 构造, 避免出现第二份表. */
    ItemTable() = default;
};

}  /* namespace immirror::game::item */
