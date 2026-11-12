package com.julardepick.immirror.lobby;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.R;
import com.julardepick.immirror.account.AccountTheme;
import com.julardepick.immirror.account.Session;
import com.julardepick.immirror.account.SessionStore;
import com.julardepick.immirror.generated.UiResources;
import com.julardepick.immirror.inventory.InventoryActivity;
import com.julardepick.immirror.progress.ProgressActivity;
import com.julardepick.immirror.ui.UiTextures;
import com.julardepick.immirror.wallet.WalletActivity;
import com.julardepick.immirror.wallet.WalletApi;
import com.julardepick.immirror.wallet.WalletBoard;

/*
 * 玩家中心.
 *
 * 资料卡与背包, 成就, 钱包入口.
 */
public final class PlayerCenterActivity extends Activity {

    private static final String TAG = "ImMirrorPlayerCenter";

    private TextView noticeView;
    private TextView uidView;
    private TextView emailView;
    private TextView balanceView;
    private WalletApi walletApi;
    private Session session;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        walletApi = new WalletApi(this);
        session = SessionStore.load(this);
        setContentView(buildPage());
        renderProfile();
        refreshBalance();
    }

    /* 页面结构: 标题栏, 资料卡, 功能入口三行按钮, 返回大厅. */
    private View buildPage() {
        final LinearLayout page = new LinearLayout(this);
        page.setOrientation(LinearLayout.VERTICAL);
        page.setBackground(AccountTheme.rounded(this, AccountTheme.COLOR_PAGE, 0f));
        final int padding = AccountTheme.dp(this, AccountTheme.PAGE_PADDING_DP);
        page.setPadding(padding, padding, padding, padding);

        /* 标题栏: 返回按钮 + 玩家中心标题. */
        final LinearLayout titleBar = new LinearLayout(this);
        titleBar.setOrientation(LinearLayout.HORIZONTAL);
        titleBar.setGravity(Gravity.CENTER_VERTICAL);
        final Button back = AccountTheme.outlineButton(this, getString(R.string.lobby_back),
                AccountTheme.COLOR_ACCENT);
        back.setOnClickListener(view -> finish());
        titleBar.addView(back, new LinearLayout.LayoutParams(
                AccountTheme.dp(this, 90), AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP)));
        final TextView title = new TextView(this);
        title.setText(getString(R.string.playercenter_title));
        title.setTextColor(AccountTheme.COLOR_TEXT_ON_DARK);
        title.setTextSize(AccountTheme.tierSize(AccountTheme.TEXT_SIZE_TITLE));
        title.setTypeface(Typeface.DEFAULT_BOLD);
        final LinearLayout.LayoutParams titleParams = new LinearLayout.LayoutParams(0,
                LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
        titleParams.leftMargin = AccountTheme.dp(this, 16f);
        titleBar.addView(title, titleParams);
        page.addView(titleBar);

        /* 提示条: 未登录等状态提示. */
        noticeView = new TextView(this);
        noticeView.setTextColor(AccountTheme.COLOR_NOTICE_ERROR);
        noticeView.setTextSize(AccountTheme.tierSize(AccountTheme.TEXT_SIZE_SUB));
        noticeView.setPadding(0, AccountTheme.dp(this, 12f), 0, 0);
        noticeView.setVisibility(View.GONE);
        page.addView(noticeView);

        /* 资料卡: 头像框, UID, 邮箱与镜币余额. */
        final LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setBackground(AccountTheme.rounded(this, AccountTheme.COLOR_CARD,
                AccountTheme.CARD_RADIUS_DP));
        card.setGravity(Gravity.CENTER_HORIZONTAL);
        final int cardPadding = AccountTheme.dp(this, 20f);
        card.setPadding(cardPadding, cardPadding, cardPadding, cardPadding);

        /* 头像框: 原版 slot/avatar_frame.png 九宫, 缺素材时回落纯色圆角底. */
        final int avatarHalf = AccountTheme.dp(this, 52f);
        final ImageView avatar = new ImageView(this);
        final Drawable frame = UiTextures.nineSlice(this,
                UiResources.Widget.SLOT_AVATAR_FRAME);
        if (frame != null) {
            avatar.setImageDrawable(frame);
        } else {
            avatar.setImageDrawable(AccountTheme.rounded(this, AccountTheme.COLOR_ACCENT_SOFT,
                    AccountTheme.CARD_RADIUS_DP));
        }
        avatar.setScaleType(ImageView.ScaleType.FIT_CENTER);
        final LinearLayout.LayoutParams avatarParams = new LinearLayout.LayoutParams(
                avatarHalf * 2, avatarHalf * 2);
        avatarParams.topMargin = AccountTheme.dp(this, 8f);
        card.addView(avatar, avatarParams);

        uidView = new TextView(this);
        uidView.setTextColor(AccountTheme.COLOR_TEXT);
        uidView.setTextSize(AccountTheme.tierSize(AccountTheme.TEXT_SIZE_BODY));
        uidView.setTypeface(Typeface.DEFAULT_BOLD);
        uidView.setGravity(Gravity.CENTER);
        uidView.setPadding(0, AccountTheme.dp(this, 12f), 0, 0);
        card.addView(uidView);

        emailView = new TextView(this);
        emailView.setTextColor(AccountTheme.COLOR_TEXT_MUTED);
        emailView.setTextSize(AccountTheme.tierSize(AccountTheme.TEXT_SIZE_SUB));
        emailView.setGravity(Gravity.CENTER);
        card.addView(emailView);

        balanceView = new TextView(this);
        balanceView.setTextColor(AccountTheme.COLOR_TEXT);
        balanceView.setTextSize(AccountTheme.tierSize(AccountTheme.TEXT_SIZE_BODY));
        balanceView.setGravity(Gravity.CENTER);
        balanceView.setPadding(0, AccountTheme.dp(this, 12f), 0, 0);
        card.addView(balanceView);

        final LinearLayout.LayoutParams cardParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        cardParams.topMargin = AccountTheme.dp(this, 16f);
        page.addView(card, cardParams);

        /* 功能入口: 背包, 成就, 钱包. */
        page.addView(entryButton(getString(R.string.playercenter_inventory),
                () -> startActivity(new Intent(this, InventoryActivity.class))));
        page.addView(entryButton(getString(R.string.playercenter_progress),
                () -> startActivity(new Intent(this, ProgressActivity.class))));
        page.addView(entryButton(getString(R.string.playercenter_wallet),
                () -> startActivity(new Intent(this, WalletActivity.class))));
        return page;
    }

    /* 资料卡回填: 会话可用时取 UID 与邮箱, 未登录显示空态提示. */
    private void renderProfile() {
        if (session == null || !session.isUsable()) {
            uidView.setText(getString(R.string.playercenter_uid_format, "-"));
            emailView.setText(getString(R.string.playercenter_email_format, "-"));
            balanceView.setText(getString(R.string.playercenter_balance_failed));
            noticeView.setVisibility(View.VISIBLE);
            noticeView.setText(getString(R.string.playercenter_login_required));
            return;
        }
        noticeView.setVisibility(View.GONE);
        uidView.setText(getString(R.string.playercenter_uid_format, session.uid()));
        emailView.setText(getString(R.string.playercenter_email_format, session.email()));
    }

    /* 镜币余额: 复用钱包接口的概览拉取, 失败显示读取失败且不崩溃. */
    private void refreshBalance() {
        if (session == null || !session.isUsable()) {
            return;
        }
        balanceView.setText(getString(R.string.playercenter_balance_loading));
        walletApi.fetchOverview(session, new WalletApi.Callback() {
            @Override
            public void onSuccess(WalletBoard value) {
                balanceView.setText(getString(R.string.playercenter_balance_format, value.balance()));
            }

            @Override
            public void onFailure(com.julardepick.immirror.account.ApiException error) {
                balanceView.setText(getString(R.string.playercenter_balance_failed));
                Log.w(TAG, "balance fetch failed: " + error);
            }
        });
    }

    /* 功能入口按钮: 线程安全的点击动作由调用方给出. */
    private View entryButton(String label, Runnable action) {
        final Button button = AccountTheme.outlineButton(this, label,
                AccountTheme.COLOR_ACCENT);
        button.setOnClickListener(view -> action.run());
        final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP));
        params.topMargin = AccountTheme.dp(this, 10f);
        return button;
    }
}
