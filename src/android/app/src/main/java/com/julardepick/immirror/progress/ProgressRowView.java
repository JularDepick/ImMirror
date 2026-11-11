package com.julardepick.immirror.progress;

import android.content.Context;
import android.graphics.Typeface;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.account.AccountTheme;

/*
 * 成就与任务列表的单行控件.
 *
 * 一行包含名称, 描述, 进度条, 进度文案, 状态文案与操作按钮,
 * 成就行与任务行只有文案与按钮语义不同, 因此共用这一种行控件.
 * 行与行之间画一条淡分隔线, 避免长列表连成一片.
 */
public final class ProgressRowView extends LinearLayout {

    /* 行内操作回调, 由界面层决定按钮点下去做什么. */
    public interface ActionListener {
        void onAction();
    }

    private final TextView titleView;
    private final TextView stateView;
    private final TextView descView;
    private final TextView progressView;
    private final ProgressBarView barView;
    private final Button actionButton;

    public ProgressRowView(Context context) {
        super(context);
        setOrientation(VERTICAL);
        int horizontal = AccountTheme.dp(context, 2f);
        int vertical = AccountTheme.dp(context, 10f);
        setPadding(horizontal, vertical, horizontal, vertical);

        LinearLayout headerRow = new LinearLayout(context);
        headerRow.setOrientation(HORIZONTAL);
        headerRow.setGravity(Gravity.CENTER_VERTICAL);

        titleView = new TextView(context);
        titleView.setTextColor(AccountTheme.COLOR_TEXT);
        titleView.setTextSize(TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_SUB);
        titleView.setTypeface(Typeface.DEFAULT_BOLD);
        titleView.setSingleLine(true);
        headerRow.addView(titleView, new LinearLayout.LayoutParams(0,
                LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        stateView = new TextView(context);
        stateView.setTextColor(AccountTheme.COLOR_TEXT_MUTED);
        stateView.setTextSize(TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_SUB);
        stateView.setGravity(Gravity.END);
        headerRow.addView(stateView);
        addView(headerRow);

        descView = new TextView(context);
        descView.setTextColor(AccountTheme.COLOR_TEXT_MUTED);
        descView.setTextSize(TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_SUB);
        descView.setMaxLines(2);
        descView.setLineSpacing(AccountTheme.dp(context, 2f), 1f);
        LinearLayout.LayoutParams descParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        descParams.topMargin = AccountTheme.dp(context, 3f);
        addView(descView, descParams);

        LinearLayout footerRow = new LinearLayout(context);
        footerRow.setOrientation(HORIZONTAL);
        footerRow.setGravity(Gravity.CENTER_VERTICAL);
        LinearLayout.LayoutParams footerParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        footerParams.topMargin = AccountTheme.dp(context, 8f);

        barView = new ProgressBarView(context);
        footerRow.addView(barView, new LinearLayout.LayoutParams(0,
                LinearLayout.LayoutParams.WRAP_CONTENT, 1f));

        progressView = new TextView(context);
        progressView.setTextColor(AccountTheme.COLOR_TEXT_MUTED);
        progressView.setTextSize(TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_SUB);
        LinearLayout.LayoutParams progressParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        progressParams.leftMargin = AccountTheme.dp(context, 8f);
        footerRow.addView(progressView, progressParams);

        actionButton = AccountTheme.outlineButton(context, ProgressText.ACTION_CLAIM,
                AccountTheme.COLOR_ACCENT);
        actionButton.setTextSize(TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_SUB);
        actionButton.setSingleLine(true);
        LinearLayout.LayoutParams actionParams = new LinearLayout.LayoutParams(
                AccountTheme.dp(context, 88f), AccountTheme.dp(context, 34f));
        actionParams.leftMargin = AccountTheme.dp(context, 10f);
        footerRow.addView(actionButton, actionParams);

        addView(footerRow, footerParams);

        View divider = new View(context);
        divider.setBackgroundColor(0x14000000);
        LinearLayout.LayoutParams dividerParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, Math.max(1, AccountTheme.dp(context, 1f)));
        dividerParams.topMargin = AccountTheme.dp(context, 10f);
        addView(divider, dividerParams);
    }

    /*
     * 绑定一次展示数据. 进度条比例按目标值折算, 目标为 0 时按 1 处理;
     * 操作按钮在本行不需要动手时隐藏, 保持列表干净.
     */
    public void bind(String title, String description, int progress, int target, boolean completed,
                     String stateText, String actionText, boolean actionEnabled,
                     final ActionListener listener) {
        titleView.setText(title);
        descView.setText(description == null ? "" : description);
        descView.setVisibility(description == null || description.isEmpty() ? GONE : VISIBLE);
        stateView.setText(stateText == null ? "" : stateText);
        stateView.setTextColor(completed ? AccountTheme.COLOR_SUCCESS : AccountTheme.COLOR_TEXT_MUTED);

        int safeTarget = Math.max(target, 1);
        int safeProgress = Math.max(0, Math.min(progress, safeTarget));
        barView.setFinished(completed);
        barView.setRatio((float) safeProgress / (float) safeTarget);
        progressView.setText(ProgressText.progressOf(safeProgress, safeTarget, ""));

        if (actionText == null || actionText.isEmpty()) {
            actionButton.setVisibility(GONE);
            actionButton.setOnClickListener(null);
            return;
        }
        actionButton.setVisibility(VISIBLE);
        actionButton.setText(actionText);
        actionButton.setEnabled(actionEnabled);
        actionButton.setAlpha(actionEnabled ? 1f : 0.55f);
        actionButton.setOnClickListener(view -> {
            if (listener != null) {
                listener.onAction();
            }
        });
    }
}
