package com.julardepick.immirror.account;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.RippleDrawable;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.R;

/*
 * 账号相关界面的视觉常量与控件工厂.
 *
 * 颜色, 圆角, 间距, 字号等可自由调整的设计细节集中在本文件,
 * 登录界面与入口面板只引用这里的常量和方法, 便于整体换肤.
 * 页面作者信息也在本文件定义, 供各页脚统一引用.
 */
public final class AccountTheme {

    /* 作者信息显示名, 页面底部统一由此常量生成. */
    public static final String AUTHOR_NAME = "JularDepick";

    public static final int COLOR_PAGE = 0xFF1A1209;
    public static final int COLOR_SCRIM = 0x8C0D0906;
    public static final int COLOR_CARD = 0xF2281C10;
    public static final int COLOR_CARD_STROKE = 0x3FC08A4E;
    public static final int COLOR_FIELD = 0xFF1F150C;
    public static final int COLOR_FIELD_STROKE = 0xFF5A4028;
    public static final int COLOR_ACCENT = 0xFFC88A4A;
    public static final int COLOR_ACCENT_TEXT = 0xFF2A1B0C;
    public static final int COLOR_ACCENT_SOFT = 0x2EC88A4A;
    public static final int COLOR_TEXT = 0xFFF5EEDC;
    public static final int COLOR_TEXT_MUTED = 0xFFB8A88C;
    public static final int COLOR_TEXT_FAINT = 0xFF7A6A52;
    public static final int COLOR_ERROR = 0xFFFF8A80;
    public static final int COLOR_SUCCESS = 0xFF8FC46A;
    public static final int COLOR_DANGER = 0xFFD9603C;

    /* 卡片最大宽度, 横向大屏时避免表单被拉得过宽. */
    public static final int CARD_MAX_WIDTH_DP = 560;
    public static final int CARD_RADIUS_DP = 18;
    public static final int FIELD_RADIUS_DP = 10;
    public static final int BUTTON_HEIGHT_DP = 46;
    public static final int PAGE_PADDING_DP = 24;

    private AccountTheme() {
    }

    public static int dp(Context context, float value) {
        return Math.round(value * context.getResources().getDisplayMetrics().density);
    }

    public static int screenWidth(Context context) {
        return context.getResources().getDisplayMetrics().widthPixels;
    }

    public static GradientDrawable rounded(Context context, int color, float radiusDp) {
        GradientDrawable drawable = new GradientDrawable();
        drawable.setShape(GradientDrawable.RECTANGLE);
        drawable.setColor(color);
        drawable.setCornerRadius(dp(context, radiusDp));
        return drawable;
    }

    public static GradientDrawable stroked(Context context, int color, float radiusDp, int strokeColor, float strokeDp) {
        GradientDrawable drawable = rounded(context, color, radiusDp);
        drawable.setStroke(Math.max(1, dp(context, strokeDp)), strokeColor);
        return drawable;
    }

    /* 触摸反馈: 把静态背景包一层水波纹. */
    public static Drawable pressed(int rippleColor, Drawable content) {
        return new RippleDrawable(ColorStateList.valueOf(rippleColor), content, null);
    }

    public static TextView title(Context context, CharSequence text) {
        TextView view = new TextView(context);
        view.setText(text);
        view.setTextColor(COLOR_TEXT);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 28f);
        view.setTypeface(Typeface.DEFAULT_BOLD);
        view.setGravity(Gravity.CENTER);
        return view;
    }

    public static TextView subtitle(Context context, CharSequence text) {
        TextView view = new TextView(context);
        view.setText(text);
        view.setTextColor(COLOR_TEXT_MUTED);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13f);
        view.setGravity(Gravity.CENTER);
        return view;
    }

    public static TextView label(Context context, CharSequence text) {
        TextView view = new TextView(context);
        view.setText(text);
        view.setTextColor(COLOR_TEXT_MUTED);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12f);
        return view;
    }

    public static TextView body(Context context, CharSequence text, int color, float sizeSp) {
        TextView view = new TextView(context);
        view.setText(text);
        view.setTextColor(color);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, sizeSp);
        return view;
    }

    public static TextView link(Context context, CharSequence text, int color) {
        TextView view = new TextView(context);
        view.setText(text);
        view.setTextColor(color);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13f);
        view.setGravity(Gravity.CENTER);
        int horizontal = dp(context, 6);
        int vertical = dp(context, 4);
        view.setPadding(horizontal, vertical, horizontal, vertical);
        view.setClickable(true);
        view.setFocusable(true);
        return view;
    }

    public static EditText field(Context context, CharSequence hint, float textSizeSp) {
        EditText view = new EditText(context);
        view.setHint(hint);
        view.setTextColor(COLOR_TEXT);
        view.setHintTextColor(COLOR_TEXT_FAINT);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, textSizeSp);
        view.setBackground(stroked(context, COLOR_FIELD, FIELD_RADIUS_DP, COLOR_FIELD_STROKE, 1f));
        int horizontal = dp(context, 12);
        int vertical = dp(context, 12);
        view.setPadding(horizontal, vertical, horizontal, vertical);
        view.setSingleLine(true);
        view.setGravity(Gravity.CENTER_VERTICAL);
        view.setMinHeight(dp(context, BUTTON_HEIGHT_DP));
        view.setHighlightColor(COLOR_ACCENT);
        return view;
    }

    public static Button primaryButton(Context context, CharSequence text) {
        Button button = new Button(context);
        button.setText(text);
        button.setAllCaps(false);
        button.setTextColor(COLOR_ACCENT_TEXT);
        button.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16f);
        button.setTypeface(Typeface.DEFAULT_BOLD);
        button.setBackground(pressed(0x33000000, rounded(context, COLOR_ACCENT, FIELD_RADIUS_DP + 2f)));
        button.setMinHeight(dp(context, BUTTON_HEIGHT_DP + 2));
        button.setGravity(Gravity.CENTER);
        button.setStateListAnimator(null);
        return button;
    }

    public static Button outlineButton(Context context, CharSequence text, int contentColor) {
        Button button = new Button(context);
        button.setText(text);
        button.setAllCaps(false);
        button.setTextColor(contentColor);
        button.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15f);
        button.setBackground(pressed(COLOR_ACCENT_SOFT,
                stroked(context, COLOR_FIELD, FIELD_RADIUS_DP + 2f, contentColor, 1f)));
        button.setMinHeight(dp(context, BUTTON_HEIGHT_DP));
        button.setGravity(Gravity.CENTER);
        button.setStateListAnimator(null);
        return button;
    }

    public static View spacer(Context context, float heightDp) {
        View view = new View(context);
        view.setLayoutParams(new LinearLayout.LayoutParams(LinearLayout.LayoutParams.MATCH_PARENT,
                dp(context, heightDp)));
        return view;
    }

    /* 行内控件通用参数: 宽度撑满, 指定上边距. */
    public static LinearLayout.LayoutParams block(Context context, float topMarginDp) {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.topMargin = dp(context, topMarginDp);
        return params;
    }

    public static CharSequence authorFooter(Context context) {
        return context.getString(R.string.account_author_format, AUTHOR_NAME);
    }
}