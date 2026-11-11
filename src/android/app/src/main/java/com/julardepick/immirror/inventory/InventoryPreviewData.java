package com.julardepick.immirror.inventory;

/*
 * 背包界面的预览数据.
 *
 * 只在原生桥接未接线时使用: 让界面在没有原生侧的情况下也能显示网格, 图标占位与
 * 数量文字, 便于单独联调界面本身. 数据只覆盖开局可见的十四种客户端方块,
 * 名称与 scripts/gen_item_data.py 生成的原生物品表保持字面一致, 但这里不是数据源,
 * 原生接线后整份数据都会被 InventoryBridge 的返回值替换.
 */
public final class InventoryPreviewData {

    private InventoryPreviewData() {
    }

    /* 客户端方块物品: id, 英文键, 中文名, 客户端方块 id. */
    private static final Object[][] BLOCK_ITEMS = {
        {1, "GrassBlock", "草块"},
        {2, "Soil", "土块"},
        {3, "Stone", "岩石"},
        {4, "Sand", "细沙块"},
        {5, "Water", "水"},
        {6, "Fruitwood", "果木"},
        {7, "FruitwoodLeaves", "果木树叶"},
        {8, "UltraStone", "地心基石"},
        {9, "RedBrickBlock", "灰砂砖块"},
        {10, "Rock", "岩石块"},
        {11, "Snow", "积雪"},
        {12, "Ice", "冰块"},
        {13, "Gravel", "碎石堆"},
        {14, "FruitwoodBoard", "果木板"},
    };

    /* 预览用物品表, 方块图标走图集, 这里统一留空由界面绘制占位色块. */
    public static InventoryCatalog catalog() {
        InventoryItem[] items = new InventoryItem[BLOCK_ITEMS.length];
        for (int index = 0; index < BLOCK_ITEMS.length; index++) {
            Object[] row = BLOCK_ITEMS[index];
            int id = (Integer) row[0];
            items[index] = new InventoryItem(id, (String) row[1], (String) row[2], "", 64, id,
                    InventoryItem.Category.BLOCK);
        }
        return InventoryCatalog.of(items);
    }

    /* 预览用物品栏: 快捷栏五格, 背包三格, 数量各不相同以便检查数字排版. */
    public static InventorySnapshot snapshot() {
        InventorySnapshot snapshot = new InventorySnapshot();
        snapshot.slot(0).set(1, 64);
        snapshot.slot(1).set(3, 32);
        snapshot.slot(2).set(6, 16);
        snapshot.slot(3).set(10, 9);
        snapshot.slot(4).set(14, 4);
        snapshot.slot(9).set(4, 48);
        snapshot.slot(10).set(13, 7);
        snapshot.slot(11).set(12, 1);
        snapshot.setSelected(0);
        return snapshot;
    }
}
