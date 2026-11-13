#pragma once

#include <cstdint>

#include "game/item/item_def.h"

namespace immirror::game::item {

/*
 * 装备数值: 近战攻击与八类百分比减伤.
 * 类型 8 头盔, 9 胸甲, 10 护腿, 11 靴子, 其余值属于工具与武器.
 */
struct ToolCombat {
    uint8_t type = 0;
    uint8_t level = 0;
    uint16_t enchant = 0;
    float attackPunch = 0.0f;
    /* 顺序与 kDamageTypeCount 一致: 物理, 打击, 远程, 爆炸, 法术, 混沌, 燃烧, 剧毒. */
    float armor[8] = {};
    /* 移动速度加成百分比, 穿戴类装备用. */
    float moveSpeed = 0.0f;
    /* 其余五类攻击值: 远程, 爆炸, 混沌, 燃烧, 剧毒. */
    float attack[5] = {};
    /* 修理配方: 六组材料与数量, 编号 0 表示该组不用. */
    ItemId repairItem[6] = {};
    int32_t repairAmount[6] = {};
    /* 音效路径: 攻击, 发射与使用; 空串表示没有. */
    const char* atkSound = "";
    const char* fireSound = "";
    const char* useSound = "";
};

/*
 * 装备数值表: 物品 id 到数值的只读查表.
 * 数据按 id 升序, 查找用二分; 未登记的物品返回空.
 */
class ToolTable {
public:
    static const ToolTable& Instance();

    bool Find(ItemId id, ToolCombat& out) const;

    /* 是否属于四个护甲部位之一. */
    static bool IsArmorType(uint8_t type) {
        return type >= 8 && type <= 11;
    }

    int32_t Count() const;
};

}  /* namespace immirror::game::item */
