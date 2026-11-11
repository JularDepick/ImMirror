package com.julardepick.immirror.progress;

import org.json.JSONObject;

/*
 * 单条每日任务的展示数据, 与服务端 /api/v1/achievements/daily 返回的条目一一对应.
 * 任务由服务端按日期确定性地从模板中抽取, 客户端只负责展示当日进度.
 */
public final class DailyTaskEntry {

    private final int slot;
    private final String name;
    private final String kindTitle;
    private final String unit;
    private final int target;
    private final int value;
    private final boolean completed;
    private final int coinReward;

    private DailyTaskEntry(int slot, String name, String kindTitle, String unit, int target, int value,
            boolean completed, int coinReward) {
        this.slot = slot;
        this.name = name;
        this.kindTitle = kindTitle;
        this.unit = unit;
        this.target = target;
        this.value = value;
        this.completed = completed;
        this.coinReward = coinReward;
    }

    /* 解析单条任务; 缺少名称或目标非法时返回 null. */
    public static DailyTaskEntry parse(JSONObject json) {
        if (json == null) {
            return null;
        }
        String name = json.optString("name", "");
        int target = Math.max(json.optInt("target", 0), 0);
        if (name.isEmpty() || target <= 0) {
            return null;
        }
        int value = Math.max(json.optInt("value", 0), 0);
        if (value > target) {
            value = target;
        }
        return new DailyTaskEntry(
                json.optInt("slot", 0),
                name,
                json.optString("kindTitle", ""),
                json.optString("unit", ""),
                target,
                value,
                json.optBoolean("completed", false),
                json.optInt("coinReward", 0));
    }

    public int slot() {
        return slot;
    }

    public String name() {
        return name;
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

    public float ratio() {
        if (target <= 0) {
            return 0f;
        }
        return Math.min(1f, (float) value / (float) target);
    }
}
