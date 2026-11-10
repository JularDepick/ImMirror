package com.julardepick.immirror;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.julardepick.immirror.account.AccountProfile;
import com.julardepick.immirror.account.AccountTheme;
import com.julardepick.immirror.account.ApiClient;
import com.julardepick.immirror.account.ApiException;
import com.julardepick.immirror.account.ErrorMessages;
import com.julardepick.immirror.account.LoginActivity;
import com.julardepick.immirror.account.Session;
import com.julardepick.immirror.account.SessionStore;
import com.julardepick.immirror.device.DeviceProfile;
import com.julardepick.immirror.settings.SettingsActivity;

import java.util.List;

/*
 * 入口界面.
 *
 * 启动时按本地会话判断去向: 已登录直接进入游戏, 未登录进入登录界面.
 * 从游戏返回后停留在本页, 展示账号资料与设备分级, 并提供再次进入游戏与退出登录的入口.
 * 所有提示都显示在页面内的提示条里, 不使用 Toast 与系统弹窗.
 */
public final class LauncherActivity extends Activity {

    private static final String TAG = "ImMirrorLauncher";

    /* 退出登录的二次确认等待时长, 超时自动回到初始按钮状态. */
    private static final long LOGOUT_CONFIRM_MS = 3000L;

    private ApiClient api;
    private DeviceProfile profile;

    private final Handler handler = new Handler(Looper.getMainLooper());

    private final Runnable logoutReset = new Runnable() {
        @Override
        public void run() {
            logoutPending = false;
            logoutButton.setText(R.string.launcher_logout);
        }
    };

    private TextView accountUid;
    private TextView accountEmail;
    private TextView accountBalance;
    private TextView noticeView;
    private Button logoutButton;

    /* 本次进入是否已自动打开游戏, 避免从游戏返回后再次自动进入. */
    private boolean autoEntered;
    private boolean logoutPending;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        api = new ApiClient(this);
        profile = DeviceProfile.obtain(this);
        logProfile(profile);
        setContentView(buildContentView(profile));
        if (savedInstanceState != null) {
            return;
        }
        if (SessionStore.hasUsableSession(this)) {
            autoEntered = true;
            enterGame();
        } else {
            enterLogin();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (!SessionStore.hasUsableSession(this) || autoEntered) {
            return;
        }
        refreshAccount();
    }

    @Override
    protected void onPause() {
        super.onPause();
        autoEntered = false;
    }

    @Override
    protected void onDestroy() {
        handler.removeCallbacks(logoutReset);
        super.onDestroy();
    }

    @Override
    public void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        if (SessionStore.hasUsableSession(this) && !autoEntered) {
            autoEntered = true;
            enterGame();
        }
    }

    private void enterGame() {
        startActivity(new Intent(this, PlayActivity.class));
    }

    private void enterLogin() {
        startActivity(new Intent(this, LoginActivity.class));
        finish();
    }

    private void refreshAccount() {
        final Session session = SessionStore.load(this);
        if (session == null) {
            enterLogin();
            return;
        }
        accountUid.setText(getString(R.string.launcher_account_uid_format, session.uid()));
        accountEmail.setText(session.email());
        accountBalance.setText(R.string.launcher_account_balance_loading);
        clearNotice();
        api.fetchProfile(session, new ApiClient.Callback<AccountProfile>() {
            @Override
            public void onSuccess(AccountProfile value) {
                if (isFinishing()) {
                    return;
                }
                accountUid.setText(getString(R.string.launcher_account_uid_format, value.uid()));
                accountEmail.setText(value.email());
                accountBalance.setText(getString(R.string.launcher_account_balance_format, value.balance()));
            }

            @Override
            public void onFailure(ApiException error) {
                if (isFinishing()) {
                    return;
                }
                if (error.isSessionInvalid()) {
                    SessionStore.clear(LauncherActivity.this);
                    showNotice(getString(R.string.launcher_session_invalid), AccountTheme.COLOR_ERROR);
                    enterLogin();
                    return;
                }
                accountBalance.setText(R.string.launcher_account_balance_failed);
                showNotice(ErrorMessages.messageFor(LauncherActivity.this, error), AccountTheme.COLOR_ERROR);
            }
        });
    }

    private void requestLogout() {
        if (!logoutPending) {
            logoutPending = true;
            logoutButton.setText(R.string.launcher_logout_confirm);
            handler.postDelayed(logoutReset, LOGOUT_CONFIRM_MS);
            return;
        }
        handler.removeCallbacks(logoutReset);
        logoutPending = false;
        logoutButton.setText(R.string.launcher_logout);
        logout();
    }

    private void logout() {
        Session session = SessionStore.load(this);
        SessionStore.clear(this);
        if (session != null) {
            api.logout(session, null);
        }
        enterLogin();
    }

    private void showNotice(CharSequence message, int color) {
        noticeView.setText(message);
        noticeView.setTextColor(color);
        noticeView.setBackground(AccountTheme.rounded(this, 0x1FFF8A80, 8f));
        noticeView.setVisibility(View.VISIBLE);
    }

    private void clearNotice() {
        noticeView.setText("");
        noticeView.setVisibility(View.GONE);
    }

    private void logProfile(DeviceProfile deviceProfile) {
        Log.i(TAG, String.format("device grade=%s score=%d", deviceProfile.gradeCode(), deviceProfile.gradeScore()));
        for (String line : deviceProfile.toDisplayLines()) {
            Log.i(TAG, "device " + line);
        }
        Log.i(TAG, "native payload:\n" + deviceProfile.toNativePayload());
    }

    private View buildContentView(DeviceProfile deviceProfile) {
        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(AccountTheme.COLOR_PAGE);

        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setGravity(Gravity.CENTER_HORIZONTAL);
        int padding = AccountTheme.dp(this, AccountTheme.PAGE_PADDING_DP);
        column.setPadding(padding, AccountTheme.dp(this, 22f), padding, AccountTheme.dp(this, 22f));
        scroll.addView(column, new ScrollView.LayoutParams(
                ScrollView.LayoutParams.MATCH_PARENT, ScrollView.LayoutParams.WRAP_CONTENT));

        column.addView(AccountTheme.title(this, getString(R.string.launcher_title)));
        column.addView(AccountTheme.subtitle(this, getString(R.string.launcher_subtitle)),
                AccountTheme.block(this, 4f));

        TextView version = AccountTheme.body(this, getString(R.string.launcher_version_format,
                BuildConfig.VERSION_NAME, BuildConfig.VERSION_CODE), AccountTheme.COLOR_TEXT_FAINT, 12f);
        version.setGravity(Gravity.CENTER);
        column.addView(version, AccountTheme.block(this, 6f));

        column.addView(buildAccountCard(), cardParams());

        LinearLayout deviceCard = new LinearLayout(this);
        deviceCard.setOrientation(LinearLayout.VERTICAL);
        int innerPadding = AccountTheme.dp(this, 18f);
        deviceCard.setPadding(innerPadding, innerPadding, innerPadding, innerPadding);
        deviceCard.setBackground(AccountTheme.stroked(this, AccountTheme.COLOR_CARD,
                AccountTheme.CARD_RADIUS_DP, AccountTheme.COLOR_CARD_STROKE, 1f));

        TextView deviceHeader = AccountTheme.body(this, getString(R.string.launcher_device_header),
                AccountTheme.COLOR_TEXT, 14f);
        deviceCard.addView(deviceHeader);

        TextView grade = AccountTheme.body(this, getString(R.string.launcher_grade_format,
                deviceProfile.gradeLabel(), deviceProfile.gradeScore()), AccountTheme.COLOR_ACCENT, 14f);
        deviceCard.addView(grade, AccountTheme.block(this, 6f));

        TextView detail = AccountTheme.body(this, joinLines(deviceProfile.toDisplayLines()),
                AccountTheme.COLOR_TEXT_MUTED, 12f);
        detail.setTextIsSelectable(true);
        detail.setLineSpacing(AccountTheme.dp(this, 3f), 1f);
        deviceCard.addView(detail, AccountTheme.block(this, 6f));
        column.addView(deviceCard, cardParams());

        TextView author = AccountTheme.body(this, AccountTheme.authorFooter(this), AccountTheme.COLOR_TEXT_FAINT, 11f);
        author.setGravity(Gravity.CENTER);
        column.addView(author, AccountTheme.block(this, 14f));
        return scroll;
    }

    private LinearLayout buildAccountCard() {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        int padding = AccountTheme.dp(this, 18f);
        card.setPadding(padding, padding, padding, padding);
        card.setBackground(AccountTheme.stroked(this, AccountTheme.COLOR_CARD,
                AccountTheme.CARD_RADIUS_DP, AccountTheme.COLOR_CARD_STROKE, 1f));

        card.addView(AccountTheme.label(this, getString(R.string.launcher_account_header)));

        accountUid = AccountTheme.body(this, "", AccountTheme.COLOR_TEXT, 18f);
        accountUid.setTypeface(accountUid.getTypeface(), Typeface.BOLD);
        card.addView(accountUid, AccountTheme.block(this, 8f));

        accountEmail = AccountTheme.body(this, "", AccountTheme.COLOR_TEXT_MUTED, 13f);
        card.addView(accountEmail, AccountTheme.block(this, 4f));

        accountBalance = AccountTheme.body(this, getString(R.string.launcher_account_balance_loading),
                AccountTheme.COLOR_ACCENT, 14f);
        card.addView(accountBalance, AccountTheme.block(this, 6f));

        noticeView = AccountTheme.body(this, "", AccountTheme.COLOR_ERROR, 13f);
        int noticePadding = AccountTheme.dp(this, 10f);
        noticeView.setPadding(noticePadding, noticePadding, noticePadding, noticePadding);
        noticeView.setVisibility(View.GONE);
        card.addView(noticeView, AccountTheme.block(this, 10f));

        LinearLayout actions = new LinearLayout(this);
        actions.setOrientation(LinearLayout.HORIZONTAL);
        Button enter = AccountTheme.primaryButton(this, getString(R.string.launcher_enter_game));
        enter.setOnClickListener(view -> enterGame());
        logoutButton = AccountTheme.outlineButton(this, getString(R.string.launcher_logout),
                AccountTheme.COLOR_DANGER);
        logoutButton.setOnClickListener(view -> requestLogout());
        actions.addView(enter, new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP + 2), 1.3f));
        LinearLayout.LayoutParams logoutParams = new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f);
        logoutParams.leftMargin = AccountTheme.dp(this, 10f);
        actions.addView(logoutButton, logoutParams);
        card.addView(actions, AccountTheme.block(this, 14f));

        /*
         * 设置入口放在账号面板: 游戏内是原生 HUD, 不承载 Java 界面,
         * 因此所有设置改动都从本页进入, 返回后按需重新进入游戏.
         */
        Button settings = AccountTheme.outlineButton(this, getString(R.string.launcher_settings),
                AccountTheme.COLOR_ACCENT);
        settings.setOnClickListener(view -> startActivity(new Intent(this, SettingsActivity.class)));
        card.addView(settings, AccountTheme.block(this, 10f));
        return card;
    }

    private LinearLayout.LayoutParams cardParams() {
        int available = AccountTheme.screenWidth(this) - AccountTheme.dp(this, AccountTheme.PAGE_PADDING_DP * 2);
        int width = Math.min(available, AccountTheme.dp(this, AccountTheme.CARD_MAX_WIDTH_DP));
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(width,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        params.topMargin = AccountTheme.dp(this, 16f);
        return params;
    }

    private static String joinLines(List<String> lines) {
        StringBuilder builder = new StringBuilder();
        for (String line : lines) {
            builder.append(line).append('\n');
        }
        return builder.toString().trim();
    }
}
