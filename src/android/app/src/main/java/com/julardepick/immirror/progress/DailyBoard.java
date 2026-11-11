package com.julardepick.immirror.progress;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/*
 * 每日任务板.
 *
 * 数据来自服务端 GET /api/v1/achievements/daily: 每日三条, 进度由服务端按上报的
 * 增量统计结算, 客户端只负责展示与领取. 槽位顺序即服务端给出的顺序.
 */
public final class DailyBoard {

    /* 单条每日任务的展示数据. */
    public static final class Task {
        public final int slot;
        public final String name;
        public final int kind;
        public final int target;
        public final int progress;
        public final int coinReward;
        public final boolean completed;
        public final boolean claimed;

        Task(int slot, String name, int kind, int target, int progress, int coinReward,
             boolean completed, boolean claimed) {
            this.slot = slot;
            this.name = name;
            this.kind = kind;
            this.target = target;
            this.progress = progress;
            this.coinReward = coinReward;
            this.completed = completed;
            this.claimed = claimed;
        }

        public int safeTarget() {
            return Math.max(target, 1);
        }
    }

    public String date = "";
    public final List<Task> tasks = new ArrayList<>();

    public static DailyBoard parse(JSONObject data) {
        DailyBoard board = new DailyBoard();
        if (data == null) {
            return board;
        }
        board.date = data.optString("date", "");
        JSONArray array = data.optJSONArray("tasks");
        if (array == null) {
            array = data.optJSONArray("items");
        }
        if (array != null) {
            for (int index = 0; index < array.length(); index++) {
                JSONObject entry = array.optJSONObject(index);
                if (entry == null) {
                    continue;
                }
                board.tasks.add(parseTask(entry, index));
            }
        }
        Collections.sort(board.tasks, new Comparator<Task>() {
            @Override
            public int compare(Task left, Task right) {
                return Integer.compare(left.slot, right.slot);
            }
        });
        return board;
    }

    private static Task parseTask(JSONObject entry, int index) {
        int slot = entry.optInt("slot", index);
        if (slot < 0) {
            slot = index;
        }
        String name = entry.optString("name", "每日任务 " + (slot + 1));
        int kind = entry.optInt("kind", 0);
        int target = Math.max(entry.optInt("target", 1), 1);
        int progress = entry.optInt("progress", entry.optInt("current", 0));
        progress = Math.max(0, Math.min(progress, target));
        int coinReward = entry.optInt("coinReward",
                entry.optInt("coins", entry.optInt("reward", 0)));
        boolean completed = entry.optBoolean("completed",
                entry.optBoolean("done", progress >= target));
        boolean claimed = entry.optBoolean("claimed",
                entry.optBoolean("awarded", false));
        return new Task(slot, name, kind, target, progress, coinReward, completed, claimed);
    }

    /* 已完成且未领取的条数, 用于页签上的角标提示. */
    public int claimableCount() {
        int count = 0;
        for (Task task : tasks) {
            if (task.completed && !task.claimed) {
                count++;
            }
        }
        return count;
    }
}
