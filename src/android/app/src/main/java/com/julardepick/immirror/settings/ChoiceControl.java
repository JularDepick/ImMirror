package com.julardepick.immirror.settings;

import android.content.Context;
import android.graphics.Typeface;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.account.AccountTheme;

import java.util.ArrayList;
import java.util.List;

/*
 * 分段选择型设置行: 有限档位的枚举, 档位与文案由 SettingsCatalog 给出.
 *
 * 每个档位是一个等宽的可点击文本块, 选中项用强调色填充, 未选中项用描边底色,
 * 与登录页的顶部标签保持同一套视觉语言.
 */
public final class ChoiceControl extends SettingControl {

    private final List<TextView> segments = new ArrayList<>();
    private final int[] levels;

    public ChoiceControl(Context context, SettingsItem item, Listener listener) {
        super(context, item, listener);

        levels = new int[item.levelCount()];
        LinearLayout row = new LinearLayout(context);
        row.setOrientation(LinearLayout.HORIZONTAL);

        for (int index = 0; index < item.levelCount(); index++) {
            levels[index] = item.levelAt(index);
            final int level = levels[index];
            TextView segment = buildSegment(context, item.levelLabelAt(index));
            segment.setOnClickListener(view -> emit(level));
            LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(0,
                    AccountTheme.dp(context, 38f), 1f);
            if (index > 0) {
                params.leftMargin = AccountTheme.dp(context, 6f);
            }
            row.addView(segment, params);
            segments.add(segment);
        }

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.topMargin = AccountTheme.dp(context, 8f);
        root.addView(row, params);
        bindInitial();
    }

    @Override
    protected void showValue(int value) {
        int selected = item.levelIndex(value);
        for (int index = 0; index < segments.size(); index++) {
            style(segments.get(index), index == selected);
        }
    }

    private TextView buildSegment(Context context, String text) {
        TextView segment = AccountTheme.body(context, text, AccountTheme.COLOR_TEXT_MUTED, 12f);
        segment.setGravity(Gravity.CENTER);
        segment.setClickable(true);
        segment.setFocusable(true);
        return segment;
    }

    private void style(TextView segment, boolean selected) {
        segment.setTextColor(selected ? AccountTheme.COLOR_ACCENT_TEXT : AccountTheme.COLOR_TEXT_MUTED);
        segment.setTypeface(null, selected ? Typeface.BOLD : Typeface.NORMAL);
        if (selected) {
            segment.setBackground(AccountTheme.pressed(0x33000000, AccountTheme.rounded(context,
                    AccountTheme.COLOR_ACCENT, AccountTheme.FIELD_RADIUS_DP)));
        } else {
            segment.setBackground(AccountTheme.pressed(AccountTheme.COLOR_ACCENT_SOFT,
                    AccountTheme.stroked(context, AccountTheme.COLOR_FIELD, AccountTheme.FIELD_RADIUS_DP,
                            AccountTheme.COLOR_FIELD_STROKE, 1f)));
        }
    }
}
