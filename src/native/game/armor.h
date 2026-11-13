#pragma once

#include <cstdint>

#include "game/item/item_def.h"

namespace immirror::game {

/* 伤害类型, 顺序与装备表的护甲列一致. */
enum class DamageType : uint8_t {
    kPhysics = 0,
    kPunch,
    kRange,
    kExplosion,
    kMagic,
    kChaos,
    kBurn,
    kToxin,
};

constexpr int32_t kDamageTypeCount = 8;

/*
 * 护甲套装: 头盔, 胸甲, 护腿, 靴子四个部位.
 *
 * 每个部位记一件物品 id, 合计时按伤害类型把各部位百分比减伤相加,
 * 总减伤封顶 80%, 避免全身极品装备完全免疫.
 */
class ArmorSet {
public:
    static constexpr int32_t kSlotCount = 4;
    static constexpr float kMaxReduction = 80.0f;

    /* 部位序号: 0 头盔, 1 胸甲, 2 护腿, 3 靴子. 非护甲返回 -1. */
    static int32_t SlotOf(item::ItemId id);

    /* 穿上并返回被替换下来的物品; 部位不匹配时原样返回 id 表示未穿上. */
    item::ItemId Equip(item::ItemId id);

    item::ItemId Worn(int32_t slot) const {
        return slot < 0 || slot >= kSlotCount ? item::kNone : slots_[slot];
    }

    /* 指定伤害类型的百分比减伤, 已按上限钳制. */
    float ReductionPercent(DamageType type) const;

    /* 应用减伤; 至少保留 20% 伤害. */
    float Apply(DamageType type, float damage) const;

    void Clear();

private:
    item::ItemId slots_[kSlotCount] = {item::kNone, item::kNone, item::kNone, item::kNone};
};

}  /* namespace immirror::game */
