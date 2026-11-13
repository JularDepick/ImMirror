#include "game/armor.h"

#include "game/item/tool_table.h"

namespace immirror::game {

int32_t ArmorSet::SlotOf(item::ItemId id) {
    item::ToolCombat combat;
    if (!item::ToolTable::Instance().Find(id, combat)) {
        return -1;
    }
    /* 装备表类型 8 到 11 依次对应头盔, 胸甲, 护腿与靴子. */
    if (!item::ToolTable::IsArmorType(combat.type)) {
        return -1;
    }
    return static_cast<int32_t>(combat.type) - 8;
}

item::ItemId ArmorSet::Equip(item::ItemId id) {
    const int32_t slot = SlotOf(id);
    if (slot < 0) {
        return id;
    }
    const item::ItemId previous = slots_[slot];
    slots_[slot] = id;
    return previous;
}

float ArmorSet::ReductionPercent(DamageType type) const {
    const int32_t index = static_cast<int32_t>(type);
    if (index < 0 || index >= kDamageTypeCount) {
        return 0.0f;
    }
    float total = 0.0f;
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        item::ToolCombat combat;
        if (!item::ToolTable::Instance().Find(slots_[slot], combat)) {
            continue;
        }
        total += combat.armor[index];
    }
    return total > kMaxReduction ? kMaxReduction : total;
}

float ArmorSet::Apply(DamageType type, float damage) const {
    if (!(damage > 0.0f)) {
        return damage;
    }
    const float percent = ReductionPercent(type);
    const float scale = 1.0f - percent / 100.0f;
    /* 即使减伤到上限也保留两成伤害. */
    return damage * (scale < 0.2f ? 0.2f : scale);
}

void ArmorSet::Clear() {
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        slots_[slot] = item::kNone;
    }
}

}  /* namespace immirror::game */
