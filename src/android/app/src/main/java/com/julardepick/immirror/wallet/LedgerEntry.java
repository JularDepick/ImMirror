package com.julardepick.immirror.wallet;

import org.json.JSONObject;

/*
 * 单条钱包流水.
 * delta 为正表示入账, 为负表示支出; reason 与 ref 记录来源,
 * 事由取值与钱包流水表 reason 列一致, 文案由界面层映射.
 */
public final class LedgerEntry {

    /* 服务端记账事由取值. */
    public static final String REASON_CHECKIN = "checkin";
    public static final String REASON_CHECKIN_MILESTONE = "checkin_milestone";
    public static final String REASON_ACHIEVEMENT = "achievement";
    public static final String REASON_DAILY_TASK = "daily_task";

    private final long id;
    private final long delta;
    private final long balanceAfter;
    private final String reason;
    private final String ref;
    private final long createdAt;

    private LedgerEntry(long id, long delta, long balanceAfter, String reason, String ref, long createdAt) {
        this.id = id;
        this.delta = delta;
        this.balanceAfter = balanceAfter;
        this.reason = reason;
        this.ref = ref;
        this.createdAt = createdAt;
    }

    /* 字段缺失一律按零或空串处理, 不因少字段丢弃整条记录. */
    public static LedgerEntry parse(JSONObject raw) {
        if (raw == null) {
            return null;
        }
        return new LedgerEntry(raw.optLong("id", 0L), raw.optLong("delta", 0L),
                raw.optLong("balanceAfter", 0L), raw.optString("reason", ""),
                raw.optString("ref", ""), raw.optLong("createdAt", 0L));
    }

    public long id() {
        return id;
    }

    public long delta() {
        return delta;
    }

    public long balanceAfter() {
        return balanceAfter;
    }

    public String reason() {
        return reason;
    }

    public String ref() {
        return ref;
    }

    public long createdAt() {
        return createdAt;
    }

    /* 是否入账, 界面据此选择正负号与颜色. */
    public boolean income() {
        return delta > 0L;
    }
}
