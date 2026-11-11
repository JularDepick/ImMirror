package com.julardepick.immirror.wallet;

import android.content.Context;
import android.graphics.Typeface;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.R;
import com.julardepick.immirror.account.AccountTheme;

import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

/*
 * 本月签到日历.
 *
 * 按服务端自然月键渲染周一至周日的七列网格, 已签日期用签到色高亮,
 * 未签的今日用浅签到色描出; 服务端暂未下发逐日记录时由界面用流水反推,
 * 反推不到的日期不高亮, 只影响日历观感, 不影响签到动作.
 */
public final class CheckinCalendarView extends LinearLayout {

    /* 单元格边长与间距, 七列加间距需小于卡片内宽. */
    private static final float CELL_SIZE_DP = 36f;
    private static final float CELL_GAP_DP = 2f;

    private final LinearLayout gridHost;

    public CheckinCalendarView(Context context) {
        super(context);
        setOrientation(VERTICAL);
        setGravity(Gravity.CENTER_HORIZONTAL);
        addView(buildWeekHeader());
        gridHost = new LinearLayout(context);
        gridHost.setOrientation(VERTICAL);
        gridHost.setGravity(Gravity.CENTER_HORIZONTAL);
        addView(gridHost);
    }

    /* 整体重建网格; 数据变化时调用, 月份非法时隐藏网格. */
    public void render(String month, List<String> signedDates, boolean signedToday, String todayKey) {
        gridHost.removeAllViews();
        int[] yearMonth = parseMonth(month);
        if (yearMonth == null) {
            setVisibility(GONE);
            return;
        }
        setVisibility(VISIBLE);
        Set<String> signed = new HashSet<>();
        if (signedDates != null) {
            signed.addAll(signedDates);
        }
        /* 服务端已签但流水里找不到今日记录时, 把今日补进高亮集合. */
        if (signedToday && todayKey != null && todayKey.startsWith(month + "-")) {
            signed.add(todayKey);
        }

        Calendar first = new GregorianCalendar(yearMonth[0], yearMonth[1] - 1, 1);
        int leading = (first.get(Calendar.DAY_OF_WEEK) + 5) % 7;
        int daysInMonth = first.getActualMaximum(Calendar.DAY_OF_MONTH);

        /* 逐行铺格: 首行先补前置空位, 末行日期不足七天也补空位对齐. */
        int cells = leading + daysInMonth;
        int rows = (cells + 6) / 7;
        for (int row = 0; row < rows; row++) {
            LinearLayout line = new LinearLayout(getContext());
            line.setOrientation(HORIZONTAL);
            for (int column = 0; column < 7; column++) {
                int cell = row * 7 + column;
                if (cell < leading || cell >= cells) {
                    line.addView(placeholder(getContext()), cellParams());
                    continue;
                }
                int dayOfMonth = cell - leading + 1;
                String dateKey = month + "-" + String.format(Locale.US, "%02d", dayOfMonth);
                line.addView(buildCell(dayOfMonth, dateKey, signed.contains(dateKey),
                        dateKey.equals(todayKey)), cellParams());
            }
            gridHost.addView(line);
        }
    }

    /* 星期表头: 周一至周日, 与网格列序一致. */
    private LinearLayout buildWeekHeader() {
        String[] names = getResources().getStringArray(R.array.wallet_weekdays);
        LinearLayout header = new LinearLayout(getContext());
        header.setOrientation(HORIZONTAL);
        header.setGravity(Gravity.CENTER_HORIZONTAL);
        for (String name : names) {
            TextView cell = new TextView(getContext());
            cell.setText(name);
            cell.setTextSize(TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_SUB);
            cell.setTextColor(AccountTheme.COLOR_TEXT_MUTED);
            cell.setGravity(Gravity.CENTER);
            header.addView(cell, cellParams());
        }
        return header;
    }

    /* 日期单元格: 已签实底反白, 今日浅底强调, 其余常规文字. */
    private TextView buildCell(int dayOfMonth, String dateKey, boolean signed, boolean today) {
        TextView cell = new TextView(getContext());
        cell.setText(String.valueOf(dayOfMonth));
        cell.setTextSize(TypedValue.COMPLEX_UNIT_SP, AccountTheme.TEXT_SIZE_SUB);
        cell.setGravity(Gravity.CENTER);
        if (signed) {
            cell.setBackground(AccountTheme.rounded(getContext(), AccountTheme.COLOR_ACCENT, 18f));
            cell.setTextColor(AccountTheme.COLOR_TEXT_INVERSE);
            cell.setTypeface(cell.getTypeface(), Typeface.BOLD);
        } else if (today) {
            cell.setBackground(AccountTheme.rounded(getContext(), AccountTheme.COLOR_ACCENT_SOFT, 18f));
            cell.setTextColor(AccountTheme.COLOR_ACCENT);
            cell.setTypeface(cell.getTypeface(), Typeface.BOLD);
        } else {
            cell.setTextColor(AccountTheme.COLOR_TEXT);
        }
        return cell;
    }

    /* 空位: 补齐首末行空列, 不显示内容也不响应触摸. */
    private View placeholder(Context context) {
        View view = new View(context);
        view.setVisibility(INVISIBLE);
        view.setClickable(true);
        view.setFocusable(false);
        return view;
    }

    private LinearLayout.LayoutParams cellParams() {
        int size = AccountTheme.dp(getContext(), CELL_SIZE_DP);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(size, size);
        int gap = AccountTheme.dp(getContext(), CELL_GAP_DP) / 2;
        params.setMargins(gap, gap, gap, gap);
        return params;
    }

    /* 解析自然月键 2026-11, 非法返回 null. */
    private static int[] parseMonth(String month) {
        if (month == null) {
            return null;
        }
        int dash = month.indexOf('-');
        if (dash <= 0 || dash == month.length() - 1) {
            return null;
        }
        try {
            int year = Integer.parseInt(month.substring(0, dash));
            int mon = Integer.parseInt(month.substring(dash + 1));
            if (mon < 1 || mon > 12) {
                return null;
            }
            return new int[]{year, mon};
        } catch (NumberFormatException error) {
            return null;
        }
    }
}
