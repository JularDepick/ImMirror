#include "game/crafting/recipe_matcher.h"

#include <cstring>

#include "core/log.h"
#include "game/crafting/recipe_table.h"
#include "game/inventory.h"

namespace immirror::game::crafting {
namespace {

constexpr const char* kTag = "ImMirrorCraft";

/* 在一条配方的材料清单里找某个物品, 找不到返回 -1. */
int32_t FindMaterial(const RecipeInfo& recipe, ItemId item) {
    for (int32_t index = 0; index < recipe.materialCount && index < 9; ++index) {
        if (recipe.materials[index].item == item) {
            return index;
        }
    }
    return -1;
}

/* 网格里某种物品的总数量. */
int32_t CountInGrid(const CraftingGrid& grid, ItemId item) {
    int32_t total = 0;
    for (int32_t index = 0; index < kGridSlotCount; ++index) {
        const ItemStack& slot = grid.Slot(index);
        if (slot.item == item) {
            total += slot.count;
        }
    }
    return total;
}

/*
 * 有序配方的格位比对. 图案可以落在 3x3 网格的任意位置, 图案以外的格子必须为空,
 * 图案格内的数量不少于配方要求 (允许一次摆下整叠材料, 合成只扣配方量).
 */
bool MatchesShapeAt(const RecipeInfo& recipe, const CraftingGrid& grid, int32_t originX,
                    int32_t originY) {
    for (int32_t y = 0; y < kGridSize; ++y) {
        for (int32_t x = 0; x < kGridSize; ++x) {
            const ItemStack& cell = grid.Slot(y * kGridSize + x);
            const int32_t patternX = x - originX;
            const int32_t patternY = y - originY;
            const bool inside = patternX >= 0 && patternX < recipe.gridWidth && patternY >= 0 &&
                                patternY < recipe.gridHeight;
            if (!inside) {
                if (!cell.Empty()) {
                    return false;
                }
                continue;
            }
            const RecipeMaterial& material =
                recipe.materials[patternY * static_cast<int32_t>(recipe.gridWidth) + patternX];
            if (material.item == item::kNone) {
                if (!cell.Empty()) {
                    return false;
                }
                continue;
            }
            if (cell.item != material.item || cell.count < material.count) {
                return false;
            }
        }
    }
    return true;
}

/* 找出有序图案在网格中的落点, 没找到返回 false. */
bool FindShapeOrigin(const RecipeInfo& recipe, const CraftingGrid& grid, int32_t& originX,
                     int32_t& originY) {
    const int32_t width = static_cast<int32_t>(recipe.gridWidth);
    const int32_t height = static_cast<int32_t>(recipe.gridHeight);
    if (width <= 0 || height <= 0 || width > kGridSize || height > kGridSize) {
        return false;
    }
    for (int32_t y = 0; y + height <= kGridSize; ++y) {
        for (int32_t x = 0; x + width <= kGridSize; ++x) {
            if (MatchesShapeAt(recipe, grid, x, y)) {
                originX = x;
                originY = y;
                return true;
            }
        }
    }
    return false;
}

/*
 * 从网格里扣掉某种物品的若干数量, 用完的格子清空.
 * 调用前已确认总量足够, 因此不需要再判不足.
 */
void ConsumeInGrid(CraftingGrid& grid, ItemId item, int32_t amount) {
    int32_t remain = amount;
    for (int32_t index = 0; index < kGridSlotCount && remain > 0; ++index) {
        ItemStack& slot = grid.MutableSlot(index);
        if (slot.item != item || slot.count <= 0) {
            continue;
        }
        const int32_t taken = slot.count < remain ? slot.count : remain;
        slot.count -= taken;
        remain -= taken;
        if (slot.count <= 0) {
            slot.Clear();
        }
    }
}

/* 把配方材料摆进空网格, 材料总量已在调用前校验. */
bool PlaceIntoEmptyGrid(const RecipeInfo& recipe, CraftingGrid& grid) {
    if (recipe.kind == RecipeKind::kShaped) {
        const int32_t width = static_cast<int32_t>(recipe.gridWidth);
        const int32_t height = static_cast<int32_t>(recipe.gridHeight);
        if (width <= 0 || height <= 0 || width > kGridSize || height > kGridSize) {
            return false;
        }
        for (int32_t y = 0; y < height; ++y) {
            for (int32_t x = 0; x < width; ++x) {
                const RecipeMaterial& material = recipe.materials[y * width + x];
                if (material.item == item::kNone) {
                    continue;
                }
                grid.SetSlot(y * kGridSize + x, item::MakeStack(material.item, material.count));
            }
        }
        return true;
    }
    int32_t cursor = 0;
    for (int32_t index = 0; index < recipe.materialCount && index < 9; ++index) {
        const RecipeMaterial& material = recipe.materials[index];
        if (material.item == item::kNone || material.count <= 0) {
            continue;
        }
        if (cursor >= kGridSlotCount) {
            return false;
        }
        grid.SetSlot(cursor, item::MakeStack(material.item, material.count));
        ++cursor;
    }
    return true;
}

}  /* namespace */

const RecipeInfo* MatchResult::At(int32_t index) const {
    if (count <= 0) {
        return nullptr;
    }
    return recipes[index < 0 ? 0 : (index >= count ? count - 1 : index)];
}

bool RecipeMatcher::Matches(const RecipeInfo& recipe, const CraftingGrid& grid) {
    if (recipe.result == item::kNone || recipe.materialCount == 0 || grid.Empty()) {
        return false;
    }
    if (recipe.kind == RecipeKind::kShaped) {
        int32_t originX = 0;
        int32_t originY = 0;
        return FindShapeOrigin(recipe, grid, originX, originY);
    }
    /*
     * 无序: 网格里出现的每种物品都必须是配方材料, 且每种材料的数量不少于配方要求.
     * 允许整叠摆放, 合成时只按配方量扣料, 多余材料留在网格里.
     */
    for (int32_t index = 0; index < kGridSlotCount; ++index) {
        const ItemStack& slot = grid.Slot(index);
        if (slot.Empty()) {
            continue;
        }
        if (FindMaterial(recipe, slot.item) < 0) {
            return false;
        }
    }
    for (int32_t index = 0; index < recipe.materialCount && index < 9; ++index) {
        const RecipeMaterial& material = recipe.materials[index];
        if (material.item == item::kNone || material.count <= 0) {
            continue;
        }
        if (CountInGrid(grid, material.item) < material.count) {
            return false;
        }
    }
    return true;
}

MatchResult RecipeMatcher::MatchAll(const CraftingGrid& grid) {
    MatchResult result;
    if (grid.Empty()) {
        return result;
    }
    const RecipeTable& table = RecipeTable::Instance();
    /* 有序配方更具体, 先收集, 界面默认展示第一项. */
    for (int32_t pass = 0; pass < 2; ++pass) {
        for (int32_t index = 0; index < table.Count(); ++index) {
            const RecipeInfo& recipe = table.At(index);
            const bool shaped = recipe.kind == RecipeKind::kShaped;
            if ((pass == 0) != shaped) {
                continue;
            }
            if (!Matches(recipe, grid)) {
                continue;
            }
            if (result.count < kMaxCandidates) {
                result.recipes[result.count] = &recipe;
            }
            ++result.count;
        }
    }
    if (result.count > kMaxCandidates) {
        IM_LOGW(kTag, "candidate overflow, kept=%d total=%d", kMaxCandidates, result.count);
        result.count = kMaxCandidates;
    }
    return result;
}

bool RecipeMatcher::CanCraftFromInventory(const RecipeInfo& recipe, const Inventory& inventory) {
    if (recipe.result == item::kNone || recipe.materialCount == 0) {
        return false;
    }
    for (int32_t index = 0; index < recipe.materialCount && index < 9; ++index) {
        const RecipeMaterial& material = recipe.materials[index];
        if (material.item == item::kNone || material.count <= 0) {
            continue;
        }
        if (inventory.TotalOf(material.item) < material.count) {
            return false;
        }
    }
    return true;
}

bool RecipeMatcher::ConsumeFromGrid(CraftingGrid& grid, const RecipeInfo& recipe) {
    if (!Matches(recipe, grid)) {
        return false;
    }
    if (recipe.kind == RecipeKind::kShaped) {
        int32_t originX = 0;
        int32_t originY = 0;
        if (!FindShapeOrigin(recipe, grid, originX, originY)) {
            return false;
        }
        for (int32_t y = 0; y < recipe.gridHeight; ++y) {
            for (int32_t x = 0; x < recipe.gridWidth; ++x) {
                const RecipeMaterial& material = recipe.materials[y * recipe.gridWidth + x];
                if (material.item == item::kNone || material.count <= 0) {
                    continue;
                }
                const int32_t slotIndex = (originY + y) * kGridSize + (originX + x);
                ItemStack& slot = grid.MutableSlot(slotIndex);
                slot.count -= material.count;
                if (slot.count <= 0) {
                    slot.Clear();
                }
            }
        }
        return true;
    }
    for (int32_t index = 0; index < recipe.materialCount && index < 9; ++index) {
        const RecipeMaterial& material = recipe.materials[index];
        if (material.item == item::kNone || material.count <= 0) {
            continue;
        }
        ConsumeInGrid(grid, material.item, material.count);
    }
    return true;
}

bool RecipeMatcher::TakeMaterialsFromInventory(const RecipeInfo& recipe, Inventory& inventory,
                                              CraftingGrid& grid) {
    if (!grid.Empty()) {
        return false;
    }
    if (!CanCraftFromInventory(recipe, inventory)) {
        return false;
    }
    if (!PlaceIntoEmptyGrid(recipe, grid)) {
        grid.Clear();
        return false;
    }
    /* 网格已经摆好, 再按材料总量从物品栏扣除, 两处口径一致. */
    for (int32_t index = 0; index < recipe.materialCount && index < 9; ++index) {
        const RecipeMaterial& material = recipe.materials[index];
        if (material.item == item::kNone || material.count <= 0) {
            continue;
        }
        if (!inventory.ConsumeItem(material.item, material.count)) {
            /* 理论上不会发生: 前面已经校验过总量, 出现时把网格清掉回滚. */
            grid.Clear();
            return false;
        }
    }
    return true;
}

bool RecipeMatcher::RunSelfCheck() {
    /* 无序样板: 木板 3 份与木棍 2 份, 整叠摆放也应当匹配. */
    RecipeInfo shapeless = {};
    shapeless.sourceId = 9001u;
    shapeless.result = 4;
    shapeless.resultCount = 1;
    shapeless.kind = RecipeKind::kShapeless;
    shapeless.gridWidth = 2;
    shapeless.gridHeight = 2;
    shapeless.materialCount = 2;
    shapeless.materials[0] = {1, 3};
    shapeless.materials[1] = {2, 2};

    CraftingGrid grid;
    grid.SetSlot(0, item::MakeStack(1, 5));
    grid.SetSlot(4, item::MakeStack(2, 2));
    if (!Matches(shapeless, grid)) {
        IM_LOGE(kTag, "self check failed: shapeless stack match");
        return false;
    }
    grid.SetSlot(8, item::MakeStack(3, 1));
    if (Matches(shapeless, grid)) {
        IM_LOGE(kTag, "self check failed: shapeless extra item");
        return false;
    }
    grid.Take(8);
    if (!ConsumeFromGrid(grid, shapeless) || grid.Slot(0).count != 2 || !grid.Slot(4).Empty()) {
        IM_LOGE(kTag, "self check failed: shapeless consume");
        return false;
    }

    /* 有序样板: 2x2 的 L 形图案, 允许在 3x3 网格内平移, 图案外不许有物品. */
    RecipeInfo shaped = {};
    shaped.sourceId = 9002u;
    shaped.result = 5;
    shaped.resultCount = 1;
    shaped.kind = RecipeKind::kShaped;
    shaped.gridWidth = 2;
    shaped.gridHeight = 2;
    shaped.materialCount = 4;
    shaped.materials[0] = {1, 1};
    shaped.materials[1] = {1, 1};
    shaped.materials[2] = {1, 1};
    shaped.materials[3] = {2, 1};

    CraftingGrid shapedGrid;
    shapedGrid.SetSlot(4, item::MakeStack(1, 3));
    shapedGrid.SetSlot(5, item::MakeStack(1, 3));
    shapedGrid.SetSlot(7, item::MakeStack(1, 3));
    shapedGrid.SetSlot(8, item::MakeStack(2, 1));
    if (!Matches(shaped, shapedGrid)) {
        IM_LOGE(kTag, "self check failed: shaped offset match");
        return false;
    }
    shapedGrid.SetSlot(0, item::MakeStack(3, 1));
    if (Matches(shaped, shapedGrid)) {
        IM_LOGE(kTag, "self check failed: shaped stray item");
        return false;
    }
    shapedGrid.Take(0);
    if (!ConsumeFromGrid(shapedGrid, shaped) || shapedGrid.Slot(4).count != 2) {
        IM_LOGE(kTag, "self check failed: shaped consume");
        return false;
    }
    return true;
}

}  /* namespace immirror::game::crafting */
