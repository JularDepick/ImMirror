package com.julardepick.immirror.lobby;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.PlayActivity;
import com.julardepick.immirror.R;
import com.julardepick.immirror.account.AccountTheme;
import com.julardepick.immirror.account.ApiException;
import com.julardepick.immirror.account.Session;
import com.julardepick.immirror.account.SessionStore;
import com.julardepick.immirror.net.RoomApi;
import com.julardepick.immirror.ui.MobilePage;

import java.util.ArrayList;
import java.util.List;

/*
 * 联机大厅.
 *
 * 面板内自上而下是状态提示, 列表状态, 房间列表区与底部按钮行, 末尾是作者页脚.
 * 房间列表来自账号服务端的房间通道: 进入与刷新各拉一次, 点房间即按房间号加入,
 * 底部创建按钮内联确认后建房并直接进游戏. 房主权威的方块同步在房内由 PlayActivity 承担.
 */
public final class MultiplayerLobbyActivity extends Activity {

    private static final String TAG = "ImMirrorLobby";

    /* 创建按钮内联确认的等待时长, 超时自动回到初始按钮状态. */
    private static final long CREATE_CONFIRM_MS = 3000L;

    /* 建房时申请的人数上限, 服务端允许 2 到 16. */
    private static final int CREATE_MAX_PLAYERS = 8;

    private RoomApi roomApi;
    private Session session;
    private TextView noticeView;
    private TextView stateView;
    private LinearLayout roomListHost;
    private TextView roomEmptyView;
    private Button createButton;
    private boolean createConfirmPending;
    private boolean busy;
    /* 带密码房间的输入框: 平时隐藏, 点到有密码的房间时展开. */
    private EditText passwordField;

    /* 最近一次拉取到的房间, 供列表重绘与加入时取房间号. */
    private final List<RoomApi.Room> rooms = new ArrayList<>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        roomApi = new RoomApi(this);
        session = SessionStore.load(this);
        setContentView(buildPage());
        refresh();
    }

    @Override
    protected void onResume() {
        super.onResume();
        /* 从对局返回时房间状态可能已变化, 回到本页就重新拉一次. */
        refresh();
    }

    /*
     * 面板内纵向内容: 状态提示, 列表状态, 房间列表区, 底部按钮行与作者页脚.
     * 房间列表不单独滚动, 由面板的滚动承担.
     */
    private View buildPage() {
        final LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);

        noticeView = AccountTheme.notice(this);
        noticeView.setTextColor(AccountTheme.COLOR_NOTICE_TEXT);
        noticeView.setVisibility(View.GONE);
        column.addView(noticeView, AccountTheme.block(this, 2f));

        stateView = AccountTheme.body(this, getString(R.string.lobby_room_scanning),
                AccountTheme.COLOR_TEXT_MUTED, AccountTheme.TEXT_SIZE_SUB);
        column.addView(stateView, AccountTheme.block(this, 6f));

        roomEmptyView = AccountTheme.body(this, getString(R.string.lobby_room_empty),
                AccountTheme.COLOR_TEXT_MUTED, AccountTheme.TEXT_SIZE_BODY);
        roomEmptyView.setGravity(Gravity.CENTER);
        roomEmptyView.setPadding(0, AccountTheme.dp(this, 48f), 0, 0);

        roomListHost = new LinearLayout(this);
        roomListHost.setOrientation(LinearLayout.VERTICAL);
        roomListHost.addView(roomEmptyView);
        column.addView(roomListHost, AccountTheme.block(this, 12f));

        passwordField = AccountTheme.field(this, getString(R.string.lobby_room_password_hint),
                AccountTheme.TEXT_SIZE_BODY);
        passwordField.setVisibility(View.GONE);
        column.addView(passwordField, AccountTheme.block(this, 8f));

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

        final FrameLayout root = new FrameLayout(this);
        root.addView(MobilePage.create(this, getString(R.string.lobby_title),
                MobilePage.scrollable(this, column)), new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
        return root;
    }

    /* 刷新房间列表: 会话不可用时直接落提示, 不发请求. */
    private void refresh() {
        if (session == null || !session.isUsable()) {
            showNotice(getString(R.string.lobby_room_list_failed), AccountTheme.COLOR_WARNING);
            renderRooms(new ArrayList<>());
            stateView.setText(getString(R.string.lobby_room_empty));
            return;
        }
        stateView.setText(getString(R.string.lobby_room_scanning));
        roomApi.list(session, new RoomApi.ListCallback() {
            @Override
            public void onSuccess(List<RoomApi.Room> listed) {
                Log.i(TAG, "rooms listed, count=" + listed.size());
                stateView.setText(getString(R.string.lobby_room_count_format, listed.size()));
                renderRooms(listed);
            }

            @Override
            public void onFailure(ApiException error) {
                Log.w(TAG, "room list failed: " + error.code() + " " + error.detail());
                stateView.setText(getString(R.string.lobby_room_empty));
                renderRooms(new ArrayList<>());
                showNotice(getString(R.string.lobby_room_list_failed), AccountTheme.COLOR_WARNING);
            }
        });
    }

    /* 房间列表重绘: 空列表落空态, 否则每行一个可点卡片. */
    private void renderRooms(List<RoomApi.Room> listed) {
        rooms.clear();
        rooms.addAll(listed);
        roomListHost.removeAllViews();
        if (rooms.isEmpty()) {
            roomEmptyView.setText(getString(R.string.lobby_room_empty));
            roomListHost.addView(roomEmptyView);
            return;
        }
        for (RoomApi.Room room : rooms) {
            roomListHost.addView(buildRoomRow(room), AccountTheme.block(this, 8f));
        }
    }

    /* 单行房间卡片: 名称, 房间号, 房主与人数; 整行可点即加入. */
    private View buildRoomRow(final RoomApi.Room room) {
        final LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setBackground(AccountTheme.panelBackground(this));
        final int padding = AccountTheme.dp(this, 12f);
        card.setPadding(padding, padding, padding, padding);

        final TextView name = AccountTheme.label(this, room.name);
        card.addView(name);

        String codeLine = getString(R.string.lobby_room_code_format, room.code)
                + "  " + getString(R.string.lobby_room_players_format, room.memberCount,
                room.maxPlayers);
        if (room.hasPassword) {
            codeLine += "  " + getString(R.string.lobby_room_locked);
        }
        final TextView meta = AccountTheme.body(this, codeLine,
                AccountTheme.COLOR_TEXT_MUTED, AccountTheme.TEXT_SIZE_SUB);
        card.addView(meta, AccountTheme.block(this, 4f));

        final TextView host = AccountTheme.body(this,
                getString(R.string.lobby_room_host_format, room.ownerUid),
                AccountTheme.COLOR_TEXT_FAINT, AccountTheme.TEXT_SIZE_SUB);
        card.addView(host, AccountTheme.block(this, 2f));

        card.setOnClickListener(view -> joinRoom(room));
        return card;
    }

    /* 加入房间: 成功后带房间标识进入对局, 失败按错误码给提示. */
    private void joinRoom(final RoomApi.Room room) {
        if (busy) {
            return;
        }
        /*
         * 有密码的房间先要密码: 第一次点击只展开输入框并提示,
         * 输入后再点同一个房间才真正加入.
         */
        String password = "";
        if (room.hasPassword) {
            password = passwordField.getText().toString().trim();
            if (password.isEmpty()) {
                passwordField.setVisibility(View.VISIBLE);
                passwordField.requestFocus();
                showNotice(getString(R.string.lobby_room_password_required),
                        AccountTheme.COLOR_WARNING);
                return;
            }
        }
        busy = true;
        showNotice(getString(R.string.lobby_room_joining));
        roomApi.join(session, room.code, password, new RoomApi.JoinCallback() {
            @Override
            public void onSuccess(RoomApi.Room joined, long seq) {
                busy = false;
                Log.i(TAG, "room joined, code=" + joined.code + " members=" + joined.memberCount);
                openPlay(joined);
            }

            @Override
            public void onFailure(ApiException error) {
                busy = false;
                Log.w(TAG, "room join failed: " + error.code() + " " + error.detail());
                String message = getString(R.string.lobby_room_join_failed);
                if ("ROOM_FULL".equals(error.code())) {
                    message = getString(R.string.lobby_room_full);
                } else if ("ROOM_PASSWORD_INVALID".equals(error.code())) {
                    message = getString(R.string.lobby_room_password_invalid);
                    passwordField.setText("");
                    passwordField.setVisibility(View.VISIBLE);
                } else if ("ROOM_PASSWORD_REQUIRED".equals(error.code())) {
                    message = getString(R.string.lobby_room_password_required);
                    passwordField.setVisibility(View.VISIBLE);
                }
                showNotice(message, AccountTheme.COLOR_WARNING);
            }
        });
    }

    /* 创建房间点击: 先内联二次确认, 再建房并直接进入. */
    private void onCreateRoomClicked() {
        if (!createConfirmPending) {
            createConfirmPending = true;
            createButton.setText(getString(R.string.lobby_create_confirm));
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
        if (session == null || !session.isUsable()) {
            showNotice(getString(R.string.lobby_room_create_failed), AccountTheme.COLOR_WARNING);
            return;
        }
        if (busy) {
            return;
        }
        busy = true;
        roomApi.create(session, "", CREATE_MAX_PLAYERS, new RoomApi.RoomCallback() {
            @Override
            public void onSuccess(RoomApi.Room room) {
                busy = false;
                Log.i(TAG, "room created, code=" + room.code + " seed=" + room.seed);
                openPlay(room);
            }

            @Override
            public void onFailure(ApiException error) {
                busy = false;
                Log.w(TAG, "room create failed: " + error.code() + " " + error.detail());
                showNotice(getString(R.string.lobby_room_create_failed), AccountTheme.COLOR_WARNING);
            }
        });
    }

    /* 进入对局: 房间号与种子随附加字段下发, 原生侧据此生成同一份地形. */
    private void openPlay(RoomApi.Room room) {
        final Intent intent = new Intent(this, PlayActivity.class);
        intent.putExtra(PlayActivity.EXTRA_ROOM_ID, room.id);
        intent.putExtra(PlayActivity.EXTRA_ROOM_CODE, room.code);
        intent.putExtra(PlayActivity.EXTRA_ROOM_SEED, (int) room.seed);
        /* 房主身份决定进局后是上行生物快照还是应用对方下发的快照. */
        intent.putExtra(PlayActivity.EXTRA_ROOM_OWNER,
                session != null && session.uid() != null && session.uid().equals(room.ownerUid));
        startActivity(intent);
        finish();
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
