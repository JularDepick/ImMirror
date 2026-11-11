package com.julardepick.immirror.wallet;

import android.content.Context;
import android.graphics.Typeface;
import android.util.TypedValue;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.account.AccountTheme;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

/*
 * 单条流水行.
 *
 * 左侧事由加时间, 右侧带正负号与颜色的金额和事后余额,
 * 支出行与入账行共用本视图, 只换文字与配色.
 */
public final class LedgerRowView extends LinearLayout {

    /* 流水时间展示格式, 与界面规范一致, 秒级精度. */
    private static final String TIME_FORMAT = "yyyy-MM-dd HH:mm:ss";

    private final TextView title;
    private final TextView subtitle;
    private final TextView amount;
    private final TextView balance;

    public LedgerRowView(Context context) {
        super(context);
        setOrientation(HORIZONTAL);
        setGravity(Gravity.CENTER_VERTICAL);
        int vertical = AccountTheme.dp(context, 10f);
        setPadding(0, vertical, 0, vertical);

        LinearLayout left = new LinearLayout(context);
        left.setOrientation(VERTICAL);

        title = AccountTheme.body(context, "", AccountTheme.COLOR_TEXT, 13f);
        title.setTypeface(title.getTypeface(), Typeface.BOLD);
        left.addView(title);

        subtitle = AccountTheme.body(context, "", AccountTheme.COLOR_TEXT_FAINT, 11f);
        left.addView(subtitle, AccountTheme.block(context, 2f));

        LinearLayout right = new LinearLayout(context);
        right.setOrientation(VERTICAL);
        right.setGravity(Gravity.END);

        amount = AccountTheme.body(context, "", AccountTheme.COLOR_SUCCESS, 13f);
        amount.setTypeface(amount.getTypeface(), Typeface.BOLD);
        right.addView(amount);

        balance = AccountTheme.body(context, "", AccountTheme.COLOR_TEXT_MUTED, 11f);
        right.addView(balance, AccountTheme.block(context, 2f));

        addView(left, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1f));
        addView(right, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT));
    }

    /* 填充一条流水; reason 未收录时直接展示服务端原始事由. */
    public void bind(LedgerEntry entry) {
        title.setText(reasonTitle(entry.reason()));
        subtitle.setText(formatTime(entry.createdAt()));
        String amountText = (entry.income() ? "+" : "-") + Math.abs(entry.delta());
        amount.setText(amountText);
        amount.setTextColor(entry.income() ? AccountTheme.COLOR_SUCCESS : AccountTheme.COLOR_ERROR);
        balance.setText(String.format(Locale.getDefault(), "%d", entry.balanceAfter()));
    }

    /* 事由映射: 与服务端钱包流水 reason 列取值一致. */
    private static String reasonTitle(String reason) {
        if (LedgerEntry.REASON_CHECKIN.equals(reason)) {
            return "每日签到";
        }
        if (LedgerEntry.REASON_CHECKIN_MILESTONE.equals(reason)) {
            return "签到里程碑";
        }
        if (LedgerEntry.REASON_ACHIEVEMENT.equals(reason)) {
            return "成就奖励";
        }
        if (LedgerEntry.REASON_DAILY_TASK.equals(reason)) {
            return "每日任务";
        }
        return reason;
    }

    private static String formatTime(long createdAt) {
        if (createdAt <= 0L) {
            return "";
        }
        return new SimpleDateFormat(TIME_FORMAT, Locale.getDefault()).format(new Date(createdAt));
    }
}
