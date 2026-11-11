package com.julardepick.immirror.editor;

import android.util.Log;

import com.julardepick.immirror.bridge.NativeBridge;

/*
 * 编辑器与原生侧的桥.
 *
 * 编辑器不是逐帧界面: 只有用户点按钮时才需要跟原生交换一次状态, 因此这里
 * 统一走"一条命令一段文本"的协议, 由原生 editor_api 解释执行并回填状态字段.
 * 协议全部是低频调用, 主线程直接调用即可, 不引入 Handler 与线程池.
 *
 * 原生方法由 jni/native_entry.cpp 提供包装, 本文件不直接接触引擎对象;
 * 未接线时调用会抛 UnsatisfiedLinkError, 这里捕获后把界面降级为只读,
 * 不会因为缺少包装而崩溃.
 */
public final class EditorBridge {

    private static final String TAG = "ImMirrorEditorBridge";

    /* 试玩模式标记. 进入 PlayActivity 时随 Intent 下发, 由游戏侧决定加载哪个世界. */
    public static final String EXTRA_PLAYTEST = "immirror.editor.playtest";

    /* 原生接口可用性缓存: 探测一次后不再重复尝试. */
    private static Boolean available;

    private EditorBridge() {
    }

    /* 探测原生编辑器接口是否已接线; 结果缓存到进程结束. */
    public static synchronized boolean available() {
        if (available != null) {
            return available;
        }
        if (!NativeBridge.ensureLoaded()) {
            available = false;
            return false;
        }
        try {
            /* 探针命令不改变任何状态, 只验证符号是否存在. */
            nativeEditorCommand("ping");
            available = true;
            Log.i(TAG, "editor bridge ready");
        } catch (Throwable error) {
            available = false;
            Log.w(TAG, "editor bridge unavailable: " + error);
        }
        return available;
    }

    /* 发送一条命令. 原生接口缺失或抛异常时返回不可用响应. */
    public static EditorReply command(String command) {
        if (command == null || command.isEmpty()) {
            return EditorReply.unavailable(EditorText.NATIVE_UNAVAILABLE);
        }
        if (!available()) {
            return EditorReply.unavailable(EditorText.NATIVE_UNAVAILABLE);
        }
        try {
            return EditorReply.parse(nativeEditorCommand(command));
        } catch (Throwable error) {
            Log.w(TAG, "editor command failed: " + command + " " + error);
            return EditorReply.unavailable(String.valueOf(error));
        }
    }

    /* 指定应用内部存储目录, 编辑器把工程放在其下的 maps 子目录. */
    public static EditorReply init(String dataPath) {
        return command("init " + (dataPath == null ? "" : dataPath));
    }

    /* 状态查询, 属性面板与工具选择栏都从这一条命令取数. */
    public static EditorReply status() {
        return command("status");
    }

    /* 工程清单. */
    public static EditorReply listProjects() {
        return command("projects");
    }

    /* 方块调色板, 行格式 key|中文名|方块 id. */
    public static EditorReply palette() {
        return command("palette");
    }

    /* 触发器清单, 行格式 id|名称|事件|动作数|启用. */
    public static EditorReply triggers() {
        return command("triggers");
    }

    /* 新建工程. 名称会转成安全的目录名, 显示名保留在工程描述里. */
    public static EditorReply createProject(String name) {
        return command("new " + (name == null ? "" : name));
    }

    public static EditorReply openProject(String directory) {
        return command("open " + (directory == null ? "" : directory));
    }

    public static EditorReply saveProject() {
        return command("save");
    }

    public static EditorReply selectTool(EditorTool tool) {
        return command("tool " + (tool == null ? EditorTool.PLACE.token() : tool.token()));
    }

    public static EditorReply selectBlock(String key) {
        return command("block " + (key == null ? "" : key));
    }

    public static EditorReply setCursor(int x, int y, int z) {
        return command("cursor " + x + " " + y + " " + z);
    }

    /* 在光标处执行当前工具; 放置与删除共用这一条命令. */
    public static EditorReply applyTool() {
        return command("apply");
    }

    public static EditorReply markSelection(boolean second) {
        return command(second ? "sel b" : "sel a");
    }

    public static EditorReply clearSelection() {
        return command("sel clear");
    }

    public static EditorReply fillSelection() {
        return command("fill");
    }

    public static EditorReply undo() {
        return command("undo");
    }

    public static EditorReply redo() {
        return command("redo");
    }

    /* 导出可玩世界. target 为 playtest 时写入应用存档目录, 供试玩加载. */
    public static EditorReply exportWorld(String target) {
        return command("export " + (target == null ? "playtest" : target));
    }

    public static EditorReply addTrigger(String eventType) {
        return command("trigger_add " + (eventType == null ? "enter_region" : eventType));
    }

    public static EditorReply removeTrigger(String id) {
        return command("trigger_remove " + (id == null ? "" : id));
    }

    public static EditorReply runTrigger(String id) {
        return command("trigger_run " + (id == null ? "" : id));
    }

    private static native String nativeEditorCommand(String command);
}
