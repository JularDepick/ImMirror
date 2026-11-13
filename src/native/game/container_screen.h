#pragma once

#include <cstdint>

#include "game/containers.h"
#include "game/inventory.h"
#include "ui/hud_renderer.h"

namespace immirror::game {

using item::ItemStack;

/*
 * 熔炉面板状态机. 交互按"点击移动"实现, 与背包面板同一套动作:
 * 空手点格子取走整叠, 手上有堆叠时点空格放下, 同种合并, 异种交换;
 * 产出格只允许取走, 手上有同种堆叠时可以并回去.
 * 面板关闭时把携带物与输入格, 燃料格全部退回物品栏.
 */
class FurnaceScreen {
public:
    void Reset();

    bool Open() const { return open_; }
    /* 只读模式下不响应搬动, 客人看房主的箱子与熔炉用. */
    void SetReadOnly(bool readOnly) { readOnly_ = readOnly; }
    bool ReadOnly() const { return readOnly_; }

    /* 打开指定熔炉; close 把携带物与格子退回物品栏. */
    void SetOpen(bool open, Inventory& inventory);
    void OpenFurnace(FurnaceState& state, Inventory& inventory);

    /* 面板命中处理, 返回是否消费了这次点击. */
    bool HandlePress(const ui::InventoryHit& hit, Inventory& inventory);

    /* 携带中的堆叠跟随触点, 归一化屏幕坐标. */
    void SetCarryPosition(float normalizedX, float normalizedY);

    bool Carrying() const { return !carry_.Empty(); }

    /* 关闭时退回全部物品, 总装在世界卸载或退出时调用. */
    void ReturnAllToInventory(Inventory& inventory);

    /* 组装一帧绘制数据. */
    void BuildFrame(const Inventory& inventory, ui::FurnacePanelFrame& frame);

private:
    /* 一次整格取放, 与背包面板的语义一致. */
    void ActOnStack(ItemStack& slot, bool takeOnly);

    /* 产出格收取: 空手取走整叠, 手上有同种堆叠时并入. */
    void CollectOutput(Inventory& inventory);

    FurnaceState* furnace_ = nullptr;
    ItemStack carry_;
    float carryX_ = 0.5f;
    float carryY_ = 0.5f;
    bool open_ = false;
    /* 只读: 客人侧打开房主的箱子与熔炉时不允许搬动物品, 否则本地改动会被下一次同步覆盖, 物品反而多出来. */
    bool readOnly_ = false;
};

/*
 * 储物箱面板状态机. 27 格容器与物品栏对拖,
 * 取放语义与熔炉面板一致, 关闭时携带物退回物品栏.
 */
class ChestScreen {
public:
    void Reset();

    bool Open() const { return open_; }
    /* 只读模式下不响应搬动, 客人看房主的箱子与熔炉用. */
    void SetReadOnly(bool readOnly) { readOnly_ = readOnly; }
    bool ReadOnly() const { return readOnly_; }

    void SetOpen(bool open, Inventory& inventory);
    void OpenChest(ChestState& state, Inventory& inventory);

    bool HandlePress(const ui::InventoryHit& hit, Inventory& inventory);

    void SetCarryPosition(float normalizedX, float normalizedY);

    bool Carrying() const { return !carry_.Empty(); }

    void ReturnAllToInventory(Inventory& inventory);

    void BuildFrame(const Inventory& inventory, ui::ChestPanelFrame& frame);

private:
    void ActOnStack(ItemStack& slot);

    ChestState* chest_ = nullptr;
    ItemStack carry_;
    float carryX_ = 0.5f;
    float carryY_ = 0.5f;
    bool open_ = false;
    /* 只读: 客人侧打开房主的箱子与熔炉时不允许搬动物品, 否则本地改动会被下一次同步覆盖, 物品反而多出来. */
    bool readOnly_ = false;
};

}  /* namespace immirror::game */
