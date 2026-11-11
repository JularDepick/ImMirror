package com.julardepick.immirror.progress;

import android.app.Activity;
import android.graphics.Typeface;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.julardepick.immirror.account.AccountTheme;
import com.julardepick.immirror.account.ApiException;
import com.julardepick.immirror.account.NoticeBar;
import com.julardepick.immirror.account.Session;
import com.julardepick.immirror.account.SessionStore;

import java.util.ArrayList;
import java.util.List;

/*
 * 成就与任务界面.
 *
 * 从账号面板进入, 分两个标签页: 成就按分组筛选并逐条显示进度条与完成状态,
 * 每日任务显示服务端按当日日期确定性地抽出的三条任务.
 * 数据全部来自服务端 (成就定义与进度都在服务端合并下发), 本页只读不写,
 * 上报由原生进度系统在游戏侧累积后提交, 因此进入本页只发查询请求.
 * 提示统一走页面内提示条, 不使用 Toast 与系统弹窗.
 */
public final class ProgressActivity extends Activity {

    private static final String TAG = "ImMirrorProgressUi";

    /* 标签页下标: 成就与每日任务. */
    private static final int TAB_ACHIEVEMENT = 0;
    private static final int TAB_DAILY = 1;

    private ProgressApi api;
    private Session session;

    private TextView summaryView;
    private TextView balanceView;
    private TextView tabAchievementView;
    private TextView tabDailyView;
    private HorizontalScrollView filterScroll;
    private LinearLayout chipsHost;
    private LinearLayout listHost;
    private NoticeBar noticeBar;

    private ProgressBoard board = ProgressBoard.empty();
    private int tab = TAB_ACHIEVEMENT;
    private int filterCategory;
    private boolean loading;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(buildPage());
        api = new ProgressApi(this);
        session = SessionStore.load(this);
        Log.i(TAG, "progress ui opened, session=" + (session != null && session.isUsable()));
        refresh();
    }

    @Override
    protected void onDestroy() {
        if (noticeBar != null) {
            noticeBar.clear();
        }
        super.onDestroy();
    }

    /* 读取成就总览与当日任务; 未登录时直接显示空态并提示登录. */
    private void refresh() {
        if (session == null || !session.isUsable()) {
            loading = false;
            board = ProgressBoard.empty();
            render();
            showNotice(ProgressText.LOGIN_REQUIRED, AccountTheme.COLOR_ERROR);
            return;
        }
        loading = true;
        summaryView.setText(ProgressText.LOADING);
        api.fetchBoard(session, new ProgressApi.Callback() {
            @Override
            public void onSuccess(ProgressBoard value) {
                loading = false;
                board = value;
                render();
                Log.i(TAG, "board loaded, achievements=" + value.achievements().size()
                        + " daily=" + value.dailyTasks().size());
            }

            @Override
            public void onFailure(ApiException error) {
                loading = false;
                board = ProgressBoard.empty();
                render();
                String message = error != null && error.isSessionInvalid()
                        ? ProgressText.NOTICE_SESSION : ProgressText.NOTICE_NETWORK;
                showNotice(message, AccountTheme.COLOR_ERROR);
                Log.w(TAG, "board failed, code=" + (error == null ? "" : error.code()));
            }
        });
    }

    /* 按当前标签页, 筛选与数据重建列表; 头部汇总与标签选中态一并刷新. */
    private void render() {
        summaryView.setText(getString(com.julardepick.immirror.R.string.app_name).isEmpty()
                ? "" : String.format(ProgressText.SUMMARY_FORMAT, board.completedCount(), board.totalCount(),
                        board.dailyCompleted(), board.dailyTotal()));
        summaryView.setText(String.format(ProgressText.SUMMARY_FORMAT, board.completedCount(),
                board.totalCount(), board.dailyCompleted(), board.dailyTotal()));
        balanceView.setText(String.format(ProgressText.BALANCE_FORMAT, board.balance()));

        styleTab(tabAchievementView, tab == TAB_ACHIEVEMENT);
        styleTab(tabDailyView, tab == TAB_DAILY);
        filterScroll.setVisibility(tab == TAB_ACHIEVEMENT ? View.VISIBLE : View.GONE);
        rebuildChips();
        rebuildList();
    }

    private void rebuildChips() {
        chipsHost.removeAllViews();
        if (tab != TAB_ACHIEVEMENT) {
            return;
        }
        chipsHost.addView(buildChip(0, ProgressText.FILTER_ALL, board.achievements().size()));
        for (Integer category : board.categories()) {
            int value = category == null ? 0 : category;
            String name = board.categoryName(value);
            chipsHost.addView(buildChip(value, name, board.countOf(value)));
        }
    }

    private void rebuildList() {
        listHost.removeAllViews();
        if (tab == TAB_DAILY) {
            buildDailyList();
            return;
        }
        buildAchievementList();
    }

    /* 成就列表: 全部筛选时按分类分段显示小标题, 选定分类时只显示该段. */
    private void buildAchievementList() {
        if (board.achievements().isEmpty()) {
            listHost.addView(buildEmpty());
            return;
        }
        if (filterCategory != 0) {
            for (AchievementEntry entry : board.achievementsOf(filterCategory)) {
                listHost.addView(buildAchievementCard(entry), cardParams(10f));
            }
            return;
        }
        boolean first = true;
        for (Integer category : board.categories()) {
            int value = category == null ? 0 : category;
            listHost.addView(buildSectionHeader(board.categoryName(value), board.countOf(value)),
                    sectionParams(first));
            first = false;
            for (AchievementEntry entry : board.achievementsOf(value)) {
                listHost.addView(buildAchievementCard(entry), cardParams(8f));
            }
        }
    }

    private void buildDailyList() {
        if (board.dailyTasks().isEmpty()) {
            listHost.addView(buildEmpty());
            return;
        }
        listHost.addView(buildSectionHeader(String.format(ProgressText.DAILY_DATE_FORMAT, board.dailyDate()),
                board.dailyTotal()), sectionParams(true));
        for (DailyTaskEntry entry : board.dailyTasks()) {
            listHost.addView(buildDailyCard(entry), cardParams(8f));
        }
    }

    private LinearLayout buildAchievementCard(AchievementEntry entry) {
        LinearLayout card = newCard();

        LinearLayout titleRow = newRow();
        TextView name = AccountTheme.body(this, entry.name(), AccountTheme.COLOR_TEXT, 15f);
        name.setTypeface(name.getTypeface(), Typeface.BOLD);
        titleRow.addView(name, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        titleRow.addView(buildStatus(entry.completed()));
        card.addView(titleRow, AccountTheme.block(this, 0f));

        if (!entry.description().isEmpty()) {
            TextView desc = AccountTheme.body(this, entry.description(), AccountTheme.COLOR_TEXT_MUTED, 11f);
            desc.setLineSpacing(AccountTheme.dp(this, 2f), 1f);
            card.addView(desc, AccountTheme.block(this, 4f));
        }

        ProgressBarView bar = new ProgressBarView(this);
        bar.setProgress(entry.ratio(), entry.completed());
        card.addView(bar, AccountTheme.block(this, 8f));

        LinearLayout footRow = newRow();
        String progress = String.format(ProgressText.PROGRESS_FORMAT, entry.value(), entry.target(), entry.unit());
        String label = entry.kindTitle().isEmpty() ? progress : entry.kindTitle() + " · " + progress;
        footRow.addView(AccountTheme.body(this, label, AccountTheme.COLOR_TEXT_MUTED, 11f),
                new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        footRow.addView(AccountTheme.body(this, String.format(ProgressText.REWARD_FORMAT, entry.coinReward()),
                AccountTheme.COLOR_ACCENT, 11f));
        card.addView(footRow, AccountTheme.block(this, 6f));
        return card;
    }

    private LinearLayout buildDailyCard(DailyTaskEntry entry) {
        LinearLayout card = newCard();

        LinearLayout titleRow = newRow();
        TextView name = AccountTheme.body(this, entry.name(), AccountTheme.COLOR_TEXT, 15f);
        name.setTypeface(name.getTypeface(), Typeface.BOLD);
        titleRow.addView(name, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        titleRow.addView(buildStatus(entry.completed()));
        card.addView(titleRow, AccountTheme.block(this, 0f));

        ProgressBarView bar = new ProgressBarView(this);
        bar.setProgress(entry.ratio(), entry.completed());
        card.addView(bar, AccountTheme.block(this, 8f));

        LinearLayout footRow = newRow();
        String progress = String.format(ProgressText.PROGRESS_FORMAT, entry.value(), entry.target(), entry.unit());
        String label = entry.kindTitle().isEmpty() ? progress : entry.kindTitle() + " · " + progress;
        footRow.addView(AccountTheme.body(this, label, AccountTheme.COLOR_TEXT_MUTED, 11f),
                new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        footRow.addView(AccountTheme.body(this, String.format(ProgressText.REWARD_FORMAT, entry.coinReward()),
                AccountTheme.COLOR_ACCENT, 11f));
        card.addView(footRow, AccountTheme.block(this, 6f));
        return card;
    }

    private LinearLayout newCard() {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        int horizontal = AccountTheme.dp(this, 16f);
        int vertical = AccountTheme.dp(this, 12f);
        card.setPadding(horizontal, vertical, horizontal, vertical);
        card.setBackground(AccountTheme.panelBackground(this));
        return card;
    }

    private LinearLayout newRow() {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        return row;
    }

    /* 完成状态徽标: 完成取成功色, 进行中取淡色, 只有文字不加底色, 与卡片风格一致. */
    private TextView buildStatus(boolean completed) {
        TextView view = AccountTheme.body(this, completed ? ProgressText.STATUS_DONE : ProgressText.STATUS_DOING,
                completed ? AccountTheme.COLOR_SUCCESS : AccountTheme.COLOR_TEXT_FAINT, 11f);
        view.setTypeface(view.getTypeface(), Typeface.BOLD);
        return view;
    }

    private TextView buildSectionHeader(String title, int count) {
        String text = count > 0 ? String.format(ProgressText.CATEGORY_COUNT_FORMAT, title, count) : title;
        TextView header = AccountTheme.body(this, text, AccountTheme.COLOR_TEXT_ON_DARK_MUTED, 13f);
        header.setTypeface(header.getTypeface(), Typeface.BOLD);
        header.setGravity(Gravity.CENTER);
        return header;
    }

    private TextView buildEmpty() {
        TextView empty = AccountTheme.body(this, ProgressText.EMPTY, AccountTheme.COLOR_TEXT_ON_DARK_MUTED, 12f);
        empty.setGravity(Gravity.CENTER);
        empty.setPadding(0, AccountTheme.dp(this, 24f), 0, AccountTheme.dp(this, 24f));
        return empty;
    }

    /* 筛选标签: 选中态换近白底的选中态素材, 未选中用常态素材. */
    private TextView buildChip(int category, String name, int count) {
        boolean selected = filterCategory == category;
        String text = count > 0 ? String.format(ProgressText.CATEGORY_COUNT_FORMAT, name, count) : name;
        TextView chip = new TextView(this);
        chip.setText(text);
        chip.setTextSize(android.util.TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_SUB);
        chip.setTextColor(selected ? AccountTheme.COLOR_TEXT : AccountTheme.COLOR_TEXT_ON_DARK_MUTED);
        chip.setGravity(Gravity.CENTER);
        chip.setBackground(AccountTheme.tabBackground(this, selected));
        int horizontal = AccountTheme.dp(this, 12f);
        int vertical = AccountTheme.dp(this, 6f);
        chip.setPadding(horizontal, vertical, horizontal, vertical);
        chip.setClickable(true);
        chip.setFocusable(true);
        chip.setOnClickListener(view -> {
            filterCategory = category;
            render();
        });
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.rightMargin = AccountTheme.dp(this, 8f);
        chip.setLayoutParams(params);
        return chip;
    }

    private void styleTab(TextView view, boolean selected) {
        view.setBackground(AccountTheme.tabBackground(this, selected));
        view.setTextColor(selected ? AccountTheme.COLOR_TEXT : AccountTheme.COLOR_TEXT_ON_DARK_MUTED);
        view.setTypeface(view.getTypeface(), selected ? Typeface.BOLD : Typeface.NORMAL);
    }

    private void showNotice(CharSequence message, int color) {
        if (noticeBar != null) {
            noticeBar.show(message, color);
        }
    }

    private View buildPage() {
        FrameLayout root = new FrameLayout(this);
        root.setBackgroundColor(AccountTheme.COLOR_PAGE);

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setClipToPadding(false);
        /* 底部留出提示条的高度, 滚到末尾时最后一张卡片不会被提示条盖住. */
        scroll.setPadding(0, 0, 0, AccountTheme.dp(this, 84f));

        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setGravity(Gravity.CENTER_HORIZONTAL);
        int padding = AccountTheme.dp(this, AccountTheme.PAGE_PADDING_DP);
        column.setPadding(padding, AccountTheme.dp(this, 20f), padding, AccountTheme.dp(this, 18f));
        scroll.addView(column, new ScrollView.LayoutParams(ScrollView.LayoutParams.MATCH_PARENT,
                ScrollView.LayoutParams.WRAP_CONTENT));
        root.addView(scroll, new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT));

        column.addView(AccountTheme.title(this, ProgressText.TITLE));
        column.addView(AccountTheme.subtitle(this, ProgressText.SUBTITLE), AccountTheme.block(this, 4f));

        summaryView = AccountTheme.body(this, "", AccountTheme.COLOR_TEXT_ON_DARK_MUTED, 12f);
        summaryView.setGravity(Gravity.CENTER);
        column.addView(summaryView, AccountTheme.block(this, 8f));

        balanceView = AccountTheme.body(this, "", AccountTheme.COLOR_ACCENT, 13f);
        balanceView.setGravity(Gravity.CENTER);
        balanceView.setTypeface(balanceView.getTypeface(), Typeface.BOLD);
        column.addView(balanceView, AccountTheme.block(this, 2f));

        column.addView(buildTabRow(), AccountTheme.block(this, 10f));

        chipsHost = new LinearLayout(this);
        chipsHost.setOrientation(LinearLayout.HORIZONTAL);
        chipsHost.setGravity(Gravity.CENTER_VERTICAL);
        filterScroll = new HorizontalScrollView(this);
        filterScroll.setHorizontalScrollBarEnabled(false);
        filterScroll.setClipToPadding(false);
        filterScroll.addView(chipsHost, new HorizontalScrollView.LayoutParams(
                HorizontalScrollView.LayoutParams.WRAP_CONTENT, HorizontalScrollView.LayoutParams.WRAP_CONTENT));
        column.addView(filterScroll, AccountTheme.block(this, 8f));

        listHost = new LinearLayout(this);
        listHost.setOrientation(LinearLayout.VERTICAL);
        listHost.setGravity(Gravity.CENTER_HORIZONTAL);
        column.addView(listHost, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));

        column.addView(buildActionRow(), cardParams(16f));

        TextView author = AccountTheme.body(this, ProgressText.TITLE,
                AccountTheme.COLOR_TEXT_FAINT, 11f);
        author.setGravity(Gravity.CENTER);
        column.addView(author, AccountTheme.block(this, 14f));

        TextView noticeView = AccountTheme.notice(this);
        noticeBar = new NoticeBar(noticeView);
        FrameLayout.LayoutParams noticeParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM);
        int margin = AccountTheme.dp(this, 16f);
        noticeParams.setMargins(margin, 0, margin, margin);
        root.addView(noticeView, noticeParams);
        return root;
    }

    private LinearLayout buildTabRow() {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);

        tabAchievementView = buildTab(ProgressText.TAB_ACHIEVEMENT, TAB_ACHIEVEMENT);
        tabDailyView = buildTab(ProgressText.TAB_DAILY, TAB_DAILY);
        row.addView(tabAchievementView, new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f));
        LinearLayout.LayoutParams dailyParams = new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f);
        dailyParams.leftMargin = AccountTheme.dp(this, 10f);
        row.addView(tabDailyView, dailyParams);
        return row;
    }

    private TextView buildTab(String text, int target) {
        TextView tabView = new TextView(this);
        tabView.setText(text);
        tabView.setTextSize(android.util.TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_BODY);
        tabView.setGravity(Gravity.CENTER);
        tabView.setClickable(true);
        tabView.setFocusable(true);
        tabView.setOnClickListener(view -> {
            if (tab != target) {
                tab = target;
                filterCategory = 0;
                render();
            }
        });
        return tabView;
    }

    private LinearLayout buildActionRow() {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);

        Button back = AccountTheme.outlineButton(this, ProgressText.BACK, AccountTheme.COLOR_ACCENT);
        back.setOnClickListener(view -> finish());
        row.addView(back, new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f));

        Button refresh = AccountTheme.outlineButton(this, ProgressText.REFRESH, AccountTheme.COLOR_ACCENT);
        refresh.setOnClickListener(view -> {
            if (!loading) {
                refresh();
            }
        });
        LinearLayout.LayoutParams refreshParams = new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f);
        refreshParams.leftMargin = AccountTheme.dp(this, 10f);
        row.addView(refresh, refreshParams);
        return row;
    }

    private LinearLayout.LayoutParams cardParams(float topMarginDp) {
        int available = AccountTheme.screenWidth(this)
                - AccountTheme.dp(this, AccountTheme.PAGE_PADDING_DP * 2);
        int width = Math.min(available, AccountTheme.dp(this, AccountTheme.CARD_MAX_WIDTH_DP));
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(width,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        params.topMargin = AccountTheme.dp(this, topMarginDp);
        return params;
    }

    private LinearLayout.LayoutParams sectionParams(boolean first) {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.topMargin = AccountTheme.dp(this, first ? 6f : 16f);
        params.bottomMargin = AccountTheme.dp(this, 2f);
        return params;
    }
}
