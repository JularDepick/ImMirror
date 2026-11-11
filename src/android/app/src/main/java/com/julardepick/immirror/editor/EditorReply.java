package com.julardepick.immirror.editor;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/*
 * 编辑器命令的响应.
 *
 * 原生侧统一返回 "键=值" 的多行文本: 首部是 code 与 message,
 * 其余是本次命令回填的状态字段; 同一个键出现多次表示列表 (工程, 触发器, 日志行等).
 * 这里只做文本到结构的解析, 不做语义判断, 具体取值由界面按需读取.
 */
public final class EditorReply {

    /* 命令成功. */
    public static final int CODE_OK = 0;

    /* 原生接口未接线, 由 Java 侧伪造, 不来自原生. */
    public static final int CODE_UNAVAILABLE = -1;

    private final int code;
    private final String message;
    private final Map<String, List<String>> values;

    private EditorReply(int code, String message, Map<String, List<String>> values) {
        this.code = code;
        this.message = message;
        this.values = values;
    }

    /* 原生接口不可用时的占位响应, 让界面走与失败一致的路径. */
    public static EditorReply unavailable(String reason) {
        return new EditorReply(CODE_UNAVAILABLE, reason, new LinkedHashMap<String, List<String>>());
    }

    /* 解析原生返回的多行文本; 空文本得到 code 缺失的响应, 按不可用处理. */
    public static EditorReply parse(String payload) {
        Map<String, List<String>> values = new LinkedHashMap<>();
        if (payload != null) {
            for (String rawLine : payload.split("\n")) {
                String line = rawLine.trim();
                if (line.isEmpty() || line.startsWith("#")) {
                    continue;
                }
                int split = line.indexOf('=');
                if (split <= 0) {
                    continue;
                }
                String key = line.substring(0, split).trim();
                String value = line.substring(split + 1);
                List<String> bucket = values.get(key);
                if (bucket == null) {
                    bucket = new ArrayList<>();
                    values.put(key, bucket);
                }
                bucket.add(value);
            }
        }
        int code = parseInt(firstOf(values, "code"), CODE_UNAVAILABLE);
        return new EditorReply(code, firstOf(values, "message"), values);
    }

    public boolean ok() {
        return code == CODE_OK;
    }

    public int code() {
        return code;
    }

    public String message() {
        return message;
    }

    /* 单值字段, 缺失返回空串. */
    public String first(String key) {
        return firstOf(values, key);
    }

    public String first(String key, String fallback) {
        String value = firstOf(values, key);
        return value.isEmpty() ? fallback : value;
    }

    /* 列表字段的全部取值, 缺失返回空表. */
    public List<String> all(String key) {
        List<String> bucket = values.get(key);
        return bucket == null ? new ArrayList<String>() : bucket;
    }

    public int count(String key) {
        return all(key).size();
    }

    public int intValue(String key, int fallback) {
        return parseInt(first(key), fallback);
    }

    public float floatValue(String key, float fallback) {
        try {
            return Float.parseFloat(first(key).trim());
        } catch (NumberFormatException error) {
            return fallback;
        }
    }

    /* 逗号分隔的整数三元组, 例如光标坐标; 解析失败返回 null. */
    public int[] triple(String key) {
        String value = first(key);
        String[] parts = value.split(",");
        if (parts.length != 3) {
            return null;
        }
        int[] result = new int[3];
        for (int index = 0; index < 3; index++) {
            try {
                result[index] = Integer.parseInt(parts[index].trim());
            } catch (NumberFormatException error) {
                return null;
            }
        }
        return result;
    }

    /* 逗号分隔的浮点三元组, 例如出生点与试玩玩家位置. */
    public float[] floats(String key) {
        String value = first(key);
        String[] parts = value.split(",");
        if (parts.length != 3) {
            return null;
        }
        float[] result = new float[3];
        for (int index = 0; index < 3; index++) {
            try {
                result[index] = Float.parseFloat(parts[index].trim());
            } catch (NumberFormatException error) {
                return null;
            }
        }
        return result;
    }

    /* 把一行 "a|b|c" 按竖线切成字段; 行内空段保留, 便于按位置取值. */
    public static String[] fields(String row) {
        return row == null ? new String[0] : row.split("\\|", -1);
    }

    private static String firstOf(Map<String, List<String>> values, String key) {
        List<String> bucket = values.get(key);
        if (bucket == null || bucket.isEmpty()) {
            return "";
        }
        return bucket.get(0);
    }

    private static int parseInt(String text, int fallback) {
        try {
            return Integer.parseInt(text.trim());
        } catch (NumberFormatException error) {
            return fallback;
        }
    }
}
