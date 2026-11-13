#include "game/item/tool_table.h"

#include "game/generated/tool_combat_gen.h"

namespace immirror::game::item {

const ToolTable& ToolTable::Instance() {
    static const ToolTable instance;
    return instance;
}

int32_t ToolTable::Count() const {
    return gen::kToolCombatCount;
}

bool ToolTable::Find(ItemId id, ToolCombat& out) const {
    if (id == kNone) {
        return false;
    }
    int32_t low = 0;
    int32_t high = gen::kToolCombatCount - 1;
    const uint16_t key = static_cast<uint16_t>(id);
    while (low <= high) {
        const int32_t mid = (low + high) / 2;
        const gen::ToolCombatData& data = gen::kToolCombat[mid];
        if (data.id == key) {
            out.type = data.type;
            out.level = data.level;
            out.enchant = data.enchant;
            out.attackPunch = data.attackPunch;
            out.armor[0] = data.armorPhysics;
            out.armor[1] = data.armorPunch;
            out.armor[2] = data.armorRange;
            out.armor[3] = data.armorExplosion;
            out.armor[4] = data.armorMagic;
            out.armor[5] = data.armorChaos;
            out.armor[6] = data.armorBurn;
            out.armor[7] = data.armorToxin;
            out.moveSpeed = data.moveSpeed;
            out.attack[0] = data.attackRange;
            out.attack[1] = data.attackExplosion;
            out.attack[2] = data.attackChaos;
            out.attack[3] = data.attackBurn;
            out.attack[4] = data.attackToxin;
            for (int32_t slot = 0; slot < 6; ++slot) {
                out.repairItem[slot] = static_cast<ItemId>(data.repairItem[slot]);
                out.repairAmount[slot] = static_cast<int32_t>(data.repairAmount[slot]);
            }
            return true;
        }
        if (data.id < key) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    return false;
}

}  /* namespace immirror::game::item */
