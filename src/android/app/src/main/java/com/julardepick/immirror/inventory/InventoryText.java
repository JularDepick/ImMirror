package com.julardepick.immirror.inventory;

/*
 * 背包界面的文案常量.
 *
 * 集中在一处便于统一改词与后续做多语言: 接入随包语言表时只需要把本文件的取值
 * 换成读资源, 界面代码不用动. 中文标点按项目排版规范使用半角.
 */
public final class InventoryText {

    private InventoryText() {
    }

    /* 页头. */
    public static final String PAGE_TITLE = "背包";
    public static final String PAGE_SUBTITLE = "快捷栏 9 格, 背包 36 格";
    public static final String PAGE_SUMMARY_FORMAT = "已用 %1$d / %2$d 格";
    public static final String PREVIEW_BADGE = "预览数据";

    /* 分区标题. */
    public static final String SECTION_BACKPACK = "背包";
    public static final String SECTION_HOTBAR = "快捷栏";

    /* 详情面板. */
    public static final String DETAIL_EMPTY = "选择一个格子查看详情";
    public static final String DETAIL_COUNT_FORMAT = "数量 %1$d / %2$d";
    public static final String DETAIL_CATEGORY_FORMAT = "类别 %1$s";
    public static final String DETAIL_BLOCK_FORMAT = "放置为方块 %1$d";
    public static final String DETAIL_KEY_FORMAT = "键 %1$s";

    /* 操作按钮. */
    public static final String ACTION_DROP = "丢弃一个";
    public static final String ACTION_DROP_CONFIRM = "再按一次丢弃";
    public static final String ACTION_MOVE_HOTBAR = "移到快捷栏";
    public static final String ACTION_CLOSE = "返回游戏";

    /* 提示条. */
    public static final String NOTICE_NATIVE_MISSING = "原生物品栏未接线, 当前显示预览数据";
    public static final String NOTICE_SLOT_EMPTY = "这个格子是空的";
    public static final String NOTICE_SNAPSHOT_EMPTY = "物品栏读取为空";
    public static final String NOTICE_DROP_FORMAT = "已丢弃 %1$s x%2$d";
    public static final String NOTICE_MOVED_FORMAT = "已把 %1$s 移到快捷栏";
    public static final String NOTICE_FAILED = "操作未生效, 原生侧拒绝了这次改动";
    public static final String NOTICE_HOTBAR_FULL = "快捷栏已满";
    public static final String NOTICE_HOTBAR_ONLY_FORMAT = "快捷栏已选中第 %1$d 格";

    /* 类别名, 与原生 ItemCategory 一一对应. */
    public static final String CATEGORY_EQUIPMENT = "装备";
    public static final String CATEGORY_TOOL = "工具";
    public static final String CATEGORY_BLOCK = "方块";
    public static final String CATEGORY_MATERIAL = "材料";
    public static final String CATEGORY_UNKNOWN = "未分类";
}
