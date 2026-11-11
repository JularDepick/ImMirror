#include "game/crafting/recipe_table.h"

#include "game/generated/recipe_data_gen.h"

namespace immirror::game::crafting {
namespace {

static_assert(gen::kRecipeInfoCount == static_cast<int32_t>(sizeof(gen::kRecipeInfos) /
                                                            sizeof(gen::kRecipeInfos[0])),
              "配方表条目数与声明条数不一致, 请重新运行 scripts/gen_item_data.py");

/* 表空时的兜底条目, 与 RecipeTable::At 的回落约定一致. */
const RecipeInfo kFallbackRecipe = {0u, item::kNone, 0, RecipeKind::kShapeless, 1, 1, 0,
                                    {{item::kNone, 0}}};

}  /* namespace */

const RecipeTable& RecipeTable::Instance() {
    static const RecipeTable instance;
    return instance;
}

int32_t RecipeTable::Count() const {
    return gen::kRecipeInfoCount;
}

const RecipeInfo& RecipeTable::At(int32_t index) const {
    if (index < 0 || index >= gen::kRecipeInfoCount) {
        return gen::kRecipeInfoCount > 0 ? gen::kRecipeInfos[0] : kFallbackRecipe;
    }
    return gen::kRecipeInfos[index];
}

}  /* namespace immirror::game::crafting */
