package com.julardepick.immirror.lobby;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.R;
import com.julardepick.immirror.account.AccountTheme;
import com.julardepick.immirror.account.Session;
import com.julardepick.immirror.account.SessionStore;
import com.julardepick.immirror.net.LanDiscovery;
import com.julardepick.immirror.ui.MobilePage;

import java.util.ArrayList;
import java.util.List;

/*
 * 联机大厅.
 *
 * 面板内自上而下是状态提示, 扫描状态, 房间列表区与底部按钮行, 末尾是作者页脚.
 * 标题与关闭由面板标题行承担, 房间列表随面板一起滚动, 列表项全部可见.
 */
public final class MultiplayerLobbyActivity extends Activity {

    private static final String TAG = "ImMirrorLobby";

    /* 创建按钮内联确认的等待时长, 超时自动回到初始按钮状态. */
    private static final long CREATE_CONFIRM_MS = 3000L;

    private LanDiscovery discovery;
    private TextView noticeView;
    private TextView scanStateView;
    private LinearLayout roomListHost;
    private TextView roomEmptyView;
    private Button createButton;
    private boolean createConfirmPending;

    /* 最近一次扫描发现的服务端地址, 房间服就位后按地址逐个拉房间列表. */
    private final List<String> discoveredServers = new ArrayList<>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        discovery = new LanDiscovery(this);
        setContentView(buildPage());
        refresh();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    /*
     * 面板内纵向内容: 状态提示, 扫描状态, 房间列表区, 底部按钮行与作者页脚.
     * 房间列表不再自带滚动容器, 由面板的滚动承担.
     */
    private View buildPage() {
        final LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);

        /* 提示条与扫描状态. */
        noticeView = AccountTheme.notice(this);
        noticeView.setTextColor(AccountTheme.COLOR_NOTICE_TEXT);
        noticeView.setVisibility(View.GONE);
        column.addView(noticeView, AccountTheme.block(this, 2f));

        scanStateView = AccountTheme.body(this, getString(R.string.lobby_room_scanning),
                AccountTheme.COLOR_TEXT_MUTED, AccountTheme.TEXT_SIZE_SUB);
        column.addView(scanStateView, AccountTheme.block(this, 6f));

        /* 房间列表区: 空态文案与动态房间卡片共用一个容器. */
        roomEmptyView = AccountTheme.body(this, getString(R.string.lobby_room_empty),
                AccountTheme.COLOR_TEXT_MUTED, AccountTheme.TEXT_SIZE_BODY);
        roomEmptyView.setGravity(Gravity.CENTER);
        roomEmptyView.setPadding(0, AccountTheme.dp(this, 48f), 0, 0);

        roomListHost = new LinearLayout(this);
        roomListHost.setOrientation(LinearLayout.VERTICAL);
        roomListHost.addView(roomEmptyView);
        column.addView(roomListHost, AccountTheme.block(this, 12f));

        /* 底部按钮行: 刷新与创建房间. */
        final LinearLayout actions = new LinearLayout(this);
        actions.setOrientation(LinearLayout.HORIZONTAL);
        final Button refresh = AccountTheme.outlineButton(this, getString(R.string.lobby_refresh),
                AccountTheme.COLOR_ACCENT);
        refresh.setOnClickListener(view -> refresh());
        createButton = AccountTheme.primaryButton(this, getString(R.string.lobby_create_room));
        createButton.setOnClickListener(view -> onCreateRoomClicked());
        actions.addView(refresh, new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f));
        final LinearLayout.LayoutParams createParams = new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f);
        createParams.leftMargin = AccountTheme.dp(this, 10f);
        actions.addView(createButton, createParams);
        final LinearLayout.LayoutParams actionsParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        actionsParams.topMargin = AccountTheme.dp(this, 12f);
        column.addView(actions, actionsParams);

        final TextView author = AccountTheme.body(this, AccountTheme.authorFooter(this),
                AccountTheme.COLOR_TEXT_FAINT, 11f);
        author.setGravity(Gravity.CENTER);
        column.addView(author, AccountTheme.block(this, 14f));

        /*
         * 外层: 场景底图 + 遮罩 + 居中面板, 标题与关闭按钮由面板标题行承担,
         * 内容在面板内部滚动.
         */
        final FrameLayout root = new FrameLayout(this);
        root.addView(MobilePage.create(this, getString(R.string.lobby_title),
                MobilePage.scrollable(this, column)), new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
        return root;
    }

    /* 扫描入口: 局域网发现, 回调在主线程, 空结果直接落空态. */
    private void refresh() {
        discoveredServers.clear();
        roomListHost.removeAllViews();
        roomListHost.addView(roomEmptyView);
        roomEmptyView.setText(getString(R.string.lobby_room_scanning));
        scanStateView.setText(getString(R.string.lobby_room_scanning));
        discovery.discover(new LanDiscovery.Callback() {
            @Override
            public void onFound(java.util.Set<String> addresses) {
                discoveredServers.clear();
                discoveredServers.addAll(addresses);
            }

            @Override
            public void onFinished(int found) {
                onScanFinished(found);
            }
        });
    }

    /*
     * 扫描结束: found 为 0 表示局域网内没有服务端, 落空态;
     * 找到服务端则逐个尝试拉房间列表, 房间服未部署 (404 或网络错误)
     * 时按空态处理并记日志, 界面不崩溃.
     */
    private void onScanFinished(int found) {
        scanStateView.setText(getString(R.string.lobby_room_scanning).replace("...", "")
                + (found > 0 ? " " + found : "") );
        if (found == 0) {
            roomEmptyView.setText(getString(R.string.lobby_room_empty));
            showNotice(null);
            Log.i(TAG, "lan scan finished, no server found");
            return;
        }
        Log.i(TAG, "lan scan finished, servers=" + found);
        /*
         * 房间列表接口尚未部署: 这里按"发现服务端但无房间服务"降级,
         * 保持空态文案, 并把发现的服务端地址留在日志里供联机接入时核对.
         */
        for (String server : discoveredServers) {
            Log.i(TAG, "server discovered: " + server);
        }
        roomEmptyView.setText(getString(R.string.lobby_room_empty));
        showNotice(getString(R.string.lobby_server_unavailable), AccountTheme.COLOR_WARNING);
    }

    /* 创建房间点击: 先内联二次确认, 再按会话与房间服可用性降级. */
    private void onCreateRoomClicked() {
        if (!createConfirmPending) {
            /* 内联确认: 3 秒内再按才真正创建, 超时回归初始状态. */
            createConfirmPending = true;
            createButton.setText(getString(R.string.settings_reset_confirm));
            createButton.postDelayed(() -> {
                if (createConfirmPending) {
                    createConfirmPending = false;
                    createButton.setText(getString(R.string.lobby_create_room));
                }
            }, CREATE_CONFIRM_MS);
            return;
        }
        createConfirmPending = false;
        createButton.setText(getString(R.string.lobby_create_room));
        /* 房间服未部署: 提示并记日志, 不崩溃. 输入房间名的入口保留, 服务端就位后接上. */
        final Session session = SessionStore.load(this);
        Log.i(TAG, "create room requested, session=" + (session != null && session.isUsable())
                + " servers=" + discoveredServers.size());
        showNotice(getString(R.string.lobby_server_unavailable), AccountTheme.COLOR_WARNING);
    }

    /* 页内提示条; 空文本隐藏, 与账号界面的 NoticeBar 语义一致. */
    private void showNotice(String text, int color) {
        if (text == null || text.isEmpty()) {
            noticeView.setVisibility(View.GONE);
            return;
        }
        noticeView.setVisibility(View.VISIBLE);
        noticeView.setText(text);
        noticeView.setTextColor(color);
    }

    private void showNotice(String text) {
        showNotice(text, AccountTheme.COLOR_TEXT_ON_DARK_MUTED);
    }
}
