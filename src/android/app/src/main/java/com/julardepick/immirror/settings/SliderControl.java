package com.julardepick.immirror.settings;

import android.content.Context;
import android.content.res.ColorStateList;
import android.widget.LinearLayout;
import android.widget.SeekBar;

import com.julardepick.immirror.account.AccountTheme;

/*
 * 滑杆型设置行: 0 到 100 这类连续取值.
 *
 * 拖动过程中每一项改动都立即上报, 页面逐次写入原生设置;
 * 原生侧只置脏标记, 真正落盘由主循环低频完成, 因此拖动不会造成频繁写文件.
 */
public final class SliderControl extends SettingControl {

    private final SeekBar bar;

    public SliderControl(Context context, SettingsItem item, Listener listener) {
        super(context, item, listener);

        bar = new SeekBar(context);
        bar.setMax(Math.max(1, item.max() - item.min()));
        bar.setProgressTintList(ColorStateList.valueOf(AccountTheme.COLOR_ACCENT));
        bar.setThumbTintList(ColorStateList.valueOf(AccountTheme.COLOR_ACCENT));
        bar.setProgressBackgroundTintList(ColorStateList.valueOf(AccountTheme.COLOR_FIELD_STROKE));
        bar.setSplitTrack(false);

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.topMargin = AccountTheme.dp(context, 2f);
        root.addView(bar, params);

        bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (!fromUser) {
                    return;
                }
                int value = item.min() + progress;
                valueView.setText(String.valueOf(value));
                emit(value);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });
        bindInitial();
    }

    @Override
    protected void showValue(int value) {
        bar.setProgress(value - item.min());
        valueView.setText(String.valueOf(value));
    }
}
