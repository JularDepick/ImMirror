#pragma once

#include <cstdint>

#include "game/crafting/recipe.h"
#include "game/crafting/recipe_matcher.h"
#include "game/item/item_def.h"
#include "ui/hud_renderer.h"

namespace immirror::game {

using item::ItemStack;

class Inventory;

/*
 * 背包界面状态机. 持有合成网格, 手指上的携带堆叠与配方候选,
 * 负责把界面命中翻译成物品移动与合成动作; 绘制数据交给 HUD 渲染层,
 * 两者的格子几何都来自 ui::HudRenderer 的命中判定, 只有一份来源.
 *
 * 交互按"点击移动"实现: 空手点格子取走整叠, 手上有堆叠时点空格放下,
 * 点同种物品合并到上限, 点不同物品交换. 合成网格与物品栏共用同一套动作,
 * 因此拖放与取放没有第二套规则. 面板关闭时网格里的材料原路退回物品栏.
 */
class InventoryScreen {
public:
    InventoryScreen() = default;

    /* 复位到关闭状态并清空合成网格与携带堆叠, 不归还物品 (供开局与读档使用). */
    void Reset();

    bool Open() const { return open_; }

    /* 打开或关闭面板; 关闭时自动把合成网格与携带物退回物品栏. */
    void SetOpen(bool open, Inventory& inventory);
    void Toggle(Inventory& inventory) { SetOpen(!open_, inventory); }

    /* 面板命中处理, 返回是否消费了这次点击. */
    bool HandlePress(const ui::InventoryHit& hit, Inventory& inventory);

    /* 携带中的堆叠跟随触点, 坐标为归一化屏幕坐标. */
    void SetCarryPosition(float normalizedX, float normalizedY);

    bool Carrying() const { return !carry_.Empty(); }

    /* 把合成网格与携带物全部退回物品栏, 面板关闭与退出游戏时调用. */
    void ReturnAllToInventory(Inventory& inventory);

    /* 组装一帧绘制数据, 内部按需重算配方候选. */
    void BuildFrame(const Inventory& inventory, ui::InventoryPanelFrame& frame);

    /* 当前候选配方, 没有匹配时返回空指针. */
    const crafting::RecipeInfo* CurrentRecipe();

    /* 合成网格的只读视图, 供日志与自检使用. */
    const crafting::CraftingGrid& Grid() const { return grid_; }

    /*
     * 面板内的合成完成计数: 每次成功合成累加本次产出件数,
     * 由总装逐帧取走清零, 折算成成就事件. 只增不减, 溢出量级远超实际游玩.
     */
    int64_t TakeCraftEventCount() {
        const int64_t value = craftEvents_;
        craftEvents_ = 0;
        return value;
    }

private:
    /* 网格变化后重算候选; 候选顺序与配方表顺序一致. */
    void EnsureCandidates();

    /* 一次整格取放: 空手取走, 手上有货时放下, 合并或交换. */
    void ActOnStack(ItemStack& slot);

    /* 按当前候选合成一次, 产出入物品栏; 条件不足时不做改动. */
    bool CraftOnce(Inventory& inventory);

    /* 轮换到下一个候选配方. */
    bool CycleCandidate();

    /* 自动摆出下一张物品栏备齐材料的配方. */
    bool FillNextCraftable(Inventory& inventory);

    /* 物品栏是否还有可自动摆出的配方. */
    bool BookAvailable(const Inventory& inventory) const;

    crafting::CraftingGrid grid_;
    crafting::MatchResult candidates_;
    int32_t candidateIndex_ = 0;
    /* 自动配方的扫描游标, 依次向后轮换, 走完一圈回到开头. */
    int32_t bookCursor_ = 0;
    ItemStack carry_;
    float carryX_ = 0.5f;
    float carryY_ = 0.5f;
    bool open_ = false;
    bool gridDirty_ = true;
    /* 面板自上次取走以来完成的合成产出件数. */
    int64_t craftEvents_ = 0;
};

}  /* namespace immirror::game */
