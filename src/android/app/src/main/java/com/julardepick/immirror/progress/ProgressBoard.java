package com.julardepick.immirror.progress;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/*
 * 成就与每日任务的一次完整视图.
 *
 * 由 ProgressApi 把 /api/v1/achievements 与 /api/v1/achievements/daily 两份响应合并而成:
 * 前者给出全部成就, 完成数与镜币余额, 后者给出当日三条任务.
 * 对象不可变, 界面每次刷新整体替换, 不做局部更新.
 */
public final class ProgressBoard {

    private final long balance;
    private final int completedCount;
    private final int totalCount;
    private final int earnedCoins;
    private final String dailyDate;
    private final int dailyCompleted;
    private final List<AchievementEntry> achievements;
    private final List<DailyTaskEntry> dailyTasks;

    public ProgressBoard(long balance, int completedCount, int totalCount, int earnedCoins, String dailyDate,
            int dailyCompleted, List<AchievementEntry> achievements, List<DailyTaskEntry> dailyTasks) {
        this.balance = balance;
        this.completedCount = completedCount;
        this.totalCount = totalCount;
        this.earnedCoins = earnedCoins;
        this.dailyDate = dailyDate == null ? "" : dailyDate;
        this.dailyCompleted = dailyCompleted;
        this.achievements = achievements == null
                ? Collections.emptyList() : Collections.unmodifiableList(new ArrayList<>(achievements));
        this.dailyTasks = dailyTasks == null
                ? Collections.emptyList() : Collections.unmodifiableList(new ArrayList<>(dailyTasks));
    }

    /* 空视图, 未登录或读取失败时占位, 界面据此显示空态而不必判空. */
    public static ProgressBoard empty() {
        return new ProgressBoard(0L, 0, 0, 0, "", 0, null, null);
    }

    public long balance() {
        return balance;
    }

    public int completedCount() {
        return completedCount;
    }

    public int totalCount() {
        return totalCount;
    }

    /* 已完成成就的镜币合计, 用于总览卡片. */
    public int earnedCoins() {
        return earnedCoins;
    }

    public String dailyDate() {
        return dailyDate;
    }

    public int dailyCompleted() {
        return dailyCompleted;
    }

    public int dailyTotal() {
        return dailyTasks.size();
    }

    public List<AchievementEntry> achievements() {
        return achievements;
    }

    public List<DailyTaskEntry> dailyTasks() {
        return dailyTasks;
    }

    /* 按分类筛出成就, category 传 0 表示全部. */
    public List<AchievementEntry> achievementsOf(int category) {
        if (category == 0) {
            return achievements;
        }
        List<AchievementEntry> filtered = new ArrayList<>();
        for (AchievementEntry entry : achievements) {
            if (entry.category() == category) {
                filtered.add(entry);
            }
        }
        return filtered;
    }

    /* 出现在数据里的分类下标, 升序; 界面据此生成筛选标签, 不写死分类表. */
    public List<Integer> categories() {
        List<Integer> found = new ArrayList<>();
        for (AchievementEntry entry : achievements) {
            if (!found.contains(entry.category())) {
                found.add(entry.category());
            }
        }
        Collections.sort(found);
        return found;
    }

    /* 分类下标到中文名, 取数据里第一条同分类成就的分类名. */
    public String categoryName(int category) {
        for (AchievementEntry entry : achievements) {
            if (entry.category() == category) {
                return entry.categoryName();
            }
        }
        return "";
    }

    public int countOf(int category) {
        int total = 0;
        for (AchievementEntry entry : achievements) {
            if (category == 0 || entry.category() == category) {
                total++;
            }
        }
        return total;
    }
}
