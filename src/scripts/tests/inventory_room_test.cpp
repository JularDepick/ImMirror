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

    /* 消耗语义: 空格扣不掉, 返回 false, 调用方据此中止动作. */
    {
        Inventory empty;
        empty.Reset();
        check("空格扣不掉", !empty.Consume(0, 1));
        check("空格按物品扣也失败", !empty.ConsumeItem(item, 1));
    }
    {
        Inventory one;
        one.Reset();
        ItemStack stack;
        stack.item = item;
        stack.count = 1;
        stack.durability = -1;
        one.SetSlot(0, stack);
        check("有物品能扣掉", one.Consume(0, 1));
        check("扣完后格子空了", one.Empty(0));
        check("再扣一次失败", !one.Consume(0, 1));
    }

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
