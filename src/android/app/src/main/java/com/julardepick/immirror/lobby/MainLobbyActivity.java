package com.julardepick.immirror.lobby;

import android.app.Activity;
import android.content.Intent;
import android.graphics.PorterDuff;
import android.graphics.Typeface;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.julardepick.immirror.BuildConfig;
import com.julardepick.immirror.LauncherActivity;
import com.julardepick.immirror.R;
import com.julardepick.immirror.account.AccountTheme;
import com.julardepick.immirror.account.ApiClient;
import com.julardepick.immirror.account.AccountProfile;
import com.julardepick.immirror.account.ApiException;
import com.julardepick.immirror.account.RebindActivity;
import com.julardepick.immirror.account.Session;
import com.julardepick.immirror.account.SessionStore;
import com.julardepick.immirror.editor.MapEditorActivity;
import com.julardepick.immirror.inventory.InventoryActivity;
import com.julardepick.immirror.progress.ProgressActivity;
import com.julardepick.immirror.settings.SettingsActivity;
import com.julardepick.immirror.wallet.WalletActivity;

/*
 * 对局外大厅主界面.
 *
 * 顶部公告位与玩家条, 中部主入口卡片, 底部次入口与退出登录.
 * 尺寸取自 res/values/dimens.xml.
 */
public final class MainLobbyActivity extends Activity {

    private static final String TAG = "ImMirrorLobby";

    /* 退出登录的二次确认等待时长, 超时自动回到初始按钮状态. */
    private static final long LOGOUT_CONFIRM_MS = 3000L;
    /* 公告条关闭后的可见性取值, 用整型避免 View 常量散落. */
    private static final int NOTICE_CLOSED_VISIBILITY = View.GONE;

    /*
     * 字号档位: 对齐main_lobby.xml 的高频档 24 / 20 / 18 与次级档 16 / 14,
     * 该文件内 BlackFont24 出现 20 次, BlackFont18 出现 13 次, BlackFont20 出现 3 次.
     * AccountTheme 的字号收敛会把非三档数值吸附到 18sp, 大厅需要更细的分层,
     * 因此这里给出实际字号并在控件上显式覆写.
     */
    private static final float TEXT_TITLE = 24f;
    private static final float TEXT_UID = 20f;
    private static final float TEXT_BALANCE = 16f;
    /* 中部卡片标题: 大卡取标题档 24, 小卡取正文档 20, 与BlackFont24 / BlackFont20 一致. */
    private static final float TEXT_CARD_TITLE = 20f;
    private static final float TEXT_SECONDARY = 14f;
    private static final float TEXT_NOTE = 14f;
    private static final float TEXT_FOOTER = 12f;

    /* 大厅顶部图标的设计尺寸 (像素), main_lobby_ex.xml 里 icon_halltop_* 都是 56x56. */
    private static final float SETTINGS_ICON_PX = 56f;

    /*
     * 素材底图上的文字色: RoleInfoBkgTemplate 与 TemplateBigBkg2 都是浅色底
     * (玩家条实测 109,194,211, 白卡片 243,243,243), 白字不可读, 统一改深色;
     * 素材缺失回落 AccountTheme 的白色面板时深色同样可读.
     */
    private static final int COLOR_ART_TEXT = 0xFF1E3A42;
    private static final int COLOR_ART_TEXT_MUTED = 0xFF31555D;

    /*
     * 顶部条压暗层: 保留常量以便调试对比, 当前不启用.
     * 早期在全局暗罩之前需要它压住底图亮部, 底图统一压暗后模板自身的 alpha=128
     * 已足够形成半透明玻璃感, 再加实心压暗会丢掉的透光观感.
     */
    @SuppressWarnings("unused")
    private static final int TOP_BAND_SHADE = 0x7A000000;

    /*
     * 全屏底图暗罩: 底图本身是明亮户外场景, 压掉约三分之一亮度后
     * 背景落在中低亮度区间, 白色面板与反白文字才有足够对比.
     */
    private static final int BACKDROP_SHADE = 0x55000000;

    /*
     * 九宫与三段边界, 取自 .agent/LOBBY_VISUAL_SPEC.md 第 2 节模板表:
     * 30x30 tile 10 的九宫图, 24x24 tile 8 的九宫图,
     * RoleInfoBkgTemplate 三段 左 200 / 中 10 / 右 87,
     * TemplateBtn11 三段 左 57 / 中 16 / 右 57.
     */
    private static final int NINE_TILE_LARGE = 10;
    private static final int NINE_TILE_SMALL = 8;
    private static final int ROLE_INFO_LEFT = 200;
    private static final int ROLE_INFO_MIDDLE = 10;
    private static final int ROLE_INFO_RIGHT = 87;
    private static final int BUTTON_LEFT = 57;
    private static final int BUTTON_MIDDLE = 16;
    private static final int BUTTON_RIGHT = 57;

    /* 角色展示区与入口按钮上的文字色, 压在按钮素材上统一取深色. */
    private static final int CARD_TEXT = 0xFFFFFFFF;

    /* 设计像素高度: 顶部条 66, 标题大图 64, 底栏大图 58; 一条设计像素按 artScale 折算. */
    private static final float TOP_BAND_PX = 66f;
    private static final float TITLE_ART_PX = 64f;
    private static final float BOTTOM_ART_PX = 58f;
    /* 关闭按钮素材 36x46, 底栏分隔线素材 2x26, 镜币图标 40x40, 公告红点 26x26. */
    private static final float CLOSE_WIDTH_PX = 36f;
    private static final float CLOSE_HEIGHT_PX = 46f;
    private static final float DIVIDER_WIDTH_PX = 2f;
    private static final float DIVIDER_HEIGHT_PX = 26f;
    private static final float COIN_ICON_PX = 20f;
    private static final float DOT_ICON_PX = 18f;

    private ApiClient api;

    private final Handler handler = new Handler(Looper.getMainLooper());

    private final Runnable logoutReset = new Runnable() {
        @Override
        public void run() {
            logoutPending = false;
            logoutButton.setText(R.string.lobby_logout);
        }
    };

    private TextView playerUid;
    private TextView playerEmail;
    private TextView playerBalance;
    private View noticeRow;
    private Button logoutButton;
    private boolean logoutPending;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        api = new ApiClient(this);
        setContentView(buildContentView());
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (!SessionStore.hasUsableSession(this)) {
            /* 会话失效 (其他页清掉) 时回路由页, 由它送登录. */
            startActivity(new Intent(this, LauncherActivity.class));
            finish();
            return;
        }
        refreshAccount();
    }

    @Override
    protected void onDestroy() {
        handler.removeCallbacks(logoutReset);
        super.onDestroy();
    }

    private void refreshAccount() {
        final Session session = SessionStore.load(this);
        if (session == null) {
            return;
        }
        playerUid.setText(getString(R.string.lobby_uid_format, session.uid()));
        playerEmail.setText(session.email());
        playerBalance.setText(getString(R.string.lobby_balance_loading));
        api.fetchProfile(session, new ApiClient.Callback<AccountProfile>() {
            @Override
            public void onSuccess(AccountProfile value) {
                if (isFinishing()) {
                    return;
                }
                playerUid.setText(getString(R.string.lobby_uid_format, value.uid()));
                playerEmail.setText(value.email());
                playerBalance.setText(getString(R.string.lobby_balance_format, value.balance()));
            }

            @Override
            public void onFailure(ApiException error) {
                if (isFinishing()) {
                    return;
                }
                if (error.isSessionInvalid()) {
                    SessionStore.clear(MainLobbyActivity.this);
                    startActivity(new Intent(MainLobbyActivity.this, LauncherActivity.class));
                    finish();
                    return;
                }
                playerBalance.setText(getString(R.string.lobby_balance_failed));
            }
        });
    }

    private View buildContentView() {
        final ScrollView scroll = new ScrollView(this);
        /*
         * 最底层用大厅全屏底图 (texture0/bigtex/main_lobby, 640x360 设计分辨率, 铺满不留白),
         * 顶部条与中部卡片浮在其上; 底图缺失时回落页面纯色, 不出现空白块.
         *
         * 底图是一张明亮的户外场景, 直接铺会让白色面板与浅色文字无从分辨,
         * 因此在底图之上再压一层统一暗罩: 背景整体沉到中低亮度, 前景白卡与
         * 反白文字才有对比; 各控件自身的局部压暗层叠加后层次更强, 不与本层冲突.
         */
        final Drawable lobbyBackdrop = LobbyArt.image(this, LobbyArt.MAIN_LOBBY_BG);
        if (lobbyBackdrop != null) {
            scroll.setBackground(new LayerDrawable(new Drawable[]{
                    lobbyBackdrop, new ColorDrawable(BACKDROP_SHADE)}));
        } else {
            scroll.setBackgroundColor(AccountTheme.COLOR_PAGE);
        }
        scroll.setFillViewport(true);

        /*
         * 顶部条与底栏横向铺满屏幕 (MiniLobbyFrameTop 与 MiniLobbyFrameBottom 宽 1.0 相对),
         * 因此纵向容器不再留左右内边距; 中部三列卡片与页脚另按居中带宽约束, 避免被拉宽.
         */
        final LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);
        column.setGravity(Gravity.CENTER_HORIZONTAL);
        column.setPadding(0, dimen(R.dimen.lobby_block_gap), 0, dimen(R.dimen.lobby_block_gap));
        scroll.addView(column, new ScrollView.LayoutParams(
                ScrollView.LayoutParams.MATCH_PARENT, ScrollView.LayoutParams.WRAP_CONTENT));

        /* 公告位: 顶部一条, 内容为版本更新提示, 点关闭按钮整条收起 (对应 $parentNotice). */
        noticeRow = buildNoticeRow();
        column.addView(noticeRow, fullRowParams());

        /* 顶部: 顶部条 (标题大图) 与玩家条两层, 对应 MiniLobbyFrameTop 与 TopRoleInfo. */
        column.addView(buildHeader(), fullRowParams());

        /* 中部主入口 (一大两小结构): 单人游戏, 联机大厅, 迷你工坊, 我的背包. */
        column.addView(buildCenterBand(), centerParams());

        /* 底栏: 次入口一行: 成就, 钱包, 换绑邮箱, 玩家中心. */
        column.addView(buildBottomBand(), fullRowParams());

        /* 底部: 退出登录 + 版本与作者页脚. */
        column.addView(buildFooter(), bandParams());
        return scroll;
    }

    /*
     * 公告条: 深色九宫底板 (TemplateBkg8) + 红点 + 公告文案 + 右侧关闭钮 (TemplateCloseBtn0).
     * 文案先放版本更新提示; 后续接服务端公告时替换文本来源即可.
     */
    private View buildNoticeRow() {
        final LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        final Drawable board = LobbyArt.nineSlice(this, LobbyArt.BOARD_DARK,
                NINE_TILE_SMALL, NINE_TILE_SMALL, NINE_TILE_SMALL, NINE_TILE_SMALL);
        row.setBackground(board != null ? board
                : AccountTheme.rounded(this, AccountTheme.COLOR_SCRIM,
                        AccountTheme.FIELD_RADIUS_DP));
        final int pad = AccountTheme.dp(this, 10f);
        row.setPadding(AccountTheme.dp(this, 16f), pad, AccountTheme.dp(this, 16f), pad);

        final Drawable dot = LobbyArt.image(this, LobbyArt.ICON_DOT);
        if (dot != null) {
            ImageView badge = new ImageView(this);
            badge.setImageDrawable(dot);
            badge.setScaleType(ImageView.ScaleType.FIT_XY);
            final LinearLayout.LayoutParams dotParams = new LinearLayout.LayoutParams(
                    LobbyArt.designSize(this, DOT_ICON_PX), LobbyArt.designSize(this, DOT_ICON_PX));
            dotParams.rightMargin = AccountTheme.dp(this, 8f);
            row.addView(badge, dotParams);
        }

        final TextView text = text(getString(R.string.lobby_notice_text),
                AccountTheme.COLOR_NOTICE_TEXT, TEXT_NOTE);
        text.setMaxLines(2);
        row.addView(text, new LinearLayout.LayoutParams(0,
                LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        final Drawable closeArt = LobbyArt.image(this, LobbyArt.CLOSE_BTN);
        if (closeArt != null) {
            final ImageButton close = new ImageButton(this);
            close.setBackground(AccountTheme.pressed(0x55FFFFFF, closeArt));
            close.setContentDescription(getString(R.string.lobby_notice_close));
            close.setOnClickListener(view -> row.setVisibility(NOTICE_CLOSED_VISIBILITY));
            final LinearLayout.LayoutParams closeParams = new LinearLayout.LayoutParams(
                    LobbyArt.designSize(this, CLOSE_WIDTH_PX),
                    LobbyArt.designSize(this, CLOSE_HEIGHT_PX));
            closeParams.leftMargin = AccountTheme.dp(this, 10f);
            row.addView(close, closeParams);
        } else {
            final Button close = button(false, getString(R.string.lobby_notice_close),
                    AccountTheme.COLOR_TEXT_ON_DARK, TEXT_SECONDARY);
            close.setOnClickListener(view -> row.setVisibility(NOTICE_CLOSED_VISIBILITY));
            final LinearLayout.LayoutParams closeParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.WRAP_CONTENT,
                    dimen(R.dimen.lobby_secondary_button_height));
            closeParams.leftMargin = AccountTheme.dp(this, 10f);
            row.addView(close, closeParams);
        }
        return row;
    }

    /* 顶部两层: 顶部条 (66 设计像素) 与玩家条, 玩家条高度沿用 dimens 的 80dp. */
    private View buildHeader() {
        final LinearLayout header = new LinearLayout(this);
        header.setOrientation(LinearLayout.VERTICAL);
        header.addView(buildTopBand(), new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LobbyArt.designSize(this, TOP_BAND_PX)));
        header.addView(buildPlayerBar(), new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dimen(R.dimen.lobby_player_bar_height)));
        return header;
    }

    /*
     * 顶部条: TopBkgTemplate 九宫底色, 其上叠 bigtex/img_title_hall 大图, 最上层是标题文字.
     * 该模板整图 alpha 为 128, 本身半透明, 保持原样叠加, 不再压不透明色.
     */
    private View buildTopBand() {
        final FrameLayout band = new FrameLayout(this);
        final Drawable base = LobbyArt.nineSlice(this, LobbyArt.TOP_BKG,
                NINE_TILE_LARGE, NINE_TILE_LARGE, NINE_TILE_LARGE, NINE_TILE_LARGE);
        if (base != null) {
            /*
             * 只叠模板自身: TopBkgTemplate 整图 alpha=128, 压在全局暗罩后的底图上
             * 就是半透明玻璃质感, 再垫一层实心压暗会把它变成不透明的深青板.
             */
            band.setBackground(base);
        } else {
            band.setBackground(AccountTheme.rounded(this, AccountTheme.COLOR_PAGE,
                    AccountTheme.CARD_RADIUS_DP));
        }

        final Drawable titleArt = LobbyArt.image(this, LobbyArt.TITLE_HALL);
        if (titleArt != null) {
            ImageView art = new ImageView(this);
            art.setImageDrawable(titleArt);
            art.setScaleType(ImageView.ScaleType.FIT_XY);
            band.addView(art, new FrameLayout.LayoutParams(
                    FrameLayout.LayoutParams.MATCH_PARENT,
                    LobbyArt.designSize(this, TITLE_ART_PX)));
        }

        final LinearLayout titles = new LinearLayout(this);
        titles.setOrientation(LinearLayout.VERTICAL);
        titles.setGravity(Gravity.CENTER);
        final TextView title = AccountTheme.title(this, getString(R.string.launcher_title),
                AccountTheme.COLOR_TEXT_ON_DARK);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, TEXT_TITLE);
        titles.addView(title, wrapParams(0f));

        final TextView subtitle = text(getString(R.string.launcher_subtitle),
                AccountTheme.COLOR_TEXT_ON_DARK_MUTED, TEXT_SECONDARY);
        subtitle.setGravity(Gravity.CENTER);
        titles.addView(subtitle, wrapParams(2f));

        final FrameLayout.LayoutParams titleParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        titleParams.gravity = Gravity.CENTER_VERTICAL;
        band.addView(titles, titleParams);
        return band;
    }

    /*
     * 玩家条: RoleInfoBkgTemplate 三段图做底 (左 200 / 中 10 / 右 87, 高 76),
     * 左侧头像 (有角色头像图时用图, 否则用邮箱首字母块), 中间 UID 与邮箱,
     * 右侧镜币胶囊 (TemplateBkg55) 与设置入口 (对应 $parentSetting).
     */
    private View buildPlayerBar() {
        final LinearLayout bar = new LinearLayout(this);
        bar.setOrientation(LinearLayout.HORIZONTAL);
        bar.setGravity(Gravity.CENTER_VERTICAL);
        final Drawable roleBkg = LobbyArt.centerSlice(this, LobbyArt.ROLE_INFO,
                ROLE_INFO_LEFT, ROLE_INFO_MIDDLE, ROLE_INFO_RIGHT);
        bar.setBackground(roleBkg != null ? roleBkg : AccountTheme.panelBackground(this));
        final int horizontal = AccountTheme.dp(this, 14f);
        final int vertical = AccountTheme.dp(this, 6f);
        bar.setPadding(horizontal, vertical, horizontal, vertical);

        final int avatarSize = dimen(R.dimen.lobby_avatar_size);
        bar.addView(buildAvatar(), new LinearLayout.LayoutParams(avatarSize, avatarSize));

        final LinearLayout identity = new LinearLayout(this);
        identity.setOrientation(LinearLayout.VERTICAL);
        identity.setGravity(Gravity.CENTER_VERTICAL);
        playerUid = text("", COLOR_ART_TEXT, TEXT_UID);
        playerUid.setTypeface(playerUid.getTypeface(), Typeface.BOLD);
        identity.addView(playerUid, wrapParams(0f));
        playerEmail = text("", COLOR_ART_TEXT_MUTED, TEXT_SECONDARY);
        identity.addView(playerEmail, wrapParams(1f));
        final LinearLayout.LayoutParams identityParams = new LinearLayout.LayoutParams(0,
                LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
        identityParams.leftMargin = AccountTheme.dp(this, 12f);
        identityParams.rightMargin = AccountTheme.dp(this, 8f);
        bar.addView(identity, identityParams);

        bar.addView(buildWalletChip(), new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT));

        final View divider = buildDivider();
        if (divider != null) {
            bar.addView(divider, new LinearLayout.LayoutParams(
                    LobbyArt.designSize(this, DIVIDER_WIDTH_PX),
                    LobbyArt.designSize(this, DIVIDER_HEIGHT_PX)));
        }

        final Button settings = button(false, getString(R.string.lobby_settings),
                COLOR_ART_TEXT, TEXT_SECONDARY);
        /* 顶部设置按钮是 56x56 的齿轮图标, 有素材时贴在文字左侧, 与大厅同一套视觉. */
        final Drawable settingsIcon = LobbyArt.image(this, LobbyArt.ICON_HALLTOP_SET);
        if (settingsIcon != null) {
            final int iconSize = LobbyArt.designSize(this, SETTINGS_ICON_PX);
            settingsIcon.setBounds(0, 0, iconSize, iconSize);
            settings.setCompoundDrawables(settingsIcon, null, null, null);
            settings.setCompoundDrawablePadding(AccountTheme.dp(this, 4f));
        }
        settings.setOnClickListener(view ->
                startActivity(new Intent(this, SettingsActivity.class)));
        final LinearLayout.LayoutParams settingsParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                dimen(R.dimen.lobby_secondary_button_height));
        settingsParams.leftMargin = AccountTheme.dp(this, 10f);
        bar.addView(settings, settingsParams);
        return bar;
    }

    /* 头像: 有角色头像素材时整图绘制, 缺失时回落邮箱首字母色块. */
    private View buildAvatar() {
        final Drawable portrait = LobbyArt.image(this, LobbyArt.AVATAR_ROLE);
        if (portrait != null) {
            final ImageView view = new ImageView(this);
            view.setImageDrawable(portrait);
            view.setScaleType(ImageView.ScaleType.FIT_XY);
            view.setContentDescription(firstLetter());
            return view;
        }
        final TextView avatar = new TextView(this);
        avatar.setGravity(Gravity.CENTER);
        avatar.setTextColor(AccountTheme.COLOR_TEXT_INVERSE);
        avatar.setTextSize(TypedValue.COMPLEX_UNIT_SP, TEXT_TITLE);
        avatar.setTypeface(avatar.getTypeface(), Typeface.BOLD);
        avatar.setBackground(AccountTheme.rounded(this, AccountTheme.COLOR_ACCENT_SOFT,
                AccountTheme.CARD_RADIUS_DP));
        avatar.setText(firstLetter());
        return avatar;
    }

    /* 镜币胶囊: TemplateBkg55 九宫底 (顶部迷你币数量底板), 左侧配镜币图标. */
    private View buildWalletChip() {
        final LinearLayout chip = new LinearLayout(this);
        chip.setOrientation(LinearLayout.HORIZONTAL);
        chip.setGravity(Gravity.CENTER_VERTICAL);
        final Drawable board = LobbyArt.nineSlice(this, LobbyArt.CHIP,
                NINE_TILE_LARGE, NINE_TILE_LARGE, NINE_TILE_LARGE, NINE_TILE_LARGE);
        chip.setBackground(board != null ? board
                : AccountTheme.rounded(this, AccountTheme.COLOR_ACCENT_SOFT,
                        AccountTheme.CARD_RADIUS_DP));
        final int horizontal = AccountTheme.dp(this, 8f);
        final int vertical = AccountTheme.dp(this, 3f);
        chip.setPadding(horizontal, vertical, horizontal, vertical);

        final Drawable coin = LobbyArt.image(this, LobbyArt.ICON_COIN);
        if (coin != null) {
            final ImageView icon = new ImageView(this);
            icon.setImageDrawable(coin);
            icon.setScaleType(ImageView.ScaleType.FIT_XY);
            final LinearLayout.LayoutParams iconParams = new LinearLayout.LayoutParams(
                    LobbyArt.designSize(this, COIN_ICON_PX),
                    LobbyArt.designSize(this, COIN_ICON_PX));
            iconParams.rightMargin = AccountTheme.dp(this, 6f);
            chip.addView(icon, iconParams);
        }

        playerBalance = text(getString(R.string.lobby_balance_loading),
                COLOR_ART_TEXT, TEXT_BALANCE);
        playerBalance.setTypeface(playerBalance.getTypeface(), Typeface.BOLD);
        chip.addView(playerBalance, wrapParams(0f));
        return chip;
    }

    /* 玩家条右侧竖分隔线: line_blue_hall (2x26), 素材缺失时不插入. */
    private View buildDivider() {
        final Drawable line = LobbyArt.image(this, LobbyArt.LINE_BLUE);
        if (line == null) {
            return null;
        }
        final ImageView divider = new ImageView(this);
        divider.setImageDrawable(line);
        divider.setScaleType(ImageView.ScaleType.FIT_XY);
        return divider;
    }

    /*
     * 中部主入口: 角色展示区在上, 四个入口按钮在下, 与 MiniLobbyFrameCenter 一致.
     *
     * 原版把角色模型摆在中间 ($parentRoleView 300x350), 底边一行入口按钮:
     * $parentMiniWorks (btn_blue_hall 221x88), $parentResourceShop (btn_purple_hall 160x54),
     * $parentLocalMap (btn_orange_hall 298x100, 最大的"开始游戏"), $parentMultiplayer
     * (btn_lgreen_hall 221x88). 本界面按同一顺序与同一素材摆放, 角色模型用头像大图替代.
     */
    private View buildCenterBand() {
        final LinearLayout band = new LinearLayout(this);
        band.setOrientation(LinearLayout.VERTICAL);
        band.setGravity(Gravity.CENTER_HORIZONTAL);

        band.addView(buildRoleView(), new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));
        band.addView(buildHallRow(), new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT));
        return band;
    }

    /* 角色展示区: 头像大图加玩家名, 对应 $parentRoleView; 头像缺失时只留文案. */
    private View buildRoleView() {
        final LinearLayout view = new LinearLayout(this);
        view.setOrientation(LinearLayout.VERTICAL);
        view.setGravity(Gravity.CENTER);
        view.setMinimumHeight(dimen(R.dimen.lobby_role_view_height));

        final Drawable portrait = LobbyArt.image(this, LobbyArt.AVATAR_ROLE);
        if (portrait != null) {
            final ImageView image = new ImageView(this);
            image.setImageDrawable(portrait);
            image.setScaleType(ImageView.ScaleType.FIT_CENTER);
            final int size = dimen(R.dimen.lobby_role_portrait_size);
            final LinearLayout.LayoutParams imageParams = new LinearLayout.LayoutParams(size, size);
            imageParams.gravity = Gravity.CENTER_HORIZONTAL;
            view.addView(image, imageParams);
        }

        final TextView name = text(getString(R.string.lobby_role_name), CARD_TEXT, TEXT_CARD_TITLE);
        name.setGravity(Gravity.CENTER);
        name.setShadowLayer(AccountTheme.dp(this, 4f), 0f, AccountTheme.dp(this, 1f), 0x99000000);
        view.addView(name, wrapParams(dimen(R.dimen.lobby_card_title_gap)));
        return view;
    }

    /*
     * 入口按钮行: 四个原尺寸大厅按钮横排, 顺序与素材一一对应.
     * 每个按钮的尺寸取素材原设计像素, 文字叠在按钮中央, 按钮素材缺失时回落幽灵底.
     */
    private View buildHallRow() {
        final LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER);
        final int gap = dimen(R.dimen.lobby_hall_gap);

        final Button works = hallButton(LobbyArt.HALL_BLUE, getString(R.string.lobby_mini_works),
                R.dimen.lobby_hall_wide_width, R.dimen.lobby_hall_wide_height);
        works.setOnClickListener(view -> startActivity(new Intent(this, MapEditorActivity.class)));

        final Button bag = hallButton(LobbyArt.HALL_PURPLE, getString(R.string.lobby_buddy),
                R.dimen.lobby_hall_small_width, R.dimen.lobby_hall_small_height);
        bag.setOnClickListener(view -> startActivity(new Intent(this, InventoryActivity.class)));

        final Button single = hallButton(LobbyArt.HALL_ORANGE, getString(R.string.lobby_single),
                R.dimen.lobby_hall_orange_width, R.dimen.lobby_hall_orange_height);
        single.setOnClickListener(view -> startActivity(new Intent(this, WorldListActivity.class)));

        final Button multiplayer = hallButton(LobbyArt.HALL_GREEN,
                getString(R.string.lobby_multiplayer), R.dimen.lobby_hall_wide_width,
                R.dimen.lobby_hall_wide_height);
        multiplayer.setOnClickListener(view -> openMultiplayerLobby());

        row.addView(works, hallParams(R.dimen.lobby_hall_wide_width,
                R.dimen.lobby_hall_wide_height, 0));
        row.addView(bag, hallParams(R.dimen.lobby_hall_small_width,
                R.dimen.lobby_hall_small_height, gap));
        row.addView(single, hallParams(R.dimen.lobby_hall_orange_width,
                R.dimen.lobby_hall_orange_height, gap));
        row.addView(multiplayer, hallParams(R.dimen.lobby_hall_wide_width,
                R.dimen.lobby_hall_wide_height, gap));
        return row;
    }

    /* 单个入口按钮: 素材做底, 深色文字居中, 按压时整体缩放由 AccountTheme 统一处理. */
    private Button hallButton(String assetPath, CharSequence label, int widthRes, int heightRes) {
        final Button view = new Button(this);
        view.setText(label);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, TEXT_CARD_TITLE);
        view.setTextColor(COLOR_ART_TEXT);
        view.setTypeface(view.getTypeface(), Typeface.BOLD);
        view.setGravity(Gravity.CENTER);
        view.setPadding(0, 0, 0, 0);
        view.setAllCaps(false);
        view.setMinWidth(0);
        view.setMinHeight(0);
        view.setMinimumWidth(0);
        view.setMinimumHeight(0);
        view.setBackground(AccountTheme.pressed(0x33FFFFFF, hallSurface(assetPath)));
        view.setContentDescription(label);
        return view;
    }

    /* 入口按钮底: 优先用原尺寸素材, 缺失时回落深色圆角底, 保证按钮仍然可点可读. */
    private Drawable hallSurface(String assetPath) {
        final Drawable art = LobbyArt.image(this, assetPath);
        if (art != null) {
            return art;
        }
        return AccountTheme.panelBackground(this);
    }

    private LinearLayout.LayoutParams hallParams(int widthRes, int heightRes, int leftMargin) {
        final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                dimen(widthRes), dimen(heightRes));
        params.leftMargin = leftMargin;
        params.gravity = Gravity.CENTER_VERTICAL;
        return params;
    }

    /* 联机大厅入口: 跳转联机大厅页. */
    private void openMultiplayerLobby() {
        startActivity(new Intent(this, MultiplayerLobbyActivity.class));
    }

    /*
     * 底栏: TopBkgTemplate 九宫做底条, 其上叠 bigtex/img_board_bottom 大图 (底栏大图),
     * 最上层是次入口一行, 位于$parentBottom 的按钮串位置.
     */
    private View buildBottomBand() {
        final FrameLayout band = new FrameLayout(this);
        final Drawable base = LobbyArt.nineSlice(this, LobbyArt.TOP_BKG,
                NINE_TILE_LARGE, NINE_TILE_LARGE, NINE_TILE_LARGE, NINE_TILE_LARGE);
        if (base != null) {
            band.setBackground(base);
        }

        final Drawable art = LobbyArt.image(this, LobbyArt.BOARD_BOTTOM);
        if (art != null) {
            final ImageView bottom = new ImageView(this);
            bottom.setImageDrawable(art);
            bottom.setScaleType(ImageView.ScaleType.FIT_XY);
            final FrameLayout.LayoutParams artParams = new FrameLayout.LayoutParams(
                    FrameLayout.LayoutParams.MATCH_PARENT,
                    LobbyArt.designSize(this, BOTTOM_ART_PX));
            artParams.gravity = Gravity.BOTTOM;
            band.addView(bottom, artParams);
        }

        final LinearLayout row = buildSecondaryRow();
        final FrameLayout.LayoutParams rowParams = new FrameLayout.LayoutParams(
                bandWidth(), FrameLayout.LayoutParams.WRAP_CONTENT);
        rowParams.gravity = Gravity.CENTER;
        band.addView(row, rowParams);
        return band;
    }

    /* 次入口一行: 成就, 钱包, 换绑邮箱, 玩家中心 (均为已有页面), 对应底部栏按钮串. */
    private LinearLayout buildSecondaryRow() {
        final LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        final int vertical = AccountTheme.dp(this, 7f);
        row.setPadding(0, vertical, 0, vertical);

        final Button progress = secondaryButton(getString(R.string.progress_title));
        progress.setOnClickListener(view ->
                startActivity(new Intent(this, ProgressActivity.class)));
        final Button wallet = secondaryButton(getString(R.string.wallet_title));
        wallet.setOnClickListener(view ->
                startActivity(new Intent(this, WalletActivity.class)));
        final Button rebind = secondaryButton(getString(R.string.rebind_title));
        rebind.setOnClickListener(view ->
                startActivity(new Intent(this, RebindActivity.class)));
        final Button playerCenter = secondaryButton(getString(R.string.lobby_player_center));
        playerCenter.setOnClickListener(view ->
                startActivity(new Intent(this, PlayerCenterActivity.class)));
        row.addView(progress, secondaryCellParams());
        row.addView(wallet, secondaryCellParams());
        row.addView(rebind, secondaryCellParams());
        row.addView(playerCenter, secondaryCellParams());
        return row;
    }

    /* 次入口按钮: TemplateBtn11 三段黄按钮 (左 57 / 中 16 / 右 57), 素材缺失回落幽灵底. */
    private Button secondaryButton(CharSequence label) {
        final Drawable art = LobbyArt.centerSlice(this, LobbyArt.BUTTON_MIDDLE,
                BUTTON_LEFT, BUTTON_MIDDLE, BUTTON_RIGHT);
        final Button view = AccountTheme.outlineButton(this, label, AccountTheme.COLOR_ACCENT);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, TEXT_SECONDARY);
        if (art != null) {
            view.setBackground(art);
            view.setTextColor(COLOR_ART_TEXT);
            view.setTypeface(view.getTypeface(), Typeface.BOLD);
        }
        return view;
    }

    /* 底部: 退出登录 (内联二次确认) + 版本与作者页脚, 两行文字间距收紧到 2dp. */
    private View buildFooter() {
        final LinearLayout footer = new LinearLayout(this);
        footer.setOrientation(LinearLayout.VERTICAL);
        footer.setGravity(Gravity.CENTER_HORIZONTAL);
        /*
         * 页脚同样需要托底: 版本与作者是浅色小字, 直接压在全屏大厅底图上读不清,
         * 用与公告条同一张 TemplateBkg8 深色九宫做信息条底板.
         */
        final Drawable board = LobbyArt.nineSlice(this, LobbyArt.BOARD_DARK,
                NINE_TILE_SMALL, NINE_TILE_SMALL, NINE_TILE_SMALL, NINE_TILE_SMALL);
        if (board != null) {
            footer.setBackground(board);
            final int pad = AccountTheme.dp(this, 10f);
            footer.setPadding(pad, pad, pad, pad);
        }

        logoutButton = button(false, getString(R.string.lobby_logout),
                AccountTheme.COLOR_DANGER, TEXT_SECONDARY);
        logoutButton.setOnClickListener(view -> requestLogout());
        footer.addView(logoutButton, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dimen(R.dimen.lobby_footer_button_height)));

        final TextView version = text(getString(R.string.launcher_version_format,
                BuildConfig.VERSION_NAME, BuildConfig.VERSION_CODE),
                AccountTheme.COLOR_TEXT_ON_DARK_MUTED, TEXT_FOOTER);
        version.setGravity(Gravity.CENTER);
        footer.addView(version, wrapParams(6f));

        final TextView author = text(AccountTheme.authorFooter(this),
                AccountTheme.COLOR_TEXT_ON_DARK_MUTED, TEXT_FOOTER);
        author.setGravity(Gravity.CENTER);
        footer.addView(author, wrapParams(2f));
        return footer;
    }

    /* 退出登录: 与旧入口页同一套内联二次确认, 3 秒超时回归初始态. */
    private void requestLogout() {
        if (!logoutPending) {
            logoutPending = true;
            logoutButton.setText(R.string.lobby_logout_confirm);
            handler.postDelayed(logoutReset, LOGOUT_CONFIRM_MS);
            return;
        }
        handler.removeCallbacks(logoutReset);
        logoutPending = false;
        logoutButton.setText(R.string.lobby_logout);
        final Session session = SessionStore.load(this);
        SessionStore.clear(this);
        if (session != null) {
            api.logout(session, null);
        }
        final Intent intent = new Intent(this, LauncherActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        startActivity(intent);
        finish();
    }

    /* 头像首字母: 取邮箱首字符大写; 无会话时回落占位字. */
    private String firstLetter() {
        final Session session = SessionStore.load(this);
        if (session == null || session.email() == null || session.email().isEmpty()) {
            return getString(R.string.lobby_avatar_fallback);
        }
        return session.email().substring(0, 1).toUpperCase();
    }

    /* 尺寸表读取: 只用于 dp 取值, 与 AccountTheme.dp 等价. */
    private int dimen(int resId) {
        return getResources().getDimensionPixelSize(resId);
    }

    /*
     * 中部容器宽度: $parentCenter 宽 1258 像素 (629dp) 的等宽带,
     * 屏幕比该值更窄时退化为可用宽度, 保证四宫格不被压扁.
     */
    private int bandWidth() {
        final int pagePadding = AccountTheme.dp(this, AccountTheme.PAGE_PADDING_DP);
        final int available = AccountTheme.screenWidth(this) - pagePadding * 2;
        return Math.min(available, dimen(R.dimen.lobby_grid_max_width));
    }

    /* 正文字号: 走 AccountTheme 取色与字体, 字号按大厅档位覆写, 绕开其三档收敛. */
    private TextView text(CharSequence value, int color, float sizeSp) {
        final TextView view = AccountTheme.body(this, value, color, sizeSp);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, sizeSp);
        return view;
    }

    /* 按钮: primary 用于唯一主入口, 其余走 outline 幽灵底, 字号按入口层级覆写. */
    private Button button(boolean primary, CharSequence label, int color, float sizeSp) {
        final Button view = primary ? AccountTheme.primaryButton(this, label)
                : AccountTheme.outlineButton(this, label, color);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, sizeSp);
        return view;
    }

    /* 中部区域参数: 宽度沿用 630dp 居中带, 高度取517 像素折算的 259dp. */
    private LinearLayout.LayoutParams centerParams() {
        final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                bandWidth(), dimen(R.dimen.lobby_center_height));
        params.gravity = Gravity.CENTER_HORIZONTAL;
        params.topMargin = dimen(R.dimen.lobby_block_gap);
        return params;
    }

    /* 卡内竖列参数: 整宽换行, 指定上边距, 文字居中. */
    private LinearLayout.LayoutParams centerParams(int topMargin) {
        final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.gravity = Gravity.CENTER_HORIZONTAL;
        params.topMargin = topMargin;
        return params;
    }

    /* 两色线性混合: ratio 取零全为 base, 取一全为 target, 用于压暗强调色得到卡片渐变. */
    private static int mix(int base, int target, float ratio) {
        final float t = Math.max(0f, Math.min(1f, ratio));
        final float keep = 1f - t;
        final int a = Math.round(((base >>> 24) & 0xFF) * keep + ((target >>> 24) & 0xFF) * t);
        final int r = Math.round(((base >> 16) & 0xFF) * keep + ((target >> 16) & 0xFF) * t);
        final int g = Math.round(((base >> 8) & 0xFF) * keep + ((target >> 8) & 0xFF) * t);
        final int b = Math.round((base & 0xFF) * keep + (target & 0xFF) * t);
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    /* 替换颜色透明度, 保留 RGB. */
    private static int withAlpha(int color, int alpha) {
        return (Math.max(0, Math.min(255, alpha)) << 24) | (color & 0x00FFFFFF);
    }

    private LinearLayout.LayoutParams secondaryCellParams() {
        final int gap = AccountTheme.dp(this, 6f);
        final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(0,
                dimen(R.dimen.lobby_secondary_button_height), 1f);
        params.setMargins(gap, 0, gap, 0);
        return params;
    }

    private LinearLayout.LayoutParams fullRowParams() {
        final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.topMargin = dimen(R.dimen.lobby_block_gap);
        return params;
    }

    private LinearLayout.LayoutParams bandParams() {
        final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                bandWidth(), LinearLayout.LayoutParams.WRAP_CONTENT);
        params.gravity = Gravity.CENTER_HORIZONTAL;
        params.topMargin = dimen(R.dimen.lobby_block_gap);
        return params;
    }

    private LinearLayout.LayoutParams wrapParams(float topMarginDp) {
        final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.topMargin = AccountTheme.dp(this, topMarginDp);
        return params;
    }
}
