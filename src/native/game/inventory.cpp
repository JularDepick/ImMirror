#include "game/inventory.h"

namespace immirror::game {

void Inventory::Reset(const voxel::BlockId* blocks, int32_t slotCount, int32_t count) {
    const int32_t clamped = count < 0 ? 0 : (count > kMaxStack ? kMaxStack : count);
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        slots_[slot].block = voxel::block::Air;
        slots_[slot].count = 0;
    }
    if (blocks == nullptr) {
        discarded_ = 0;
        return;
    }
    const int32_t limit = slotCount < kSlotCount ? slotCount : kSlotCount;
    for (int32_t slot = 0; slot < limit; ++slot) {
        slots_[slot].block = blocks[slot];
        /* 空气槽位无法放置也无法拾取, 保持 0 让界面直接显示为耗尽. */
        slots_[slot].count = blocks[slot] == voxel::block::Air ? 0 : clamped;
    }
    discarded_ = 0;
}

int32_t Inventory::Count(int32_t slot) const {
    return slots_[ClampSlot(slot)].count;
}

voxel::BlockId Inventory::Block(int32_t slot) const {
    return slots_[ClampSlot(slot)].block;
}

const InventorySlot& Inventory::Slot(int32_t slot) const {
    return slots_[ClampSlot(slot)];
}

int32_t Inventory::SlotOf(voxel::BlockId block) const {
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        if (slots_[slot].block == block) {
            return slot;
        }
    }
    return -1;
}

int32_t Inventory::TotalOf(voxel::BlockId block) const {
    int32_t total = 0;
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        if (slots_[slot].block == block) {
            total += slots_[slot].count;
        }
    }
    return total;
}

bool Inventory::Consume(int32_t slot, int32_t amount) {
    if (amount <= 0) {
        return true;
    }
    InventorySlot& target = slots_[ClampSlot(slot)];
    if (target.count < amount) {
        return false;
    }
    target.count -= amount;
    return true;
}

bool Inventory::Add(voxel::BlockId block, int32_t amount) {
    if (amount <= 0) {
        return true;
    }
    if (block == voxel::block::Air) {
        /* 空气不算掉落物, 按成功处理, 调用方不必额外判空. */
        return true;
    }
    int32_t remain = amount;
    for (int32_t slot = 0; slot < kSlotCount && remain > 0; ++slot) {
        InventorySlot& target = slots_[slot];
        if (target.block != block || target.count >= kMaxStack) {
            continue;
        }
        const int32_t room = kMaxStack - target.count;
        const int32_t moved = remain < room ? remain : room;
        target.count += moved;
        remain -= moved;
    }
    if (remain > 0) {
        discarded_ += remain;
        return false;
    }
    return true;
}

int32_t Inventory::ClampSlot(int32_t slot) {
    if (slot < 0) {
        return 0;
    }
    if (slot >= kSlotCount) {
        return kSlotCount - 1;
    }
    return slot;
}

}  /* namespace immirror::game */
