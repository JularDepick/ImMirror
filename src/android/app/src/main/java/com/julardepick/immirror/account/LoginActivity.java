package com.julardepick.immirror.account;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Typeface;
import android.os.Bundle;
import android.text.InputType;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.julardepick.immirror.lobby.MainLobbyActivity;
import com.julardepick.immirror.R;
import com.julardepick.immirror.net.LanDiscovery;

/*
 * 登录界面.
 *
 * 登录, 注册与找回密码共用一页, 通过顶部标签切换; 登录支持邮箱或 UID 加密码,
 * 也可以切换为邮箱验证码登录. 背景沿用游戏登录底图, 控件全部用原生 View 以编程方式构建.
 * 所有提示都显示在页面内的提示条里, 不使用 Toast 与系统弹窗.
 */
public final class LoginActivity extends Activity {

    private static final int MODE_LOGIN = 0;
    private static final int MODE_REGISTER = 1;
    private static final int MODE_RESET = 2;

    /* 与服务端 PASSWORD_MIN_LENGTH 默认值一致, 仅用于提交前的本地提示. */
    private static final int PASSWORD_MIN_LENGTH = 8;

    private static final String TAG = "ImMirrorLogin";

    private ApiClient api;
    private final CodeCountdown countdown = new CodeCountdown();

    private TextView tabLogin;
    private TextView tabRegister;
    private TextView tabReset;

    private LinearLayout identifierBlock;
    private LinearLayout emailBlock;
    private LinearLayout codeBlock;
    private LinearLayout passwordBlock;
    private TextView passwordLabel;

    private EditText identifierInput;
    private EditText emailInput;
    private EditText codeInput;
    private EditText passwordInput;

    private Button sendCodeButton;
    private Button primaryButton;
    private TextView linkPrimary;
    private TextView linkSecondary;
    private TextView noticeView;
    private NoticeBar noticeBar;

    private TextView settingsSummary;
    private LinearLayout settingsPanel;
    private EditText settingsInput;

    private int mode = MODE_LOGIN;
    private boolean codeLogin;
    private boolean busy;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        api = new ApiClient(this);
        setContentView(buildContentView());
        applyMode();
        refreshSettingsSummary();
    }

    @Override
    protected void onDestroy() {
        countdown.cancel();
        super.onDestroy();
    }

    private View buildContentView() {
        FrameLayout root = new FrameLayout(this);
        root.setBackgroundColor(AccountTheme.COLOR_PAGE);

        ImageView background = new ImageView(this);
        background.setImageResource(R.drawable.login_bg);
        background.setScaleType(ImageView.ScaleType.CENTER_CROP);
        root.addView(background, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));

        View scrim = new View(this);
        scrim.setBackgroundColor(AccountTheme.COLOR_SCRIM);
        root.addView(scrim, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setClipToPadding(false);

        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setGravity(Gravity.CENTER_HORIZONTAL);
        int padding = AccountTheme.dp(this, AccountTheme.PAGE_PADDING_DP);
        column.setPadding(padding, AccountTheme.dp(this, 20f), padding, AccountTheme.dp(this, 22f));
        scroll.addView(column, new ScrollView.LayoutParams(
                ScrollView.LayoutParams.MATCH_PARENT, ScrollView.LayoutParams.WRAP_CONTENT));
        root.addView(scroll, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));

        column.addView(AccountTheme.title(this, getString(R.string.launcher_title),
                AccountTheme.COLOR_TEXT_ON_DARK));
        column.addView(AccountTheme.subtitle(this, getString(R.string.launcher_subtitle),
                        AccountTheme.COLOR_TEXT_ON_DARK_MUTED),
                AccountTheme.block(this, 4f));

        LinearLayout card = buildCard();
        column.addView(card, cardParams());

        TextView author = AccountTheme.body(this, AccountTheme.authorFooter(this),
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
        params.topMargin = AccountTheme.dp(this, 18f);
        return params;
    }

    private LinearLayout buildCard() {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        int horizontal = AccountTheme.dp(this, 20f);
        int vertical = AccountTheme.dp(this, 18f);
        card.setPadding(horizontal, vertical, horizontal, vertical);
        card.setBackground(AccountTheme.panelBackground(this));

        card.addView(buildTabs());
        card.addView(buildDivider());

        identifierBlock = buildInputBlock(R.string.account_label_identifier,
                identifierInput = AccountTheme.field(this, getString(R.string.account_hint_identifier), 15f));
        identifierInput.setInputType(InputType.TYPE_CLASS_TEXT);
        identifierInput.setImeOptions(EditorInfo.IME_ACTION_NEXT);
        card.addView(identifierBlock, AccountTheme.block(this, 14f));

        emailBlock = buildInputBlock(R.string.account_label_email,
                emailInput = AccountTheme.field(this, getString(R.string.account_hint_email), 15f));
        emailInput.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS);
        emailInput.setImeOptions(EditorInfo.IME_ACTION_NEXT);
        card.addView(emailBlock, AccountTheme.block(this, 12f));

        codeBlock = buildCodeBlock();
        card.addView(codeBlock, AccountTheme.block(this, 12f));

        passwordBlock = new LinearLayout(this);
        passwordBlock.setOrientation(LinearLayout.VERTICAL);
        passwordLabel = AccountTheme.label(this, getString(R.string.account_label_password));
        passwordInput = AccountTheme.field(this, getString(R.string.account_hint_password), 15f);
        passwordInput.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
        passwordInput.setImeOptions(EditorInfo.IME_ACTION_DONE);
        passwordInput.setOnEditorActionListener((view, actionId, event) -> {
            boolean done = actionId == EditorInfo.IME_ACTION_DONE
                    || actionId == EditorInfo.IME_ACTION_GO
                    || (event != null && event.getKeyCode() == KeyEvent.KEYCODE_ENTER
                    && event.getAction() == KeyEvent.ACTION_DOWN);
            if (done) {
                submit();
                return true;
            }
            return false;
        });
        passwordBlock.addView(passwordLabel);
        passwordBlock.addView(passwordInput, AccountTheme.block(this, 6f));
        card.addView(passwordBlock, AccountTheme.block(this, 12f));

        noticeView = AccountTheme.notice(this);
        noticeBar = new NoticeBar(noticeView);
        card.addView(noticeView, AccountTheme.block(this, 12f));

        primaryButton = AccountTheme.primaryButton(this, getString(R.string.account_action_login));
        primaryButton.setOnClickListener(view -> submit());
        card.addView(primaryButton, AccountTheme.block(this, 14f));

        card.addView(buildLinkRow(), AccountTheme.block(this, 8f));

        settingsSummary = AccountTheme.body(this, "", AccountTheme.COLOR_TEXT_ON_DARK_MUTED, 12f);
        settingsSummary.setGravity(Gravity.CENTER);
        settingsSummary.setPadding(0, AccountTheme.dp(this, 12f), 0, 0);
        settingsSummary.setOnClickListener(view -> toggleSettings());
        card.addView(settingsSummary);

        settingsPanel = buildSettingsPanel();
        settingsPanel.setVisibility(View.GONE);
        card.addView(settingsPanel, AccountTheme.block(this, 8f));
        return card;
    }

    private LinearLayout buildTabs() {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        tabLogin = buildTab(MODE_LOGIN, R.string.account_tab_login);
        tabRegister = buildTab(MODE_REGISTER, R.string.account_tab_register);
        tabReset = buildTab(MODE_RESET, R.string.account_tab_reset);
        row.addView(tabLogin, weightedParams());
        row.addView(tabRegister, weightedParams());
        row.addView(tabReset, weightedParams());
        return row;
    }

    private TextView buildTab(int targetMode, int textRes) {
        TextView tab = new TextView(this);
        tab.setText(textRes);
        tab.setTextSize(TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_BODY);
        tab.setGravity(Gravity.CENTER);
        tab.setPadding(0, AccountTheme.dp(this, 9f), 0, AccountTheme.dp(this, 9f));
        tab.setClickable(true);
        tab.setFocusable(true);
        tab.setOnClickListener(view -> switchMode(targetMode));
        return tab;
    }

    private LinearLayout.LayoutParams weightedParams() {
        return new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
    }

    /*
     * 分隔线. 纯 View 在 wrap_content 下会按可用空间撑满, 因此这里显式给出 1dp 高度,
     * 只保留上边距作为与标签行的间隔.
     */
    private View buildDivider() {
        View divider = new View(this);
        divider.setBackgroundColor(AccountTheme.COLOR_CARD_STROKE);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, Math.max(1, AccountTheme.dp(this, 1f)));
        params.topMargin = AccountTheme.dp(this, 6f);
        divider.setLayoutParams(params);
        return divider;
    }

    private LinearLayout buildInputBlock(int labelRes, EditText input) {
        LinearLayout block = new LinearLayout(this);
        block.setOrientation(LinearLayout.VERTICAL);
        block.addView(AccountTheme.label(this, getString(labelRes)));
        block.addView(input, AccountTheme.block(this, 6f));
        return block;
    }

    private LinearLayout buildCodeBlock() {
        LinearLayout block = new LinearLayout(this);
        block.setOrientation(LinearLayout.VERTICAL);
        block.addView(AccountTheme.label(this, getString(R.string.account_label_code)));

        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        codeInput = AccountTheme.field(this, getString(R.string.account_hint_code), 15f);
        codeInput.setInputType(InputType.TYPE_CLASS_NUMBER);
        codeInput.setImeOptions(EditorInfo.IME_ACTION_NEXT);
        row.addView(codeInput, new LinearLayout.LayoutParams(0, AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f));

        sendCodeButton = AccountTheme.outlineButton(this, getString(R.string.account_action_send_code),
                AccountTheme.COLOR_ACCENT);
        sendCodeButton.setTextSize(TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_SUB);
        sendCodeButton.setSingleLine(true);
        sendCodeButton.setOnClickListener(view -> requestCode());
        LinearLayout.LayoutParams sendParams = new LinearLayout.LayoutParams(
                AccountTheme.dp(this, 126f), AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP));
        sendParams.leftMargin = AccountTheme.dp(this, 8f);
        row.addView(sendCodeButton, sendParams);
        block.addView(row, AccountTheme.block(this, 6f));
        return block;
    }

    private LinearLayout buildLinkRow() {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER);
        linkPrimary = AccountTheme.link(this, "", AccountTheme.COLOR_ACCENT);
        linkSecondary = AccountTheme.link(this, "", AccountTheme.COLOR_TEXT_ON_DARK_MUTED);
        row.addView(linkPrimary);
        row.addView(linkSecondary);
        return row;
    }

    private LinearLayout buildSettingsPanel() {
        LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        int padding = AccountTheme.dp(this, 12f);
        panel.setPadding(padding, padding, padding, padding);
        panel.setBackground(AccountTheme.panelBackground(this));

        panel.addView(AccountTheme.label(this, getString(R.string.account_settings_title)));

        settingsInput = AccountTheme.field(this, AccountSettings.DEFAULT_BASE_URL, 13f);
        settingsInput.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_URI);
        settingsInput.setText(AccountSettings.baseUrl(this));
        panel.addView(settingsInput, AccountTheme.block(this, 6f));

        /* 自动发现: 紧跟输入框, 保证在屏幕可视区内. */
        TextView presetDiscover = AccountTheme.link(this, getString(R.string.account_settings_preset_discover),
                AccountTheme.COLOR_ACCENT);
        presetDiscover.setOnClickListener(view -> runDiscovery(settingsInput, presetDiscover));
        panel.addView(presetDiscover, AccountTheme.block(this, 4f));

        TextView hint = AccountTheme.body(this, getString(R.string.account_settings_hint),
                AccountTheme.COLOR_TEXT_FAINT, 11f);
        hint.setLineSpacing(AccountTheme.dp(this, 3f), 1f);
        panel.addView(hint, AccountTheme.block(this, 6f));

        LinearLayout presetRow = new LinearLayout(this);
        presetRow.setOrientation(LinearLayout.HORIZONTAL);
        presetRow.setGravity(Gravity.CENTER_VERTICAL);
        TextView presetEmulator = AccountTheme.link(this, getString(R.string.account_settings_preset_emulator),
                AccountTheme.COLOR_ACCENT);
        presetEmulator.setOnClickListener(view -> settingsInput.setText(AccountSettings.DEFAULT_BASE_URL));
        TextView presetLoopback = AccountTheme.link(this, getString(R.string.account_settings_preset_loopback),
                AccountTheme.COLOR_ACCENT);
        presetLoopback.setOnClickListener(view -> settingsInput.setText(AccountSettings.LOOPBACK_BASE_URL));
        /* 两个预设链接要真的挂进这一行, 否则点了看不见也点不到. */
        presetRow.addView(presetEmulator, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        final LinearLayout.LayoutParams loopbackParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        loopbackParams.leftMargin = AccountTheme.dp(this, 12f);
        presetRow.addView(presetLoopback, loopbackParams);
        panel.addView(presetRow, AccountTheme.block(this, 4f));

        LinearLayout actionRow = new LinearLayout(this);
        actionRow.setOrientation(LinearLayout.HORIZONTAL);
        Button saveButton = AccountTheme.outlineButton(this, getString(R.string.account_settings_save),
                AccountTheme.COLOR_TEXT);
        saveButton.setTextSize(TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_SUB);
        saveButton.setOnClickListener(view -> saveSettings());
        Button testButton = AccountTheme.outlineButton(this, getString(R.string.account_settings_test),
                AccountTheme.COLOR_TEXT_MUTED);
        testButton.setTextSize(TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_SUB);
        testButton.setOnClickListener(view -> testConnection());
        LinearLayout.LayoutParams halfParams = new LinearLayout.LayoutParams(
                0, AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f);
        actionRow.addView(saveButton, halfParams);
        LinearLayout.LayoutParams testParams = new LinearLayout.LayoutParams(
                0, AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f);
        testParams.leftMargin = AccountTheme.dp(this, 8f);
        actionRow.addView(testButton, testParams);
        panel.addView(actionRow, AccountTheme.block(this, 10f));
        return panel;
    }

    /*
     * 局域网自动发现: 广播探测包, 收到回应自动填充第一个地址.
     * 探测期间按钮置灰, 结束后恢复; 找不到时提示手动输入.
     */
    private void runDiscovery(android.widget.EditText input, android.view.View button) {
        button.setEnabled(false);
        showNotice(getString(R.string.account_notice_discovering), AccountTheme.COLOR_TEXT_MUTED);
        new LanDiscovery(this).discover(new LanDiscovery.Callback() {
            @Override
            public void onFound(java.util.Set<String> addresses) {
                button.setEnabled(true);
                if (!addresses.isEmpty()) {
                    final String first = addresses.iterator().next();
                    input.setText("http://" + first);
                    showNotice(getString(R.string.account_notice_discovered, first),
                            AccountTheme.COLOR_SUCCESS);
                }
            }

            @Override
            public void onFinished(int found) {
                button.setEnabled(true);
                if (found == 0) {
                    showNotice(getString(R.string.account_notice_discovery_empty),
                            AccountTheme.COLOR_TEXT_MUTED);
                }
            }
        });
    }

    private void switchMode(int targetMode) {
        if (mode == targetMode) {
            return;
        }
        mode = targetMode;
        codeLogin = false;
        clearNotice();
        applyMode();
    }

    private void applyMode() {
        boolean isLogin = mode == MODE_LOGIN;
        boolean passwordLogin = isLogin && !codeLogin;

        setVisible(identifierBlock, passwordLogin);
        setVisible(emailBlock, !passwordLogin);
        setVisible(codeBlock, !passwordLogin);
        setVisible(passwordBlock, passwordLogin || mode != MODE_LOGIN);

        passwordLabel.setText(mode == MODE_REGISTER ? R.string.account_label_password_set
                : mode == MODE_RESET ? R.string.account_label_password_new
                : R.string.account_label_password);

        primaryButton.setText(primaryLabelResource());

        if (isLogin) {
            linkPrimary.setText(codeLogin ? R.string.account_link_use_password : R.string.account_link_use_code);
            linkPrimary.setOnClickListener(view -> {
                codeLogin = !codeLogin;
                clearNotice();
                applyMode();
            });
            linkSecondary.setText(R.string.account_link_forgot);
            linkSecondary.setVisibility(View.VISIBLE);
            linkSecondary.setOnClickListener(view -> switchMode(MODE_RESET));
        } else if (mode == MODE_REGISTER) {
            linkPrimary.setText(R.string.account_link_to_login);
            linkPrimary.setOnClickListener(view -> switchMode(MODE_LOGIN));
            linkSecondary.setVisibility(View.GONE);
        } else {
            linkPrimary.setText(R.string.account_link_back_login);
            linkPrimary.setOnClickListener(view -> switchMode(MODE_LOGIN));
            linkSecondary.setVisibility(View.GONE);
        }

        styleTab(tabLogin, isLogin);
        styleTab(tabRegister, mode == MODE_REGISTER);
        styleTab(tabReset, mode == MODE_RESET);
        updateSendButtonState();
    }

    private void styleTab(TextView tab, boolean selected) {
        tab.setTextColor(selected ? AccountTheme.COLOR_TEXT : AccountTheme.COLOR_TEXT_MUTED);
        tab.setTypeface(selected ? Typeface.DEFAULT_BOLD : Typeface.DEFAULT);
        tab.setBackground(AccountTheme.tabBackground(this, selected));
    }

    private int primaryLabelResource() {
        if (mode == MODE_REGISTER) {
            return R.string.account_action_register;
        }
        if (mode == MODE_RESET) {
            return R.string.account_action_reset;
        }
        return R.string.account_action_login;
    }

    private void submit() {
        if (busy) {
            return;
        }
        String identifier = textOf(identifierInput);
        String email = textOf(emailInput);
        String code = textOf(codeInput);
        String password = textOf(passwordInput);

        if (mode == MODE_LOGIN && !codeLogin) {
            if (identifier.isEmpty()) {
                showNotice(R.string.account_notice_identifier_required, AccountTheme.COLOR_ERROR);
                return;
            }
            if (password.isEmpty()) {
                showNotice(R.string.account_notice_password_required, AccountTheme.COLOR_ERROR);
                return;
            }
            setBusy(true);
            api.login(identifier, password, sessionCallback(R.string.account_notice_login_ok));
            return;
        }

        if (email.isEmpty()) {
            showNotice(R.string.account_notice_email_required, AccountTheme.COLOR_ERROR);
            return;
        }
        if (code.isEmpty()) {
            showNotice(R.string.account_notice_code_required, AccountTheme.COLOR_ERROR);
            return;
        }

        if (mode == MODE_LOGIN) {
            setBusy(true);
            api.loginWithCode(email, code, sessionCallback(R.string.account_notice_login_ok));
            return;
        }

        if (password.length() < PASSWORD_MIN_LENGTH) {
            showNotice(R.string.account_notice_password_short, AccountTheme.COLOR_ERROR);
            return;
        }
        setBusy(true);
        if (mode == MODE_REGISTER) {
            api.register(email, code, password, sessionCallback(R.string.account_notice_register_ok));
            return;
        }
        api.resetPassword(email, code, password, new ApiClient.Callback<Void>() {
            @Override
            public void onSuccess(Void value) {
                if (isFinishing()) {
                    return;
                }
                setBusy(false);
                codeInput.setText("");
                passwordInput.setText("");
                mode = MODE_LOGIN;
                codeLogin = false;
                applyMode();
                showNotice(R.string.account_notice_reset_ok, AccountTheme.COLOR_SUCCESS);
            }

            @Override
            public void onFailure(ApiException error) {
                if (isFinishing()) {
                    return;
                }
                setBusy(false);
                showApiError(error);
            }
        });
    }

    private ApiClient.Callback<Session> sessionCallback(final int successMessageRes) {
        return new ApiClient.Callback<Session>() {
            @Override
            public void onSuccess(Session value) {
                if (isFinishing()) {
                    return;
                }
                setBusy(false);
                SessionStore.save(LoginActivity.this, value);
                showNotice(successMessageRes, AccountTheme.COLOR_SUCCESS);
                enterEntryScreen();
            }

            @Override
            public void onFailure(ApiException error) {
                if (isFinishing()) {
                    return;
                }
                setBusy(false);
                showApiError(error);
            }
        };
    }

    private void requestCode() {
        if (busy || countdown.isRunning()) {
            return;
        }
        final String email = textOf(emailInput);
        if (email.isEmpty()) {
            showNotice(R.string.account_notice_email_required, AccountTheme.COLOR_ERROR);
            return;
        }
        sendCodeButton.setEnabled(false);
        ApiClient.Callback<CodeDelivery> callback = new ApiClient.Callback<CodeDelivery>() {
            @Override
            public void onSuccess(CodeDelivery value) {
                if (isFinishing()) {
                    return;
                }
                showNotice(getString(R.string.account_notice_code_sent, value.ttlSeconds()),
                        AccountTheme.COLOR_SUCCESS);
                startCountdown(value.cooldownSeconds());
            }

            @Override
            public void onFailure(ApiException error) {
                if (isFinishing()) {
                    return;
                }
                showApiError(error);
                updateSendButtonState();
            }
        };
        if (mode == MODE_REGISTER) {
            api.sendRegisterCode(email, callback);
        } else if (mode == MODE_RESET) {
            api.sendResetCode(email, callback);
        } else {
            api.sendLoginCode(email, callback);
        }
    }

    private void startCountdown(int seconds) {
        countdown.start(seconds, new CodeCountdown.Listener() {
            @Override
            public void onTick(int remainingSeconds) {
                sendCodeButton.setEnabled(false);
                sendCodeButton.setText(getString(R.string.account_action_code_countdown, remainingSeconds));
            }

            @Override
            public void onFinish() {
                sendCodeButton.setText(R.string.account_action_send_code);
                sendCodeButton.setEnabled(!busy);
            }
        });
    }

    private void updateSendButtonState() {
        if (!countdown.isRunning()) {
            sendCodeButton.setText(R.string.account_action_send_code);
        }
        sendCodeButton.setEnabled(!busy && !countdown.isRunning());
    }

    private void setBusy(boolean value) {
        busy = value;
        primaryButton.setEnabled(!value);
        primaryButton.setText(value ? getString(R.string.account_action_busy)
                : getString(primaryLabelResource()));
        identifierInput.setEnabled(!value);
        emailInput.setEnabled(!value);
        codeInput.setEnabled(!value);
        passwordInput.setEnabled(!value);
        tabLogin.setEnabled(!value);
        tabRegister.setEnabled(!value);
        tabReset.setEnabled(!value);
        linkPrimary.setEnabled(!value);
        linkSecondary.setEnabled(!value);
        settingsSummary.setEnabled(!value);
        updateSendButtonState();
    }

    private void enterEntryScreen() {
        /* 登录与注册与换绑成功的落点都是大厅, 不再回旧入口页. */
        Intent intent = new Intent(this, MainLobbyActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(intent);
        finish();
    }

    private void showApiError(ApiException error) {
        Log.w(TAG, "api failed code=" + error.code() + " detail=" + error.detail());
        showNotice(ErrorMessages.messageFor(this, error), AccountTheme.COLOR_ERROR);
    }

    private void showNotice(int messageRes, int color) {
        showNotice(getString(messageRes), color);
    }

    private void showNotice(CharSequence message, int color) {
        noticeBar.show(message, color);
    }

    private void clearNotice() {
        noticeBar.clear();
    }

    private void toggleSettings() {
        boolean expanding = settingsPanel.getVisibility() != View.VISIBLE;
        settingsPanel.setVisibility(expanding ? View.VISIBLE : View.GONE);
        if (expanding) {
            settingsInput.setText(AccountSettings.baseUrl(this));
        }
    }

    private void refreshSettingsSummary() {
        settingsSummary.setText(getString(R.string.account_settings_summary_format,
                AccountSettings.baseUrl(this)));
    }

    private void saveSettings() {
        String raw = settingsInput.getText().toString();
        if (!AccountSettings.saveBaseUrl(this, raw)) {
            showNotice(R.string.account_notice_server_invalid, AccountTheme.COLOR_ERROR);
            return;
        }
        refreshSettingsSummary();
        settingsPanel.setVisibility(View.GONE);
        showNotice(R.string.account_notice_server_saved, AccountTheme.COLOR_SUCCESS);
    }

    private void testConnection() {
        String raw = settingsInput.getText().toString();
        if (!AccountSettings.saveBaseUrl(this, raw)) {
            showNotice(R.string.account_notice_server_invalid, AccountTheme.COLOR_ERROR);
            return;
        }
        refreshSettingsSummary();
        showNotice(R.string.account_notice_pinging, AccountTheme.COLOR_TEXT_MUTED);
        api.ping(new ApiClient.Callback<String>() {
            @Override
            public void onSuccess(String value) {
                if (isFinishing()) {
                    return;
                }
                showNotice(getString(R.string.account_notice_ping_ok, value), AccountTheme.COLOR_SUCCESS);
            }

            @Override
            public void onFailure(ApiException error) {
                if (isFinishing()) {
                    return;
                }
                showApiError(error);
            }
        });
    }

    private static void setVisible(View view, boolean visible) {
        view.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    private static String textOf(TextView view) {
        return view.getText().toString().trim();
    }
}