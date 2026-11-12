package com.julardepick.immirror.inventory;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.account.AccountTheme;

import java.util.Locale;
import com.julardepick.immirror.account.NoticeBar;
import com.julardepick.immirror.ui.MobilePage;

/*
 * 背包界面.
 *
 * 一页同时显示 36 格背包与 9 格快捷栏: 面板内自上而下是概览行, 格子网格,
 * 选中格详情与操作按钮, 内容在面板内部滚动, 标题与关闭由面板标题行承担.
 * 物品数据每次打开页面时从原生侧取一次快照, 之后每次操作都重新取,
 * 因此界面永远是原生物品栏的镜像, 不存在需要对齐的两份状态.
 *
 * 原生桥接未接线时整页退回预览数据, 页面仍可点选, 丢弃与移动只改内存副本,
 * 便于脱离原生单独联调界面; 概览行会出现"预览数据"角标区分这两种模式.
 *
 * 全程不使用 Toast 与系统弹窗: 提示走页面内提示条, 丢弃这类破坏性操作
 * 按项目规范改成按钮二次确认, 三秒内不再按就回到初始文案.
 */
public final class InventoryActivity extends Activity implements InventoryGridView.Listener {

    private static final String TAG = "ImMirrorInventoryUi";

    /* 二次确认的等待时长, 超时后按钮回到初始文案. */
    private static final long CONFIRM_TIMEOUT_MS = 3000L;

    private final Handler handler = new Handler(Looper.getMainLooper());

    private InventoryGridView grid;
    private InventoryCatalog catalog = new InventoryCatalog();
    private InventorySnapshot snapshot = InventorySnapshot.empty();

    private TextView summaryView;
    private TextView badgeView;
    private TextView detailTitleView;
    private TextView detailBodyView;
    private Button dropButton;
    private Button moveButton;
    private NoticeBar noticeBar;

    private int selectedSlot = InventorySnapshot.NO_SELECTION;
    private boolean previewMode;
    private boolean dropPending;

    private final Runnable dropTimeout = new Runnable() {
        @Override
        public void run() {
            dropPending = false;
            if (dropButton != null) {
                dropButton.setText(InventoryText.ACTION_DROP);
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(buildPage());
        reload();
    }

    @Override
    protected void onResume() {
        super.onResume();
        /* 回到前台时原生可能已经因为拾取或合成改变了槽位, 重新取一次快照. */
        reload();
    }

    @Override
    protected void onDestroy() {
        handler.removeCallbacks(dropTimeout);
        if (noticeBar != null) {
            noticeBar.clear();
        }
        super.onDestroy();
    }

    /* ---------------- 页面搭建 ---------------- */

    private View buildPage() {
        /*
         * 面板内纵向内容: 概览行, 格子网格, 选中格详情与作者页脚.
         * 网格与快捷栏的绘制方式不变, 只是外层由整页换成面板.
         */
        LinearLayout column = new LinearLayout(this);
        column.setOrientation(LinearLayout.VERTICAL);

        column.addView(buildOverviewRow());
        column.addView(buildGridArea(), AccountTheme.block(this, 10f));
        column.addView(buildDetailPanel(), AccountTheme.block(this, 12f));

        TextView author = AccountTheme.body(this, AccountTheme.authorFooter(this),
                AccountTheme.COLOR_TEXT_FAINT, 11f);
        author.setGravity(Gravity.CENTER);
        column.addView(author, AccountTheme.block(this, 14f));

        /*
         * 外层: 场景底图 + 遮罩 + 居中面板, 标题与关闭按钮由面板标题行承担,
         * 内容在面板内部滚动.
         */
        FrameLayout root = new FrameLayout(this);
        root.addView(MobilePage.create(this, InventoryText.PAGE_TITLE,
                MobilePage.scrollable(this, column)), new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));

        /* 提示条浮在面板底部, 不参与内容滚动, 内容再长也不会把它挤出屏幕. */
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

    /* 概览行: 格位说明, 已用格数与预览角标, 标题本身由面板标题行承担. */
    private View buildOverviewRow() {
        LinearLayout overview = new LinearLayout(this);
        overview.setOrientation(LinearLayout.VERTICAL);
        overview.addView(AccountTheme.subtitle(this, InventoryText.PAGE_SUBTITLE));

        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);

        summaryView = AccountTheme.body(this, "", AccountTheme.COLOR_TEXT_MUTED, 12f);
        row.addView(summaryView, new LinearLayout.LayoutParams(0,
                ViewGroup.LayoutParams.WRAP_CONTENT, 1f));

        badgeView = AccountTheme.body(this, InventoryText.PREVIEW_BADGE,
                AccountTheme.COLOR_WARNING, 12f);
        badgeView.setPadding(AccountTheme.dp(this, 8f), AccountTheme.dp(this, 2f),
                AccountTheme.dp(this, 8f), AccountTheme.dp(this, 2f));
        badgeView.setBackground(AccountTheme.rounded(this, AccountTheme.COLOR_ACCENT_SOFT, 6f));
        badgeView.setVisibility(View.GONE);
        row.addView(badgeView);

        overview.addView(row, AccountTheme.block(this, 4f));
        return overview;
    }

    private View buildGridArea() {
        FrameLayout host = new FrameLayout(this);
        host.setBackground(AccountTheme.panelBackground(this));
        int padding = AccountTheme.dp(this, 10f);
        host.setPadding(padding, padding, padding, padding);

        grid = new InventoryGridView(this);
        grid.setListener(this);
        host.addView(grid, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT,
                Gravity.TOP));
        return host;
    }

    /* 选中格详情与操作按钮, 排在网格下方, 高度随内容. */
    private View buildDetailPanel() {
        LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL);
        panel.setBackground(AccountTheme.panelBackground(this));
        int padding = AccountTheme.dp(this, 12f);
        panel.setPadding(padding, padding, padding, padding);

        detailTitleView = AccountTheme.label(this, InventoryText.DETAIL_EMPTY);
        panel.addView(detailTitleView);
        panel.addView(AccountTheme.spacer(this, 6f));

        detailBodyView = AccountTheme.body(this, "", AccountTheme.COLOR_TEXT_MUTED, 12f);
        panel.addView(detailBodyView);

        panel.addView(AccountTheme.spacer(this, 8f));

        dropButton = AccountTheme.outlineButton(this, InventoryText.ACTION_DROP,
                AccountTheme.COLOR_DANGER);
        dropButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                onDropClicked();
            }
        });
        panel.addView(dropButton);

        moveButton = AccountTheme.outlineButton(this, InventoryText.ACTION_MOVE_HOTBAR,
                AccountTheme.COLOR_ACCENT);
        moveButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                onMoveToHotbarClicked();
            }
        });
        LinearLayout.LayoutParams moveParams = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        moveParams.topMargin = AccountTheme.dp(this, 8f);
        panel.addView(moveButton, moveParams);

        Button closeButton = AccountTheme.primaryButton(this, InventoryText.ACTION_CLOSE);
        closeButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                finish();
            }
        });
        LinearLayout.LayoutParams closeParams = new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        closeParams.topMargin = AccountTheme.dp(this, 8f);
        panel.addView(closeButton, closeParams);
        return panel;
    }

    /* ---------------- 数据同步 ---------------- */

    /*
     * 从原生侧取物品表与槽位快照; 原生未接线时退回预览数据.
     * 物品表解析为空也退回预览数据, 否则整页只剩占位色块, 看不出问题出在哪一层.
     */
    private void reload() {
        boolean nativeReady = InventoryBridge.isNativeReady();
        previewMode = !nativeReady;
        InventoryCatalog loaded = nativeReady ? InventoryBridge.catalog() : new InventoryCatalog();
        if (loaded.isEmpty()) {
            loaded = InventoryPreviewData.catalog();
            previewMode = true;
        }
        catalog = loaded;
        snapshot = nativeReady ? InventoryBridge.snapshot() : InventoryPreviewData.snapshot();

        if (selectedSlot == InventorySnapshot.NO_SELECTION
                || snapshot.slot(selectedSlot).isEmpty()) {
            selectedSlot = firstNonEmptySlot();
        }
        applySnapshot();
        Log.i(TAG, "背包载入, 原生就绪=" + nativeReady + " 已用格=" + snapshot.usedSlots()
                + " 物品条目=" + catalog.size());
        if (previewMode) {
            showNotice(InventoryText.NOTICE_NATIVE_MISSING, AccountTheme.COLOR_WARNING);
        }
    }

    private int firstNonEmptySlot() {
        for (int slot = 0; slot < InventorySnapshot.SLOT_COUNT; slot++) {
            if (!snapshot.slot(slot).isEmpty()) {
                return slot;
            }
        }
        return InventorySnapshot.NO_SELECTION;
    }

    /* 把当前快照与选中项铺到界面上. */
    private void applySnapshot() {
        grid.setSnapshot(snapshot, catalog);
        summaryView.setText(String.format(Locale.ROOT, InventoryText.PAGE_SUMMARY_FORMAT,
                snapshot.usedSlots(), InventorySnapshot.SLOT_COUNT));
        badgeView.setVisibility(previewMode ? View.VISIBLE : View.GONE);
        updateDetail();
    }

    private void updateDetail() {
        InventorySlot entry = selectedSlot == InventorySnapshot.NO_SELECTION
                ? new InventorySlot() : snapshot.slot(selectedSlot);
        if (entry.isEmpty()) {
            detailTitleView.setText(InventoryText.DETAIL_EMPTY);
            detailBodyView.setText("");
            dropButton.setEnabled(false);
            moveButton.setEnabled(false);
            return;
        }
        InventoryItem item = catalog.get(entry.item);
        detailTitleView.setText(item.label());
        StringBuilder body = new StringBuilder();
        body.append(String.format(InventoryText.DETAIL_CATEGORY_FORMAT,
                InventoryCatalog.categoryLabel(item.category)));
        body.append('\n');
        body.append(String.format(InventoryText.DETAIL_COUNT_FORMAT, entry.count, item.maxStack));
        if (item.block >= 0) {
            body.append('\n');
            body.append(String.format(InventoryText.DETAIL_BLOCK_FORMAT, item.block));
        }
        if (!item.key.isEmpty()) {
            body.append('\n');
            body.append(String.format(InventoryText.DETAIL_KEY_FORMAT, item.key));
        }
        detailBodyView.setText(body.toString());
        dropButton.setEnabled(true);
        moveButton.setEnabled(!InventorySnapshot.isHotbarSlot(selectedSlot));
    }

    /* ---------------- 交互 ---------------- */

    @Override
    public void onSlotTapped(int slot) {
        selectedSlot = slot;
        if (InventorySnapshot.isHotbarSlot(slot)) {
            /* 选中快捷栏格要同步给原生侧, 底部 HUD 的选中框与这里保持同一格. */
            if (!previewMode) {
                InventoryBridge.selectHotbar(slot);
            }
            snapshot.setSelected(slot);
        }
        grid.setSnapshot(snapshot, catalog);
        updateDetail();
    }

    @Override
    public void onSlotLongPressed(int slot) {
        /* 长按等价于选中并直接把物品挪进第一个空的快捷栏格. */
        selectedSlot = slot;
        onMoveToHotbarClicked();
    }

    private void onDropClicked() {
        InventorySlot entry = currentEntry();
        if (entry.isEmpty()) {
            showNotice(InventoryText.NOTICE_SLOT_EMPTY, AccountTheme.COLOR_ERROR);
            return;
        }
        if (!dropPending) {
            dropPending = true;
            dropButton.setText(InventoryText.ACTION_DROP_CONFIRM);
            handler.postDelayed(dropTimeout, CONFIRM_TIMEOUT_MS);
            return;
        }
        handler.removeCallbacks(dropTimeout);
        dropPending = false;
        dropButton.setText(InventoryText.ACTION_DROP);

        InventoryItem item = catalog.get(entry.item);
        boolean applied = previewMode
                ? dropInPreview(selectedSlot, 1)
                : InventoryBridge.dropSlot(selectedSlot, 1);
        if (!applied) {
            showNotice(InventoryText.NOTICE_FAILED, AccountTheme.COLOR_ERROR);
            return;
        }
        if (!previewMode) {
            snapshot = InventoryBridge.snapshot();
        }
        applySnapshot();
        showNotice(String.format(InventoryText.NOTICE_DROP_FORMAT, item.label(), 1),
                AccountTheme.COLOR_SUCCESS);
    }

    private void onMoveToHotbarClicked() {
        InventorySlot entry = currentEntry();
        if (entry.isEmpty()) {
            showNotice(InventoryText.NOTICE_SLOT_EMPTY, AccountTheme.COLOR_ERROR);
            return;
        }
        int target = firstEmptyHotbarSlot();
        if (target < 0) {
            showNotice(InventoryText.NOTICE_HOTBAR_FULL, AccountTheme.COLOR_ERROR);
            return;
        }
        InventoryItem item = catalog.get(entry.item);
        if (InventorySnapshot.isHotbarSlot(selectedSlot)) {
            showNotice(String.format(InventoryText.NOTICE_HOTBAR_ONLY_FORMAT, selectedSlot + 1),
                    AccountTheme.COLOR_SUCCESS);
            return;
        }
        boolean applied = previewMode
                ? moveInPreview(selectedSlot, target)
                : InventoryBridge.moveSlot(selectedSlot, target);
        if (!applied) {
            showNotice(InventoryText.NOTICE_FAILED, AccountTheme.COLOR_ERROR);
            return;
        }
        selectedSlot = target;
        if (!previewMode) {
            snapshot = InventoryBridge.snapshot();
        }
        applySnapshot();
        showNotice(String.format(InventoryText.NOTICE_MOVED_FORMAT, item.label()),
                AccountTheme.COLOR_SUCCESS);
    }

    private InventorySlot currentEntry() {
        return selectedSlot == InventorySnapshot.NO_SELECTION
                ? new InventorySlot() : snapshot.slot(selectedSlot);
    }

    /* 第一个空的快捷栏格, 全满返回 -1. */
    private int firstEmptyHotbarSlot() {
        for (int slot = 0; slot < InventorySnapshot.HOTBAR_SLOTS; slot++) {
            if (snapshot.slot(slot).isEmpty()) {
                return slot;
            }
        }
        return -1;
    }

    /* 预览模式的丢弃: 只改内存副本, 行为与原生侧的扣减一致. */
    private boolean dropInPreview(int slot, int count) {
        InventorySlot entry = snapshot.slot(slot);
        if (entry.isEmpty() || count <= 0) {
            return false;
        }
        entry.count -= count;
        if (entry.count <= 0) {
            entry.clear();
        }
        return true;
    }

    /* 预览模式的移动: 目标为空则搬过去, 同种物品先合并再搬剩余. */
    private boolean moveInPreview(int from, int to) {
        InventorySlot source = snapshot.slot(from);
        InventorySlot target = snapshot.slot(to);
        if (source.isEmpty() || from == to) {
            return false;
        }
        if (target.isEmpty()) {
            target.set(source.item, source.count);
            source.clear();
            return true;
        }
        if (target.item == source.item) {
            int room = catalog.get(target.item).maxStack - target.count;
            int moved = Math.min(room, source.count);
            if (moved <= 0) {
                return false;
            }
            target.count += moved;
            source.count -= moved;
            if (source.count <= 0) {
                source.clear();
            }
            return true;
        }
        int item = target.item;
        int count = target.count;
        target.set(source.item, source.count);
        source.set(item, count);
        return true;
    }

    private void showNotice(String text, int color) {
        if (noticeBar == null) {
            return;
        }
        noticeBar.show(text, color);
    }
}
