package com.julardepick.immirror.account;

import android.app.Activity;
import android.content.Intent;
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
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.julardepick.immirror.R;

/*
 * 换绑邮箱界面.
 *
 * 放在 account 包内而不是新开 auth 包: 本页与登录注册共用 ApiClient,
 * AccountTheme, SessionStore 与 ErrorMessages 等账号基础设施, 换绑流程
 * 服务端也归在 auth 路由下, 收敛在同一包内避免跨包依赖扩散.
 *
 * 流程: 输入新邮箱, 发送验证码 (倒计时), 填验证码与当前密码后提交换绑;
 * 服务端换绑成功即作废旧会话, 客户端清空本地会话回登录页重新登录.
 * 所有提示都显示在页面内的提示条里, 不使用 Toast 与系统弹窗.
 */
public final class RebindActivity extends Activity {

    private static final String TAG = "ImMirrorRebind";

    /* 与服务端 PASSWORD_MIN_LENGTH 默认值一致, 仅用于提交前的本地提示. */
    private static final int PASSWORD_MIN_LENGTH = 8;

    private ApiClient api;
    private Session session;
    private final CodeCountdown countdown = new CodeCountdown();

    private TextView emailSummary;
    private EditText emailInput;
    private EditText codeInput;
    private EditText passwordInput;
    private Button sendCodeButton;
    private Button rebindButton;
    private TextView noticeView;
    private NoticeBar noticeBar;

    private boolean busy;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        api = new ApiClient(this);
        session = SessionStore.load(this);
        setContentView(buildContentView());
        refreshEmailSummary();
        if (session == null || !session.isUsable()) {
            showNotice(getString(R.string.rebind_notice_login_required), AccountTheme.COLOR_ERROR);
        }
    }

    @Override
    protected void onDestroy() {
        countdown.cancel();
        if (noticeBar != null) {
            noticeBar.clear();
        }
        super.onDestroy();
    }

    private View buildContentView() {
        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setBackgroundColor(AccountTheme.COLOR_PAGE);
        scroll.setClipToPadding(false);
        /* 底部留出提示条高度, 滚到末尾时按钮不被提示条盖住. */
        scroll.setPadding(0, 0, 0, AccountTheme.dp(this, 84f));

        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setGravity(Gravity.CENTER_HORIZONTAL);
        int padding = AccountTheme.dp(this, AccountTheme.PAGE_PADDING_DP);
        column.setPadding(padding, AccountTheme.dp(this, 20f), padding, AccountTheme.dp(this, 18f));
        scroll.addView(column, new ScrollView.LayoutParams(ScrollView.LayoutParams.MATCH_PARENT,
                ScrollView.LayoutParams.WRAP_CONTENT));

        column.addView(AccountTheme.title(this, getString(R.string.rebind_title)));
        column.addView(AccountTheme.subtitle(this, getString(R.string.rebind_subtitle)),
                AccountTheme.block(this, 4f));

        emailSummary = AccountTheme.body(this, "", AccountTheme.COLOR_TEXT_ON_DARK_MUTED, 12f);
        emailSummary.setGravity(Gravity.CENTER);
        column.addView(emailSummary, AccountTheme.block(this, 8f));

        LinearLayout card = buildCard();
        column.addView(card, cardParams());

        TextView author = AccountTheme.body(this, AccountTheme.authorFooter(this),
                AccountTheme.COLOR_TEXT_FAINT, 11f);
        author.setGravity(Gravity.CENTER);
        column.addView(author, AccountTheme.block(this, 14f));

        TextView noticeView = AccountTheme.notice(this);
        noticeBar = new NoticeBar(noticeView);
        android.widget.FrameLayout root = new android.widget.FrameLayout(this);
        android.widget.FrameLayout.LayoutParams noticeParams = new android.widget.FrameLayout.LayoutParams(
                android.widget.FrameLayout.LayoutParams.MATCH_PARENT,
                android.widget.FrameLayout.LayoutParams.WRAP_CONTENT, Gravity.BOTTOM);
        int margin = AccountTheme.dp(this, 16f);
        noticeParams.setMargins(margin, 0, margin, margin);
        root.addView(scroll, new android.widget.FrameLayout.LayoutParams(
                android.widget.FrameLayout.LayoutParams.MATCH_PARENT,
                android.widget.FrameLayout.LayoutParams.MATCH_PARENT));
        root.addView(noticeView, noticeParams);
        return root;
    }

    private LinearLayout buildCard() {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        int horizontal = AccountTheme.dp(this, 20f);
        int vertical = AccountTheme.dp(this, 18f);
        card.setPadding(horizontal, vertical, horizontal, vertical);
        card.setBackground(AccountTheme.panelBackground(this));

        card.addView(AccountTheme.label(this, getString(R.string.account_label_email)));
        emailInput = AccountTheme.field(this, getString(R.string.account_hint_email), 15f);
        emailInput.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS);
        emailInput.setImeOptions(EditorInfo.IME_ACTION_NEXT);
        card.addView(emailInput, AccountTheme.block(this, 6f));

        card.addView(AccountTheme.label(this, getString(R.string.account_label_code)),
                AccountTheme.block(this, 12f));
        LinearLayout codeRow = new LinearLayout(this);
        codeRow.setOrientation(LinearLayout.HORIZONTAL);
        codeInput = AccountTheme.field(this, getString(R.string.account_hint_code), 15f);
        codeInput.setInputType(InputType.TYPE_CLASS_NUMBER);
        codeInput.setImeOptions(EditorInfo.IME_ACTION_NEXT);
        codeRow.addView(codeInput, new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f));
        sendCodeButton = AccountTheme.outlineButton(this, getString(R.string.account_action_send_code),
                AccountTheme.COLOR_ACCENT);
        sendCodeButton.setTextSize(TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_SUB);
        sendCodeButton.setSingleLine(true);
        sendCodeButton.setOnClickListener(view -> requestCode());
        LinearLayout.LayoutParams sendParams = new LinearLayout.LayoutParams(
                AccountTheme.dp(this, 126f), AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP));
        sendParams.leftMargin = AccountTheme.dp(this, 8f);
        codeRow.addView(sendCodeButton, sendParams);
        card.addView(codeRow, AccountTheme.block(this, 6f));

        card.addView(AccountTheme.label(this, getString(R.string.rebind_label_password)),
                AccountTheme.block(this, 12f));
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
        card.addView(passwordInput, AccountTheme.block(this, 6f));

        TextView hint = AccountTheme.body(this, getString(R.string.rebind_hint_password),
                AccountTheme.COLOR_TEXT_FAINT, 11f);
        hint.setLineSpacing(AccountTheme.dp(this, 3f), 1f);
        card.addView(hint, AccountTheme.block(this, 6f));

        noticeView = AccountTheme.notice(this);
        noticeBar = new NoticeBar(noticeView);
        card.addView(noticeView, AccountTheme.block(this, 12f));

        rebindButton = AccountTheme.primaryButton(this, getString(R.string.rebind_action_submit));
        rebindButton.setOnClickListener(view -> submit());
        card.addView(rebindButton, AccountTheme.block(this, 14f));
        return card;
    }

    private LinearLayout.LayoutParams cardParams() {
        int available = AccountTheme.screenWidth(this)
                - AccountTheme.dp(this, AccountTheme.PAGE_PADDING_DP * 2);
        int width = Math.min(available, AccountTheme.dp(this, AccountTheme.CARD_MAX_WIDTH_DP));
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(width,
                LinearLayout.LayoutParams.WRAP_CONTENT);
        params.topMargin = AccountTheme.dp(this, 16f);
        return params;
    }

    private void refreshEmailSummary() {
        String email = session == null ? "" : session.email();
        emailSummary.setText(email.isEmpty()
                ? getString(R.string.rebind_email_unknown)
                : getString(R.string.rebind_email_summary_format, email));
    }

    /* 请求向新邮箱发送换绑验证码; 冷却秒数由服务端返回. */
    private void requestCode() {
        if (busy || countdown.isRunning()) {
            return;
        }
        if (session == null || !session.isUsable()) {
            showNotice(getString(R.string.rebind_notice_login_required), AccountTheme.COLOR_ERROR);
            return;
        }
        String email = textOf(emailInput);
        if (email.isEmpty()) {
            showNotice(getString(R.string.account_notice_email_required), AccountTheme.COLOR_ERROR);
            return;
        }
        if (session.email().equals(email)) {
            showNotice(getString(R.string.rebind_notice_same_email), AccountTheme.COLOR_ERROR);
            return;
        }
        sendCodeButton.setEnabled(false);
        api.requestRebindCode(session, email, new ApiClient.Callback<CodeDelivery>() {
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
                handleSessionError(error);
                showApiError(error);
                updateSendButtonState();
            }
        });
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

    /* 提交换绑: 服务端校验验证码与密码, 成功后作废旧会话. */
    private void submit() {
        if (busy) {
            return;
        }
        if (session == null || !session.isUsable()) {
            showNotice(getString(R.string.rebind_notice_login_required), AccountTheme.COLOR_ERROR);
            return;
        }
        String email = textOf(emailInput);
        String code = textOf(codeInput);
        String password = textOf(passwordInput);
        if (email.isEmpty()) {
            showNotice(getString(R.string.account_notice_email_required), AccountTheme.COLOR_ERROR);
            return;
        }
        if (session.email().equals(email)) {
            showNotice(getString(R.string.rebind_notice_same_email), AccountTheme.COLOR_ERROR);
            return;
        }
        if (code.isEmpty()) {
            showNotice(getString(R.string.account_notice_code_required), AccountTheme.COLOR_ERROR);
            return;
        }
        if (password.length() < PASSWORD_MIN_LENGTH) {
            showNotice(getString(R.string.account_notice_password_short), AccountTheme.COLOR_ERROR);
            return;
        }
        setBusy(true);
        api.rebindEmail(session, email, code, password, new ApiClient.Callback<String>() {
            @Override
            public void onSuccess(String value) {
                if (isFinishing()) {
                    return;
                }
                setBusy(false);
                /* 服务端换绑成功即作废旧会话, 本地清空并回登录页. */
                SessionStore.clear(RebindActivity.this);
                showNotice(getString(R.string.rebind_notice_ok), AccountTheme.COLOR_SUCCESS);
                passwordInput.setText("");
                codeInput.setText("");
                enterLogin();
            }

            @Override
            public void onFailure(ApiException error) {
                if (isFinishing()) {
                    return;
                }
                setBusy(false);
                handleSessionError(error);
                showApiError(error);
            }
        });
    }

    /* 会话失效: 换绑页无法继续, 清会话后回登录页. */
    private void handleSessionError(ApiException error) {
        if (error == null || !error.isSessionInvalid()) {
            return;
        }
        SessionStore.clear(this);
        showNotice(getString(R.string.launcher_session_invalid), AccountTheme.COLOR_ERROR);
        enterLogin();
    }

    private void enterLogin() {
        Intent intent = new Intent(this, LoginActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
        startActivity(intent);
        finish();
    }

    private void setBusy(boolean value) {
        busy = value;
        rebindButton.setEnabled(!value);
        rebindButton.setText(value ? getString(R.string.account_action_busy)
                : getString(R.string.rebind_action_submit));
        emailInput.setEnabled(!value);
        codeInput.setEnabled(!value);
        passwordInput.setEnabled(!value);
        updateSendButtonState();
    }

    private void showApiError(ApiException error) {
        Log.w(TAG, "api failed code=" + error.code() + " detail=" + error.detail());
        showNotice(ErrorMessages.messageFor(this, error), AccountTheme.COLOR_ERROR);
    }

    private void showNotice(CharSequence message, int color) {
        noticeBar.show(message, color);
    }

    private static String textOf(TextView view) {
        return view.getText().toString().trim();
    }
}
