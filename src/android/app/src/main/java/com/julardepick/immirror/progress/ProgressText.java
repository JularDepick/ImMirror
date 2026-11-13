package com.julardepick.immirror.progress;

/*
 * 成就与任务界面的骨架文案.
 *
 * 成就名称, 描述, 分类与每日任务名称都由服务端随数据下发, 这里只放页面标题,
 * 标签, 状态与提示这类固定文本. 与 settings 包把中文语义留在 Java 常量里的做法一致,
 * 好处是本页新增时不必改动 values/strings.xml, 换肤与多语言时只看本文件.
 */
public final class ProgressText {

    public static final String TITLE = "成就与任务";
    public static final String SUBTITLE = "完成成就与每日任务可获得镜币";

    public static final String TAB_ACHIEVEMENT = "成就";
    public static final String TAB_DAILY = "每日任务";

    public static final String FILTER_ALL = "全部";

    public static final String STATUS_DONE = "已完成";
    public static final String STATUS_DOING = "进行中";

    public static final String REWARD_FORMAT = "奖励 %1$d 镜币";
    public static final String PROGRESS_FORMAT = "%1$d / %2$d %3$s";
    public static final String SUMMARY_FORMAT = "成就 %1$d / %2$d · 今日任务 %3$d / %4$d";
    public static final String BALANCE_FORMAT = "镜币 %1$d";
    public static final String EARNED_FORMAT = "已获得 %1$d 镜币";
    public static final String DAILY_DATE_FORMAT = "%1$s 的每日任务";
    public static final String KIND_FORMAT = "%1$s · 每 %2$s 计一次";
    public static final String CATEGORY_COUNT_FORMAT = "%1$s (%2$d)";

    public static final String EMPTY = "暂无可显示的条目";
    public static final String LOGIN_REQUIRED = "请先登录后再查看成就";
    public static final String LOADING = "正在读取成就与任务...";

    public static final String BACK = "返回";
    public static final String REFRESH = "刷新";

    public static final String NOTICE_NETWORK = "网络不可用, 请检查服务端地址";

    /* 成就领取入口: 列表页只展示进度, 领取在独立界面完成. */
    public static final String CLAIM_ENTRY = "领取成就奖励";
    public static final String NOTICE_SESSION = "登录已失效, 请重新登录";
    public static final String NOTICE_REFRESHED = "成就与任务已刷新";

    public static final String ACTION_REFRESH = REFRESH;
    public static final String NOTICE_LOADING = LOADING;

    public static String progressOf(int value, int target, String unit) {
        return String.format(PROGRESS_FORMAT, value, target, unit);
    }

    private static final String[] CATEGORY_NAMES = {"", "新手", "主线", "生产", "战斗", "其他", "极限"};

    public static String categoryName(int category) {
        if (category < 1 || category >= CATEGORY_NAMES.length) {
            return "其他";
        }
        return CATEGORY_NAMES[category];
    }

    private static final String[] KIND_TITLES = {"破坏方块", "放置方块", "采集物品", "行走距离", "在线时长", "签到", "在线天数"};

    public static String kindTitle(int kind) {
        if (kind < 1 || kind > KIND_TITLES.length) {
            return "其他";
        }
        return KIND_TITLES[kind - 1];
    }

    private ProgressText() {
    }
}