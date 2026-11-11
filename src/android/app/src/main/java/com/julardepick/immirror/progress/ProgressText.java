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
    public static final String NOTICE_SESSION = "登录已失效, 请重新登录";
    public static final String NOTICE_REFRESHED = "成就与任务已刷新";

    private ProgressText() {
    }
}
