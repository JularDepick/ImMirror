package com.julardepick.immirror.inventory;

/*
 * 一格物品堆叠的 Java 侧副本.
 *
 * 只有物品 id 与数量两个字段, 与原生 item::ItemStack 逐字段对应.
 * 界面持有的副本只用于这一次绘制, 任何改动都要经 InventoryBridge 回写原生侧,
 * 因此不存在两份互相覆盖的真值.
 */
public final class InventorySlot {

    public int item = InventoryItem.NONE;
    public int count;

    public InventorySlot() {
    }

    public InventorySlot(int item, int count) {
        set(item, count);
    }

    public void set(int item, int count) {
        if (item <= InventoryItem.NONE || count <= 0) {
            clear();
            return;
        }
        this.item = item;
        this.count = count;
    }

    public void clear() {
        this.item = InventoryItem.NONE;
        this.count = 0;
    }

    public boolean isEmpty() {
        return item <= InventoryItem.NONE || count <= 0;
    }

    /* 数量文本: 单格只有一个时不显示数字, 与快捷栏一致. */
    public String countText() {
        return count > 1 ? Integer.toString(count) : "";
    }

    public InventorySlot copy() {
        return new InventorySlot(item, count);
    }

    @Override
    public String toString() {
        return isEmpty() ? "Slot(empty)" : "Slot(" + item + " x" + count + ")";
    }
}
