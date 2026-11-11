package com.julardepick.immirror.progress;

import org.json.JSONObject;

/*
 * 单条成就的展示数据, 与服务端 /api/v1/achievements 返回的条目一一对应.
 * 只保留界面需要的字段, 未知字段忽略, 因此服务端加字段不会影响本页.
 */
public final class AchievementEntry {

    private final int id;
    private final String name;
    private final String description;
    private final int category;
    private final String categoryName;
    private final String kindTitle;
    private final String unit;
    private final int target;
    private final int value;
    private final boolean completed;
    private final int coinReward;

    private AchievementEntry(int id, String name, String description, int category, String categoryName,
            String kindTitle, String unit, int target, int value, boolean completed, int coinReward) {
        this.id = id;
        this.name = name;
        this.description = description;
        this.category = category;
        this.categoryName = categoryName;
        this.kindTitle = kindTitle;
        this.unit = unit;
        this.target = target;
        this.value = value;
        this.completed = completed;
        this.coinReward = coinReward;
    }

    /* 解析单条成就; 缺少必要字段时返回 null, 由调用方跳过. */
    public static AchievementEntry parse(JSONObject json) {
        if (json == null) {
            return null;
        }
        int id = json.optInt("id", 0);
        String name = json.optString("name", "");
        if (id <= 0 || name.isEmpty()) {
            return null;
        }
        int target = Math.max(json.optInt("target", 0), 0);
        int value = Math.max(json.optInt("value", 0), 0);
        if (target > 0 && value > target) {
            value = target;
        }
        return new AchievementEntry(
                id,
                name,
                json.optString("description", ""),
                json.optInt("category", 0),
                json.optString("categoryName", ""),
                json.optString("kindTitle", ""),
                json.optString("unit", ""),
                target,
                value,
                json.optBoolean("completed", false),
                json.optInt("coinReward", 0));
    }

    public int id() {
        return id;
    }

    public String name() {
        return name;
    }

    public String description() {
        return description;
    }

    public int category() {
        return category;
    }

    public String categoryName() {
        return categoryName;
    }

    public String kindTitle() {
        return kindTitle;
    }

    public String unit() {
        return unit;
    }

    public int target() {
        return target;
    }

    public int value() {
        return value;
    }

    public boolean completed() {
        return completed;
    }

    public int coinReward() {
        return coinReward;
    }

    /* 进度比例, 0 到 1; 目标为 0 时按未开始处理. */
    public float ratio() {
        if (target <= 0) {
            return 0f;
        }
        return Math.min(1f, (float) value / (float) target);
    }
}
