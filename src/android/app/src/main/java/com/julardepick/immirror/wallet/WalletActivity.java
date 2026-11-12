package com.julardepick.immirror.wallet;

import android.app.Activity;
import android.graphics.Typeface;
import android.os.Bundle;
import android.util.Log;
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

import java.util.List;
import java.util.Locale;

/*
 * 钱包界面.
 *
 * 从账号面板进入, 页面从上到下依次为余额卡片, 本月签到日历, 签到卡片,
 * 流水列表与作者页脚; 数据来自 GET /api/v1/wallet 与 GET /api/v1/wallet/checkin,
 * 签到动作走 POST /api/v1/wallet/checkin. 服务端暂未逐日下发签到记录,
 * 日历高亮用流水里的事由为 checkin 的条目反推, 服务端补充字段后自动切换.
 * 所有提示都显示在页面内的提示条里, 不使用 Toast 与系统弹窗.
 */
public final class WalletActivity extends Activity {

    private static final String TAG = "ImMirrorWallet";

    private WalletApi api;
    private Session session;

    private TextView balanceView;
    private TextView balanceUpdatedView;
    private TextView calendarMonthView;
    private CheckinCalendarView calendarView;
    private CheckinCardView checkinCard;
    private LinearLayout ledgerHost;
    private TextView ledgerEmptyView;
    private NoticeBar noticeBar;

    private WalletBoard board = WalletBoard.empty();
    private boolean loading;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        api = new WalletApi(this);
        session = SessionStore.load(this);
        setContentView(buildPage());
        Log.i(TAG, "wallet ui opened, session=" + (session != null && session.isUsable()));
        refresh();
    }

    @Override
    protected void onDestroy() {
        if (noticeBar != null) {
            noticeBar.clear();
        }
        super.onDestroy();
    }

    /* 拉取余额流水与签到状态; 未登录直接显示空态并提示. */
    private void refresh() {
        if (session == null || !session.isUsable()) {
            loading = false;
            board = WalletBoard.empty();
            render();
            showNotice(getString(R.string.wallet_notice_login_required), AccountTheme.COLOR_ERROR);
            return;
        }
        loading = true;
        balanceView.setText(getString(R.string.wallet_balance_loading));
        api.fetchOverview(session, new WalletApi.Callback() {
            @Override
            public void onSuccess(WalletBoard value) {
                loading = false;
                board = value;
                render();
                Log.i(TAG, "wallet loaded, ledger=" + value.ledger().size()
                        + " monthCount=" + value.checkin().monthCount());
            }

            @Override
            public void onFailure(ApiException error) {
                loading = false;
                board = WalletBoard.empty();
                render();
                if (error != null && error.isSessionInvalid()) {
                    SessionStore.clear(WalletActivity.this);
                    showNotice(getString(R.string.launcher_session_invalid), AccountTheme.COLOR_ERROR);
                    return;
                }
                showNotice(ErrorMessages.messageFor(WalletActivity.this, error), AccountTheme.COLOR_ERROR);
                Log.w(TAG, "wallet failed, code=" + (error == null ? "" : error.code()));
            }
        });
    }

    /* 提交签到; 已签过的重复提交按服务端提示切换为已签状态. */
    private void submitCheckin() {
        if (loading || session == null || !session.isUsable()) {
            return;
        }
        checkinCard.setBusy(true);
        api.checkin(session, new WalletApi.ResultCallback<CheckinResult>() {
            @Override
            public void onSuccess(CheckinResult value) {
                if (isFinishing()) {
                    return;
                }
                applyCheckinResult(value);
            }

            @Override
            public void onFailure(ApiException error) {
                if (isFinishing()) {
                    return;
                }
                checkinCard.setBusy(false);
                if (error != null && "WALLET_ALREADY_CHECKED_IN".equals(error.code())) {
                    /* 已签提示按服务端约定不作为失败展示, 直接刷新成已签状态. */
                    showNotice(getString(R.string.wallet_notice_already_checked),
                            AccountTheme.COLOR_TEXT_MUTED);
                    refresh();
                    return;
                }
                showNotice(ErrorMessages.messageFor(WalletActivity.this, error), AccountTheme.COLOR_ERROR);
                Log.w(TAG, "checkin failed, code=" + (error == null ? "" : error.code()));
            }
        });
    }

    /* 签到成功: 先用响应里的新余额与次数重绘, 再拉一次流水与签到状态补齐. */
    private void applyCheckinResult(CheckinResult value) {
        board = WalletBoard.withCheckinResult(board, value);
        render();
        checkinCard.setBusy(false);
        if (value.milestoneReward() > 0L) {
            showNotice(getString(R.string.wallet_notice_checkin_milestone, value.reward(),
                    value.milestoneReward()), AccountTheme.COLOR_SUCCESS);
        } else {
            showNotice(getString(R.string.wallet_notice_checkin_ok, value.reward()),
                    AccountTheme.COLOR_SUCCESS);
        }
        refresh();
    }

    /* 整体重绘: 余额, 日历, 签到卡片与流水列表. */
    private void render() {
        balanceView.setText(getString(R.string.wallet_balance_format, board.balance()));
        balanceUpdatedView.setText(updatedText());
        calendarMonthView.setText(monthText());
        calendarView.render(board.checkin().month(), board.signedDates(),
                board.checkin().signedToday(), todayKey());
        checkinCard.render(board.checkin());
        rebuildLedger();
    }

    /* 余额更新时间: 服务端未回传时不展示该行. */
    private String updatedText() {
        if (board.updatedAt() <= 0L) {
            balanceUpdatedView.setVisibility(View.GONE);
            return "";
        }
        balanceUpdatedView.setVisibility(View.VISIBLE);
        return getString(R.string.wallet_balance_updated_format, formatTime(board.updatedAt()));
    }

    private String monthText() {
        String month = board.checkin().month();
        if (month.length() != 7 || month.charAt(4) != '-') {
            return "";
        }
        try {
            int year = Integer.parseInt(month.substring(0, 4));
            int mon = Integer.parseInt(month.substring(5));
            return getString(R.string.wallet_calendar_month_format, year, mon);
        } catch (NumberFormatException error) {
            return "";
        }
    }

    /* 今日日期键, 与服务端格式一致; 时区按本机, 仅用于日历标注. */
    private static String todayKey() {
        java.util.Calendar now = java.util.Calendar.getInstance();
        return String.format(Locale.US, "%04d-%02d-%02d", now.get(java.util.Calendar.YEAR),
                now.get(java.util.Calendar.MONTH) + 1, now.get(java.util.Calendar.DAY_OF_MONTH));
    }

    /* 重建流水列表: 有数据显示逐行, 否则显示空态. */
    private void rebuildLedger() {
        ledgerHost.removeAllViews();
        List<LedgerEntry> entries = board.ledger();
        ledgerEmptyView.setVisibility(entries.isEmpty() ? View.VISIBLE : View.GONE);
        for (LedgerEntry entry : entries) {
            ledgerHost.addView(buildLedgerRow(entry));
        }
    }

    private LinearLayout buildLedgerRow(LedgerEntry entry) {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        int horizontal = AccountTheme.dp(this, 16f);
        int vertical = AccountTheme.dp(this, 4f);
        card.setPadding(horizontal, vertical, horizontal, vertical);
        card.setBackground(AccountTheme.panelBackground(this));
        LedgerRowView row = new LedgerRowView(this);
        row.bind(entry);
        card.addView(row);
        return card;
    }

    private static String formatTime(long createdAt) {
        return new java.text.SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.getDefault())
                .format(new java.util.Date(createdAt));
    }

    private void showNotice(CharSequence message, int color) {
        if (noticeBar != null) {
            noticeBar.show(message, color);
        }
    }

    private View buildPage() {
        FrameLayout root = new FrameLayout(this);
        PageBackground.apply(root);

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setClipToPadding(false);
        /* 底部留出提示条高度, 滚到末尾时最后一张卡片不被提示条盖住. */
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

        column.addView(AccountTheme.title(this, getString(R.string.wallet_title)));
        column.addView(AccountTheme.subtitle(this, getString(R.string.wallet_subtitle)),
                AccountTheme.block(this, 4f));

        column.addView(buildBalanceCard(), cardParams(14f));

        calendarMonthView = AccountTheme.body(this, "", AccountTheme.COLOR_TEXT_ON_DARK_MUTED, 13f);
        calendarMonthView.setTypeface(calendarMonthView.getTypeface(), Typeface.BOLD);
        calendarMonthView.setGravity(Gravity.CENTER);
        column.addView(calendarMonthView, cardParams(12f));

        calendarView = new CheckinCalendarView(this);
        column.addView(calendarView, cardParams(6f));

        checkinCard = new CheckinCardView(this);
        checkinCard.setCheckinListener(new CheckinCardView.CheckinListener() {
            @Override
            public void onCheckinClick() {
                submitCheckin();
            }
        });
        column.addView(checkinCard, cardParams(12f));

        column.addView(buildLedgerSection(), cardParams(14f));

        column.addView(buildActionRow(), cardParams(16f));

        TextView author = AccountTheme.body(this, AccountTheme.authorFooter(this),
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

    /* 余额卡片: 大号余额加更新时间, 更新时间缺省时隐藏. */
    private LinearLayout buildBalanceCard() {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setGravity(Gravity.CENTER);
        int padding = AccountTheme.dp(this, 18f);
        card.setPadding(padding, padding, padding, padding);
        card.setBackground(AccountTheme.panelBackground(this));

        card.addView(AccountTheme.label(this, getString(R.string.wallet_balance_header)));

        balanceView = AccountTheme.body(this, getString(R.string.wallet_balance_loading),
                AccountTheme.COLOR_ACCENT, 28f);
        balanceView.setTypeface(balanceView.getTypeface(), Typeface.BOLD);
        balanceView.setGravity(Gravity.CENTER);
        card.addView(balanceView, AccountTheme.block(this, 8f));

        balanceUpdatedView = AccountTheme.body(this, "", AccountTheme.COLOR_TEXT_FAINT, 11f);
        balanceUpdatedView.setGravity(Gravity.CENTER);
        card.addView(balanceUpdatedView, AccountTheme.block(this, 4f));
        return card;
    }

    private LinearLayout buildLedgerSection() {
        LinearLayout section = new LinearLayout(this);
        section.setOrientation(LinearLayout.VERTICAL);

        TextView header = AccountTheme.body(this,
                getString(R.string.wallet_ledger_header, board.ledger().size()),
                AccountTheme.COLOR_TEXT_ON_DARK_MUTED, 13f);
        header.setTypeface(header.getTypeface(), Typeface.BOLD);
        header.setGravity(Gravity.CENTER);
        section.addView(header);

        ledgerHost = new LinearLayout(this);
        ledgerHost.setOrientation(LinearLayout.VERTICAL);
        section.addView(ledgerHost, AccountTheme.block(this, 8f));

        ledgerEmptyView = AccountTheme.body(this, getString(R.string.wallet_ledger_empty),
                AccountTheme.COLOR_TEXT_ON_DARK_MUTED, 12f);
        ledgerEmptyView.setGravity(Gravity.CENTER);
        ledgerEmptyView.setPadding(0, AccountTheme.dp(this, 12f), 0, AccountTheme.dp(this, 12f));
        section.addView(ledgerEmptyView);
        return section;
    }

    private LinearLayout buildActionRow() {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);

        Button back = AccountTheme.outlineButton(this, getString(R.string.wallet_action_back),
                AccountTheme.COLOR_ACCENT);
        back.setOnClickListener(view -> finish());
        row.addView(back, new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f));

        Button reload = AccountTheme.outlineButton(this, getString(R.string.wallet_action_refresh),
                AccountTheme.COLOR_ACCENT);
        reload.setOnClickListener(view -> {
            if (!loading) {
                refresh();
            }
        });
        LinearLayout.LayoutParams reloadParams = new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f);
        reloadParams.leftMargin = AccountTheme.dp(this, 10f);
        row.addView(reload, reloadParams);
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
}
