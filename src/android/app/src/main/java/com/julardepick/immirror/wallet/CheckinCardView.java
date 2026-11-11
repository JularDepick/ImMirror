package com.julardepick.immirror.wallet;

import android.content.Context;
import android.graphics.Typeface;
import android.util.TypedValue;
import android.view.Gravity;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.account.AccountTheme;

import java.util.Locale;

/*
 * 签到卡片.
 *
 * 展示签到按钮, 今日状态与本月累计, 加上从服务端契约固化的奖励规则说明,
 * 里程碑进度与说明一并呈现; 已签后按钮禁用, 文案保持可读.
 */
public final class CheckinCardView extends LinearLayout {

    /* 签到按钮与监听. */
    private Button checkinButton;
    private TextView statusView;
    private TextView milestoneView;
    private TextView rulesView;
    private CheckinListener listener;

    /* 防抖: 请求进行中忽略重复点击, 成功或失败后恢复. */
    private boolean busy;

    public CheckinCardView(Context context) {
        super(context);
        setOrientation(VERTICAL);
        int padding = AccountTheme.dp(context, 16f);
        setPadding(padding, padding, padding, padding);
        setBackground(AccountTheme.panelBackground(context));

        TextView header = AccountTheme.body(context, context.getString(
                com.julardepick.immirror.R.string.wallet_checkin_header), AccountTheme.COLOR_TEXT, 14f);
        header.setTypeface(header.getTypeface(), Typeface.BOLD);
        addView(header);

        statusView = AccountTheme.body(context, "", AccountTheme.COLOR_TEXT_MUTED, 12f);
        addView(statusView, AccountTheme.block(context, 6f));

        checkinButton = AccountTheme.primaryButton(context, context.getString(
                com.julardepick.immirror.R.string.wallet_checkin_action));
        checkinButton.setOnClickListener(view -> {
            if (!busy && listener != null) {
                listener.onCheckinClick();
            }
        });
        addView(checkinButton, AccountTheme.block(context, 10f));

        milestoneView = AccountTheme.body(context, "", AccountTheme.COLOR_TEXT_MUTED, 12f);
        milestoneView.setGravity(Gravity.CENTER);
        addView(milestoneView, AccountTheme.block(context, 8f));

        rulesView = AccountTheme.body(context, context.getString(
                com.julardepick.immirror.R.string.wallet_checkin_rules), AccountTheme.COLOR_TEXT_FAINT, 11f);
        rulesView.setLineSpacing(AccountTheme.dp(context, 2f), 1f);
        addView(rulesView, AccountTheme.block(context, 6f));
    }

    /* 签到按钮回调. */
    public interface CheckinListener {
        void onCheckinClick();
    }

    public void setCheckinListener(CheckinListener target) {
        listener = target;
    }

    /* 用签到状态重建卡片文案与按钮可用性. */
    public void render(CheckinStatus status) {
        if (status.signedToday()) {
            String text = status.todayRewardKnown()
                    ? String.format(Locale.getDefault(),
                            getContext().getString(com.julardepick.immirror.R.string.wallet_checkin_done_reward),
                            status.todayReward())
                    : getContext().getString(com.julardepick.immirror.R.string.wallet_checkin_done);
            statusView.setText(text);
            statusView.setTextColor(AccountTheme.COLOR_SUCCESS);
            checkinButton.setEnabled(false);
            checkinButton.setText(com.julardepick.immirror.R.string.wallet_checkin_done_button);
        } else {
            String text = status.nextRewardKnown()
                    ? String.format(Locale.getDefault(),
                            getContext().getString(com.julardepick.immirror.R.string.wallet_checkin_todo_reward),
                            status.monthCount(), status.nextReward())
                    : getContext().getString(com.julardepick.immirror.R.string.wallet_checkin_todo);
            statusView.setText(text);
            statusView.setTextColor(AccountTheme.COLOR_TEXT_MUTED);
            checkinButton.setEnabled(!busy);
            checkinButton.setText(com.julardepick.immirror.R.string.wallet_checkin_action);
        }
        rebuildMilestones(status);
    }

    /* 里程碑行: 逐项列出本月第几次与奖励, 达成后换成功色. */
    private void rebuildMilestones(CheckinStatus status) {
        StringBuilder builder = new StringBuilder();
        for (CheckinStatus.Milestone milestone : status.milestones()) {
            if (builder.length() > 0) {
                builder.append("  ");
            }
            builder.append(getContext().getString(
                    com.julardepick.immirror.R.string.wallet_milestone_item,
                    milestone.count(), milestone.reward(), milestone.achieved() ? 1 : 0));
        }
        if (builder.length() > 0) {
            milestoneView.setText(builder.toString());
            milestoneView.setVisibility(VISIBLE);
        } else {
            milestoneView.setVisibility(GONE);
        }
    }

    /*
     * 签到请求期间锁按钮, 避免连点重复提交.
     * 恢复可用性交给随后的 render: 未签恢复可点, 已签保持禁用.
     */
    public void setBusy(boolean value) {
        busy = value;
        if (value) {
            checkinButton.setEnabled(false);
        }
    }
}
