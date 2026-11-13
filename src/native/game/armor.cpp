#include "game/armor.h"

#include "game/item/enchant_table.h"
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

item::ItemStack ArmorSet::Equip(const item::ItemStack& stack) {
    const int32_t slot = SlotOf(stack.item);
    if (slot < 0) {
        return stack;
    }
    const item::ItemStack previous = slots_[slot];
    slots_[slot] = stack;
    return previous;
}

item::ItemId ArmorSet::Equip(item::ItemId id) {
    item::ItemStack stack;
    stack.item = id;
    stack.count = 1;
    const int32_t slot = SlotOf(id);
    if (slot < 0) {
        return id;
    }
    const item::ItemId previous = slots_[slot].item;
    slots_[slot] = stack;
    return previous;
}

const item::ItemStack& ArmorSet::WornStack(int32_t slot) const {
    static const item::ItemStack kEmpty;
    if (slot < 0 || slot >= kSlotCount) {
        return kEmpty;
    }
    return slots_[slot];
}

float ArmorSet::ReductionPercent(DamageType type) const {
    const int32_t index = static_cast<int32_t>(type);
    if (index < 0 || index >= kDamageTypeCount) {
        return 0.0f;
    }
    float total = 0.0f;
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        item::ToolCombat combat;
        if (!item::ToolTable::Instance().Find(slots_[slot].item, combat)) {
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

int32_t ArmorSet::EnchantValue(const char* type) const {
    if (type == nullptr) {
        return 0;
    }
    int32_t total = 0;
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        const item::ItemStack& stack = slots_[slot];
        if (stack.item == item::kNone || stack.enchantId == 0) {
            continue;
        }
        const gen::EnchantEntry* entry =
            item::EnchantTable::Instance().Find(stack.enchantId, stack.enchantLevel);
        if (entry != nullptr && item::EnchantTable::IsType(*entry, type)) {
            total += entry->value1;
        }
    }
    return total;
}

void ArmorSet::Clear() {
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        slots_[slot].Clear();
    }
}

}  /* namespace immirror::game */
