#include "game/item/inventory_screen.h"

#include <cstdio>

#include "core/log.h"
#include "game/crafting/recipe_table.h"
#include "game/inventory.h"
#include "game/item/item_table.h"

namespace immirror::game {
namespace {

constexpr const char* kTag = "ImMirrorBag";

/* 把一格堆叠翻译成绘制数据: 图标格, 首字母, 类别配色与数量. */
void FillSlotView(ui::InventorySlotView& view, const ItemStack& stack) {
    view = ui::InventorySlotView{};
    if (stack.Empty()) {
        return;
    }
    const item::ItemTable& table = item::ItemTable::Instance();
    const item::ItemInfo& info = table.Info(stack.item);
    view.hasItem = true;
    view.atlasCell = table.AtlasCellOf(stack.item);
    view.initial = table.Initial(stack.item);
    view.tone = static_cast<uint8_t>(info.category);
    view.count = stack.count;
}

}  /* namespace */

void InventoryScreen::Reset() {
    grid_.Clear();
    candidates_ = crafting::MatchResult{};
    candidateIndex_ = 0;
    bookCursor_ = 0;
    carry_.Clear();
    carryX_ = 0.5f;
    carryY_ = 0.5f;
    open_ = false;
    gridDirty_ = true;
}

void InventoryScreen::SetOpen(bool open, Inventory& inventory) {
    if (open_ == open) {
        return;
    }
    open_ = open;
    if (!open) {
        ReturnAllToInventory(inventory);
    } else {
        gridDirty_ = true;
        EnsureCandidates();
    }
    IM_LOGI(kTag, "bag panel %s, grid=%d carry=%d", open ? "open" : "closed",
            grid_.FilledSlots(), carry_.count);
}

void InventoryScreen::SetCarryPosition(float normalizedX, float normalizedY) {
    carryX_ = normalizedX < 0.0f ? 0.0f : (normalizedX > 1.0f ? 1.0f : normalizedX);
    carryY_ = normalizedY < 0.0f ? 0.0f : (normalizedY > 1.0f ? 1.0f : normalizedY);
}

bool InventoryScreen::HandlePress(const ui::InventoryHit& hit, Inventory& inventory) {
    switch (hit.region) {
        case ui::InventoryRegion::kClose:
            SetOpen(false, inventory);
            return true;
        case ui::InventoryRegion::kCrafting:
            if (hit.index < 0 || hit.index >= crafting::kGridSlotCount) {
                return false;
            }
            ActOnStack(grid_.MutableSlot(hit.index));
            gridDirty_ = true;
            return true;
        case ui::InventoryRegion::kBackpack:
            if (hit.index < 0 || hit.index >= Inventory::kBackpackSlots) {
                return false;
            }
            ActOnStack(inventory.MutableSlot(Inventory::kHotbarSlots + hit.index));
            return true;
        case ui::InventoryRegion::kHotbar:
            if (hit.index < 0 || hit.index >= Inventory::kHotbarSlots) {
                return false;
            }
            ActOnStack(inventory.MutableSlot(hit.index));
            return true;
        case ui::InventoryRegion::kResult:
            return CraftOnce(inventory);
        case ui::InventoryRegion::kNextRecipe:
            return CycleCandidate();
        case ui::InventoryRegion::kBookRecipe:
            return FillNextCraftable(inventory);
        case ui::InventoryRegion::kPanel:
            /* 点面板空白处不做动作, 但吃掉这次点击, 避免穿透到世界. */
            return true;
        case ui::InventoryRegion::kNone:
        default:
            return false;
    }
}

void InventoryScreen::ActOnStack(ItemStack& slot) {
    if (carry_.Empty()) {
        if (slot.Empty()) {
            return;
        }
        carry_ = slot;
        slot.Clear();
        return;
    }
    if (slot.Empty()) {
        slot = carry_;
        carry_.Clear();
        return;
    }
    if (slot.item == carry_.item) {
        const int32_t maxStack = item::ItemTable::Instance().MaxStack(slot.item);
        const int32_t room = maxStack - slot.count;
        const int32_t moved = carry_.count < room ? carry_.count : room;
        if (moved <= 0) {
            return;
        }
        slot.count += moved;
        carry_.count -= moved;
        if (carry_.count <= 0) {
            carry_.Clear();
        }
        return;
    }
    const ItemStack swap = slot;
    slot = carry_;
    carry_ = swap;
}

void InventoryScreen::EnsureCandidates() {
    if (!gridDirty_) {
        return;
    }
    candidates_ = crafting::RecipeMatcher::MatchAll(grid_);
    if (candidateIndex_ >= candidates_.count) {
        candidateIndex_ = 0;
    }
    gridDirty_ = false;
}

const crafting::RecipeInfo* InventoryScreen::CurrentRecipe() {
    EnsureCandidates();
    return candidates_.At(candidateIndex_);
}

bool InventoryScreen::CraftOnce(Inventory& inventory) {
    const crafting::RecipeInfo* recipe = CurrentRecipe();
    if (recipe == nullptr) {
        return false;
    }
    /* 先确认产物放得下, 再扣材料, 避免材料被扣掉而产物丢失. */
    if (inventory.RoomFor(recipe->result) < recipe->resultCount) {
        IM_LOGW(kTag, "craft rejected, bag full result=%u", static_cast<uint32_t>(recipe->result));
        return false;
    }
    if (!crafting::RecipeMatcher::ConsumeFromGrid(grid_, *recipe)) {
        return false;
    }
    const int32_t added = inventory.Add(recipe->result, recipe->resultCount);
    IM_LOGI(kTag, "craft ok recipe=%u result=%u x%d added=%d grid=%d", recipe->sourceId,
            static_cast<uint32_t>(recipe->result), recipe->resultCount, added,
            grid_.FilledSlots());
    /* 按实际入包件数累加事件计数, 由总装取走折算成成就事件. */
    craftEvents_ += added > 0 ? added : 0;
    gridDirty_ = true;
    return true;
}

bool InventoryScreen::CycleCandidate() {
    EnsureCandidates();
    if (candidates_.count <= 1) {
        return false;
    }
    candidateIndex_ = (candidateIndex_ + 1) % candidates_.count;
    const crafting::RecipeInfo* recipe = candidates_.At(candidateIndex_);
    if (recipe != nullptr) {
        IM_LOGI(kTag, "candidate %d/%d recipe=%u", candidateIndex_ + 1, candidates_.count,
                recipe->sourceId);
    }
    return true;
}

bool InventoryScreen::BookAvailable(const Inventory& inventory) const {
    const crafting::RecipeTable& table = crafting::RecipeTable::Instance();
    for (int32_t index = 0; index < table.Count(); ++index) {
        if (crafting::RecipeMatcher::CanCraftFromInventory(table.At(index), inventory)) {
            return true;
        }
    }
    return false;
}

bool InventoryScreen::FillNextCraftable(Inventory& inventory) {
    const crafting::RecipeTable& table = crafting::RecipeTable::Instance();
    if (table.Count() <= 0) {
        return false;
    }
    /* 网格里已有的材料先退回物品栏, 自动配方总是从空网格开始. */
    ReturnAllToInventory(inventory);
    for (int32_t offset = 0; offset < table.Count(); ++offset) {
        const int32_t index = (bookCursor_ + offset) % table.Count();
        const crafting::RecipeInfo& recipe = table.At(index);
        if (!crafting::RecipeMatcher::CanCraftFromInventory(recipe, inventory)) {
            continue;
        }
        if (inventory.RoomFor(recipe.result) < recipe.resultCount) {
            continue;
        }
        if (!crafting::RecipeMatcher::TakeMaterialsFromInventory(recipe, inventory, grid_)) {
            continue;
        }
        bookCursor_ = (index + 1) % table.Count();
        gridDirty_ = true;
        IM_LOGI(kTag, "auto recipe recipe=%u grid=%d", recipe.sourceId, grid_.FilledSlots());
        return true;
    }
    return false;
}

void InventoryScreen::ReturnAllToInventory(Inventory& inventory) {
    for (int32_t index = 0; index < crafting::kGridSlotCount; ++index) {
        ItemStack stack = grid_.Take(index);
        if (!stack.Empty()) {
            inventory.Add(stack.item, stack.count);
        }
    }
    if (!carry_.Empty()) {
        inventory.Add(carry_.item, carry_.count);
        carry_.Clear();
    }
    gridDirty_ = true;
}

void InventoryScreen::BuildFrame(const Inventory& inventory, ui::InventoryPanelFrame& frame) {
    EnsureCandidates();
    const crafting::RecipeInfo* recipe = candidates_.At(candidateIndex_);
    for (int32_t index = 0; index < ui::kInventoryCraftSlots; ++index) {
        FillSlotView(frame.crafting[index], grid_.Slot(index));
        /* 当前配方用到的材料格描边, 玩家一眼看出这次是按哪几张格匹配的. */
        if (recipe != nullptr && frame.crafting[index].hasItem) {
            for (int32_t material = 0; material < recipe->materialCount && material < 9;
                 ++material) {
                if (recipe->materials[material].item == grid_.Slot(index).item) {
                    frame.crafting[index].highlight = true;
                    break;
                }
            }
        }
    }
    for (int32_t index = 0; index < ui::kInventoryBackpackSlots; ++index) {
        FillSlotView(frame.backpack[index],
                     inventory.Slot(Inventory::kHotbarSlots + index));
    }
    for (int32_t index = 0; index < ui::kInventoryHotbarSlots; ++index) {
        FillSlotView(frame.hotbar[index], inventory.Slot(index));
    }

    FillSlotView(frame.result, recipe == nullptr ? ItemStack{}
                                                 : item::MakeStack(recipe->result, recipe->resultCount));
    frame.result.highlight = recipe != nullptr;
    frame.candidateIndex = candidates_.count > 0 ? candidateIndex_ + 1 : 0;
    frame.candidateCount = candidates_.count;
    frame.bookAvailable = BookAvailable(inventory);

    FillSlotView(frame.carried, carry_);
    frame.carriedVisible = !carry_.Empty();
    frame.carriedX = carryX_;
    frame.carriedY = carryY_;

    if (!carry_.Empty()) {
        frame.hint = "TAP A SLOT TO PLACE";
    } else if (recipe != nullptr) {
        frame.hint = item::ItemTable::Instance().Info(recipe->result).key;
    } else if (grid_.Empty()) {
        frame.hint = "PUT MATERIALS IN THE GRID";
    } else {
        frame.hint = "NO MATCHING RECIPE";
    }
}

}  /* namespace immirror::game */
