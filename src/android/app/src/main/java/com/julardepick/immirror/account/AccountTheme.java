package com.julardepick.immirror.account;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.RippleDrawable;
import android.graphics.drawable.StateListDrawable;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.R;
import com.julardepick.immirror.generated.UiResources;
import com.julardepick.immirror.ui.UiSound;
import com.julardepick.immirror.ui.NineSliceDrawable;
import com.julardepick.immirror.ui.UiTextures;

/*
 * 账号相关界面的视觉常量与控件工厂.
 *
 * 控件优先用 assets/ui 下的素材绘制, 九宫边界来自 scripts/gen_ui_resources.py
 * 生成的 UiResources 参数表; 素材缺失时逐项回落到下面的纯色常量, 因此裁剪掉
 * 素材的安装包也能显示完整界面.
 * 纯色回落按视觉规范第四章的像素件规则绘制: 面板是三像素黑描边加一像素白内高光,
 * 圆角为零, 四角靠黑边成角; 字号只保留规范第五章的三个高频档位; 按钮按下缩到零点九倍.
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

    /* 面板内高光: 描边内侧一像素纯白, 再过渡到面板近白填充. */
    public static final int COLOR_CARD_HIGHLIGHT = 0xFFFFFFFF;

    /* 按钮边缘色: 规范三点二节的描边近黑与内侧浅蓝高光, 纯色回落时用作三道边. */
    public static final int COLOR_BUTTON_STROKE = 0xFF37372F;
    public static final int COLOR_BUTTON_HIGHLIGHT = 0xFFB6D8E9;

    /* 素材控件上的反白文字, 深色输入框与彩底按钮共用. */
    public static final int COLOR_TEXT_INVERSE = 0xFFFFFFFF;

    /* 深色底上的文字: 登录底图叠加背幕, 标题反白, 次级说明取浅灰. */
    public static final int COLOR_TEXT_ON_DARK = 0xFFFFFFFF;
    public static final int COLOR_TEXT_ON_DARK_MUTED = 0xFFD8DEE0;

    /* 提示条: 素材底色为深棕黑, 文字取浅色, 用色相区分成功与失败. */
    public static final int COLOR_NOTICE_TEXT = 0xFFF3F3F3;
    public static final int COLOR_NOTICE_ERROR = 0xFFFF8787;
    public static final int COLOR_NOTICE_SUCCESS = 0xFF5FCB6A;

    /*
     * 字号三档: 与视觉规范第五章的高频档位对齐.
     * FONT_ 开头的是档位本体, TEXT_SIZE_ 开头的是旧名别名, 现有调用点不用改.
     */
    public static final float FONT_TITLE = 24f;
    public static final float FONT_BODY = 20f;
    public static final float FONT_CAPTION = 18f;
    public static final float TEXT_SIZE_TITLE = FONT_TITLE;
    public static final float TEXT_SIZE_BODY = FONT_BODY;
    public static final float TEXT_SIZE_SUB = FONT_CAPTION;

    /* 按钮按下缩放, 与规范第七章 PushedTexture 的零点九倍一致. */
    public static final float BUTTON_PRESS_SCALE = 0.9f;

    /*
     * 纯色件的描边宽度, 单位为素材像素: 面板三像素, 输入框两像素, 内高光一像素.
     * 折算方式与九宫素材的 artScale 同一标尺, 即一个素材像素占一个 dp.
     */
    public static final float PANEL_STROKE_PX = 3f;
    public static final float FIELD_STROKE_PX = 2f;
    public static final float HIGHLIGHT_PX = 1f;

    /* 控件禁用时的透明度, 素材态与纯色态共用. */
    private static final int DISABLED_ALPHA = 120;

    /* 卡片最大宽度, 横向大屏时避免表单被拉得过宽. */
    public static final int CARD_MAX_WIDTH_DP = 560;

    /* 圆角: 规范第 4.2 节要求零圆角, 四角不透明, 靠像素画黑边成角. */
    public static final int CARD_RADIUS_DP = 0;
    public static final int FIELD_RADIUS_DP = 0;

    public static final int BUTTON_HEIGHT_DP = 46;

    /* 小号按钮的设计尺寸, 与规范第 4.1 节的 40x43 一致. */
    public static final int COMPACT_BUTTON_WIDTH_DP = 40;
    public static final int COMPACT_BUTTON_HEIGHT_DP = 43;

    public static final int PAGE_PADDING_DP = 24;

    private AccountTheme() {
    }

    public static int dp(Context context, float value) {
        return Math.round(value * context.getResources().getDisplayMetrics().density);
    }

    /*
     * 素材像素折算屏幕像素: 三个素材像素在高密度屏上等于三乘密度个物理像素,
     * 与九宫素材按密度放大的描边粗细保持一致.
     */
    public static int pixel(Context context, float designPixels) {
        return Math.max(1, Math.round(designPixels * context.getResources().getDisplayMetrics().density));
    }

    public static int screenWidth(Context context) {
        return context.getResources().getDisplayMetrics().widthPixels;
    }

    /*
     * 字号收敛: 需求字号向三档吸附, 二十二及以上归标题档, 十九及以上归正文档,
     * 其余归说明档. 调用点因此可以继续传原来的数值, 字号档位只在规范里维护.
     */
    public static float tierSize(float requested) {
        if (requested >= 22f) {
            return FONT_TITLE;
        }
        if (requested >= 19f) {
            return FONT_BODY;
        }
        return FONT_CAPTION;
    }

    public static GradientDrawable rounded(Context context, int color, float radiusDp) {
        GradientDrawable drawable = new GradientDrawable();
        drawable.setShape(GradientDrawable.RECTANGLE);
        drawable.setColor(color);
        drawable.setCornerRadius(dp(context, radiusDp));
        return drawable;
    }

    /*
     * 描边底板.
     * 圆角为零时按规范第 4.2 节叠三层: 底层纯描边色, 中间一像素内高光, 上层填充色;
     * 三层用 inset 逐层收进, 因此描边与高光的粗细不随控件尺寸变化.
     * 仍带圆角的调用点是标签块一类的小控件, 三层 inset 会在圆角处露出楔形缺口,
     * 这种情况退回单一描边的圆角矩形.
     */
    public static Drawable stroked(Context context, int color, float radiusDp, int strokeColor,
                                   float strokeDp) {
        if (radiusDp > 0f) {
            GradientDrawable drawable = rounded(context, color, radiusDp);
            drawable.setStroke(pixel(context, strokeDp), strokeColor);
            return drawable;
        }
        return layered(context, color, strokeColor, COLOR_CARD_HIGHLIGHT, strokeDp, HIGHLIGHT_PX);
    }

    /*
     * 像素级描边底板: 描边宽度与高光宽度都按素材像素给, 圆角恒为零.
     * 高光色传零表示不要高光层, 只剩描边加填充两层.
     */
    private static Drawable layered(Context context, int fillColor, int strokeColor, int highlightColor,
                                    float strokePx, float highlightPx) {
        GradientDrawable border = rounded(context, strokeColor, 0f);
        GradientDrawable body = rounded(context, fillColor, 0f);
        int stroke = pixel(context, strokePx);
        if (highlightColor == 0) {
            LayerDrawable drawable = new LayerDrawable(new Drawable[]{border, body});
            drawable.setLayerInset(1, stroke, stroke, stroke, stroke);
            return drawable;
        }
        int highlight = pixel(context, highlightPx);
        GradientDrawable halo = rounded(context, highlightColor, 0f);
        LayerDrawable drawable = new LayerDrawable(new Drawable[]{border, halo, body});
        drawable.setLayerInset(1, stroke, stroke, stroke, stroke);
        int inner = stroke + highlight;
        drawable.setLayerInset(2, inner, inner, inner, inner);
        return drawable;
    }

    /* 触摸反馈: 把静态背景包一层水波纹, 仅在素材缺失时作为回落使用. */
    public static Drawable pressed(int rippleColor, Drawable content) {
        return new RippleDrawable(ColorStateList.valueOf(rippleColor), content, null);
    }

    /*
     * 素材背景状态机: 常态与按下态共用同一张位图, 按下缩小由控件自身缩放完成,
     * 若图层再缩一次会叠成零点八一倍; 禁用态压低透明度.
     */
    private static Drawable pressStates(NineSliceDrawable base) {
        NineSliceDrawable disabled = base.copy();
        disabled.setAlpha(DISABLED_ALPHA);
        StateListDrawable states = new StateListDrawable();
        states.addState(new int[]{-android.R.attr.state_enabled}, disabled);
        states.addState(new int[]{}, base);
        return states;
    }

    /* 按钮素材背景, 素材缺失返回空. */
    private static Drawable buttonBackground(Context context, UiResources.Widget widget) {
        NineSliceDrawable base = UiTextures.nineSlice(context, widget);
        return base == null ? null : pressStates(base);
    }

    /*
     * 按下缩放: 规范第七章要求按下即换成零点九倍的同图, 没有过渡时长.
     * 这里直接改控件缩放, 并返回假把触摸事件交回原有点击监听, 不吞掉点击.
     */
    /*
     * 统一的按压反馈: 按下缩放, 抬起回弹, 抬起瞬间播一次界面点击音.
     * 只对真正落在控件内部的抬起播声音, 滑出后取消不发声.
     */
    private static void attachPressScale(View view) {
        view.setOnTouchListener((target, event) -> {
            switch (event.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    target.setScaleX(BUTTON_PRESS_SCALE);
                    target.setScaleY(BUTTON_PRESS_SCALE);
                    break;
                case MotionEvent.ACTION_UP:
                    target.setScaleX(1f);
                    target.setScaleY(1f);
                    if (target.isClickable() && target.isEnabled()) {
                        UiSound.click(target.getContext());
                    }
                    break;
                case MotionEvent.ACTION_CANCEL:
                    target.setScaleX(1f);
                    target.setScaleY(1f);
                    break;
                default:
                    break;
            }
            return false;
        });
    }

    /* 面板背景: 白色近白面板素材, 三像素黑描边与内高光由素材自带, 缺失时回落像素件. */
    public static Drawable panelBackground(Context context) {
        NineSliceDrawable panel = UiTextures.nineSlice(context, UiResources.Widget.PANEL_SURFACE_WHITE);
        if (panel != null) {
            return panel;
        }
        return layered(context, COLOR_CARD, COLOR_CARD_STROKE, COLOR_CARD_HIGHLIGHT, PANEL_STROKE_PX, HIGHLIGHT_PX);
    }

    /* 输入框背景: 深灰输入框素材, 缺失时回落两像素描边的方角浅底. */
    public static Drawable fieldBackground(Context context) {
        NineSliceDrawable field = UiTextures.nineSlice(context, UiResources.Widget.SLOT_CHECK_OFF);
        if (field != null) {
            return field;
        }
        return layered(context, COLOR_FIELD, COLOR_FIELD_STROKE, 0, FIELD_STROKE_PX, HIGHLIGHT_PX);
    }

    /* 提示条背景: 深棕黑提示条素材, 缺失时回落方角半透明底. */
    public static Drawable noticeBackground(Context context) {
        NineSliceDrawable tips = UiTextures.nineSlice(context, UiResources.Widget.DIALOG_TIPS_BODY);
        if (tips != null) {
            return tips;
        }
        return rounded(context, 0x1FFF8A80, 0f);
    }

    /* 标签页背景: 选中换近白选中态素材, 未选中用常态素材. */
    public static Drawable tabBackground(Context context, boolean selected) {
        UiResources.Widget widget = selected ? UiResources.Widget.TAB_LEAF_ACTIVE
                : UiResources.Widget.TAB_LEAF_NORMAL;
        NineSliceDrawable leaf = UiTextures.nineSlice(context, widget);
        if (leaf != null) {
            return leaf;
        }
        return rounded(context, selected ? COLOR_ACCENT_SOFT : 0x00000000, 0f);
    }

    public static TextView title(Context context, CharSequence text) {
        return title(context, text, COLOR_TEXT);
    }

    /* 标题: 默认深灰, 叠在深色底上时由调用方传入反白色. */
    public static TextView title(Context context, CharSequence text, int color) {
        TextView view = new TextView(context);
        view.setText(text);
        view.setTextColor(color);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, FONT_TITLE);
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
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, FONT_BODY);
        view.setGravity(Gravity.CENTER);
        return view;
    }

    public static TextView label(Context context, CharSequence text) {
        TextView view = new TextView(context);
        view.setText(text);
        view.setTextColor(COLOR_TEXT_MUTED);
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, FONT_CAPTION);
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
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, FONT_BODY);
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
        view.setTextSize(TypedValue.COMPLEX_UNIT_SP, FONT_CAPTION);
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

    /* 主按钮: 大号蓝, 字号取标题档; 素材缺失时回落蓝底三道边. */
    public static Button primaryButton(Context context, CharSequence text) {
        Button button = new Button(context);
        button.setText(text);
        button.setAllCaps(false);
        button.setTextColor(COLOR_ACCENT_TEXT);
        button.setTextSize(TypedValue.COMPLEX_UNIT_SP, FONT_TITLE);
        button.setTypeface(Typeface.DEFAULT_BOLD);
        Drawable material = buttonBackground(context, UiResources.Widget.BUTTON_PRIMARY_BLUE);
        button.setBackground(material != null ? material
                : layered(context, COLOR_ACCENT, COLOR_BUTTON_STROKE, COLOR_BUTTON_HIGHLIGHT,
                        FIELD_STROKE_PX, HIGHLIGHT_PX));
        button.setMinHeight(dp(context, BUTTON_HEIGHT_DP + 2));
        button.setGravity(Gravity.CENTER);
        button.setStateListAnimator(null);
        attachPressScale(button);
        return button;
    }

    /* 次级按钮: 空底白, 字号取正文档; 素材缺失时回落浅底描边. */
    public static Button outlineButton(Context context, CharSequence text, int contentColor) {
        Button button = new Button(context);
        button.setText(text);
        button.setAllCaps(false);
        button.setTextColor(contentColor);
        button.setTextSize(TypedValue.COMPLEX_UNIT_SP, FONT_BODY);
        Drawable material = buttonBackground(context, UiResources.Widget.BUTTON_GHOST_WHITE);
        button.setBackground(material != null ? material
                : pressed(COLOR_ACCENT_SOFT,
                        layered(context, COLOR_FIELD, contentColor, 0, FIELD_STROKE_PX, HIGHLIGHT_PX)));
        button.setMinHeight(dp(context, BUTTON_HEIGHT_DP));
        button.setGravity(Gravity.CENTER);
        button.setStateListAnimator(null);
        attachPressScale(button);
        return button;
    }

    /* 小号按钮: 素材 40x43, 字号取正文档, 供行内的次级动作使用. */
    public static Button compactButton(Context context, CharSequence text, int contentColor) {
        Button button = new Button(context);
        button.setText(text);
        button.setAllCaps(false);
        button.setTextColor(contentColor);
        button.setTextSize(TypedValue.COMPLEX_UNIT_SP, FONT_BODY);
        Drawable material = buttonBackground(context, UiResources.Widget.BUTTON_COMPACT_BLUE);
        button.setBackground(material != null ? material
                : pressed(COLOR_ACCENT_SOFT,
                        layered(context, COLOR_ACCENT, COLOR_BUTTON_STROKE, COLOR_BUTTON_HIGHLIGHT,
                                FIELD_STROKE_PX, HIGHLIGHT_PX)));
        button.setMinWidth(dp(context, COMPACT_BUTTON_WIDTH_DP));
        button.setMinHeight(dp(context, COMPACT_BUTTON_HEIGHT_DP));
        button.setGravity(Gravity.CENTER);
        button.setStateListAnimator(null);
        attachPressScale(button);
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