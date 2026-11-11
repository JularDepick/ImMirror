package com.julardepick.immirror.wallet;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

/*
 * 本月签到状态.
 *
 * 服务端契约: signedToday 与 monthCount 恒有; nextReward 只在未签时给出,
 * todayReward 只在已签时给出, 另一侧为 null, 这里按 signedToday 判定有效侧;
 * 服务端后续若补充 signedDates 逐日数组, 这里一并读取, 缺失时由流水反推.
 */
public final class CheckinStatus {

    /* 里程碑: 当月累计第 count 次签到额外发 reward 镜币. */
    public static final class Milestone {
        private final int count;
        private final long reward;
        private final boolean achieved;

        private Milestone(int count, long reward, boolean achieved) {
            this.count = count;
            this.reward = reward;
            this.achieved = achieved;
        }

        static Milestone parse(JSONObject raw) {
            if (raw == null) {
                return null;
            }
            return new Milestone(raw.optInt("count", 0), raw.optLong("reward", 0L),
                    raw.optBoolean("achieved", false));
        }

        public int count() {
            return count;
        }

        public long reward() {
            return reward;
        }

        public boolean achieved() {
            return achieved;
        }
    }

    private final boolean signedToday;
    private final int monthCount;
    private final String month;
    private final long nextReward;
    private final boolean nextRewardKnown;
    private final long todayReward;
    private final boolean todayRewardKnown;
    private final List<Milestone> milestones;
    private final List<String> signedDates;

    private CheckinStatus(boolean signedToday, int monthCount, String month, long nextReward,
                          boolean nextRewardKnown, long todayReward, boolean todayRewardKnown,
                          List<Milestone> milestones, List<String> signedDates) {
        this.signedToday = signedToday;
        this.monthCount = monthCount;
        this.month = month;
        this.nextReward = nextReward;
        this.nextRewardKnown = nextRewardKnown;
        this.todayReward = todayReward;
        this.todayRewardKnown = todayRewardKnown;
        this.milestones = milestones;
        this.signedDates = signedDates;
    }

    public static CheckinStatus parse(JSONObject raw) {
        boolean signed = raw != null && raw.optBoolean("signedToday", false);
        long next = raw == null ? 0L : raw.optLong("nextReward", 0L);
        long today = raw == null ? 0L : raw.optLong("todayReward", 0L);
        List<Milestone> milestones = new ArrayList<>();
        List<String> dates = new ArrayList<>();
        if (raw != null) {
            JSONArray rawMilestones = raw.optJSONArray("milestones");
            if (rawMilestones != null) {
                for (int index = 0; index < rawMilestones.length(); index++) {
                    Milestone milestone = Milestone.parse(rawMilestones.optJSONObject(index));
                    if (milestone != null) {
                        milestones.add(milestone);
                    }
                }
            }
            JSONArray rawDates = raw.optJSONArray("signedDates");
            if (rawDates != null) {
                for (int index = 0; index < rawDates.length(); index++) {
                    String date = rawDates.optString(index, "");
                    if (!date.isEmpty()) {
                        dates.add(date);
                    }
                }
            }
        }
        return new CheckinStatus(signed, raw == null ? 0 : raw.optInt("monthCount", 0),
                raw == null ? "" : raw.optString("month", ""), next, !signed, today, signed,
                milestones, dates);
    }

    public boolean signedToday() {
        return signedToday;
    }

    public int monthCount() {
        return monthCount;
    }

    /* 服务端自然月键, 形如 2026-11, 日历按它渲染. */
    public String month() {
        return month;
    }

    /* 本次签到可领的奖励, 仅未签时有效. */
    public long nextReward() {
        return nextReward;
    }

    public boolean nextRewardKnown() {
        return nextRewardKnown;
    }

    /* 今日已领的奖励, 仅已签时有效. */
    public long todayReward() {
        return todayReward;
    }

    public boolean todayRewardKnown() {
        return todayRewardKnown;
    }

    public List<Milestone> milestones() {
        return milestones;
    }

    /* 服务端直发的逐日签到日期, 当前契约未包含, 恒为空表. */
    public List<String> signedDates() {
        return signedDates;
    }
}
