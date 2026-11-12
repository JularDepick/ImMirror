package com.julardepick.immirror.settings;

import android.app.Activity;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
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
import com.julardepick.immirror.account.NoticeBar;
import com.julardepick.immirror.bridge.NativeBridge;
import com.julardepick.immirror.ui.MobilePage;
import com.julardepick.immirror.ui.PageBackground;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/*
 * 设置界面.
 *
 * 键名, 默认值与取值范围来自原生 describeSettings, 当前值来自 exportSettings,
 * 控件按取值范围推断生成, 不按每个键手写布局; 中文语义与分组来自 SettingsCatalog 的映射表.
 * 改动即时写入原生设置单例, 退出界面时统一落盘; 提示使用页面内提示条, 二次确认使用按钮变体,
 * 全程不使用 Toast 与系统弹窗. 页脚作者信息取自 AccountTheme 常量.
 */
public final class SettingsActivity extends Activity implements SettingControl.Listener {

    private static final String TAG = "ImMirrorSettingsUi";

    /* 恢复默认的二次确认等待时长, 超时回到初始按钮状态. */
    private static final long CONFIRM_TIMEOUT_MS = 3000L;

    private final Handler handler = new Handler(Looper.getMainLooper());
    private final Map<String, SettingControl> controls = new LinkedHashMap<>();

    private List<SettingsItem> items = new ArrayList<>();
    private LinearLayout groupsHost;
    private TextView summaryView;
    private NoticeBar noticeBar;
    private Button resetButton;

    private boolean resetPending;
    private boolean loaded;

    private final Runnable resetTimeout = new Runnable() {
        @Override
        public void run() {
            resetPending = false;
            if (resetButton != null) {
                resetButton.setText(R.string.settings_reset);
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(buildPage());
        loaded = NativeBridge.initSettings(getFilesDir().getAbsolutePath());
        Log.i(TAG, "settings ui opened, init=" + loaded + " path=" + getFilesDir().getAbsolutePath());
        reload();
    }

    @Override
    protected void onPause() {
        /*
         * 原生侧平时由主循环低频落盘, 设置界面打开时游戏可能处于后台,
         * 因此离开本页时主动写一次, 保证改动离开界面就落到设置文件.
         */
        NativeBridge.flushSettings();
        Log.i(TAG, "settings flushed on leave");
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        handler.removeCallbacks(resetTimeout);
        if (noticeBar != null) {
            noticeBar.clear();
        }
        NativeBridge.flushSettings();
        super.onDestroy();
    }

    @Override
    public void onValueEdited(SettingControl control, int value) {
        SettingsItem item = control.item();
        int applied = NativeBridge.setSetting(item.key(), value);
        if (applied < 0) {
            showNotice(getString(R.string.settings_notice_unknown_key), AccountTheme.COLOR_ERROR);
            return;
        }
        if (applied != value) {
            /* 原生侧会裁剪取值, 以落定值为准回写显示. */
            control.update(applied);
        }
        item.setValue(applied);
        Log.i(TAG, "setting changed, key=" + item.key() + " value=" + applied);
        showNotice(getString(R.string.settings_notice_saved_format, item.label(),
                describeValue(item, applied)), AccountTheme.COLOR_SUCCESS);
        if (SettingsCatalog.KEY_CLASSICAL.equals(item.key())
                || SettingsCatalog.KEY_ROCKER.equals(item.key())) {
            syncFromNative();
        }
    }

    private void reload() {
        String describe = NativeBridge.describeSettings();
        String export = NativeBridge.exportSettings();
        items = SettingsCatalog.merge(describe, export);
        Log.i(TAG, "settings loaded, items=" + items.size() + " exportBytes=" + export.length());
        if (items.isEmpty()) {
            summaryView.setText("");
            showNotice(getString(R.string.settings_notice_unavailable), AccountTheme.COLOR_ERROR);
            return;
        }
        buildGroups();
        updateSummary();
    }

    private void buildGroups() {
        groupsHost.removeAllViews();
        controls.clear();
        for (SettingsCatalog.Group group : SettingsCatalog.Group.values()) {
            List<SettingsItem> groupItems = new ArrayList<>();
            for (SettingsItem item : items) {
                if (item.group() == group) {
                    groupItems.add(item);
                }
            }
            if (groupItems.isEmpty()) {
                continue;
            }
            groupsHost.addView(buildGroupCard(group, groupItems), cardParams(16f));
        }
    }

    private LinearLayout buildGroupCard(SettingsCatalog.Group group, List<SettingsItem> groupItems) {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        int horizontal = AccountTheme.dp(this, 18f);
        int vertical = AccountTheme.dp(this, 14f);
        card.setPadding(horizontal, vertical, horizontal, vertical);
        card.setBackground(AccountTheme.panelBackground(this));

        LinearLayout titleRow = new LinearLayout(this);
        titleRow.setOrientation(LinearLayout.HORIZONTAL);
        titleRow.setGravity(Gravity.CENTER_VERTICAL);
        TextView title = AccountTheme.body(this, group.title(), AccountTheme.COLOR_ACCENT, 15f);
        title.setTypeface(title.getTypeface(), Typeface.BOLD);
        titleRow.addView(title);
        TextView count = AccountTheme.body(this,
                getString(R.string.settings_group_count_format, groupItems.size()),
                AccountTheme.COLOR_TEXT_FAINT, 11f);
        LinearLayout.LayoutParams countParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        countParams.leftMargin = AccountTheme.dp(this, 8f);
        titleRow.addView(count, countParams);
        card.addView(titleRow, AccountTheme.block(this, 2f));

        for (int index = 0; index < groupItems.size(); index++) {
            SettingsItem item = groupItems.get(index);
            if (index > 0) {
                card.addView(buildDivider());
            }
            SettingControl control = createControl(item);
            controls.put(item.key(), control);
            card.addView(control.view());
        }
        return card;
    }

    private SettingControl createControl(SettingsItem item) {
        switch (item.type()) {
            case SettingsItem.TYPE_TOGGLE:
                return new ToggleControl(this, item, this);
            case SettingsItem.TYPE_SLIDER:
                return new SliderControl(this, item, this);
            default:
                return new ChoiceControl(this, item, this);
        }
    }

    private View buildDivider() {
        View line = new View(this);
        line.setBackgroundColor((AccountTheme.COLOR_CARD_STROKE & 0x00FFFFFF) | 0x33000000);
        line.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                Math.max(1, AccountTheme.dp(this, 0.5f))));
        return line;
    }

    /* 关键帧之外的显示同步: 经典与摇杆互斥, 写入其中一项后另一项由原生侧改写. */
    private void syncFromNative() {
        String export = NativeBridge.exportSettings();
        if (export.isEmpty()) {
            return;
        }
        for (String rawLine : export.split("\n")) {
            String line = rawLine.trim();
            int split = line.indexOf('=');
            if (split <= 0) {
                continue;
            }
            SettingControl control = controls.get(line.substring(0, split).trim());
            if (control == null) {
                continue;
            }
            int value = parse(line.substring(split + 1), control.item().value());
            if (control.item().value() != value) {
                control.update(value);
            }
        }
    }

    private void requestReset() {
        if (!resetPending) {
            resetPending = true;
            resetButton.setText(R.string.settings_reset_confirm);
            handler.postDelayed(resetTimeout, CONFIRM_TIMEOUT_MS);
            return;
        }
        handler.removeCallbacks(resetTimeout);
        resetPending = false;
        resetButton.setText(R.string.settings_reset);
        boolean ok = NativeBridge.resetSetting("");
        NativeBridge.flushSettings();
        syncFromNative();
        Log.i(TAG, "settings reset all, applied=" + ok);
        showNotice(getString(R.string.settings_notice_reset), AccountTheme.COLOR_ACCENT);
    }

    /* 提示条由 AccountTheme 的提示条工厂生成, 动效与其它页面共用 NoticeBar. */
    private void showNotice(CharSequence message, int color) {
        if (noticeBar != null) {
            noticeBar.show(message, color);
        }
    }

    private String describeValue(SettingsItem item, int value) {
        if (item.type() == SettingsItem.TYPE_TOGGLE) {
            return getString(value != 0 ? R.string.settings_value_on : R.string.settings_value_off);
        }
        if (item.type() == SettingsItem.TYPE_CHOICE) {
            int index = item.levelIndex(value);
            if (index >= 0) {
                return getString(R.string.settings_choice_format, item.levelLabelAt(index), value);
            }
        }
        return String.valueOf(value);
    }

    private void updateSummary() {
        int wired = 0;
        for (SettingsItem item : items) {
            if (item.wired()) {
                wired++;
            }
        }
        summaryView.setText(getString(R.string.settings_summary_format, items.size(), wired,
                items.size() - wired));
    }

    private int parse(String text, int fallback) {
        try {
            return Integer.parseInt(text.trim());
        } catch (NumberFormatException error) {
            return fallback;
        }
    }

    private View buildPage() {
        /* 面板内纵向内容: 说明行, 分组设置, 底部操作行与作者页脚. */
        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setGravity(Gravity.CENTER_HORIZONTAL);

        column.addView(AccountTheme.subtitle(this, getString(R.string.settings_subtitle)),
                AccountTheme.block(this, 2f));

        summaryView = AccountTheme.body(this, "", AccountTheme.COLOR_TEXT_FAINT, 12f);
        summaryView.setGravity(Gravity.CENTER);
        column.addView(summaryView, AccountTheme.block(this, 6f));

        TextView legend = AccountTheme.body(this, getString(R.string.settings_legend),
                AccountTheme.COLOR_TEXT_FAINT, 11f);
        legend.setGravity(Gravity.CENTER);
        legend.setLineSpacing(AccountTheme.dp(this, 2f), 1f);
        column.addView(legend, AccountTheme.block(this, 2f));

        groupsHost = new LinearLayout(this);
        groupsHost.setOrientation(LinearLayout.VERTICAL);
        groupsHost.setGravity(Gravity.CENTER_HORIZONTAL);
        column.addView(groupsHost, new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));

        column.addView(buildActionRow(), cardParams(18f));

        TextView author = AccountTheme.body(this, AccountTheme.authorFooter(this),
                AccountTheme.COLOR_TEXT_FAINT, 11f);
        author.setGravity(Gravity.CENTER);
        column.addView(author, AccountTheme.block(this, 14f));

        /* 外层: 场景底图 + 遮罩 + 居中面板, 长内容在面板内部滚动. */
        FrameLayout root = new FrameLayout(this);
        root.addView(MobilePage.create(this, getString(R.string.settings_title),
                MobilePage.scrollable(this, column)), new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));

        TextView noticeView = AccountTheme.notice(this);
        noticeBar = new NoticeBar(noticeView);
        FrameLayout.LayoutParams noticeParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.BOTTOM);
        int margin = AccountTheme.dp(this, 16f);
        noticeParams.setMargins(margin, 0, margin, margin);
        root.addView(noticeView, noticeParams);
        return root;
    }

    private LinearLayout buildActionRow() {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);

        Button back = AccountTheme.outlineButton(this, getString(R.string.settings_back),
                AccountTheme.COLOR_ACCENT);
        back.setOnClickListener(view -> finish());
        row.addView(back, new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f));

        resetButton = AccountTheme.outlineButton(this, getString(R.string.settings_reset),
                AccountTheme.COLOR_DANGER);
        resetButton.setOnClickListener(view -> requestReset());
        LinearLayout.LayoutParams resetParams = new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1.6f);
        resetParams.leftMargin = AccountTheme.dp(this, 10f);
        row.addView(resetButton, resetParams);
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
