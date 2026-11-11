#pragma once

#include <cstdint>

#include "voxel/block_registry.h"

namespace immirror::game {

/*
 * 物品栏槽位: 一种方块与当前堆叠数量. 方块在初始化时绑定, 运行期只改变数量,
 * 因此槽位图标与方块栏的对应关系始终稳定, 不会因为拾取而换图.
 */
struct InventorySlot {
    voxel::BlockId block = voxel::block::Air;
    int32_t count = 0;
};

/*
 * 数量制物品栏. 槽位数与方块栏一致, 每槽记一种方块与数量.
 * 破坏方块产生的掉落物优先叠进同种槽位, 单槽上限 kMaxStack, 堆满或没有
 * 对应槽位时丢弃并计入丢计数, 由总装写日志与提示.
 * 放置前用 Count 判断数量, 为 0 时由调用方拒绝放置.
 * 只在原生主线程访问, 不做加锁.
 */
class Inventory {
public:
    /* 槽位数, 与 ui::kHudSlotCount 一致, 由 game_shell 在编译期校验. */
    static constexpr int32_t kSlotCount = 9;
    /* 单槽堆叠上限. */
    static constexpr int32_t kMaxStack = 64;
    /* 开局给每种方块的数量. */
    static constexpr int32_t kInitialStack = 64;

    /*
     * 按方块栏顺序播种槽位: blocks 为槽位方块表, slotCount 为表中的条目数,
     * 超出 kSlotCount 的条目忽略, 不足的槽位保持空气; count 会裁剪到堆叠上限.
     */
    void Reset(const voxel::BlockId* blocks, int32_t slotCount, int32_t count);

    int32_t Count(int32_t slot) const;
    voxel::BlockId Block(int32_t slot) const;
    const InventorySlot& Slot(int32_t slot) const;
    bool Empty(int32_t slot) const { return Count(slot) <= 0; }

    /* 反查方块所在的槽位, 未收录返回 -1. */
    int32_t SlotOf(voxel::BlockId block) const;

    /* 某方块在物品栏中的可用总数, 用于日志与提示. */
    int32_t TotalOf(voxel::BlockId block) const;

    /* 消耗指定槽位的若干数量; 数量不足时不做改动并返回 false. */
    bool Consume(int32_t slot, int32_t amount = 1);

    /* 放入若干数量; 全部放进同种槽位返回 true, 放不下的部分计入丢弃. */
    bool Add(voxel::BlockId block, int32_t amount = 1);

    /* 因堆满或没有对应槽位而累计丢弃的数量. */
    int32_t Discarded() const { return discarded_; }

private:
    /* 槽位索引裁剪, 越界访问回落到首末槽位, 与方块栏的取值约定一致. */
    static int32_t ClampSlot(int32_t slot);

    InventorySlot slots_[kSlotCount];
    int32_t discarded_ = 0;
};

}  /* namespace immirror::game */
