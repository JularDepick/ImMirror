package com.julardepick.immirror.lobby;

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
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.PlayActivity;
import com.julardepick.immirror.R;
import com.julardepick.immirror.account.AccountTheme;
import com.julardepick.immirror.bridge.NativeBridge;
import com.julardepick.immirror.ui.MobilePage;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

/*
 * 世界列表页.
 *
 * 面板内自上而下是操作行与世界列表: 每行一个世界, 点击进游戏, 行内按钮二次确认后删除,
 * 刷新与新建世界留在面板内容里, 标题与关闭由面板标题行承担.
 */
public final class WorldListActivity extends Activity {

    private static final String TAG = "ImMirrorWorldList";

    /* 删除按钮的二次确认等待时长, 超时自动回到初始文案. */
    private static final long DELETE_CONFIRM_MS = 3000L;
    /* 世界列表显示的时间格式: yyyy-MM-dd HH:mm:ss+08:00. */
    private static final String TIME_FORMAT = "yyyy-MM-dd HH:mm:ss+08:00";

    private LinearLayout listHost;
    /* 新建世界的输入区: 内联在列表上方, 默认隐藏, 不走系统弹窗. */
    private LinearLayout createPanel;
    private EditText createInput;
    private TextView emptyView;
    private final List<WorldEntry> worlds = new ArrayList<>();

    /* 一个世界的展示数据: 列表行与删除操作共用. */
    private static final class WorldEntry {
        String name = "";
        String file = "";
        long lastPlayed = 0L;
        long seed = 0L;
        Button deleteButton = null;
        boolean deletePending = false;
    }

    private final Handler handler = new Handler(Looper.getMainLooper());

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(buildContentView());
        reload();
    }

    private View buildContentView() {
        /*
         * 面板内纵向内容: 操作行, 世界列表与作者页脚.
         * 刷新与新建世界留在面板内容里, 列表在面板内部滚动.
         */
        final LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);

        column.addView(buildActionRow());

        /* 世界列表: 空态文案与列表行放在同一容器里切换. */
        /* 输入区先建空容器, 首次点新建时才填内容, 避免每次进页面都构造输入框. */
        createPanel = new LinearLayout(this);
        createPanel.setOrientation(LinearLayout.VERTICAL);
        createPanel.setVisibility(View.GONE);
        column.addView(createPanel, AccountTheme.block(this, 8f));

        listHost = new LinearLayout(this);
        listHost.setOrientation(LinearLayout.VERTICAL);
        column.addView(listHost, AccountTheme.block(this, 12f));

        final TextView author = AccountTheme.body(this, AccountTheme.authorFooter(this),
                AccountTheme.COLOR_TEXT_FAINT, 11f);
        author.setGravity(Gravity.CENTER);
        column.addView(author, AccountTheme.block(this, 14f));

        /*
         * 外层: 场景底图 + 遮罩 + 居中面板, 标题与关闭按钮由面板标题行承担,
         * 内容在面板内部滚动.
         */
        final FrameLayout root = new FrameLayout(this);
        root.addView(MobilePage.create(this, getString(R.string.world_list_title),
                MobilePage.scrollable(this, column)), new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));
        return root;
    }

    /* 操作行: 刷新重新扫描存档目录, 新建世界带名字进游戏. */
    private View buildActionRow() {
        final LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);

        final Button refresh = AccountTheme.outlineButton(this,
                getString(R.string.world_list_refresh), AccountTheme.COLOR_ACCENT);
        refresh.setOnClickListener(view -> reload());
        row.addView(refresh, new LinearLayout.LayoutParams(0,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP), 1f));

        final Button create = AccountTheme.primaryButton(this,
                getString(R.string.launcher_world_new));
        create.setOnClickListener(view -> showCreateWorld());
        final LinearLayout.LayoutParams createParams = new LinearLayout.LayoutParams(0,
                LinearLayout.LayoutParams.WRAP_CONTENT, 1.6f);
        createParams.leftMargin = AccountTheme.dp(this, 10f);
        row.addView(create, createParams);
        return row;
    }

    /* 重新扫描世界存档目录并重建列表. */
    private void reload() {
        worlds.clear();
        try {
            final String json = NativeBridge.listWorlds(
                    getFilesDir().getAbsolutePath() + "/worlds");
            final JSONArray array = new JSONArray(json);
            final SimpleDateFormat format = new SimpleDateFormat(TIME_FORMAT, Locale.getDefault());
            for (int index = 0; index < array.length(); ++index) {
                final JSONObject item = array.getJSONObject(index);
                final WorldEntry entry = new WorldEntry();
                entry.name = item.optString("name", "default");
                entry.file = item.optString("file", "");
                entry.lastPlayed = item.optLong("lastPlayed", 0L);
                entry.seed = item.optLong("seed", 0L);
                worlds.add(entry);
            }
        } catch (Throwable error) {
            Log.w(TAG, "world list parse failed: " + error);
        }
        rebuildList();
    }

    private void rebuildList() {
        listHost.removeAllViews();
        if (worlds.isEmpty()) {
            /* 空态: 提示文案占住列表位置, 新建世界按钮在内容顶部常驻. */
            if (emptyView == null) {
                emptyView = AccountTheme.body(this, getString(R.string.world_list_empty),
                        AccountTheme.COLOR_TEXT_MUTED, 15f);
                emptyView.setGravity(Gravity.CENTER);
            }
            listHost.addView(emptyView, new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT,
                    AccountTheme.dp(this, 120f)));
            return;
        }
        for (final WorldEntry entry : worlds) {
            listHost.addView(buildRow(entry), rowParams());
        }
    }

    /* 单个世界行: 左侧信息两行 (名字 + 时间与种子), 右侧删除小按钮. */
    private View buildRow(final WorldEntry entry) {
        final LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        final int pad = AccountTheme.dp(this, 14f);
        row.setPadding(pad, pad, pad, pad);
        row.setBackground(AccountTheme.panelBackground(this));

        final LinearLayout info = new LinearLayout(this);
        info.setOrientation(LinearLayout.VERTICAL);
        final TextView name = AccountTheme.body(this, entry.name, AccountTheme.COLOR_TEXT, 17f);
        name.setTypeface(name.getTypeface(), Typeface.BOLD);
        info.addView(name);
        final String when = entry.lastPlayed > 0L
                ? getString(R.string.launcher_world_last_format,
                        new SimpleDateFormat(TIME_FORMAT, Locale.getDefault())
                                .format(new Date(entry.lastPlayed * 1000L)))
                : getString(R.string.launcher_world_last_never);
        final TextView meta = AccountTheme.body(this,
                getString(R.string.world_list_meta_format, when, entry.seed),
                AccountTheme.COLOR_TEXT_MUTED, 12f);
        info.addView(meta);
        row.addView(info, new LinearLayout.LayoutParams(0,
                LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        /* 点击进游戏: 世界列表页同时结束自身, 让游戏返回键直接回大厅.
         * 从大厅再进本页可重新选择世界. */
        row.setOnClickListener(view -> {
            final Intent intent = new Intent(this, PlayActivity.class);
            intent.putExtra(PlayActivity.EXTRA_WORLD, entry.name);
            startActivity(intent);
            finish();
        });
        final Button delete = AccountTheme.outlineButton(this, getString(R.string.world_list_delete),
                AccountTheme.COLOR_DANGER);
        delete.setOnClickListener(view -> requestDelete(entry));
        entry.deleteButton = delete;
        row.addView(delete, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP)));
        return row;
    }

    /* 长按删除走按钮内联二次确认: 第一次点变"确认删除", 3 秒内再点才删文件. */
    private void requestDelete(final WorldEntry entry) {
        if (entry.deletePending) {
            deleteWorld(entry);
            return;
        }
        entry.deletePending = true;
        if (entry.deleteButton != null) {
            entry.deleteButton.setText(R.string.world_list_delete_confirm);
        }
        handler.postDelayed(() -> {
            entry.deletePending = false;
            if (entry.deleteButton != null && entry.deleteButton.isAttachedToWindow()) {
                entry.deleteButton.setText(R.string.world_list_delete);
            }
        }, DELETE_CONFIRM_MS);
    }

    /*
     * 删除世界存档: 优先按 listWorlds 返回的 file 字段删, 字段为空时按
     * "存档目录 + 安全化名字 + .world" 推算 (与游戏侧名字安全化规则保持一致:
     * 保留中文与数字字母, 其余字符换成下划线). 删除后刷新列表.
     */
    private void deleteWorld(final WorldEntry entry) {
        File target = null;
        if (entry.file != null && !entry.file.isEmpty()) {
            target = new File(entry.file);
        } else {
            final String safe = entry.name.replaceAll("[^\\p{L}\\p{N}]+", "_");
            target = new File(getFilesDir().getAbsolutePath() + "/worlds/" + safe + ".world");
        }
        final boolean removed = target != null && target.exists() && target.delete();
        Log.i(TAG, "world delete " + entry.name + " removed=" + removed);
        reload();
    }

    /*
     * 新建世界: 面板内展开一段输入区, 不走系统弹窗.
     * 输入框与两个按钮都排在列表上方, 取消后收起, 确认后按名字进游戏.
     */
    private void showCreateWorld() {
        if (createPanel == null) {
            return;
        }
        if (createInput == null) {
            createInput = AccountTheme.field(this, getString(R.string.launcher_world_name_hint), 16f);
            createInput.setText(getString(R.string.launcher_world_default_name));
            final LinearLayout.LayoutParams inputParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
            inputParams.bottomMargin = AccountTheme.dp(this, 8f);
            createPanel.addView(createInput, inputParams);

            final LinearLayout actions = new LinearLayout(this);
            actions.setOrientation(LinearLayout.HORIZONTAL);
            final Button cancel = AccountTheme.outlineButton(this,
                    getString(R.string.settings_back), AccountTheme.COLOR_TEXT_MUTED);
            cancel.setOnClickListener(view -> hideCreatePanel());
            final Button confirm = AccountTheme.primaryButton(this,
                    getString(R.string.launcher_world_enter));
            confirm.setOnClickListener(view -> commitCreateWorld());
            final LinearLayout.LayoutParams actionParams = new LinearLayout.LayoutParams(
                    0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
            actions.addView(cancel, actionParams);
            final LinearLayout.LayoutParams confirmParams = new LinearLayout.LayoutParams(
                    0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
            confirmParams.leftMargin = AccountTheme.dp(this, 8f);
            actions.addView(confirm, confirmParams);
            createPanel.addView(actions, new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        }
        createInput.setText(getString(R.string.launcher_world_default_name));
        createInput.setSelection(createInput.getText().length());
        createPanel.setVisibility(View.VISIBLE);
        createInput.requestFocus();
    }

    /* 收起输入区并把焦点交回列表. */
    private void hideCreatePanel() {
        if (createPanel != null) {
            createPanel.setVisibility(View.GONE);
        }
    }

    /* 按输入框里的名字进游戏, 空输入回落默认名. */
    private void commitCreateWorld() {
        String name = createInput == null ? "" : createInput.getText().toString().trim();
        if (name.isEmpty()) {
            name = getString(R.string.launcher_world_default_name);
        }
        final Intent intent = new Intent(this, PlayActivity.class);
        intent.putExtra(PlayActivity.EXTRA_WORLD, name);
        startActivity(intent);
        finish();
    }

    private LinearLayout.LayoutParams rowParams() {
        final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.bottomMargin = AccountTheme.dp(this, 12f);
        return params;
    }
}
