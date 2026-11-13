/* 背包容量语义断言: 满包时 RoomFor 归零, Add 返回剩余件数而不是静默丢弃. */
#include <cstdio>

#include "game/inventory.h"

using namespace immirror::game;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

int main() {
    Inventory inventory;
    inventory.Reset();

    const ItemId item = 1;
    check("空包有空间", inventory.RoomFor(item) > 0);

    /* 逐格填满: 每格放最大堆叠数. */
    for (int32_t slot = 0; slot < Inventory::kSlotCount; ++slot) {
        ItemStack stack;
        stack.item = item;
        stack.count = Inventory::kMaxStack;
        stack.durability = -1;
        inventory.SetSlot(slot, stack);
    }
    std::printf("满包后 RoomFor=%d\n", inventory.RoomFor(item));
    check("满包没有空间", inventory.RoomFor(item) == 0);

    /* 满包再放: 返回实际放入数量, 为 0 时调用方据此把物品放回地面而不是丢弃. */
    const int32_t added = inventory.Add(item, 5);
    std::printf("满包 Add 返回放入=%d\n", added);
    check("满包 Add 放入 0 件", added == 0);

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
