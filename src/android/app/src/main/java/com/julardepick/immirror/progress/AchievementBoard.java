package com.julardepick.immirror.progress;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

/*
 * 成就总览.
 *
 * 数据来自服务端 GET /api/v1/achievements, 字段全部按可选处理:
 * 服务端补充字段时界面不需要同步改动, 缺字段时用本地默认值兜底.
 * 排序规则为未完成在前, 同组内按 ID 升序, 便于玩家先看到待推进的条目.
 */
public final class AchievementBoard {

    /* 单条成就的展示数据. */
    public static final class Item {
        public final int id;
        public final String name;
        public final String description;
        public final int category;
        public final int kind;
        public final int target;
        public final int progress;
        public final int coinReward;
        public final boolean completed;
        public final boolean claimed;

        Item(int id, String name, String description, int category, int kind, int target, int progress,
             int coinReward, boolean completed, boolean claimed) {
            this.id = id;
            this.name = name;
            this.description = description;
            this.category = category;
            this.kind = kind;
            this.target = target;
            this.progress = progress;
            this.coinReward = coinReward;
            this.completed = completed;
            this.claimed = claimed;
        }

        /* 目标值兜底为 1, 避免进度条除零. */
        public int safeTarget() {
            return Math.max(target, 1);
        }
    }

    public long balance;
    public int total;
    public int completed;
    public int claimed;
    public long coins;
    public final List<Item> items = new ArrayList<>();

    /* 解析总览响应; 数组缺失时给出空列表, 由界面显示空状态. */
    public static AchievementBoard parse(JSONObject data) {
        AchievementBoard board = new AchievementBoard();
        if (data == null) {
            return board;
        }
        board.balance = data.optLong("balance", 0L);
        JSONObject summary = data.optJSONObject("summary");
        if (summary != null) {
            board.total = summary.optInt("total", 0);
            board.completed = summary.optInt("completed", 0);
            board.claimed = summary.optInt("claimed", 0);
            board.coins = summary.optLong("coins", 0L);
        }
        JSONArray array = data.optJSONArray("achievements");
        if (array == null) {
            array = data.optJSONArray("items");
        }
        if (array != null) {
            for (int index = 0; index < array.length(); index++) {
                JSONObject entry = array.optJSONObject(index);
                if (entry == null) {
                    continue;
                }
                board.items.add(parseItem(entry));
            }
        }
        if (board.total <= 0) {
            board.total = board.items.size();
        }
        if (board.completed <= 0) {
            int done = 0;
            for (Item item : board.items) {
                if (item.completed) {
                    done++;
                }
            }
            board.completed = done;
        }
        if (board.coins <= 0L) {
            long sum = 0L;
            for (Item item : board.items) {
                if (item.claimed) {
                    sum += item.coinReward;
                }
            }
            board.coins = sum;
        }
        board.sort();
        return board;
    }

    private static Item parseItem(JSONObject entry) {
        int id = entry.optInt("id", 0);
        String name = entry.optString("name", "成就 " + id);
        String description = entry.optString("description", entry.optString("desc", ""));
        int category = entry.optInt("category", entry.optInt("group", 0));
        int kind = entry.optInt("kind", 0);
        int target = Math.max(entry.optInt("target", 1), 1);
        int progress = entry.optInt("progress", entry.optInt("current", 0));
        progress = Math.max(0, Math.min(progress, target));
        int coinReward = entry.optInt("coinReward",
                entry.optInt("coins", entry.optInt("reward", 0)));
        boolean completed = entry.optBoolean("completed",
                entry.optBoolean("done", progress >= target));
        boolean claimed = entry.optBoolean("claimed",
                entry.optBoolean("awarded", entry.optBoolean("granted", completed)));
        return new Item(id, name, description, category, kind, target, progress, coinReward,
                completed, claimed);
    }

    /* 未完成在前, 其次按分组与 ID 升序, 保证同一份数据每次渲染顺序一致. */
    private void sort() {
        Collections.sort(items, new Comparator<Item>() {
            @Override
            public int compare(Item left, Item right) {
                if (left.completed != right.completed) {
                    return left.completed ? 1 : -1;
                }
                if (left.category != right.category) {
                    return Integer.compare(left.category, right.category);
                }
                return Integer.compare(left.id, right.id);
            }
        });
    }
}
