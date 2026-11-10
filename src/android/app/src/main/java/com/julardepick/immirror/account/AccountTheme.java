package com.julardepick.immirror.account;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.RippleDrawable;
import android.graphics.drawable.StateListDrawable;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.R;
import com.julardepick.immirror.generated.UiResources;
import com.julardepick.immirror.ui.NineSliceDrawable;
import com.julardepick.immirror.ui.UiTextures;

/*
 * 账号相关界面的视觉常量与控件工厂.
 *
 * 控件优先用 assets/ui 下的原版素材绘制, 九宫边界来自 scripts/gen_ui_resources.py
 * 生成的 UiResources 参数表; 素材缺失时逐项回落到下面的纯色常量, 因此裁剪掉
 * 素材的安装包也能显示完整界面.
 * 颜色, 间距, 字号等可自由调整的设计细节集中在本文件, 登录界面与入口面板
 * 只引用这里的常量和方法, 便于整体换肤. 页面作者信息也在此定义, 供页脚统一引用.
 */
public final class AccountTheme {

    /* 作者信息显示名, 页面底部统一由此常量生成. */
    public static final String AUTHOR_NAME = "JularDepick";

    public static final int COLOR_PAGE = 0xFF2E4A63;
    public static final int COLOR_SCRIM = 0xCC000000;
    public static final int COLOR_CARD = 0xFFF3F3F3;
    public static final int COLOR_CARD_STROKE = 0xFF000000;
    public static final int COLOR_FIELD = 0xFFFFFFFF;
    public static final int COLOR_FIELD_STROKE = 0xFFB9B9B9;
    public static final int COLOR_ACCENT = 0xFF68D3FC;
    public static final int COLOR_ACCENT_TEXT = 0xFFFFFFFF;
    public static final int COLOR_ACCENT_SOFT = 0x3368D3FC;
    public static final int COLOR_TEXT = 0xFF3D4546;
    public static final int COLOR_TEXT_MUTED = 0xFF6B7476;
    public static final int COLOR_TEXT_FAINT = 0xFFB9B9B9;
    public static final int COLOR_ERROR = 0xFFD61919;
    public static final int COLOR_SUCCESS = 0xFF5FCB6A;
    public static final int COLOR_DANGER = 0xFFFF8787;

    /* 辅助色: 次级按钮与货币底板. */
    public static final int COLOR_WARNING = 0xFFFFDF11;

    /* 素材控件上的反白文字, 深色输入框与彩底按钮共用. */
    public static final int COLOR_TEXT_INVERSE = 0xFFFFFFFF;

    /* 深色底上的文字: 登录底图叠加背幕, 标题反白, 次级说明取浅灰. */
    public static final int COLOR_TEXT_ON_DARK = 0xFFFFFFFF;
    public static final int COLOR_TEXT_ON_DARK_MUTED = 0xFFD8DEE0;

    /* 提示条: 素材底色为深棕黑, 文字取浅色, 用色相区分成功与失败. */
    public static final int COLOR_NOTICE_TEXT = 0xFFF3F3F3;
    public static final int COLOR_NOTICE_ERROR = 0xFFFF8787;
    public static final int COLOR_NOTICE_SUCCESS = 0xFF5FCB6A;

    /* 字号三档: 与视觉规范第五章的高频档位对齐, 其余字号统一吸附到这三档. */
    public static final float TEXT_SIZE_TITLE = 24f;
    public static final float TEXT_SIZE_BODY = 20f;
    public static final float TEXT_SIZE_SUB = 18f;

    /* 按钮按下缩放, 与规范第七章 PushedTexture 的零点九倍一致. */
    public static final float BUTTON_PRESS_SCALE = 0.9f;

    /* 控件禁用时的透明度, 素材态与纯色态共用. */
    private static final int DISABLED_ALPHA = 120;

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

    /*
     * 字号收敛: 需求字号向三档吸附, 二十二及以上归标题档, 十九及以上归正文档,
     * 其余归次级档. 调用点因此可以继续传原来的数值, 字号档位只在规范里维护.
     */
    public static float tierSize(float requested) {
        if (requested >= 22f) {
            return TEXT_SIZE_TITLE;
        }
        if (requested >= 19f) {
            return TEXT_SIZE_BODY;
        }
        return TEXT_SIZE_SUB;
    }

    public static GradientDrawable rounded(Context context, int color, float radiusDp) {
        GradientDrawable drawable = new GradientDrawable();
        drawable.setShape(GradientDrawable.RECTANGLE);
        drawable.setColor(color);
        drawable.setCornerRadius(dp(context, radiusDp));
        return drawable;
    }

    public static GradientDrawable stroked(Context context, int color, float radiusDp, int strokeColor,
                                           float strokeDp) {
        GradientDrawable drawable = rounded(context, color, radiusDp);
        drawable.setStroke(Math.max(1, dp(context, strokeDp)), strokeColor);
        return drawable;
    }

    /* 触摸反馈: 把静态背景包一层水波纹, 仅在素材缺失时作为回落使用. */
    public static Drawable pressed(int rippleColor, Drawable content) {
        return new RippleDrawable(ColorStateList.valueOf(rippleColor), content, null);
    }

    /*
     * 素材背景状态机: 常态用原尺寸素材, 按下换零点九倍缩放的同图,
     * 禁用态压低透明度, 三种状态共用同一张位图.
     */
    private static Drawable pressStates(NineSliceDrawable base) {
        NineSliceDrawable pressed = base.copy();
        pressed.setScale(BUTTON_PRESS_SCALE);
        NineSliceDrawable disabled = base.copy();
        disabled.setAlpha(DISABLED_ALPHA);
        StateListDrawable states = new StateListDrawable();
        states.addState(new int[]{-android.R.attr.state_enabled}, disabled);
        states.addState(new int[]{android.R.attr.state_pressed}, pressed);
        states.addState(new int[]{}, base);
        return states;
    }

    /* 按钮素材背景, 素材缺失返回空. */
    private static Drawable buttonBackground(Context context, UiResources.Widget widget) {
        NineSliceDrawable base = UiTextures.nineSlice(context, widget);
        return base == null ? null : pressStates(base);
    }

    /* 面板背景: 白色近白面板素材, 三像素黑描边由素材自带, 缺失时回落纯色圆角. */
    public static Drawable panelBackground(Context context) {
        NineSliceDrawable panel = UiTextures.nineSlice(context, UiResources.Widget.PANEL_SURFACE_WHITE);
        if (panel != null) {
            return panel;
        }
        return stroked(context, COLOR_CARD, CARD_RADIUS_DP, COLOR_CARD_STROKE, 1f);
    }

    /* 输入框背景: 深灰输入框素材, 缺失时回落浅底描边. */
    public static Drawable fieldBackground(Context context) {
        NineSliceDrawable field = UiTextures.nineSlice(context, UiResources.Widget.SLOT_CHECK_OFF);
        if (field != null) {
            return field;
        }
        return stroked(context, COLOR_FIELD, FIELD_RADIUS_DP, COLOR_FIELD_STROKE, 1f);
    }

    /* 提示条背景: 深棕黑提示条素材, 缺失时回落半透明圆角. */
    public static Drawable noticeBackground(Context context) {
        NineSliceDrawable tips = UiTextures.nineSlice(context, UiResources.Widget.DIALOG_TIPS_BODY);
        if (tips != null) {
            return tips;
        }
        return rounded(context, 0x1FFF8A80, 8f);
    }

    /* 标签页背景: 选中换近白选中态素材, 未选中用常态素材. */
    public static Drawable tabBackground(Context context, boolean selected) {
        UiResources.Widget widget = selected ? UiResources.Widget.TAB_LEAF_ACTIVE
                : UiResources.Widget.TAB_LEAF_NORMAL;
        NineSliceDrawable leaf = UiTextures.nineSlice(context, widget);
        if (leaf != null) {
            return leaf;
        }
        return rounded(context, selected ? COLOR_ACCENT_SOFT : 0x00000000, 10f);
    }

    public static TextView title(Context context, CharSequence text) {
        return title(context, text, COLOR_TEXT);
    }

    /* 标题: 默认深灰, 叠在深色底上时由调用方传入反白色. */
    public static TextView title(Context context, CharSequence text, int color) {
        TextView view = new TextView(context);
        view.setText(text);
        view.setTextColor(color);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, TEXT_SIZE_TITLE);
        view.setTypeface(Typeface.DEFAULT_BOLD);
        view.setGravity(Gravity.CENTER);
        return view;
    }

    public static TextView subtitle(Context context, CharSequence text) {
        return subtitle(context, text, COLOR_TEXT_MUTED);
    }

    public static TextView subtitle(Context context, CharSequence text, int color) {
        TextView view = new TextView(context);
        view.setText(text);
        view.setTextColor(color);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, TEXT_SIZE_BODY);
        view.setGravity(Gravity.CENTER);
        return view;
    }

    public static TextView label(Context context, CharSequence text) {
        TextView view = new TextView(context);
        view.setText(text);
        view.setTextColor(COLOR_TEXT_MUTED);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, TEXT_SIZE_SUB);
        return view;
    }

    public static TextView body(Context context, CharSequence text, int color, float sizeSp) {
        TextView view = new TextView(context);
        view.setText(text);
        view.setTextColor(color);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, tierSize(sizeSp));
        return view;
    }

    public static TextView link(Context context, CharSequence text, int color) {
        TextView view = new TextView(context);
        view.setText(text);
        view.setTextColor(color);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, TEXT_SIZE_SUB);
        view.setGravity(Gravity.CENTER);
        int horizontal = dp(context, 6);
        int vertical = dp(context, 4);
        view.setPadding(horizontal, vertical, horizontal, vertical);
        view.setClickable(true);
        view.setFocusable(true);
        return view;
    }

    /* 提示条控件: 深棕黑九宫底配浅色文字, 动效由 NoticeBar 控制. */
    public static TextView notice(Context context) {
        TextView view = new TextView(context);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, TEXT_SIZE_SUB);
        view.setBackground(noticeBackground(context));
        view.setLineSpacing(dp(context, 2f), 1f);
        int horizontal = dp(context, 12f);
        int vertical = dp(context, 8f);
        view.setPadding(horizontal, vertical, horizontal, vertical);
        view.setVisibility(View.GONE);
        return view;
    }

    public static EditText field(Context context, CharSequence hint, float textSizeSp) {
        EditText view = new EditText(context);
        view.setHint(hint);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, tierSize(textSizeSp));
        view.setHintTextColor(COLOR_TEXT_FAINT);
        Drawable background = fieldBackground(context);
        view.setBackground(background);
        /* 输入框素材为深灰底, 文字要反白; 回落浅底时仍用深灰文字. */
        boolean asset = UiTextures.bitmap(context, UiResources.Widget.SLOT_CHECK_OFF) != null;
        view.setTextColor(asset ? COLOR_TEXT_INVERSE : COLOR_TEXT);
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
        button.setTextSize(TypedValue.COMPLEX_UNIT_SP, TEXT_SIZE_BODY);
        button.setTypeface(Typeface.DEFAULT_BOLD);
        Drawable material = buttonBackground(context, UiResources.Widget.BUTTON_PRIMARY_BLUE);
        button.setBackground(material != null ? material
                : pressed(0x33000000, rounded(context, COLOR_ACCENT, FIELD_RADIUS_DP + 2f)));
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
        button.setTextSize(TypedValue.COMPLEX_UNIT_SP, TEXT_SIZE_SUB);
        Drawable material = buttonBackground(context, UiResources.Widget.BUTTON_GHOST_WHITE);
        button.setBackground(material != null ? material
                : pressed(COLOR_ACCENT_SOFT,
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
