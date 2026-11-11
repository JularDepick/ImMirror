package com.julardepick.immirror.inventory;

import android.util.Log;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.Map;

/*
 * 物品属性表在 Java 侧的检索壳.
 *
 * 属性本体仍由原生物品表持有, 这里只在打开背包界面时取一次全表快照并解析成 Map,
 * 界面按物品 id 查名称, 图标与堆叠上限. 快照解析失败时整表退回空表,
 * 于是所有槽位都按占位色块绘制, 页面不会崩.
 *
 * 桥接用的 JSON 结构:
 * {"items":[{"id":1,"key":"GrassBlock","name":"草块","icon":"icon1","stack":64,"category":3,"block":1}]}
 */
public final class InventoryCatalog {

    private static final String TAG = "ImMirrorInventory";

    private final Map<Integer, InventoryItem> items = new HashMap<>();

    public InventoryCatalog() {
    }

    /* 直接从桥接文本构造; 失败时返回空表, 由调用方决定是否退回预览数据. */
    public static InventoryCatalog parse(String json) {
        InventoryCatalog catalog = new InventoryCatalog();
        if (json == null || json.isEmpty()) {
            return catalog;
        }
        try {
            JSONObject root = new JSONObject(json);
            JSONArray array = root.optJSONArray("items");
            if (array == null) {
                return catalog;
            }
            for (int index = 0; index < array.length(); index++) {
                JSONObject entry = array.optJSONObject(index);
                if (entry == null) {
                    continue;
                }
                int id = entry.optInt("id", InventoryItem.NONE);
                if (id <= InventoryItem.NONE) {
                    continue;
                }
                catalog.items.put(id, new InventoryItem(
                        id,
                        entry.optString("key", ""),
                        entry.optString("name", ""),
                        entry.optString("icon", ""),
                        entry.optInt("stack", 64),
                        entry.optInt("block", -1),
                        categoryOf(entry.optInt("category", 0))));
            }
        } catch (Exception error) {
            Log.w(TAG, "catalog parse failed, 退回占位绘制", error);
            catalog.items.clear();
        }
        return catalog;
    }

    /* 用给定的物品列表建表, 预览数据与自检都走这条路径. */
    public static InventoryCatalog of(InventoryItem[] source) {
        InventoryCatalog catalog = new InventoryCatalog();
        for (InventoryItem item : source) {
            if (item != null && item.id > InventoryItem.NONE) {
                catalog.items.put(item.id, item);
            }
        }
        return catalog;
    }

    public boolean isEmpty() {
        return items.isEmpty();
    }

    public int size() {
        return items.size();
    }

    /* 查属性, 未收录时回落到同一份占位属性, 调用方不必判空. */
    public InventoryItem get(int id) {
        InventoryItem item = items.get(id);
        return item == null ? InventoryItem.EMPTY : item;
    }

    /* 原生 ItemCategory 的整数值到枚举. */
    public static InventoryItem.Category categoryOf(int value) {
        switch (value) {
            case 1:
                return InventoryItem.Category.EQUIPMENT;
            case 2:
                return InventoryItem.Category.TOOL;
            case 3:
                return InventoryItem.Category.BLOCK;
            case 4:
                return InventoryItem.Category.MATERIAL;
            default:
                return InventoryItem.Category.UNKNOWN;
        }
    }

    /* 类别的中文说明, 用于详情面板. */
    public static String categoryLabel(InventoryItem.Category category) {
        switch (category) {
            case EQUIPMENT:
                return InventoryText.CATEGORY_EQUIPMENT;
            case TOOL:
                return InventoryText.CATEGORY_TOOL;
            case BLOCK:
                return InventoryText.CATEGORY_BLOCK;
            case MATERIAL:
                return InventoryText.CATEGORY_MATERIAL;
            default:
                return InventoryText.CATEGORY_UNKNOWN;
        }
    }
}
