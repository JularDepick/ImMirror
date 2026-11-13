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
 * 每个部位记一整份堆叠而不是单个物品编号: 附魔与耐久都挂在堆叠上,
 * 只记编号会让穿上身的装备丢掉附魔. 合计时按伤害类型把各部位
 * 百分比减伤相加, 总减伤封顶 80%, 避免全身极品装备完全免疫.
 */
class ArmorSet {
public:
    static constexpr int32_t kSlotCount = 4;
    static constexpr float kMaxReduction = 80.0f;

    /* 部位序号: 0 头盔, 1 胸甲, 2 护腿, 3 靴子. 非护甲返回 -1. */
    static int32_t SlotOf(item::ItemId id);

    /* 穿上并返回被替换下来的堆叠; 部位不匹配时把入参原样返回. */
    item::ItemStack Equip(const item::ItemStack& stack);

    /* 便捷重载: 只给编号时按无附魔无耐久的整份物品处理. */
    item::ItemId Equip(item::ItemId id);

    item::ItemId Worn(int32_t slot) const {
        return slot < 0 || slot >= kSlotCount ? item::kNone : slots_[slot].item;
    }

    const item::ItemStack& WornStack(int32_t slot) const;

    /* 指定伤害类型的百分比减伤, 已按上限钳制. */
    float ReductionPercent(DamageType type) const;

    /* 应用减伤; 至少保留 20% 伤害. */
    float Apply(DamageType type, float damage) const;

    /*
     * 把四个部位上指定类型的附魔数值相加. 只认效果类型字符串,
     * 不关心是哪个部位, 因此头盔与靴子上的同类附魔都能叠加.
     */
    int32_t EnchantValue(const char* type) const;

    /* 近战抗性点数: 四个部位上抗性附魔的数值合计, 按点数直接抵扣近战伤害. */
    int32_t MeleeResistPoints() const { return EnchantValue("PROTECTION"); }

    /* 移动速度加成百分比: 四个部位装备表数值之和, 封顶 50%. */
    float MoveSpeedPercent() const;

    void Clear();

private:
    item::ItemStack slots_[kSlotCount];
};

}  /* namespace immirror::game */
