package com.julardepick.immirror.inventory;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;

import com.julardepick.immirror.account.AccountTheme;

/*
 * 物品栏网格: 上四行背包, 空一行, 再一行快捷栏.
 *
 * 版式与数据表一致, 9 列 5 行, 格子边长随控件宽度等比缩放, 因此在各种屏宽下都保持正方形.
 * 子视图按槽位号一一对应地创建一次, 之后只在下发快照时改写内容, 网格本身不增删视图,
 * 滚动与点击目标都不会因为物品变化而错位.
 *
 * 格位与原生槽位的对应关系: 子视图下标即槽位号, 0 到 8 画在最下面一行 (快捷栏),
 * 9 到 44 画在上面的四行 (背包), 与 game::Inventory 的槽位布局完全一致.
 */
public final class InventoryGridView extends ViewGroup {

    /* 每行格数, 与九列布局一致. */
    public static final int COLUMNS = 9;

    /* 点击回调; 页面据此更新详情面板并回写原生侧. */
    public interface Listener {
        void onSlotTapped(int slot);

        void onSlotLongPressed(int slot);
    }

    private final InventorySlotView[] slotViews = new InventorySlotView[InventorySnapshot.SLOT_COUNT];
    private final int spacing;
    private final int sectionGap;

    private Listener listener;
    private InventorySnapshot snapshot = InventorySnapshot.empty();
    private InventoryCatalog catalog = new InventoryCatalog();

    public InventoryGridView(Context context) {
        super(context);
        spacing = AccountTheme.dp(context, 4f);
        sectionGap = AccountTheme.dp(context, 12f);
        setWillNotDraw(true);
        for (int slot = 0; slot < InventorySnapshot.SLOT_COUNT; slot++) {
            final int index = slot;
            InventorySlotView view = new InventorySlotView(context);
            view.setFocusable(true);
            view.setClickable(true);
            view.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View target) {
                    if (listener != null) {
                        listener.onSlotTapped(index);
                    }
                }
            });
            view.setOnLongClickListener(new OnLongClickListener() {
                @Override
                public boolean onLongClick(View target) {
                    if (listener != null) {
                        listener.onSlotLongPressed(index);
                    }
                    return true;
                }
            });
            slotViews[slot] = view;
            addView(view);
        }
        bindAll();
    }

    public void setListener(Listener listener) {
        this.listener = listener;
    }

    /* 换一份快照与物品表, 整页重绑一次; 打开背包与每次操作后各调用一次. */
    public void setSnapshot(InventorySnapshot snapshot, InventoryCatalog catalog) {
        this.snapshot = snapshot == null ? InventorySnapshot.empty() : snapshot;
        this.catalog = catalog == null ? new InventoryCatalog() : catalog;
        bindAll();
        invalidate();
    }

    public InventorySnapshot snapshot() {
        return snapshot;
    }

    public InventoryCatalog catalog() {
        return catalog;
    }

    /* 槽位对应的格子视图, 供页面做滚动定位与无障碍聚焦. */
    public InventorySlotView slotView(int slot) {
        return slotViews[InventorySnapshot.clampSlot(slot)];
    }

    private void bindAll() {
        for (int slot = 0; slot < InventorySnapshot.SLOT_COUNT; slot++) {
            InventorySlot entry = snapshot.slot(slot);
            InventoryItem item = entry.isEmpty() ? InventoryItem.EMPTY : catalog.get(entry.item);
            boolean selected = InventorySnapshot.isHotbarSlot(slot) && snapshot.selected() == slot;
            slotViews[slot].bind(slot, item, entry.count, selected,
                    InventorySnapshot.isHotbarSlot(slot));
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = MeasureSpec.getSize(widthMeasureSpec);
        int heightMode = MeasureSpec.getMode(heightMeasureSpec);
        int height = MeasureSpec.getSize(heightMeasureSpec);
        int cell = cellSize(width);
        int contentHeight = contentHeight(cell);
        int measuredWidth = resolveSize(width, widthMeasureSpec);
        int measuredHeight = heightMode == MeasureSpec.EXACTLY ? height
                : resolveSize(contentHeight, heightMeasureSpec);
        int cellWidth = MeasureSpec.makeMeasureSpec(cell, MeasureSpec.EXACTLY);
        for (InventorySlotView view : slotViews) {
            view.measure(cellWidth, cellWidth);
        }
        setMeasuredDimension(measuredWidth, measuredHeight);
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
        int width = right - left;
        int cell = cellSize(width);
        int hotbarTop = hotbarTop(cell);
        for (int slot = 0; slot < InventorySnapshot.SLOT_COUNT; slot++) {
            int column = slot % COLUMNS;
            int x = column * (cell + spacing);
            int y;
            if (InventorySnapshot.isHotbarSlot(slot)) {
                y = hotbarTop;
            } else {
                int row = (slot - InventorySnapshot.HOTBAR_SLOTS) / COLUMNS;
                y = row * (cell + spacing);
            }
            slotViews[slot].layout(x, y, x + cell, y + cell);
        }
    }

    /* 正方形边长: 九列加八条间距占满宽度. */
    private int cellSize(int width) {
        if (width <= 0) {
            return 0;
        }
        int usable = width - spacing * (COLUMNS - 1);
        return Math.max(usable / COLUMNS, 0);
    }

    /* 五行的总高度: 背包四行, 中间空一行, 快捷栏一行. */
    private int contentHeight(int cell) {
        if (cell <= 0) {
            return 0;
        }
        return cell * 5 + spacing * 4 + sectionGap;
    }

    private int hotbarTop(int cell) {
        return cell * 4 + spacing * 4 + sectionGap;
    }
}
