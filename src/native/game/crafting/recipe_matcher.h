#pragma once

#include <cstdint>

#include "game/crafting/recipe.h"

namespace immirror::game {

class Inventory;

}  /* namespace immirror::game */

namespace immirror::game::crafting {

/*
 * 一次匹配的候选上限. 原版里有材料完全相同而产物不同的配方
 * (例如果木板六份既可以做工具箱, 也可以做台阶或门), 因此匹配结果是一个列表,
 * 界面层按序号轮换显示, 由玩家决定这一次做哪一个.
 * 匹配按"材料不少于配方要求"的口径, 一叠材料能同时满足多条配方,
 * 排序时把材料用量大的排在前面, 默认选中项就是最贴近这堆材料的那一条.
 */
constexpr int32_t kMaxCandidates = 16;

/* 匹配结果: 指向配方表内的只读条目, 生存期与进程一致. */
struct MatchResult {
    int32_t count = 0;
    const RecipeInfo* recipes[kMaxCandidates] = {};

    bool Empty() const { return count == 0; }

    /* 越界回落到首条, 空结果返回空指针. */
    const RecipeInfo* At(int32_t index) const;
};

/*
 * 配方匹配器. 无序配方只比较材料总量, 有序配方按格位逐个比对并允许在
 * 3x3 网格内任意平移; 两条路径共用同一份网格状态, 不修改网格本身.
 */
class RecipeMatcher {
public:
    /* 单条配方是否与网格内容匹配, 不查表, 供扫描与自检复用. */
    static bool Matches(const RecipeInfo& recipe, const CraftingGrid& grid);

    /* 扫描整张配方表, 有序配方优先, 其余按表内顺序追加候选. */
    static MatchResult MatchAll(const CraftingGrid& grid);

    /* 物品栏里的材料是否够合成一次该配方. */
    static bool CanCraftFromInventory(const RecipeInfo& recipe, const Inventory& inventory);

    /* 按配方从网格扣除材料; 材料不足时不做改动并返回 false. */
    static bool ConsumeFromGrid(CraftingGrid& grid, const RecipeInfo& recipe);

    /*
     * 把一条配方的材料从物品栏搬进网格, 供"自动配方"按钮使用.
     * 要求网格为空且物品栏材料充足, 任一条不满足时不做改动并返回 false.
     */
    static bool TakeMaterialsFromInventory(const RecipeInfo& recipe, Inventory& inventory,
                                           CraftingGrid& grid);

    /* 启动自检: 用构造的样板验证无序与有序两条匹配路径, 失败返回 false. */
    static bool RunSelfCheck();
};

}  /* namespace immirror::game::crafting */
