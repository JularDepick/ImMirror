#include "game/item/inventory_screen.h"

#include "game/item/tool_table.h"

#include "game/item/enchant_table.h"

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
    /* 非方块物品的图标来自图标图集, 方块物品没有图标格. */
    view.iconCell = table.IconCellOf(stack.item);
    view.initial = table.Initial(stack.item);
    view.tone = static_cast<uint8_t>(info.category);
    view.count = stack.count;
    view.enchanted = stack.enchantId != 0;
    /* 工具带耐久时把剩余量与上限一起交给界面, 由界面决定怎么画. */
    item::ItemTable::ToolInfo tool;
    if (table.ToolOf(stack.item, tool) && stack.durability >= 0) {
        view.durability = stack.durability;
        view.maxDurability = static_cast<int32_t>(tool.durability);
    }
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

/*
 * 修复.
 * 原版的修复是把两件同名工具合成一件, 耐久相加. 这里把它做成合成动作的
 * 一个前置分支: 网格里恰好两格、同一种物品、都带耐久时直接合并,
 * 其余情况交回配方匹配. 成品放进物品栏第一个空槽, 没有空槽则拒绝.
 */
bool InventoryScreen::TryRepairByRecipe(Inventory& inventory) {
    /*
     * 按装备表的修理配方修: 六个材料槽里只要有任意一组齐备, 就消耗该组
     * 并把耐久补满. 与"两件同种工具相加"互补, 材料配方更接近常规做法.
     */
    for (int32_t slot = 0; slot < Inventory::kSlotCount; ++slot) {
        const ItemStack& target = inventory.Slot(slot);
        if (target.Empty() || target.durability < 0) {
            continue;
        }
        item::ToolCombat combat;
        if (!item::ToolTable::Instance().Find(target.item, combat)) {
            continue;
        }
        for (int32_t group = 0; group < 6; ++group) {
            const item::ItemId need = combat.repairItem[group];
            const int32_t needCount = combat.repairAmount[group];
            if (need == item::kNone || needCount <= 0) {
                continue;
            }
            int32_t available = 0;
            for (int32_t probe = 0; probe < Inventory::kSlotCount; ++probe) {
                if (probe == slot) {
                    continue;
                }
                const ItemStack& candidate = inventory.Slot(probe);
                if (!candidate.Empty() && candidate.item == need) {
                    available += candidate.count;
                }
            }
            if (available < needCount) {
                continue;
            }
            /* 材料齐备: 逐格扣除, 再补满耐久. */
            int32_t remain = needCount;
            for (int32_t probe = 0; probe < Inventory::kSlotCount && remain > 0; ++probe) {
                if (probe == slot) {
                    continue;
                }
                ItemStack& candidate = inventory.MutableSlot(probe);
                if (candidate.Empty() || candidate.item != need) {
                    continue;
                }
                const int32_t take = candidate.count < remain ? candidate.count : remain;
                candidate.count -= take;
                remain -= take;
                if (candidate.count <= 0) {
                    candidate.Clear();
                }
            }
            /* 耐久上限取物品表里的满耐久值, 补齐到满. */
            item::ItemTable::ToolInfo tool;
            if (item::ItemTable::Instance().ToolOf(target.item, tool) && tool.durability > 0) {
                inventory.MutableSlot(slot).durability = static_cast<int16_t>(tool.durability);
            }
            IM_LOGI(kTag, "repair by recipe group=%d item=%u", group,
                    static_cast<uint32_t>(need));
            return true;
        }
    }
    return false;
}

bool InventoryScreen::TryRepair(Inventory& inventory) {
    int32_t first = -1;
    int32_t second = -1;
    for (int32_t index = 0; index < crafting::kGridSlotCount; ++index) {
        if (grid_.Slot(index).Empty()) {
            continue;
        }
        if (first < 0) {
            first = index;
            continue;
        }
        if (second < 0) {
            second = index;
            continue;
        }
        /* 三件及以上不按修复处理. */
        return false;
    }
    if (first < 0 || second < 0) {
        return false;
    }
    const ItemStack left = grid_.Slot(first);
    const ItemStack right = grid_.Slot(second);
    if (left.item != right.item || left.durability < 0 || right.durability < 0) {
        return false;
    }
    item::ItemTable::ToolInfo tool;
    if (!item::ItemTable::Instance().ToolOf(left.item, tool) || tool.durability == 0) {
        return false;
    }
    int32_t target = -1;
    for (int32_t slot = 0; slot < Inventory::kSlotCount; ++slot) {
        if (inventory.Empty(slot)) {
            target = slot;
            break;
        }
    }
    if (target < 0) {
        IM_LOGW(kTag, "repair rejected, bag full item=%u", static_cast<uint32_t>(left.item));
        return false;
    }
    /* 耐久相加并封顶到满耐久; 两件都满耐久时合并后仍是满耐久. */
    int32_t merged = static_cast<int32_t>(left.durability) + static_cast<int32_t>(right.durability);
    if (merged > static_cast<int32_t>(tool.durability)) {
        merged = static_cast<int32_t>(tool.durability);
    }
    ItemStack result;
    result.item = left.item;
    result.count = 1;
    result.durability = static_cast<int16_t>(merged);
    inventory.SetSlot(target, result);
    grid_.SetSlot(first, ItemStack{});
    grid_.SetSlot(second, ItemStack{});
    IM_LOGI(kTag, "repair ok item=%u durability=%d slot=%d", static_cast<uint32_t>(left.item), merged,
            target);
    gridDirty_ = true;
    return true;
}

/*
 * 附魔.
 * 原版附魔要附魔台与货币, 这里先用"工具 + 三块蓝晶石"的合成口径占位:
 * 只挂采集速度一类附魔, 等级随机 1 到 5, 已经带附魔的工具不重复附魔.
 * 判定与修复同一套网格语义: 网格里恰好两格, 一格工具一格蓝晶石.
 */
bool InventoryScreen::TryEnchant(Inventory& inventory) {
    constexpr int32_t kGemCost = 3;
    constexpr uint32_t kDigSpeedId = 25; /* enchant 表里 DIG_SPEED 的编号 */
    const item::ItemTable& items = item::ItemTable::Instance();
    const item::ItemId gem = items.FindByKey("Kyanite");
    if (gem == item::kNone) {
        return false;
    }
    int32_t toolSlot = -1;
    int32_t gemSlot = -1;
    int32_t filled = 0;
    for (int32_t index = 0; index < crafting::kGridSlotCount; ++index) {
        const ItemStack& stack = grid_.Slot(index);
        if (stack.Empty()) {
            continue;
        }
        filled += 1;
        if (stack.item == gem) {
            gemSlot = index;
            continue;
        }
        item::ItemTable::ToolInfo info;
        if (items.ToolOf(stack.item, info)) {
            toolSlot = index;
        }
    }
    if (filled != 2 || toolSlot < 0 || gemSlot < 0) {
        return false;
    }
    ItemStack tool = grid_.Slot(toolSlot);
    if (tool.enchantId != 0) {
        return false;
    }
    if (grid_.Slot(gemSlot).count < kGemCost) {
        return false;
    }
    item::ItemTable::ToolInfo info;
    if (!items.ToolOf(tool.item, info)) {
        return false;
    }
    item::ToolFamily family = item::ToolFamily::kPickaxe;
    if (info.harvest == static_cast<uint8_t>(voxel::HarvestTool::kAxe)) {
        family = item::ToolFamily::kAxe;
    } else if (info.harvest == static_cast<uint8_t>(voxel::HarvestTool::kShovel)) {
        family = item::ToolFamily::kShovel;
    }
    const item::EnchantTable& table = item::EnchantTable::Instance();
    /* 该工具族适用的采集速度附魔编号应当就是 25; 不适用则不改动网格. */
    const game::gen::EnchantEntry* base = table.Find(kDigSpeedId, 1);
    if (base == nullptr || !table.AllowsFamily(*base, family)) {
        return false;
    }
    /* 等级随机 1 到 5, 与数据表里的五档一致. */
    enchantRandom_ ^= enchantRandom_ << 13;
    enchantRandom_ ^= enchantRandom_ >> 17;
    enchantRandom_ ^= enchantRandom_ << 5;
    const uint8_t level = static_cast<uint8_t>(1 + enchantRandom_ % 5u);
    tool.enchantId = static_cast<uint16_t>(kDigSpeedId);
    tool.enchantLevel = level;
    grid_.SetSlot(toolSlot, tool);
    ItemStack gems = grid_.Slot(gemSlot);
    gems.count -= kGemCost;
    grid_.SetSlot(gemSlot, gems.count > 0 ? gems : ItemStack{});
    gridDirty_ = true;
    enchantEvents_ += 1;
    IM_LOGI(kTag, "enchant ok item=%u level=%u gems_left=%d",
            static_cast<uint32_t>(tool.item), static_cast<unsigned>(level), gems.count);
    return true;
}

bool InventoryScreen::CraftOnce(Inventory& inventory) {
    /* 修复优先于配方匹配: 两件同种工具在网格里没有对应配方. */
    /* 修理优先按装备表配方, 材料不齐时退回"两件同种相加". */
    if (TryRepairByRecipe(inventory) || TryRepair(inventory)) {
        return true;
    }
    /* 其次看附魔: 一件工具加蓝晶石. */
    if (TryEnchant(inventory)) {
        return true;
    }
    const crafting::RecipeInfo* recipe = CurrentRecipe();
    if (recipe == nullptr) {
        return false;
    }
    /* 先确认产物放得下, 再扣材料, 避免材料被扣掉而产物丢失. */
    if (inventory.RoomFor(recipe->result) < recipe->resultCount) {
        IM_LOGW(kTag, "craft rejected, bag full result=%u", static_cast<uint32_t>(recipe->result));
        craftFailEvents_ += 1;
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
    /*
     * 带耐久的工具不能走数量合并: Add 只按物品 id 入包, 会把耐久重置成满值,
     * 等于凭空修好一件工具. 这类堆叠直接占一个空槽, 原样放回.
     */
    const auto returnStack = [&inventory](const ItemStack& stack) {
        if (stack.Empty()) {
            return;
        }
        if (stack.durability < 0) {
            inventory.Add(stack.item, stack.count);
            return;
        }
        for (int32_t slot = 0; slot < Inventory::kSlotCount; ++slot) {
            if (inventory.Empty(slot)) {
                inventory.SetSlot(slot, stack);
                return;
            }
        }
        IM_LOGW(kTag, "return rejected, bag full item=%u", static_cast<uint32_t>(stack.item));
    };
    for (int32_t index = 0; index < crafting::kGridSlotCount; ++index) {
        returnStack(grid_.Take(index));
    }
    if (!carry_.Empty()) {
        returnStack(carry_);
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
