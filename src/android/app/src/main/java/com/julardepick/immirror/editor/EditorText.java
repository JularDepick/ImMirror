package com.julardepick.immirror.editor;

/*
 * 编辑器界面文案.
 *
 * 与其它界面不同, 编辑器文案集中在本文件而不是 res/values/strings.xml:
 * 一是编辑器界面在里程碑 M8 内独立演进, 文案改动频繁; 二是便于整体替换成
 * 面向 UGC 作者的措辞而不动布局代码. 接入主线多语言方案时, 把这里的常量
 * 迁移到 strings.xml 即可, 调用点不需要改动.
 */
public final class EditorText {

    /* 页面骨架. */
    public static final String PAGE_TITLE = "地图编辑器";
    public static final String PAGE_SUBTITLE = "工程为明文目录, 可直接用文本编辑器修改";
    public static final String AUTHOR_FORMAT = "作者: %s";
    public static final String SUMMARY_FORMAT = "工程 %d 个, 已打开: %s";

    /* 原生接口状态. */
    public static final String NATIVE_UNAVAILABLE = "编辑器原生接口尚未接线, 界面只读";
    public static final String NATIVE_READY = "编辑器原生接口就绪";

    /* 工程卡片. */
    public static final String PROJECT_TITLE = "工程";
    public static final String PROJECT_EMPTY = "还没有工程, 在下方输入名称后创建";
    public static final String PROJECT_NEW_HINT = "新工程名称";
    public static final String ACTION_CREATE = "创建";
    public static final String ACTION_REFRESH = "刷新";
    public static final String ACTION_OPEN = "打开";
    public static final String PROJECT_ROW_FORMAT = "%s (%d 区块 / %d 触发器)";
    public static final String PROJECT_OPENED = "已打开工程: %s";
    public static final String PROJECT_CREATED = "已创建工程: %s";
    public static final String PROJECT_CREATE_FAILED = "创建失败: %s";
    public static final String PROJECT_OPEN_FAILED = "打开失败: %s";
    public static final String PROJECT_NAME_EMPTY = "工程名称不能为空";

    /* 工具选择栏. */
    public static final String TOOL_TITLE = "工具";
    public static final String TOOL_PLACE = "放置";
    public static final String TOOL_REMOVE = "删除";
    public static final String TOOL_FILL = "框选填充";
    public static final String ACTION_UNDO = "撤销";
    public static final String ACTION_REDO = "重做";

    /* 方块栏. */
    public static final String PALETTE_TITLE = "方块";
    public static final String PALETTE_EMPTY = "方块表未就绪";

    /* 属性面板. */
    public static final String PANEL_TITLE = "属性";
    public static final String PANEL_BLOCK_FORMAT = "当前方块: %s";
    public static final String PANEL_CURSOR_FORMAT = "光标: %d, %d, %d";
    public static final String PANEL_SELECTION_FORMAT = "选区: %d x %d x %d";
    public static final String PANEL_SELECTION_EMPTY = "选区: 未设置";
    public static final String PANEL_HISTORY_FORMAT = "撤销 %d 步 / 重做 %d 步";
    public static final String PANEL_DIRTY = "工程有未保存改动";
    public static final String PANEL_CLEAN = "工程已保存";
    public static final String POINT_A = "设 A";
    public static final String POINT_B = "设 B";
    public static final String SELECTION_CLEAR = "清空选区";
    public static final String SELECTION_FILL = "填充选区";
    public static final String AXIS_LABEL_X = "X";
    public static final String AXIS_LABEL_Y = "Y";
    public static final String AXIS_LABEL_Z = "Z";
    public static final String STEP_MINUS = "-";
    public static final String STEP_PLUS = "+";

    /* 触发器卡片. */
    public static final String TRIGGER_TITLE = "触发器";
    public static final String TRIGGER_EMPTY = "还没有触发器";
    public static final String TRIGGER_ROW_FORMAT = "%s  [%s]  %d 个动作";
    public static final String TRIGGER_RUN = "试跑";
    public static final String TRIGGER_DELETE = "删除";
    public static final String TRIGGER_ADD_ENTER = "新增进入区域";
    public static final String TRIGGER_ADD_BREAK = "新增破坏方块";
    public static final String TRIGGER_ADD_TIMER = "新增定时";
    public static final String TRIGGER_RUN_FORMAT = "触发器 %s 执行: %s";
    public static final String TRIGGER_ADDED = "已新增触发器";

    /* 日志卡片. */
    public static final String LOG_TITLE = "运行日志";
    public static final String LOG_EMPTY = "暂无日志";

    /* 底部动作. */
    public static final String ACTION_SAVE = "保存";
    public static final String ACTION_LOAD = "载入";
    public static final String ACTION_PLAYTEST = "试玩";
    public static final String ACTION_BACK = "返回";
    public static final String SAVE_OK = "工程已保存";
    public static final String SAVE_FAILED = "保存失败: %s";
    public static final String LOAD_OK = "工程已载入";
    public static final String LOAD_FAILED = "载入失败: %s";
    public static final String PLAYTEST_OK = "已导出试玩世界, 正在进入游戏";
    public static final String PLAYTEST_FAILED = "试玩导出失败: %s";
    public static final String NO_PROJECT = "请先创建或打开一个工程";
    public static final String COMMAND_FAILED_FORMAT = "%s 失败: %s";

    /* 二次确认按钮的等待时长, 与设置界面保持同一节奏. */
    public static final long CONFIRM_TIMEOUT_MS = 3000L;

    private EditorText() {
    }

    public static String commandFailed(String action, String reason) {
        String detail = reason == null || reason.isEmpty() ? "原生接口无返回" : reason;
        return String.format(COMMAND_FAILED_FORMAT, action, detail);
    }
}
