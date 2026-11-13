package com.julardepick.immirror.settings;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/*
 * 设置项的语义映射表.
 *
 * 键名与中文语义取自原生设置层 settings_keys.h 的键表, 分组与接线状态在本文件声明.
 * 新增或调整设置项时只改下方表项, 设置界面会按 describeSettings 给出的键顺序生成控件,
 * 表外的未知键走兜底条目, 因此原生侧先加键也不会让界面漏项.
 *
 * wired 为 true 表示原生侧已经读取该键并影响玩法或画面, 依据是 game_shell.cpp 的读取点:
 * 视野距离, 操作灵敏度, 反转 Y 轴, 左手模式, 经典与摇杆操作, 自动跳跃, 镜头摇晃,
 * 音量, 音乐, 音效, 语音与隐藏界面. 其余键照常展示并标注暂未生效, 便于后续接线时对照.
 */
public final class SettingsCatalog {

    /* 经典与摇杆互斥, 页面据此刷新对应项. */
    public static final String KEY_CLASSICAL = "classical";
    public static final String KEY_ROCKER = "rocker";

    /* 分组顺序即页面中的展示顺序. */
    public enum Group {
        VISUAL("画面"),
        AUDIO("音效"),
        CONTROL("操作"),
        OTHER("其他");

        private final String title;

        Group(String title) {
            this.title = title;
        }

        public String title() {
            return title;
        }
    }

    /* 单个键的静态元数据: 名称, 分组, 接线状态与可选档位. */
    public static final class Spec {
        private final String key;
        private final String label;
        private final Group group;
        private final boolean wired;
        private final int[] levels;
        private final String[] levelLabels;

        private Spec(String key, String label, Group group, boolean wired, int[] levels,
                String[] levelLabels) {
            this.key = key;
            this.label = label;
            this.group = group;
            this.wired = wired;
            this.levels = levels;
            this.levelLabels = levelLabels;
        }

        public String key() {
            return key;
        }

        public String label() {
            return label;
        }

        public Group group() {
            return group;
        }

        public boolean wired() {
            return wired;
        }

        public boolean hasLevels() {
            return levels != null && levels.length > 0;
        }

        public int levelCount() {
            return hasLevels() ? levels.length : 0;
        }

        public int levelAt(int index) {
            return levels[index];
        }

        public String levelLabelAt(int index) {
            return levelLabels == null ? String.valueOf(levels[index]) : levelLabels[index];
        }
    }

    private static final Map<String, Spec> TABLE = new HashMap<>();

    static {
        /*
         * 画面. 视野距离的合法档位是 1 2 3 5 8, 与原生 NormalizeValue 的吸附值一致,
         * 因此这里显式给出档位, 不按区间等分.
         */
        addChoice("configure", Group.VISUAL, false, "画质总档",
                new int[]{1, 2, 3}, new String[]{"低", "中", "高"});
        addChoice("view", Group.VISUAL, false, "视角",
                new int[]{1, 2, 3}, new String[]{"主视角", "背视角", "正视角"});
        add("sight", Group.VISUAL, false, "准心模式");
        add("screenbright", Group.VISUAL, true, "画面亮度");
        addChoice("view_distance", Group.VISUAL, true, "视野距离",
                new int[]{1, 2, 3, 5, 8}, new String[]{"近", "中", "远", "更远", "最远"});
        addChoice("fog", Group.VISUAL, true, "雾效",
                new int[]{0, 1, 2}, new String[]{"关闭", "淡雾", "浓雾"});
        add("treeshape", Group.VISUAL, true, "树形");
        add("reflect", Group.VISUAL, false, "水面反射");
        add("shadow", Group.VISUAL, false, "实时光影");
        add("limiteFps", Group.VISUAL, false, "限制帧率");

        /* 音效. */
        add("volume", Group.AUDIO, true, "音量");
        add("musicopen", Group.AUDIO, true, "音乐开关");
        add("soundopen", Group.AUDIO, true, "音效开关");
        add("voiceopen", Group.AUDIO, true, "语音开关");

        /* 操作. */
        add("sensitivity", Group.CONTROL, true, "操作灵敏度");
        add("reversalY", Group.CONTROL, true, "反转 Y 轴");
        add("lefthanded", Group.CONTROL, true, "左手模式");
        add("autojump", Group.CONTROL, true, "自动跳跃");
        add("radarSteering", Group.CONTROL, false, "雷达转向");
        add("camerashake", Group.CONTROL, true, "镜头摇晃");
        add("vibrate", Group.CONTROL, false, "震动");
        add("classical", Group.CONTROL, true, "经典操作模式");
        add("rocker", Group.CONTROL, true, "摇杆操作模式");

        /*
         * 其他. 界面与提示类开关归到这里, 画面组只放影响场景渲染质量的项,
         * 避免同一页里质量档与界面开关混在一起.
         */
        add("peacemodel", Group.OTHER, true, "和平模式");
        add("popups", Group.OTHER, false, "邀请弹窗");
        add("fpsbuttom", Group.OTHER, false, "帧率显示按钮");
        add("physxparam", Group.OTHER, false, "物理参数面板");
        add("cantrace", Group.OTHER, false, "允许好友追踪");
        add("damageswitch", Group.OTHER, true, "伤害数值显示");
        add("bloodswitch", Group.OTHER, true, "血条显示");
        add("hideui", Group.OTHER, true, "隐藏界面");
    }

    private SettingsCatalog() {
    }

    /* 查询静态元数据, 未知键返回兜底条目, 调用方不必判空. */
    public static Spec spec(String key) {
        String safeKey = key == null ? "" : key;
        Spec found = TABLE.get(safeKey);
        if (found != null) {
            return found;
        }
        return new Spec(safeKey, safeKey, Group.OTHER, false, null, null);
    }

    /*
     * 合并 describeSettings 与 exportSettings 的输出.
     * describe 每行是 "键|默认值|最小值|最大值", 若原生侧以后追加第五列中文名则优先采用;
     * export 每行是 "键=值", 决定控件初始位置.
     */
    public static List<SettingsItem> merge(String describeText, String exportText) {
        Map<String, Integer> current = parseExport(exportText);
        List<SettingsItem> items = new ArrayList<>();
        if (describeText == null) {
            return items;
        }
        for (String rawLine : describeText.split("\n")) {
            String line = rawLine.trim();
            if (line.isEmpty()) {
                continue;
            }
            String[] parts = line.split("\\|", -1);
            if (parts.length < 4) {
                continue;
            }
            String key = parts[0].trim();
            if (key.isEmpty()) {
                continue;
            }
            int defaultValue = parseInt(parts[1], 0);
            int min = parseInt(parts[2], 0);
            int max = parseInt(parts[3], min);
            String nativeLabel = parts.length >= 5 ? parts[4].trim() : "";
            int value = current.containsKey(key) ? current.get(key) : defaultValue;
            items.add(new SettingsItem(spec(key), nativeLabel, defaultValue, min, max, value));
        }
        return items;
    }

    private static Map<String, Integer> parseExport(String text) {
        Map<String, Integer> values = new HashMap<>();
        if (text == null) {
            return values;
        }
        for (String rawLine : text.split("\n")) {
            String line = rawLine.trim();
            int split = line.indexOf('=');
            if (split <= 0) {
                continue;
            }
            values.put(line.substring(0, split).trim(), parseInt(line.substring(split + 1), 0));
        }
        return values;
    }

    private static int parseInt(String text, int fallback) {
        try {
            return Integer.parseInt(text.trim());
        } catch (NumberFormatException error) {
            return fallback;
        }
    }

    /* 无档位声明的键由取值范围推断控件类型, 这里只需登记语义. */
    private static void add(String key, Group group, boolean wired, String label) {
        TABLE.put(key, new Spec(key, label, group, wired, null, null));
    }

    private static void addChoice(String key, Group group, boolean wired, String label,
            int[] levels, String[] levelLabels) {
        TABLE.put(key, new Spec(key, label, group, wired, levels, levelLabels));
    }
}
