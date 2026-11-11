package com.julardepick.immirror.editor;

import android.content.Context;
import android.graphics.Typeface;
import android.view.Gravity;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.account.AccountTheme;

/*
 * 属性面板.
 *
 * 面板只做展示与转发: 显示当前方块, 光标坐标, 选区尺寸与撤销深度,
 * 三个坐标各有一对加减按钮用于逐格挪动光标, 选区按钮把当前光标记成 A 点或 B 点.
 * 面板不持有任何编辑状态, 每次命令返回后由界面统一回填, 因此原生侧始终是唯一事实来源.
 */
public final class EditorPropertyPanel extends LinearLayout {

    /* 坐标步进按钮与行内按钮的尺寸, 比主按钮矮一档, 避免面板过高. */
    private static final float STEPPER_WIDTH_DP = 42f;
    private static final float STEPPER_HEIGHT_DP = 38f;
    private static final float ROW_HEIGHT_DP = 40f;

    public interface Listener {
        void onStepCursor(int axis, int delta);

        void onMarkPoint(boolean second);

        void onClearSelection();

        void onFillSelection();

        void onUndo();

        void onRedo();
    }

    private final Context context;
    private final Listener listener;

    private final TextView blockView;
    private final TextView cursorView;
    private final TextView selectionView;
    private final TextView historyView;
    private final TextView dirtyView;
    private final TextView[] axisValues = new TextView[3];

    private final Button pointAButton;
    private final Button pointBButton;
    private final Button clearButton;
    private final Button fillButton;
    private final Button undoButton;
    private final Button redoButton;

    public EditorPropertyPanel(Context context, Listener listener) {
        super(context);
        this.context = context;
        this.listener = listener;
        setOrientation(VERTICAL);
        int horizontal = AccountTheme.dp(context, 16f);
        setPadding(horizontal, AccountTheme.dp(context, 12f), horizontal,
                AccountTheme.dp(context, 14f));
        setBackground(AccountTheme.panelBackground(context));

        TextView title = AccountTheme.body(context, EditorText.PANEL_TITLE,
                AccountTheme.COLOR_ACCENT, 15f);
        title.setTypeface(title.getTypeface(), Typeface.BOLD);
        addView(title);

        blockView = AccountTheme.body(context, "", AccountTheme.COLOR_TEXT, 18f);
        addView(blockView, AccountTheme.block(context, 6f));

        cursorView = AccountTheme.body(context, "", AccountTheme.COLOR_TEXT, 18f);
        addView(cursorView, AccountTheme.block(context, 2f));

        for (int axis = 0; axis < 3; axis++) {
            addView(buildAxisRow(axis), AccountTheme.block(context, 4f));
        }

        selectionView = AccountTheme.body(context, EditorText.PANEL_SELECTION_EMPTY,
                AccountTheme.COLOR_TEXT, 18f);
        addView(selectionView, AccountTheme.block(context, 8f));

        LinearLayout selectionRow = new LinearLayout(context);
        selectionRow.setOrientation(HORIZONTAL);
        pointAButton = rowButton(EditorText.POINT_A, AccountTheme.COLOR_ACCENT);
        pointAButton.setOnClickListener(view -> notifyMark(false));
        selectionRow.addView(pointAButton, weightParams(1f, false));
        pointBButton = rowButton(EditorText.POINT_B, AccountTheme.COLOR_ACCENT);
        pointBButton.setOnClickListener(view -> notifyMark(true));
        selectionRow.addView(pointBButton, weightParams(1f, true));
        clearButton = rowButton(EditorText.SELECTION_CLEAR, AccountTheme.COLOR_TEXT_MUTED);
        clearButton.setOnClickListener(view -> notifyClear());
        selectionRow.addView(clearButton, weightParams(1.4f, true));
        fillButton = rowButton(EditorText.SELECTION_FILL, AccountTheme.COLOR_TEXT);
        fillButton.setOnClickListener(view -> notifyFill());
        selectionRow.addView(fillButton, weightParams(1.4f, true));
        addView(selectionRow, AccountTheme.block(context, 6f));

        historyView = AccountTheme.body(context, "", AccountTheme.COLOR_TEXT_MUTED, 15f);
        addView(historyView, AccountTheme.block(context, 10f));

        LinearLayout historyRow = new LinearLayout(context);
        historyRow.setOrientation(HORIZONTAL);
        undoButton = rowButton(EditorText.ACTION_UNDO, AccountTheme.COLOR_TEXT);
        undoButton.setOnClickListener(view -> {
            if (listener != null) {
                listener.onUndo();
            }
        });
        historyRow.addView(undoButton, weightParams(1f, false));
        redoButton = rowButton(EditorText.ACTION_REDO, AccountTheme.COLOR_TEXT);
        redoButton.setOnClickListener(view -> {
            if (listener != null) {
                listener.onRedo();
            }
        });
        historyRow.addView(redoButton, weightParams(1f, true));
        addView(historyRow, AccountTheme.block(context, 4f));

        dirtyView = AccountTheme.body(context, "", AccountTheme.COLOR_TEXT_FAINT, 13f);
        addView(dirtyView, AccountTheme.block(context, 8f));
    }

    /* 回填原生侧状态; 传 null 表示原生接口不可用, 面板显示占位并禁用按钮. */
    public void bind(EditorReply status) {
        boolean open = status != null && status.intValue("open", 0) != 0;
        String blockLabel = status == null ? ""
                : status.first("blockLabel", status.first("block", "-"));
        blockView.setText(String.format(EditorText.PANEL_BLOCK_FORMAT,
                blockLabel.isEmpty() ? "-" : blockLabel));

        int[] cursor = status == null ? null : status.triple("cursor");
        if (cursor == null) {
            cursor = new int[]{0, 0, 0};
            cursorView.setText(String.format(EditorText.PANEL_CURSOR_FORMAT, 0, 0, 0));
        } else {
            cursorView.setText(String.format(EditorText.PANEL_CURSOR_FORMAT,
                    cursor[0], cursor[1], cursor[2]));
        }
        for (int axis = 0; axis < 3; axis++) {
            axisValues[axis].setText(String.valueOf(cursor[axis]));
        }

        boolean selectionSet = status != null && status.intValue("selectionSet", 0) != 0;
        int[] size = status == null ? null : status.triple("selectionSize");
        if (selectionSet && size != null) {
            selectionView.setText(String.format(EditorText.PANEL_SELECTION_FORMAT,
                    Math.abs(size[0]), Math.abs(size[1]), Math.abs(size[2])));
        } else {
            selectionView.setText(EditorText.PANEL_SELECTION_EMPTY);
        }

        int undo = status == null ? 0 : status.intValue("undo", 0);
        int redo = status == null ? 0 : status.intValue("redo", 0);
        historyView.setText(String.format(EditorText.PANEL_HISTORY_FORMAT, undo, redo));
        dirtyView.setText(status != null && status.intValue("dirty", 0) != 0
                ? EditorText.PANEL_DIRTY : EditorText.PANEL_CLEAN);

        boolean canUndo = open && undo > 0;
        boolean canRedo = open && redo > 0;
        undoButton.setEnabled(canUndo);
        redoButton.setEnabled(canRedo);
        undoButton.setAlpha(canUndo ? 1f : 0.55f);
        redoButton.setAlpha(canRedo ? 1f : 0.55f);

        pointAButton.setEnabled(open);
        pointBButton.setEnabled(open);
        clearButton.setEnabled(open && selectionSet);
        fillButton.setEnabled(open && selectionSet);
        for (int axis = 0; axis < 3; axis++) {
            LinearLayout row = (LinearLayout) axisValues[axis].getParent();
            for (int index = 0; index < row.getChildCount(); index++) {
                row.getChildAt(index).setEnabled(open);
            }
        }
    }

    private LinearLayout buildAxisRow(int axis) {
        LinearLayout row = new LinearLayout(context);
        row.setOrientation(HORIZONTAL);
        row.setGravity(Gravity.CENTER_VERTICAL);

        String[] labels = {EditorText.AXIS_LABEL_X, EditorText.AXIS_LABEL_Y,
                EditorText.AXIS_LABEL_Z};
        TextView label = AccountTheme.body(context, labels[axis], AccountTheme.COLOR_TEXT_MUTED,
                16f);
        LinearLayout.LayoutParams labelParams = new LinearLayout.LayoutParams(
                AccountTheme.dp(context, 22f), LinearLayout.LayoutParams.WRAP_CONTENT);
        row.addView(label, labelParams);

        Button minus = stepperButton(EditorText.STEP_MINUS);
        minus.setOnClickListener(view -> notifyStep(axis, -1));
        row.addView(minus);

        axisValues[axis] = AccountTheme.body(context, "0", AccountTheme.COLOR_TEXT, 18f);
        axisValues[axis].setGravity(Gravity.CENTER);
        LinearLayout.LayoutParams valueParams = new LinearLayout.LayoutParams(0,
                LinearLayout.LayoutParams.WRAP_CONTENT, 1f);
        row.addView(axisValues[axis], valueParams);

        Button plus = stepperButton(EditorText.STEP_PLUS);
        plus.setOnClickListener(view -> notifyStep(axis, 1));
        row.addView(plus);
        return row;
    }

    private Button stepperButton(String text) {
        Button button = AccountTheme.outlineButton(context, text, AccountTheme.COLOR_TEXT);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                AccountTheme.dp(context, STEPPER_WIDTH_DP),
                AccountTheme.dp(context, STEPPER_HEIGHT_DP));
        button.setLayoutParams(params);
        button.setMinHeight(AccountTheme.dp(context, STEPPER_HEIGHT_DP));
        button.setMinimumHeight(AccountTheme.dp(context, STEPPER_HEIGHT_DP));
        return button;
    }

    private Button rowButton(String text, int color) {
        Button button = AccountTheme.outlineButton(context, text, color);
        button.setMinHeight(AccountTheme.dp(context, ROW_HEIGHT_DP));
        button.setMinimumHeight(AccountTheme.dp(context, ROW_HEIGHT_DP));
        return button;
    }

    private LinearLayout.LayoutParams weightParams(float weight, boolean leftMargin) {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(0,
                AccountTheme.dp(context, ROW_HEIGHT_DP), weight);
        if (leftMargin) {
            params.leftMargin = AccountTheme.dp(context, 8f);
        }
        return params;
    }

    private void notifyStep(int axis, int delta) {
        if (listener != null) {
            listener.onStepCursor(axis, delta);
        }
    }

    private void notifyMark(boolean second) {
        if (listener != null) {
            listener.onMarkPoint(second);
        }
    }

    private void notifyClear() {
        if (listener != null) {
            listener.onClearSelection();
        }
    }

    private void notifyFill() {
        if (listener != null) {
            listener.onFillSelection();
        }
    }
}
