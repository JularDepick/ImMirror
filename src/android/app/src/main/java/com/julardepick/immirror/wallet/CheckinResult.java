package com.julardepick.immirror.wallet;

import org.json.JSONObject;

/*
 * 签到结果.
 * 服务端返回本次奖励, 里程碑额外奖励, 签到后的余额与月内累计次数;
 * milestoneReward 为零表示本次没有触发里程碑.
 */
public final class CheckinResult {

    private final long reward;
    private final long balance;
    private final int monthCount;
    private final long milestoneReward;

    private CheckinResult(long reward, long balance, int monthCount, long milestoneReward) {
        this.reward = reward;
        this.balance = balance;
        this.monthCount = monthCount;
        this.milestoneReward = milestoneReward;
    }

    /* 字段缺失一律按零处理, 已由服务端保证签到成功时字段齐全. */
    public static CheckinResult parse(JSONObject raw) {
        if (raw == null) {
            return new CheckinResult(0L, 0L, 0, 0L);
        }
        return new CheckinResult(raw.optLong("reward", 0L), raw.optLong("balance", 0L),
                raw.optInt("monthCount", 0), raw.optLong("milestoneReward", 0L));
    }

    public long reward() {
        return reward;
    }

    public long balance() {
        return balance;
    }

    public int monthCount() {
        return monthCount;
    }

    public long milestoneReward() {
        return milestoneReward;
    }
}
