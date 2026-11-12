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
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.julardepick.immirror.PlayActivity;
import com.julardepick.immirror.R;
import com.julardepick.immirror.account.AccountTheme;
import com.julardepick.immirror.bridge.NativeBridge;

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
 * 顶部返回与刷新, 中部每行一个世界, 点击进游戏, 长按删除, 底部新建.
 */
public final class WorldListActivity extends Activity {

    private static final String TAG = "ImMirrorWorldList";

    /* 删除按钮的二次确认等待时长, 超时自动回到初始文案. */
    private static final long DELETE_CONFIRM_MS = 3000L;
    /* 时间格式与旧对话框一致: yyyy-MM-dd HH:mm:ss+08:00. */
    private static final String TIME_FORMAT = "yyyy-MM-dd HH:mm:ss+08:00";

    private LinearLayout listHost;
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
        final LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(AccountTheme.COLOR_PAGE);

        /* 顶部: 返回 + 标题 + 刷新. */
        final LinearLayout header = new LinearLayout(this);
        header.setOrientation(LinearLayout.HORIZONTAL);
        header.setGravity(Gravity.CENTER_VERTICAL);
        final int pad = AccountTheme.dp(this, 14f);
        header.setPadding(pad, pad, pad, pad);
        final Button back = AccountTheme.outlineButton(this, getString(R.string.settings_back),
                AccountTheme.COLOR_ACCENT);
        back.setOnClickListener(view -> finish());
        header.addView(back, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP)));
        final TextView title = AccountTheme.title(this, getString(R.string.world_list_title),
                AccountTheme.COLOR_TEXT_ON_DARK);
        title.setGravity(Gravity.CENTER);
        header.addView(title, new LinearLayout.LayoutParams(0,
                LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        final Button refresh = AccountTheme.outlineButton(this, getString(R.string.world_list_refresh),
                AccountTheme.COLOR_ACCENT);
        refresh.setOnClickListener(view -> reload());
        header.addView(refresh, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP)));
        root.addView(header, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));

        /* 中部: 可滚动的世界列表, 空态文案放在同一容器里切换. */
        final ScrollView scroll = new ScrollView(this);
        listHost = new LinearLayout(this);
        listHost.setOrientation(LinearLayout.VERTICAL);
        final int listPad = AccountTheme.dp(this, AccountTheme.PAGE_PADDING_DP);
        listHost.setPadding(listPad, listPad, listPad, listPad);
        scroll.addView(listHost, new ScrollView.LayoutParams(
                ScrollView.LayoutParams.MATCH_PARENT, ScrollView.LayoutParams.WRAP_CONTENT));
        root.addView(scroll, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));

        /* 底部: 新建世界主按钮. */
        final Button create = AccountTheme.primaryButton(this, getString(R.string.launcher_world_new));
        create.setOnClickListener(view -> showCreateWorld());
        final LinearLayout.LayoutParams createParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                AccountTheme.dp(this, AccountTheme.BUTTON_HEIGHT_DP + 8));
        createParams.setMargins(pad, pad, pad, pad);
        root.addView(create, createParams);
        return root;
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
            /* 空态: 提示文案压住中部, 新建按钮在底部已常驻. */
            if (emptyView == null) {
                emptyView = AccountTheme.body(this, getString(R.string.world_list_empty),
                        AccountTheme.COLOR_TEXT_ON_DARK_MUTED, 15f);
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

    /* 新建世界: 输入框取名字, 空输入回落默认名, 确认后带名字进游戏. */
    private void showCreateWorld() {
        final EditText input = new EditText(this);
        input.setHint(R.string.launcher_world_name_hint);
        input.setText(getString(R.string.launcher_world_default_name));
        input.setSelection(input.getText().length());
        final LinearLayout holder = new LinearLayout(this);
        holder.setOrientation(LinearLayout.VERTICAL);
        final int padding = AccountTheme.dp(this, 20f);
        holder.setPadding(padding, padding, padding, 0);
        holder.addView(input, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));

        final android.app.AlertDialog dialog = new android.app.AlertDialog.Builder(this)
                .setTitle(getString(R.string.launcher_world_new))
                .setView(holder)
                .setNegativeButton(getString(R.string.settings_back), null)
                .setPositiveButton(getString(R.string.launcher_world_enter), null)
                .create();
        dialog.show();
        /* 接管确认按钮: 保留输入框语义, 点击后按名字进游戏. */
        dialog.getButton(android.app.AlertDialog.BUTTON_POSITIVE).setOnClickListener(view -> {
            String name = input.getText().toString().trim();
            if (name.isEmpty()) {
                name = getString(R.string.launcher_world_default_name);
            }
            dialog.dismiss();
            final Intent intent = new Intent(this, PlayActivity.class);
            intent.putExtra(PlayActivity.EXTRA_WORLD, name);
            startActivity(intent);
            finish();
        });
    }

    private LinearLayout.LayoutParams rowParams() {
        final LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.bottomMargin = AccountTheme.dp(this, 12f);
        return params;
    }
}
