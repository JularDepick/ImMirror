package com.julardepick.immirror.editor;

/*
 * 编辑工具. 取值与原生命令协议 tool 子命令的取值一一对应,
 * 界面只负责把选中的工具名发给原生侧, 具体手势语义由原生编辑器决定.
 */
public enum EditorTool {

    PLACE("place", EditorText.TOOL_PLACE),
    REMOVE("remove", EditorText.TOOL_REMOVE),
    FILL("fill", EditorText.TOOL_FILL);

    private final String token;
    private final String label;

    EditorTool(String token, String label) {
        this.token = token;
        this.label = label;
    }

    /* 命令协议中的取值. */
    public String token() {
        return token;
    }

    /* 工具选择栏上显示的文本. */
    public String label() {
        return label;
    }

    /* 按协议取值反查工具; 未知取值回落到给定默认值. */
    public static EditorTool fromToken(String value, EditorTool fallback) {
        if (value != null) {
            for (EditorTool tool : values()) {
                if (tool.token.equals(value.trim())) {
                    return tool;
                }
            }
        }
        return fallback;
    }
}
