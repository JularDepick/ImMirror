package com.julardepick.immirror.editor;

import android.content.Context;
import android.graphics.Typeface;
import android.view.Gravity;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.account.AccountTheme;

import java.util.ArrayList;
import java.util.List;

/*
 * 方块栏. 横向滚动的方块胶囊列表, 点击即把当前方块下发给原生编辑器.
 *
 * 方块条目来自原生命令 palette, 行格式为 key|中文名|方块 id;
 * 选中态用描边加底色区分, 与游戏内方块栏的选中反馈保持同一套语义:
 * 选中是一个持续状态, 不是在条目上弹提示.
 */
public final class EditorPaletteView extends LinearLayout {

    /* 选中条的填充色, 与强调色同一色系, 数值集中在界面层便于换肤. */
    private static final int COLOR_CHIP_SELECTED = 0xFFE8F8FF;

    /* 方块胶囊的水平与垂直内边距. */
    private static final float CHIP_PADDING_H_DP = 12f;
    private static final float CHIP_PADDING_V_DP = 7f;
    private static final float CHIP_MARGIN_DP = 5f;
    private static final float CHIP_RADIUS_DP = 10f;

    /* 方块条目的只读快照. */
    public static final class Entry {
        private final String key;
        private final String label;
        private final int blockId;

        public Entry(String key, String label, int blockId) {
            this.key = key;
            this.label = label;
            this.blockId = blockId;
        }

        public String key() {
            return key;
        }

        public String label() {
            return label;
        }

        public int blockId() {
            return blockId;
        }
    }

    public interface Listener {
        void onBlockSelected(Entry entry);
    }

    private final Listener listener;
    private final LinearLayout row;
    private final TextView hintView;
    private final List<Entry> entries = new ArrayList<>();
    private String selectedKey = "";

    public EditorPaletteView(Context context, Listener listener) {
        super(context);
        this.listener = listener;
        setOrientation(VERTICAL);
        int padding = AccountTheme.dp(context, 16f);
        setPadding(padding, AccountTheme.dp(context, 12f), padding, AccountTheme.dp(context, 14f));
        setBackground(AccountTheme.panelBackground(context));

        TextView title = AccountTheme.body(context, EditorText.PALETTE_TITLE,
                AccountTheme.COLOR_ACCENT, 15f);
        title.setTypeface(title.getTypeface(), Typeface.BOLD);
        addView(title);

        hintView = AccountTheme.body(context, EditorText.PALETTE_EMPTY,
                AccountTheme.COLOR_TEXT_FAINT, 12f);
        addView(hintView, AccountTheme.block(context, 4f));

        HorizontalScrollView scroll = new HorizontalScrollView(context);
        scroll.setHorizontalScrollBarEnabled(false);
        scroll.setFillViewport(false);
        row = new LinearLayout(context);
        row.setOrientation(HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);
        scroll.addView(row, new HorizontalScrollView.LayoutParams(
                HorizontalScrollView.LayoutParams.WRAP_CONTENT,
                HorizontalScrollView.LayoutParams.WRAP_CONTENT));
        addView(scroll, AccountTheme.block(context, 6f));
    }

    /* 用最新的方块表重建胶囊; 传入空表时保留提示文案并清空内容. */
    public void setEntries(List<Entry> newEntries) {
        entries.clear();
        if (newEntries != null) {
            entries.addAll(newEntries);
        }
        row.removeAllViews();
        hintView.setVisibility(entries.isEmpty() ? VISIBLE : GONE);
        Context context = getContext();
        for (Entry entry : entries) {
            TextView chip = createChip(context, entry);
            LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
            params.rightMargin = AccountTheme.dp(context, CHIP_MARGIN_DP);
            row.addView(chip, params);
        }
        applySelection();
    }

    /* 同步原生侧的选中方块; 未在表中时只记录不改界面. */
    public void setSelected(String key) {
        selectedKey = key == null ? "" : key;
        applySelection();
    }

    public String selected() {
        return selectedKey;
    }

    private TextView createChip(Context context, Entry entry) {
        TextView chip = AccountTheme.body(context, entry.label(), AccountTheme.COLOR_TEXT, 18f);
        chip.setGravity(Gravity.CENTER);
        int horizontal = AccountTheme.dp(context, CHIP_PADDING_H_DP);
        int vertical = AccountTheme.dp(context, CHIP_PADDING_V_DP);
        chip.setPadding(horizontal, vertical, horizontal, vertical);
        chip.setClickable(true);
        chip.setFocusable(true);
        chip.setTag(entry.key());
        chip.setOnClickListener(view -> {
            if (listener != null) {
                listener.onBlockSelected(entry);
            }
        });
        return chip;
    }

    private void applySelection() {
        Context context = getContext();
        for (int index = 0; index < row.getChildCount(); index++) {
            TextView chip = (TextView) row.getChildAt(index);
            String key = String.valueOf(chip.getTag());
            boolean selected = key.equals(selectedKey);
            chip.setTextColor(selected ? AccountTheme.COLOR_ACCENT : AccountTheme.COLOR_TEXT_MUTED);
            chip.setBackground(selected
                    ? AccountTheme.stroked(context, COLOR_CHIP_SELECTED, CHIP_RADIUS_DP,
                            AccountTheme.COLOR_ACCENT, 1f)
                    : AccountTheme.stroked(context, AccountTheme.COLOR_FIELD, CHIP_RADIUS_DP,
                            AccountTheme.COLOR_FIELD_STROKE, 1f));
        }
    }
}
