#include "game/inventory.h"

#include "game/item/item_table.h"

namespace immirror::game {

void Inventory::Reset() {
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        slots_[slot].Clear();
    }
    discarded_ = 0;
}

int32_t Inventory::Count(int32_t slot) const {
    return slots_[ClampSlot(slot)].count;
}

ItemId Inventory::ItemAt(int32_t slot) const {
    return slots_[ClampSlot(slot)].item;
}

const ItemStack& Inventory::Slot(int32_t slot) const {
    return slots_[ClampSlot(slot)];
}

ItemStack& Inventory::MutableSlot(int32_t slot) {
    return slots_[ClampSlot(slot)];
}

int32_t Inventory::SlotOf(ItemId item) const {
    if (item == item::kNone) {
        return -1;
    }
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        if (slots_[slot].item == item) {
            return slot;
        }
    }
    return -1;
}

int32_t Inventory::TotalOf(ItemId item) const {
    if (item == item::kNone) {
        return 0;
    }
    int32_t total = 0;
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        if (slots_[slot].item == item) {
            total += slots_[slot].count;
        }
    }
    return total;
}

bool Inventory::Has(ItemId item, int32_t amount) const {
    if (amount <= 0) {
        return true;
    }
    return TotalOf(item) >= amount;
}

bool Inventory::Consume(int32_t slot, int32_t amount) {
    if (amount <= 0) {
        return true;
    }
    ItemStack& target = slots_[ClampSlot(slot)];
    if (target.count < amount) {
        return false;
    }
    target.count -= amount;
    if (target.count <= 0) {
        target.Clear();
    }
    return true;
}

bool Inventory::ConsumeItem(ItemId item, int32_t amount) {
    if (amount <= 0) {
        return true;
    }
    if (item == item::kNone || TotalOf(item) < amount) {
        return false;
    }
    int32_t remain = amount;
    for (int32_t slot = 0; slot < kSlotCount && remain > 0; ++slot) {
        ItemStack& target = slots_[slot];
        if (target.item != item || target.count <= 0) {
            continue;
        }
        const int32_t taken = target.count < remain ? target.count : remain;
        target.count -= taken;
        remain -= taken;
        if (target.count <= 0) {
            target.Clear();
        }
    }
    return true;
}

bool Inventory::ConsumeDurability(int32_t slot) {
    if (slot < 0 || slot >= kSlotCount) {
        return false;
    }
    ItemStack& target = slots_[slot];
    if (target.Empty() || target.durability <= 0) {
        return false;
    }
    target.durability = static_cast<int16_t>(target.durability - 1);
    if (target.durability > 0) {
        return false;
    }
    /* 耐久耗尽: 整格清空, 工具损坏后不留残格. */
    target.Clear();
    return true;
}

int32_t Inventory::DurabilityAt(int32_t slot) const {
    if (slot < 0 || slot >= kSlotCount) {
        return -1;
    }
    return slots_[slot].durability;
}

int32_t Inventory::Add(ItemId item, int32_t amount) {
    if (amount <= 0) {
        return 0;
    }
    if (item == item::kNone) {
        /* 空物品不算掉落物, 按成功处理, 调用方不必额外判空. */
        return amount;
    }
    const int32_t maxStack = item::ItemTable::Instance().MaxStack(item);
    int32_t remain = amount;

    /* 第一轮叠进同种物品的未满堆叠, 保持原有槽位不漂移. */
    for (int32_t slot = 0; slot < kSlotCount && remain > 0; ++slot) {
        ItemStack& target = slots_[slot];
        if (target.item != item || target.count >= maxStack) {
            continue;
        }
        const int32_t room = maxStack - target.count;
        const int32_t moved = remain < room ? remain : room;
        target.count += moved;
        remain -= moved;
    }

    /* 第二轮占用空槽, 快捷栏优先, 新拾取的物品先落到手上. */
    for (int32_t slot = 0; slot < kSlotCount && remain > 0; ++slot) {
        ItemStack& target = slots_[slot];
        if (!target.Empty()) {
            continue;
        }
        const int32_t moved = remain < maxStack ? remain : maxStack;
        target.item = item;
        target.count = moved;
        /* 工具进格时按物品表填满耐久, 非工具保持 -1 表示不用耐久. */
        item::ItemTable::ToolInfo tool;
        if (item::ItemTable::Instance().ToolOf(item, tool)) {
            target.durability = static_cast<int16_t>(tool.durability);
        }
        remain -= moved;
    }

    if (remain > 0) {
        discarded_ += remain;
    }
    return amount - remain;
}

int32_t Inventory::RoomFor(ItemId item) const {
    if (item == item::kNone) {
        return 0;
    }
    const int32_t maxStack = item::ItemTable::Instance().MaxStack(item);
    int32_t room = 0;
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        const ItemStack& target = slots_[slot];
        if (target.Empty()) {
            room += maxStack;
        } else if (target.item == item && target.count < maxStack) {
            room += maxStack - target.count;
        }
    }
    return room;
}

ItemStack Inventory::Take(int32_t slot, int32_t amount) {
    ItemStack& target = slots_[ClampSlot(slot)];
    if (amount <= 0 || target.Empty()) {
        return ItemStack{};
    }
    const int32_t taken = target.count < amount ? target.count : amount;
    ItemStack stack;
    stack.item = target.item;
    stack.count = taken;
    target.count -= taken;
    if (target.count <= 0) {
        target.Clear();
    }
    return stack;
}

int32_t Inventory::Put(int32_t slot, const ItemStack& stack) {
    if (stack.Empty()) {
        return 0;
    }
    ItemStack& target = slots_[ClampSlot(slot)];
    const int32_t maxStack = item::ItemTable::Instance().MaxStack(stack.item);
    if (target.Empty()) {
        const int32_t moved = stack.count < maxStack ? stack.count : maxStack;
        target.item = stack.item;
        target.count = moved;
        return stack.count - moved;
    }
    if (target.item != stack.item || target.count >= maxStack) {
        return stack.count;
    }
    const int32_t room = maxStack - target.count;
    const int32_t moved = stack.count < room ? stack.count : room;
    target.count += moved;
    return stack.count - moved;
}

void Inventory::SetSlot(int32_t slot, const ItemStack& stack) {
    slots_[ClampSlot(slot)] = stack;
    if (slots_[ClampSlot(slot)].count <= 0) {
        slots_[ClampSlot(slot)].Clear();
    }
}

bool Inventory::MoveSlot(int32_t from, int32_t to) {
    const int32_t source = ClampSlot(from);
    const int32_t target = ClampSlot(to);
    if (source == target) {
        return false;
    }
    ItemStack& src = slots_[source];
    ItemStack& dst = slots_[target];
    if (src.Empty()) {
        return false;
    }
    if (dst.Empty()) {
        dst = src;
        src.Clear();
        return true;
    }
    if (dst.item == src.item) {
        /* 同种物品合并, 上层有空余时直接把源格剩余量并过去. */
        const int32_t maxStack = item::ItemTable::Instance().MaxStack(src.item);
        const int32_t room = maxStack - dst.count;
        if (room <= 0) {
            return false;
        }
        const int32_t moved = src.count < room ? src.count : room;
        dst.count += moved;
        src.count -= moved;
        if (src.count <= 0) {
            src.Clear();
        }
        return true;
    }
    const ItemStack swap = dst;
    dst = src;
    src = swap;
    return true;
}

int32_t Inventory::UsedSlots() const {
    int32_t used = 0;
    for (int32_t slot = 0; slot < kSlotCount; ++slot) {
        if (!slots_[slot].Empty()) {
            ++used;
        }
    }
    return used;
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
