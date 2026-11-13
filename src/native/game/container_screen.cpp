#include "game/container_screen.h"

#include "core/log.h"
#include "game/item/item_table.h"

namespace immirror::game {
namespace {

constexpr const char* kTag = "ImMirrorContainer";

/* 把一格堆叠翻译成绘制数据, 与背包面板同源. */
void FillSlotView(ui::InventorySlotView& view, const ItemStack& stack) {
    view = ui::InventorySlotView{};
    if (stack.Empty()) {
        return;
    }
    const item::ItemTable& table = item::ItemTable::Instance();
    const item::ItemInfo& info = table.Info(stack.item);
    view.hasItem = true;
    view.atlasCell = table.AtlasCellOf(stack.item);
    view.iconCell = table.IconCellOf(stack.item);
    view.initial = table.Initial(stack.item);
    view.tone = static_cast<uint8_t>(info.category);
    view.count = stack.count;
}

}  /* namespace */

void FurnaceScreen::Reset() {
    furnace_ = nullptr;
    carry_.Clear();
    carryX_ = 0.5f;
    carryY_ = 0.5f;
    open_ = false;
}

void FurnaceScreen::SetOpen(bool open, Inventory& inventory) {
    if (open_ == open) {
        return;
    }
    open_ = open;
    if (!open) {
        ReturnAllToInventory(inventory);
        furnace_ = nullptr;
    }
    IM_LOGI(kTag, "furnace panel %s", open ? "open" : "closed");
}

void FurnaceScreen::OpenFurnace(FurnaceState& state, Inventory& inventory) {
    furnace_ = &state;
    open_ = true;
    (void)inventory;
}

void FurnaceScreen::SetCarryPosition(float normalizedX, float normalizedY) {
    carryX_ = normalizedX < 0.0f ? 0.0f : (normalizedX > 1.0f ? 1.0f : normalizedX);
    carryY_ = normalizedY < 0.0f ? 0.0f : (normalizedY > 1.0f ? 1.0f : normalizedY);
}

void FurnaceScreen::ReturnAllToInventory(Inventory& inventory) {
    /*
     * 放入接口返回实际放入数量. 背包放不下时只清掉已放入的部分,
     * 剩下的留在原处, 否则关闭面板时这些东西会凭空消失.
     */
    const auto putBack = [&inventory](ItemStack& stack) {
        if (stack.Empty()) {
            return;
        }
        const int32_t added = inventory.Add(stack.item, stack.count);
        if (added >= stack.count) {
            stack.Clear();
            return;
        }
        stack.count -= added > 0 ? added : 0;
        if (stack.count <= 0) {
            stack.Clear();
        }
    };
    putBack(carry_);
    if (furnace_ != nullptr) {
        putBack(furnace_->input);
        putBack(furnace_->fuel);
    }
}

void FurnaceScreen::ActOnStack(ItemStack& slot, bool takeOnly) {
    if (carry_.Empty()) {
        if (slot.Empty()) {
            return;
        }
        carry_ = slot;
        slot.Clear();
        return;
    }
    if (takeOnly) {
        /* 只取格 (产出格): 手上有货时不能反向放入. */
        return;
    }
    if (slot.Empty()) {
        slot = carry_;
        carry_.Clear();
        return;
    }
    if (slot.item == carry_.item) {
        /* 同种合并到上限, 放不下的留在手上. */
        const int32_t room = 64 - slot.count;
        const int32_t moved = carry_.count < room ? carry_.count : room;
        slot.count += moved;
        carry_.count -= moved;
        if (carry_.count <= 0) {
            carry_.Clear();
        }
        return;
    }
    /* 异种交换. */
    const ItemStack temp = slot;
    slot = carry_;
    carry_ = temp;
}

void FurnaceScreen::CollectOutput(Inventory& inventory) {
    if (furnace_ == nullptr) {
        return;
    }
    ItemStack& output = furnace_->output;
    if (output.Empty()) {
        return;
    }
    if (carry_.Empty()) {
        carry_ = output;
        output.Clear();
        return;
    }
    if (carry_.item == output.item) {
        const int32_t room = 64 - carry_.count;
        const int32_t moved = output.count < room ? output.count : room;
        carry_.count += moved;
        output.count -= moved;
        if (output.count <= 0) {
            output.Clear();
        }
        return;
    }
    /* 手上拿着别的东西: 尝试直接收进物品栏, 放不下的留在产出格里. */
    const int32_t added = inventory.Add(output.item, output.count);
    if (added >= output.count) {
        output.Clear();
        return;
    }
    output.count -= added > 0 ? added : 0;
    if (output.count <= 0) {
        output.Clear();
    }
}

bool FurnaceScreen::HandlePress(const ui::InventoryHit& hit, Inventory& inventory) {
    /* 只读: 客人侧不搬动房主的熔炉内容, 避免本地改动被覆盖后物品凭空多出. */
    if (readOnly_) {
        return false;
    }
    if (furnace_ == nullptr) {
        return false;
    }
    switch (hit.region) {
        case ui::InventoryRegion::kClose:
            SetOpen(false, inventory);
            return true;
        case ui::InventoryRegion::kFurnaceInput:
            ActOnStack(furnace_->input, false);
            return true;
        case ui::InventoryRegion::kFurnaceFuel:
            ActOnStack(furnace_->fuel, false);
            return true;
        case ui::InventoryRegion::kFurnaceOutput:
            CollectOutput(inventory);
            return true;
        case ui::InventoryRegion::kBackpack:
            if (hit.index < 0 || hit.index >= Inventory::kBackpackSlots) {
                return false;
            }
            ActOnStack(inventory.MutableSlot(Inventory::kHotbarSlots + hit.index), false);
            return true;
        case ui::InventoryRegion::kHotbar:
            if (hit.index < 0 || hit.index >= Inventory::kHotbarSlots) {
                return false;
            }
            ActOnStack(inventory.MutableSlot(hit.index), false);
            return true;
        case ui::InventoryRegion::kPanel:
            return true;
        case ui::InventoryRegion::kNone:
        default:
            return false;
    }
}

void FurnaceScreen::BuildFrame(const Inventory& inventory, ui::FurnacePanelFrame& frame) {
    frame = ui::FurnacePanelFrame{};
    if (furnace_ == nullptr) {
        return;
    }
    FillSlotView(frame.input, furnace_->input);
    FillSlotView(frame.fuel, furnace_->fuel);
    FillSlotView(frame.output, furnace_->output);
    frame.progress = furnace_->progress;
    frame.fuelRatio =
        furnace_->fuelBurnTotal > 0.0f ? furnace_->fuelTicks / furnace_->fuelBurnTotal : 0.0f;
    if (frame.fuelRatio < 0.0f) {
        frame.fuelRatio = 0.0f;
    } else if (frame.fuelRatio > 1.0f) {
        frame.fuelRatio = 1.0f;
    }
    for (int32_t index = 0; index < Inventory::kBackpackSlots; ++index) {
        FillSlotView(frame.backpack[index], inventory.Slot(Inventory::kHotbarSlots + index));
    }
    for (int32_t index = 0; index < Inventory::kHotbarSlots; ++index) {
        FillSlotView(frame.hotbar[index], inventory.Slot(index));
    }
    FillSlotView(frame.carried, carry_);
    frame.carriedVisible = !carry_.Empty();
    frame.carriedX = carryX_;
    frame.carriedY = carryY_;
}

void ChestScreen::Reset() {
    chest_ = nullptr;
    carry_.Clear();
    carryX_ = 0.5f;
    carryY_ = 0.5f;
    open_ = false;
}

void ChestScreen::SetOpen(bool open, Inventory& inventory) {
    if (open_ == open) {
        return;
    }
    open_ = open;
    if (!open) {
        ReturnAllToInventory(inventory);
        chest_ = nullptr;
    }
    IM_LOGI(kTag, "chest panel %s", open ? "open" : "closed");
}

void ChestScreen::OpenChest(ChestState& state, Inventory& inventory) {
    chest_ = &state;
    open_ = true;
    (void)inventory;
}

void ChestScreen::SetCarryPosition(float normalizedX, float normalizedY) {
    carryX_ = normalizedX < 0.0f ? 0.0f : (normalizedX > 1.0f ? 1.0f : normalizedX);
    carryY_ = normalizedY < 0.0f ? 0.0f : (normalizedY > 1.0f ? 1.0f : normalizedY);
}

void ChestScreen::ReturnAllToInventory(Inventory& inventory) {
    if (carry_.Empty()) {
        return;
    }
    /* 同上: 放不下的部分留在手上, 不能连同已放的一起清掉. */
    const int32_t added = inventory.Add(carry_.item, carry_.count);
    if (added >= carry_.count) {
        carry_.Clear();
        return;
    }
    carry_.count -= added > 0 ? added : 0;
    if (carry_.count <= 0) {
        carry_.Clear();
    }
}

void ChestScreen::ActOnStack(ItemStack& slot) {
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
        const int32_t room = 64 - slot.count;
        const int32_t moved = carry_.count < room ? carry_.count : room;
        slot.count += moved;
        carry_.count -= moved;
        if (carry_.count <= 0) {
            carry_.Clear();
        }
        return;
    }
    const ItemStack temp = slot;
    slot = carry_;
    carry_ = temp;
}

bool ChestScreen::HandlePress(const ui::InventoryHit& hit, Inventory& inventory) {
    if (readOnly_) {
        return false;
    }
    if (chest_ == nullptr) {
        return false;
    }
    switch (hit.region) {
        case ui::InventoryRegion::kClose:
            SetOpen(false, inventory);
            return true;
        case ui::InventoryRegion::kChest:
            if (hit.index < 0 || hit.index >= ChestState::kSlots) {
                return false;
            }
            ActOnStack(chest_->MutableSlot(hit.index));
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
        case ui::InventoryRegion::kPanel:
            return true;
        case ui::InventoryRegion::kNone:
        default:
            return false;
    }
}

void ChestScreen::BuildFrame(const Inventory& inventory, ui::ChestPanelFrame& frame) {
    frame = ui::ChestPanelFrame{};
    if (chest_ == nullptr) {
        return;
    }
    for (int32_t index = 0; index < ChestState::kSlots; ++index) {
        FillSlotView(frame.chest[index], chest_->Slot(index));
    }
    for (int32_t index = 0; index < Inventory::kBackpackSlots; ++index) {
        FillSlotView(frame.backpack[index], inventory.Slot(Inventory::kHotbarSlots + index));
    }
    for (int32_t index = 0; index < Inventory::kHotbarSlots; ++index) {
        FillSlotView(frame.hotbar[index], inventory.Slot(index));
    }
    FillSlotView(frame.carried, carry_);
    frame.carriedVisible = !carry_.Empty();
    frame.carriedX = carryX_;
    frame.carriedY = carryY_;
}

}  /* namespace immirror::game */
