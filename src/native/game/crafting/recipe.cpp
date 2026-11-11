#include "game/crafting/recipe.h"

namespace immirror::game::crafting {

void CraftingGrid::Clear() {
    for (int32_t index = 0; index < kGridSlotCount; ++index) {
        slots_[index].Clear();
    }
}

const ItemStack& CraftingGrid::Slot(int32_t index) const {
    return slots_[ClampIndex(index)];
}

ItemStack& CraftingGrid::MutableSlot(int32_t index) {
    return slots_[ClampIndex(index)];
}

void CraftingGrid::SetSlot(int32_t index, const ItemStack& stack) {
    slots_[ClampIndex(index)] = stack;
}

ItemStack CraftingGrid::Take(int32_t index) {
    ItemStack stack = slots_[ClampIndex(index)];
    slots_[ClampIndex(index)].Clear();
    return stack;
}

bool CraftingGrid::Empty() const {
    return FilledSlots() == 0;
}

int32_t CraftingGrid::FilledSlots() const {
    int32_t filled = 0;
    for (int32_t index = 0; index < kGridSlotCount; ++index) {
        if (!slots_[index].Empty()) {
            ++filled;
        }
    }
    return filled;
}

int32_t CraftingGrid::ClampIndex(int32_t index) {
    if (index < 0) {
        return 0;
    }
    if (index >= kGridSlotCount) {
        return kGridSlotCount - 1;
    }
    return index;
}

}  /* namespace immirror::game::crafting */
