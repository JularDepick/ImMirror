#pragma once

#include <cstdint>

#include "game/crafting/recipe.h"

namespace immirror::game::crafting {

/*
 * 配方表. 数据来自 scripts/gen_item_data.py 生成的 recipe_data_gen.h,
 * 进程内唯一且只读; 条目顺序即原版 crafting.csv 的配方顺序, 越界回落到首条.
 * 表内只收录产物与全部材料都在客户端物品表内的配方, 其余留在原版 CSV 中.
 */
class RecipeTable {
public:
    static const RecipeTable& Instance();

    int32_t Count() const;

    /* 按序号取配方, 越界回落到首条. */
    const RecipeInfo& At(int32_t index) const;

private:
    /* 只允许 Instance() 构造, 避免出现第二份表. */
    RecipeTable() = default;
};

}  /* namespace immirror::game::crafting */
