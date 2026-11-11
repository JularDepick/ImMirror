#pragma once

#include <cstdint>

#include "game/item/item_def.h"

namespace immirror::game::crafting {

using item::ItemId;
using item::ItemStack;

/*
 * 配方匹配方式. 原版 crafting.csv 用横占格与竖占格描述摆放区域,
 * 每个材料格记一种材料与数量, 同种材料出现在多格时数量累加,
 * 也就是"材料齐备即可合成"的无序语义, 因此表内配方都是 kShapeless.
 * kShaped 按格位精确匹配, 供后续手写的图案配方使用.
 */
enum class RecipeKind : uint8_t {
    kShapeless = 0,
    kShaped = 1,
};

/* 一项配方材料: 物品 id 与数量, 有序配方里空位记为 {0, 0}. */
struct RecipeMaterial {
    ItemId item = item::kNone;
    int32_t count = 0;
};

/*
 * 一条配方的只读视图, 与生成表 recipe_data_gen.h 的条目一一对应.
 * 无序配方: materials 前 materialCount 项是聚合后的材料清单;
 * 有序配方: materials 按格位行优先排列, 长度固定为 9, 空位 item 为 0.
 * gridWidth 与 gridHeight 是原版的摆放区域尺寸, 界面据此提示配方大小.
 */
struct RecipeInfo {
    uint32_t sourceId;      /* 原版 crafting.csv 的配方 ID */
    ItemId result;          /* 产物物品 id */
    int32_t resultCount;    /* 一次合成的产物数量 */
    RecipeKind kind;
    uint8_t gridWidth;
    uint8_t gridHeight;
    uint8_t materialCount;  /* 无序配方为材料种类数, 有序配方为非空格数 */
    RecipeMaterial materials[9];
};

/* 合成网格边长与格数, 与原版工作台的 3x3 一致. */
constexpr int32_t kGridSize = 3;
constexpr int32_t kGridSlotCount = kGridSize * kGridSize;

/*
 * 合成网格. 只保存九个格子的堆叠, 匹配与扣料由 RecipeMatcher 负责,
 * 网格本身不关心配方表, 便于界面层与总装共用同一份状态.
 */
class CraftingGrid {
public:
    void Clear();

    /* 越界索引回落到首末格, 与物品栏的取值约定一致. */
    const ItemStack& Slot(int32_t index) const;
    ItemStack& MutableSlot(int32_t index);

    void SetSlot(int32_t index, const ItemStack& stack);

    /* 取出整格内容并把该格清空. */
    ItemStack Take(int32_t index);

    /* 是否九格全空. */
    bool Empty() const;

    /* 有内容的格数. */
    int32_t FilledSlots() const;

private:
    static int32_t ClampIndex(int32_t index);

    ItemStack slots_[kGridSlotCount];
};

}  /* namespace immirror::game::crafting */
