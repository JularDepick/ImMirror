package com.julardepick.immirror.inventory;

import android.util.Log;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/*
 * 背包界面一次显示所需的全部槽位状态.
 *
 * 槽位编号与原生 game::Inventory 完全一致: 0 到 8 是快捷栏, 9 到 44 是背包,
 * 因此界面上的拖放可以原样换算成原生槽位下标, 不需要任何映射表.
 *
 * 数据由 InventoryBridge 从原生侧取一次, 结构是桥接用的最小 JSON:
 * {"selected":0,"slots":[{"slot":0,"item":3,"count":12}, ...]}
 * slots 可以稀疏, 未出现的槽位按空格处理; 字段缺失或越界都回落到空格,
 * 界面宁可少显示一格, 也不因为一条脏数据整页打不开.
 */
public final class InventorySnapshot {

    private static final String TAG = "ImMirrorInventory";

    /* 快捷栏格数, 与原生 Inventory::kHotbarSlots 一致. */
    public static final int HOTBAR_SLOTS = 9;

    /* 背包格数, 界面按 9 列 4 行排布. */
    public static final int BACKPACK_SLOTS = 36;

    /* 总格数, 前 HOTBAR_SLOTS 格是快捷栏. */
    public static final int SLOT_COUNT = HOTBAR_SLOTS + BACKPACK_SLOTS;

    /* 没有选中快捷栏格时的取值. */
    public static final int NO_SELECTION = -1;

    private final InventorySlot[] slots = new InventorySlot[SLOT_COUNT];
    private int selected = NO_SELECTION;

    public InventorySnapshot() {
        for (int index = 0; index < SLOT_COUNT; index++) {
            slots[index] = new InventorySlot();
        }
    }

    /* 空格快照, 原生侧未接线或读取失败时使用. */
    public static InventorySnapshot empty() {
        return new InventorySnapshot();
    }

    /*
     * 解析原生快照. 任何解析失败都返回当时已经读到的部分,
     * 保证界面至少能显示出去掉坏行之后的内容.
     */
    public static InventorySnapshot parse(String json) {
        InventorySnapshot snapshot = new InventorySnapshot();
        if (json == null || json.isEmpty()) {
            return snapshot;
        }
        try {
            JSONObject root = new JSONObject(json);
            snapshot.selected = clampSelection(root.optInt("selected", NO_SELECTION));
            JSONArray array = root.optJSONArray("slots");
            if (array == null) {
                return snapshot;
            }
            for (int index = 0; index < array.length(); index++) {
                JSONObject entry = array.optJSONObject(index);
                if (entry == null) {
                    continue;
                }
                int slot = entry.optInt("slot", -1);
                if (!isValidSlot(slot)) {
                    continue;
                }
                int item = entry.optInt("item", InventoryItem.NONE);
                int count = entry.optInt("count", 0);
                snapshot.slots[slot].set(item, count);
            }
        } catch (JSONException error) {
            Log.w(TAG, "snapshot parse failed, 使用已解析部分", error);
        }
        return snapshot;
    }

    public static boolean isValidSlot(int slot) {
        return slot >= 0 && slot < SLOT_COUNT;
    }

    public static boolean isHotbarSlot(int slot) {
        return slot >= 0 && slot < HOTBAR_SLOTS;
    }

    /* 越界下标回落到首末格, 与原生物品栏的取值约定一致. */
    public static int clampSlot(int slot) {
        if (slot < 0) {
            return 0;
        }
        return slot >= SLOT_COUNT ? SLOT_COUNT - 1 : slot;
    }

    private static int clampSelection(int value) {
        if (value < 0 || value >= HOTBAR_SLOTS) {
            return NO_SELECTION;
        }
        return value;
    }

    public InventorySlot slot(int index) {
        return slots[clampSlot(index)];
    }

    public int selected() {
        return selected;
    }

    public void setSelected(int index) {
        selected = clampSelection(index);
    }

    /* 有内容的格数, 用于页头统计. */
    public int usedSlots() {
        int used = 0;
        for (int index = 0; index < SLOT_COUNT; index++) {
            if (!slots[index].isEmpty()) {
                used++;
            }
        }
        return used;
    }

    /* 某种物品在整包里的总数, 供合成与丢弃判定. */
    public int totalOf(int itemId) {
        int total = 0;
        for (int index = 0; index < SLOT_COUNT; index++) {
            if (slots[index].item == itemId) {
                total += slots[index].count;
            }
        }
        return total;
    }

    public boolean isEmpty() {
        return usedSlots() == 0;
    }
}
