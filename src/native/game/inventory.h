#pragma once

#include <cstdint>

#include "game/item/item_def.h"

namespace immirror::game {

using item::ItemId;
using item::ItemStack;

/*
 * 通用物品栏: 9 格快捷栏加 36 格背包, 共 45 格.
 * 槽位 0 到 8 是快捷栏, 与底部快捷栏逐格对应, 放置与挖掘都作用于选中格;
 * 槽位 9 到 44 是背包, 只在背包界面里可见.
 *
 * 每个槽位记录物品 id 与数量, 不再是单一方块, 因此同一种方块与工具, 材料可以并存.
 * 放入时优先叠进同种物品的未满堆叠, 再占用空槽, 单格上限取物品表的 maxStack;
 * 放不下的部分计入丢弃数, 由总装写日志与提示.
 * 只在原生主线程访问, 不做加锁.
 */
class Inventory {
public:
    /* 快捷栏格数, 与界面底部快捷栏一致, 由 game_shell 在编译期校验. */
    static constexpr int32_t kHotbarSlots = 9;
    /* 背包格数, 界面按 9 列 4 行排布. */
    static constexpr int32_t kBackpackSlots = 36;
    /* 总格数, 前 kHotbarSlots 格是快捷栏. */
    static constexpr int32_t kSlotCount = kHotbarSlots + kBackpackSlots;
    /* 物品表没有给出堆叠上限时的兜底值. */
    static constexpr int32_t kMaxStack = 64;

    /* 清空全部槽位并复位丢弃计数. */
    void Reset();

    int32_t Count(int32_t slot) const;
    ItemId ItemAt(int32_t slot) const;
    const ItemStack& Slot(int32_t slot) const;
    /*
     * 可写引用. 只给背包界面做整格取放: 物品栏自身仍是数量的唯一所有者,
     * 界面侧不缓存任何副本, 关掉面板不会丢失状态.
     */
    ItemStack& MutableSlot(int32_t slot);
    bool Empty(int32_t slot) const { return Slot(slot).Empty(); }

    /* 反查某种物品的首个槽位, 未收录返回 -1. */
    int32_t SlotOf(ItemId item) const;

    /* 某种物品在物品栏中的总数量. */
    int32_t TotalOf(ItemId item) const;

    /* 数量是否足够, 供合成与界面判定. */
    bool Has(ItemId item, int32_t amount = 1) const;

    /* 消耗指定槽位的若干数量; 数量不足时不做改动并返回 false. */
    bool Consume(int32_t slot, int32_t amount = 1);

    /* 跨槽位消耗某种物品; 总数不足时不做改动并返回 false. */
    bool ConsumeItem(ItemId item, int32_t amount = 1);

    /*
     * 放入若干数量; 返回实际放入的数量.
     * 放不下的部分计入丢弃数并返回偏小的值, 调用方据此提示物品栏已满.
     */
    int32_t Add(ItemId item, int32_t amount = 1);

    /* 还能再放入多少该物品, 考虑堆叠上限与空槽. */
    int32_t RoomFor(ItemId item) const;

    /* 从某格取出若干数量; 返回取出的堆叠, 数量不足时取走剩余全部. */
    ItemStack Take(int32_t slot, int32_t amount);

    /*
     * 把堆叠放进指定格: 同种物品合并且不超过上限, 空槽按上限放置.
     * 返回放不下的剩余数量, 0 表示全部放下.
     */
    int32_t Put(int32_t slot, const ItemStack& stack);

    /* 直接覆盖某格, 供读档与初始化使用. */
    void SetSlot(int32_t slot, const ItemStack& stack);

    /*
     * 两格之间移动: 同种物品合并, 否则交换. 返回是否发生变化.
     * 界面上的拖放与合成格取放都走这一条路径.
     */
    bool MoveSlot(int32_t from, int32_t to);

    /* 有内容的格数. */
    int32_t UsedSlots() const;

    /* 因堆满而累计丢弃的数量. */
    int32_t Discarded() const { return discarded_; }

private:
    /* 槽位索引裁剪, 越界访问回落到首末槽位, 与快捷栏的取值约定一致. */
    static int32_t ClampSlot(int32_t slot);

    ItemStack slots_[kSlotCount];
    int32_t discarded_ = 0;
};

}  /* namespace immirror::game */
