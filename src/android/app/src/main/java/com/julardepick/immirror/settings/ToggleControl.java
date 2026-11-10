package com.julardepick.immirror.settings;

import android.content.Context;
import android.content.res.ColorStateList;
import android.widget.LinearLayout;
import android.widget.Switch;

import com.julardepick.immirror.R;
import com.julardepick.immirror.account.AccountTheme;

/*
 * 开关型设置行: 只有 0 与 1 两档的键.
 *
 * 取值展示在标题行右侧, 开关本身不带文字, 与木色主题的强调色一致.
 */
public final class ToggleControl extends SettingControl {

    /* 开关关闭时的轨道色, 取自卡片底色与描边之间的中间调. */
    private static final int TRACK_OFF = 0x593A2A18;

    private final Switch toggle;

    public ToggleControl(Context context, SettingsItem item, Listener listener) {
        super(context, item, listener);

        toggle = new Switch(context);
        toggle.setShowText(false);
        toggle.setThumbTintList(ColorStateList.valueOf(AccountTheme.COLOR_ACCENT));
        toggle.setTrackTintList(ColorStateList.valueOf(TRACK_OFF));

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.leftMargin = AccountTheme.dp(context, 10f);
        header.addView(toggle, params);

        toggle.setOnCheckedChangeListener((button, checked) -> emit(checked ? 1 : 0));
        bindInitial();
    }

    @Override
    protected void showValue(int value) {
        toggle.setChecked(value != 0);
        valueView.setText(value != 0 ? R.string.settings_value_on : R.string.settings_value_off);
    }
}
