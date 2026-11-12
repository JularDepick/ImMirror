package com.julardepick.immirror.inventory;

import java.util.Locale;

/*
 * 物品静态属性在 Java 侧的只读副本.
 *
 * 数据源只有一份: 原生物品表 (game::item::ItemTable) 由 scripts/gen_item_data.py
 * 从 itemdef.csv 生成, 界面需要展示时才通过 InventoryBridge 取一次快照,
 * 这里不做第二份数据表, 也不缓存可写状态.
 *
 * 图标字段沿用itemdef 的写法: 形如 icon11203 的是 assets/items 下的独立 PNG,
 * 形如 [sapling_fruittree 的是方块图集里的一格, 界面对后者回落到占位绘制.
 */
public final class InventoryItem {

    /*
     * 物品类别, 取值与原生 ItemCategory 一一对应:
     * 1 装备, 2 工具, 3 方块, 4 材料.
     */
    public enum Category {
        UNKNOWN,
        EQUIPMENT,
        TOOL,
        BLOCK,
        MATERIAL
    }

    /* 空物品的 id, 表示该格没有东西. */
    public static final int NONE = 0;

    /* 原生表里没有这个 id 时统一回落到同一份占位属性. */
    public static final InventoryItem EMPTY =
            new InventoryItem(NONE, "", "", "", 64, -1, Category.UNKNOWN);

    /* 独立图标在 APK assets 内的目录, 与 pack_assets.py 的登记保持一致. */
    public static final String ICON_ASSET_ROOT = "items";

    public final int id;
    public final String key;
    public final String name;
    public final String icon;
    public final int maxStack;
    public final int block;
    public final Category category;

    public InventoryItem(int id, String key, String name, String icon, int maxStack,
            int block, Category category) {
        this.id = id;
        this.key = key == null ? "" : key;
        this.name = name == null ? "" : name;
        this.icon = icon == null ? "" : icon;
        this.maxStack = maxStack > 0 ? maxStack : 64;
        this.block = block;
        this.category = category == null ? Category.UNKNOWN : category;
    }

    public boolean isNone() {
        return id == NONE;
    }

    /* 界面标签优先用中文名, 缺中文名时退回英文键, 两者都没有则显示 id. */
    public String label() {
        if (!name.isEmpty()) {
            return name;
        }
        if (!key.isEmpty()) {
            return key;
        }
        return "#" + id;
    }

    /*
     * 独立图标的 assets 路径, 没有独立图标时返回空串.
     * 图集图标 (以 [ 开头) 与空图标都交由调用方绘制占位色块.
     */
    public String iconAssetPath() {
        if (icon.isEmpty() || icon.startsWith("[")) {
            return "";
        }
        String trimmed = icon;
        int dot = trimmed.lastIndexOf('.');
        if (dot > 0) {
            trimmed = trimmed.substring(0, dot);
        }
        return ICON_ASSET_ROOT + "/" + trimmed + ".png";
    }

    /* 占位色块的种子, 同一物品每次都取同一个色相, 拾取后不会变色. */
    public int placeholderSeed() {
        return Math.abs((id * 31) ^ (key.isEmpty() ? id : key.hashCode()));
    }

    /* 占位色块上显示的字符, 取标签首字母; 中文标签取首个字. */
    public String placeholderText() {
        String label = label();
        if (label.isEmpty()) {
            return "?";
        }
        int end = label.offsetByCodePoints(0, 1);
        String head = label.substring(0, end);
        return head.toUpperCase(Locale.ROOT);
    }

    @Override
    public String toString() {
        return "Item(" + id + ", " + label() + ")";
    }
}
