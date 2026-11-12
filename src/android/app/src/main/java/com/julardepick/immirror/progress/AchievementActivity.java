package com.julardepick.immirror.progress;

import android.app.Activity;
import android.graphics.Typeface;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.julardepick.immirror.R;
import com.julardepick.immirror.account.AccountTheme;
import com.julardepick.immirror.account.ApiException;
import com.julardepick.immirror.account.ErrorMessages;
import com.julardepick.immirror.account.NoticeBar;
import com.julardepick.immirror.account.Session;
import com.julardepick.immirror.account.SessionStore;
import com.julardepick.immirror.ui.PageBackground;

import org.json.JSONObject;

/*
 * 成就与任务页面.
 *
 * 从账号面板进入, 一页两签: 成就是全部可推进条目与总览, 每日任务是当日三条任务.
 * 列表由原生控件逐行构建, 每行带进度条与领取按钮; 数据统一取服务端,
 * 断网时回落到上一次成功响应的缓存, 因此页面不会出现空白.
 * 所有提示都显示在页面内的提示条里, 不使用 Toast 与系统弹窗.
 */
public final class AchievementActivity extends Activity {

    private static final int TAB_ACHIEVEMENT = 0;
    private static final int TAB_DAILY = 1;

    private AchievementSync sync;
    private Session session;

    private TextView tabAchievements;
    private TextView tabDaily;
    private TextView summaryView;
    private TextView noticeView;
    private NoticeBar noticeBar;
    private Button refreshButton;
    private Button filterButton;
    private LinearLayout listContainer;

    private AchievementBoard achievements;
    private DailyBoard daily;
    private int tab = TAB_ACHIEVEMENT;
    private boolean onlyUnfinished;
    private boolean busy;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        sync = new AchievementSync(this);
        session = SessionStore.load(this);
        setContentView(buildContentView());
        applyTab();
        loadCache();
        if (session == null || !session.isUsable()) {
            showNotice(ProgressText.NOTICE_SESSION, AccountTheme.COLOR_ERROR);
            refreshButton.setEnabled(false);
            renderList();
            return;
        }
        refresh();
    }

    private View buildContentView() {
        FrameLayout root = new FrameLayout(this);
        PageBackground.apply(root);

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setClipToPadding(false);
        root.addView(scroll, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));

        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setGravity(Gravity.CENTER_HORIZONTAL);
        int padding = AccountTheme.dp(this, AccountTheme.PAGE_PADDING_DP);
        column.setPadding(padding, AccountTheme.dp(this, 20f), padding, AccountTheme.dp(this, 22f));
        scroll.addView(column, new ScrollView.LayoutParams(
                ScrollView.LayoutParams.MATCH_PARENT, ScrollView.LayoutParams.WRAP_CONTENT));

        column.addView(AccountTheme.title(this, ProgressText.TITLE,
                AccountTheme.COLOR_TEXT_ON_DARK));
        column.addView(AccountTheme.subtitle(this, ProgressText.SUBTITLE,
                        AccountTheme.COLOR_TEXT_ON_DARK_MUTED),
                AccountTheme.block(this, 4f));

        LinearLayout card = buildCard();
        column.addView(card, cardParams());

        TextView author = AccountTheme.body(this, ProgressText.TITLE,
                AccountTheme.COLOR_TEXT_ON_DARK_MUTED, 11f);
        author.setGravity(Gravity.CENTER);
        column.addView(author, AccountTheme.block(this, 14f));
        return root;
    }

    private LinearLayout.LayoutParams cardParams() {
        int available = AccountTheme.screenWidth(this) - AccountTheme.dp(this, AccountTheme.PAGE_PADDING_DP * 2);
        int width = Math.min(available, AccountTheme.dp(this, AccountTheme.CARD_MAX_WIDTH_DP));
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(width,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        params.topMargin = AccountTheme.dp(this, 16f);
        return params;
    }

    private LinearLayout buildCard() {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        int horizontal = AccountTheme.dp(this, 18f);
        int vertical = AccountTheme.dp(this, 16f);
        card.setPadding(horizontal, vertical, horizontal, vertical);
        card.setBackground(AccountTheme.panelBackground(this));

        LinearLayout tabRow = new LinearLayout(this);
        tabRow.setOrientation(LinearLayout.HORIZONTAL);
        tabAchievements = buildTab(ProgressText.TAB_ACHIEVEMENT, TAB_ACHIEVEMENT);
        tabDaily = buildTab(ProgressText.TAB_DAILY, TAB_DAILY);
        tabRow.addView(tabAchievements, weightedParams());
        tabRow.addView(tabDaily, weightedParams());
        card.addView(tabRow);

        summaryView = AccountTheme.body(this, "", AccountTheme.COLOR_TEXT_MUTED, 13f);
        summaryView.setLineSpacing(AccountTheme.dp(this, 2f), 1f);
        card.addView(summaryView, AccountTheme.block(this, 10f));

        noticeView = AccountTheme.notice(this);
        noticeBar = new NoticeBar(noticeView);
        card.addView(noticeView, AccountTheme.block(this, 10f));

        LinearLayout actionRow = new LinearLayout(this);
        actionRow.setOrientation(LinearLayout.HORIZONTAL);
        refreshButton = AccountTheme.primaryButton(this, ProgressText.ACTION_REFRESH);
        refreshButton.setOnClickListener(view -> refresh());
        filterButton = AccountTheme.outlineButton(this, ProgressText.FILTER_ALL,
                AccountTheme.COLOR_TEXT_MUTED);
        filterButton.setOnClickListener(view -> toggleFilter());
        actionRow.addView(refreshButton, new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f));
        LinearLayout.LayoutParams filterParams = new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f);
        filterParams.leftMargin = AccountTheme.dp(this, 10f);
        actionRow.addView(filterButton, filterParams);
        card.addView(actionRow, AccountTheme.block(this, 10f));

        listContainer = new LinearLayout(this);
        listContainer.setOrientation(LinearLayout.VERTICAL);
        card.addView(listContainer, AccountTheme.block(this, 6f));
        return card;
    }

    private TextView buildTab(String text, final int targetTab) {
        TextView tabView = new TextView(this);
        tabView.setText(text);
        tabView.setTextSize(TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_BODY);
        tabView.setGravity(Gravity.CENTER);
        tabView.setPadding(0, AccountTheme.dp(this, 9f), 0, AccountTheme.dp(this, 9f));
        tabView.setClickable(true);
        tabView.setFocusable(true);
        tabView.setOnClickListener(view -> {
            if (tab == targetTab) {
                return;
            }
            tab = targetTab;
            applyTab();
            renderList();
        });
        return tabView;
    }

    private LinearLayout.LayoutParams weightedParams() {
        return new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
    }

    /* 页签选中态与文案同步, 两个页签共用同一套样式. */
    private void applyTab() {
        boolean achievementsTab = tab == TAB_ACHIEVEMENT;
        styleTab(tabAchievements, achievementsTab);
        styleTab(tabDaily, !achievementsTab);
        if (busy) {
            return;
        }
        updateSummary();
    }

    private void styleTab(TextView tabView, boolean selected) {
        tabView.setTextColor(selected ? AccountTheme.COLOR_TEXT : AccountTheme.COLOR_TEXT_MUTED);
        tabView.setTypeface(selected ? Typeface.DEFAULT_BOLD : Typeface.DEFAULT);
        tabView.setBackground(AccountTheme.tabBackground(this, selected));
    }

    private void loadCache() {
        String achievementsJson = AchievementCache.loadAchievements(this);
        String dailyJson = AchievementCache.loadDaily(this);
        if (!achievementsJson.isEmpty()) {
            achievements = AchievementBoard.parse(parseOrNull(achievementsJson));
        }
        if (!dailyJson.isEmpty()) {
            daily = DailyBoard.parse(parseOrNull(dailyJson));
        }
        if (achievements != null || daily != null) {
            showNotice(ProgressText.NOTICE_REFRESHED, AccountTheme.COLOR_TEXT_MUTED);
        }
        renderList();
    }

    /* 缓存文本损坏时返回空对象, 界面按无数据处理, 不让解析异常打断页面. */
    private static JSONObject parseOrNull(String text) {
        try {
            return new JSONObject(text);
        } catch (Exception error) {
            return null;
        }
    }

    private void refresh() {
        if (busy || session == null || !session.isUsable()) {
            return;
        }
        setBusy(true);
        showNotice(ProgressText.NOTICE_LOADING, AccountTheme.COLOR_TEXT_MUTED);
        sync.loadAchievements(session, new AchievementSync.Handler<AchievementBoard>() {
            @Override
            public void onSuccess(AchievementBoard value) {
                if (isFinishing()) {
                    return;
                }
                achievements = value;
                sync.loadDaily(session, new AchievementSync.Handler<DailyBoard>() {
                    @Override
                    public void onSuccess(DailyBoard dailyValue) {
                        if (isFinishing()) {
                            return;
                        }
                        daily = dailyValue;
                        setBusy(false);
                        clearNotice();
                        updateSummary();
                        renderList();
                    }

                    @Override
                    public void onFailure(ApiException error) {
                        if (isFinishing()) {
                            return;
                        }
                        setBusy(false);
                        handleFailure(error);
                        renderList();
                    }
                });
            }

            @Override
            public void onFailure(ApiException error) {
                if (isFinishing()) {
                    return;
                }
                setBusy(false);
                handleFailure(error);
                renderList();
            }
        });
    }

    private void handleFailure(ApiException error) {
        if (error != null && error.isSessionInvalid()) {
            SessionStore.clear(this);
            session = null;
            refreshButton.setEnabled(false);
            showNotice(ProgressText.NOTICE_SESSION, AccountTheme.COLOR_ERROR);
            return;
        }
        showNotice(ErrorMessages.messageFor(this, error), AccountTheme.COLOR_ERROR);
    }

    private void setBusy(boolean value) {
        busy = value;
        refreshButton.setEnabled(!value);
        refreshButton.setText(value ? ProgressText.ACTION_REFRESH : ProgressText.ACTION_REFRESH);
        filterButton.setEnabled(!value);
    }

    private void toggleFilter() {
        onlyUnfinished = !onlyUnfinished;
        filterButton.setText(onlyUnfinished ? ProgressText.STATUS_DOING : ProgressText.FILTER_ALL);
        renderList();
    }

    private void updateSummary() {
        if (tab == TAB_ACHIEVEMENT) {
            if (achievements == null) {
                summaryView.setText(ProgressText.EMPTY);
                return;
            }
            summaryView.setText(String.format(ProgressText.SUMMARY_FORMAT, achievements.completed,
                    achievements.total, achievements.claimed, achievements.coins));
            return;
        }
        if (daily == null) {
            summaryView.setText(ProgressText.EMPTY);
            return;
        }
        String dateText = daily.date.isEmpty() ? "今日" : daily.date;
        summaryView.setText(dateText + " · 可领取 " + daily.claimableCount() + " 条");
    }

    private void renderList() {
        if (listContainer == null) {
            return;
        }
        listContainer.removeAllViews();
        if (tab == TAB_ACHIEVEMENT) {
            renderAchievements();
        } else {
            renderDaily();
        }
    }

    private void renderAchievements() {
        if (achievements == null || achievements.items.isEmpty()) {
            listContainer.addView(emptyHint());
            return;
        }
        int shown = 0;
        for (AchievementBoard.Item item : achievements.items) {
            if (onlyUnfinished && item.completed) {
                continue;
            }
            shown++;
            String description = item.description;
            String category = ProgressText.categoryName(item.category);
            if (!category.isEmpty()) {
                description = description.isEmpty() ? category : description + " · " + category;
            }
            String state = item.completed ? (item.claimed ? "已领取" : "已完成") : "未完成";
            String action = item.completed && !item.claimed ? ProgressText.ACTION_REFRESH : "";
            ProgressRowView row = new ProgressRowView(this);
            row.bind(item.name, description, item.progress, item.safeTarget(), item.completed, state,
                    action, item.completed && !item.claimed && !busy,
                    item.id > 0 ? () -> claimAchievement(item) : null);
            listContainer.addView(row);
        }
        if (shown == 0) {
            listContainer.addView(emptyHint());
        }
    }

    private void renderDaily() {
        if (daily == null || daily.tasks.isEmpty()) {
            listContainer.addView(emptyHint());
            return;
        }
        for (DailyBoard.Task task : daily.tasks) {
            String unit = ProgressText.kindTitle(task.kind);
            String state = task.completed ? (task.claimed ? "已领取" : "可领取") : "进行中";
            String action = task.completed && !task.claimed ? ProgressText.ACTION_REFRESH : "";
            ProgressRowView row = new ProgressRowView(this);
            row.bind(task.name, String.format(ProgressText.REWARD_FORMAT, task.coinReward), task.progress,
                    task.target, task.completed, state, action,
                    task.completed && !task.claimed && !busy, () -> claimDaily(task));
            listContainer.addView(row);
            if (!unit.isEmpty()) {
                state = state + " · " + unit;
            }
        }
    }

    private TextView emptyHint() {
        TextView hint = AccountTheme.body(this, ProgressText.EMPTY,
                AccountTheme.COLOR_TEXT_FAINT, 13f);
        hint.setGravity(Gravity.CENTER);
        hint.setPadding(0, AccountTheme.dp(this, 12f), 0, AccountTheme.dp(this, 12f));
        return hint;
    }

    private void claimAchievement(final AchievementBoard.Item item) {
        if (busy || session == null) {
            return;
        }
        final int achievementId = item.id;
        sync.claimAchievement(session, achievementId, new AchievementSync.Handler<JSONObject>() {
            @Override
            public void onSuccess(JSONObject value) {
                if (isFinishing()) {
                    return;
                }
                int coins = value == null ? 0 : value.optInt("coins",
                        value.optInt("awarded", item.coinReward));
                showNotice(String.format(ProgressText.EARNED_FORMAT, coins),
                        AccountTheme.COLOR_SUCCESS);
                refresh();
            }

            @Override
            public void onFailure(ApiException error) {
                if (isFinishing()) {
                    return;
                }
                handleClaimFailure(error);
            }
        });
    }

    private void claimDaily(final DailyBoard.Task task) {
        if (busy || session == null) {
            return;
        }
        sync.claimDaily(session, task.slot, new AchievementSync.Handler<JSONObject>() {
            @Override
            public void onSuccess(JSONObject value) {
                if (isFinishing()) {
                    return;
                }
                int coins = value == null ? 0 : value.optInt("coins",
                        value.optInt("reward", task.coinReward));
                showNotice(String.format(ProgressText.EARNED_FORMAT, coins),
                        AccountTheme.COLOR_SUCCESS);
                refresh();
            }

            @Override
            public void onFailure(ApiException error) {
                if (isFinishing()) {
                    return;
                }
                handleClaimFailure(error);
            }
        });
    }

    /*
     * 领取失败的处理: 服务端未开放领取接口时说明奖励已自动发放,
     * 其余错误照常显示服务端给出的原因.
     */
    private void handleClaimFailure(ApiException error) {
        if (error != null && "NOT_FOUND".equals(error.code())) {
            showNotice(ProgressText.EMPTY, AccountTheme.COLOR_TEXT_MUTED);
            refresh();
            return;
        }
        handleFailure(error);
    }

    private void showNotice(CharSequence message, int color) {
        noticeBar.show(message, color);
    }

    private void clearNotice() {
        noticeBar.clear();
        noticeView.setVisibility(View.GONE);
    }
}