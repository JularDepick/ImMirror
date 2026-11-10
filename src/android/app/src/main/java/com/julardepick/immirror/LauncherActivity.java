package com.julardepick.immirror;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import com.julardepick.immirror.device.DeviceProfile;

import java.util.List;

/*
 * 启动界面: 展示版本与设备分级结果, 作为进入原生游戏界面的入口.
 * 账号与登录排版在后续版本补齐, 这里只保留占位说明.
 */
public final class LauncherActivity extends Activity {

    private static final String TAG = "ImMirrorLauncher";

    /* 作者信息统一由此常量控制, 页面底部展示, 便于后续整体替换. */
    private static final String AUTHOR_NAME = "JularDepick";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final DeviceProfile profile = DeviceProfile.obtain(this);
        logProfile(profile);
        setContentView(buildContentView(profile));
    }

    private void logProfile(DeviceProfile profile) {
        Log.i(TAG, String.format("device grade=%s score=%d", profile.gradeCode(), profile.gradeScore()));
        for (String line : profile.toDisplayLines()) {
            Log.i(TAG, "device " + line);
        }
        Log.i(TAG, "native payload:\n" + profile.toNativePayload());
    }

    private View buildContentView(DeviceProfile profile) {
        final int padding = dp(20);

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setBackgroundColor(Color.parseColor("#0B1220"));
        root.setPadding(padding, padding, padding, padding);

        TextView title = new TextView(this);
        title.setText(R.string.launcher_title);
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 30);
        title.setGravity(Gravity.CENTER_HORIZONTAL);
        root.addView(title);

        TextView subtitle = new TextView(this);
        subtitle.setText(R.string.launcher_subtitle);
        subtitle.setTextColor(Color.parseColor("#8FA3BF"));
        subtitle.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        subtitle.setGravity(Gravity.CENTER_HORIZONTAL);
        root.addView(subtitle, marginTop(dp(6)));

        TextView version = new TextView(this);
        version.setText(getString(R.string.launcher_version_format,
                BuildConfig.VERSION_NAME, BuildConfig.VERSION_CODE));
        version.setTextColor(Color.parseColor("#8FA3BF"));
        version.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        version.setGravity(Gravity.CENTER_HORIZONTAL);
        root.addView(version, marginTop(dp(4)));

        TextView grade = new TextView(this);
        grade.setText(getString(R.string.launcher_grade_format, profile.gradeLabel(), profile.gradeScore()));
        grade.setTextColor(Color.parseColor("#5FD3C4"));
        grade.setTextSize(TypedValue.COMPLEX_UNIT_SP, 20);
        grade.setGravity(Gravity.CENTER_HORIZONTAL);
        root.addView(grade, marginTop(dp(18)));

        TextView deviceHeader = new TextView(this);
        deviceHeader.setText(R.string.launcher_device_header);
        deviceHeader.setTextColor(Color.WHITE);
        deviceHeader.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        root.addView(deviceHeader, marginTop(dp(22)));

        TextView deviceDetail = new TextView(this);
        deviceDetail.setText(joinLines(profile.toDisplayLines()));
        deviceDetail.setTextColor(Color.parseColor("#C7D3E3"));
        deviceDetail.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        deviceDetail.setTextIsSelectable(true);
        deviceDetail.setLineSpacing(dp(4), 1.0f);
        root.addView(deviceDetail, marginTop(dp(8)));

        TextView accountHint = new TextView(this);
        accountHint.setText(R.string.launcher_account_placeholder);
        accountHint.setTextColor(Color.parseColor("#6E7F96"));
        accountHint.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        root.addView(accountHint, marginTop(dp(18)));

        Button enter = new Button(this);
        enter.setText(R.string.launcher_enter_game);
        enter.setOnClickListener(view -> startActivity(new Intent(this, PlayActivity.class)));
        root.addView(enter, marginTop(dp(24)));

        TextView author = new TextView(this);
        author.setText(String.format("\u00A9 2026 %s", AUTHOR_NAME));
        author.setTextColor(Color.parseColor("#4E5C70"));
        author.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        author.setGravity(Gravity.CENTER_HORIZONTAL);
        root.addView(author, marginTop(dp(20)));

        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(Color.parseColor("#0B1220"));
        scroll.addView(root);
        return scroll;
    }

    private static String joinLines(List<String> lines) {
        StringBuilder builder = new StringBuilder();
        for (String line : lines) {
            builder.append(line).append('\n');
        }
        return builder.toString().trim();
    }

    private LinearLayout.LayoutParams marginTop(int pixels) {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.topMargin = pixels;
        return params;
    }

    private int dp(int value) {
        return Math.round(value * getResources().getDisplayMetrics().density);
    }
}
