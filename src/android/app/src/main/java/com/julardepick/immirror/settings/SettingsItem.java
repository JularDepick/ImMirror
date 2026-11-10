package com.julardepick.immirror.settings;

/*
 * 运行期设置项: 语义元数据 + 默认值 + 取值范围 + 当前值.
 *
 * 控件类型完全由取值范围推断, 不按键手写:
 * 0 与 1 两档用开关, 0 到 100 这类连续大区间用滑杆, 其余小范围离散值用分段选择.
 * 档位不连续的键 (例如视野距离 1 2 3 5 8) 由 SettingsCatalog 提供显式档位,
 * 因此分段数量与原生吸附值保持一一对应.
 */
public final class SettingsItem {

    public static final int TYPE_TOGGLE = 0;
    public static final int TYPE_SLIDER = 1;
    public static final int TYPE_CHOICE = 2;

    /* 未声明档位时, 区间跨度不超过该值的键按离散枚举处理, 再大就改用滑杆. */
    private static final int CHOICE_SPAN_LIMIT = 7;

    private final SettingsCatalog.Spec spec;
    private final String label;
    private final int defaultValue;
    private final int min;
    private final int max;
    private final int type;
    private final int[] levels;
    private final String[] levelLabels;
    private int value;

    SettingsItem(SettingsCatalog.Spec spec, String nativeLabel, int defaultValue, int min, int max,
            int value) {
        this.spec = spec;
        this.label = nativeLabel == null || nativeLabel.isEmpty() ? spec.label() : nativeLabel;
        this.defaultValue = defaultValue;
        this.min = min;
        this.max = max;
        this.value = value;

        if (spec.hasLevels()) {
            this.type = TYPE_CHOICE;
            int count = spec.levelCount();
            this.levels = new int[count];
            this.levelLabels = new String[count];
            for (int index = 0; index < count; index++) {
                levels[index] = spec.levelAt(index);
                levelLabels[index] = spec.levelLabelAt(index);
            }
        } else if (min == 0 && max == 1) {
            this.type = TYPE_TOGGLE;
            this.levels = null;
            this.levelLabels = null;
        } else if (max - min <= CHOICE_SPAN_LIMIT) {
            this.type = TYPE_CHOICE;
            int count = max - min + 1;
            this.levels = new int[count];
            this.levelLabels = new String[count];
            for (int index = 0; index < count; index++) {
                levels[index] = min + index;
                levelLabels[index] = String.valueOf(min + index);
            }
        } else {
            this.type = TYPE_SLIDER;
            this.levels = null;
            this.levelLabels = null;
        }
    }

    public String key() {
        return spec.key();
    }

    public String label() {
        return label;
    }

    public SettingsCatalog.Group group() {
        return spec.group();
    }

    public boolean wired() {
        return spec.wired();
    }

    public int defaultValue() {
        return defaultValue;
    }

    public int min() {
        return min;
    }

    public int max() {
        return max;
    }

    public int type() {
        return type;
    }

    public int value() {
        return value;
    }

    void setValue(int value) {
        this.value = value;
    }

    public int levelCount() {
        return levels == null ? 0 : levels.length;
    }

    public int levelAt(int index) {
        return levels[index];
    }

    public String levelLabelAt(int index) {
        return levelLabels[index];
    }

    /* 取当前值在档位表中的下标, 不在档位上时返回 -1. */
    public int levelIndex(int candidate) {
        if (levels == null) {
            return -1;
        }
        for (int index = 0; index < levels.length; index++) {
            if (levels[index] == candidate) {
                return index;
            }
        }
        return -1;
    }
}
