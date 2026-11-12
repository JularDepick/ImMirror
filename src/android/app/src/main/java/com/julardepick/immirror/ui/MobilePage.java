package com.julardepick.immirror.ui;

import android.app.Activity;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageButton;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.julardepick.immirror.R;
import com.julardepick.immirror.account.AccountTheme;
import com.julardepick.immirror.lobby.LobbyArt;

/*
 * 移动端模态页骨架.
 *
 * 二级页不是整页长列表, 而是"场景底图 + 半透明遮罩 + 居中面板":
 * 遮罩把背景压暗, 面板固定最大宽高并居中, 内容在面板内部滚动,
 * 标题与关闭按钮固定在面板顶部, 与移动游戏的弹窗范式一致.
 * 面板素材缺失时回落主题面板, 遮罩与关闭按钮仍然生效.
 */
public final class MobilePage {

    /*
     * 模态遮罩不透明度: 与 PageBackground 的底图蒙版叠加后约为六成,
     * 面板仍然跳出来, 底图纹理也还看得见.
     */
    private static final int SCRIM_COLOR = 0x99000000;
    /* 面板与屏幕边缘的最小留白, 窄屏也不会贴边. */
    private static final float PANEL_MARGIN_DP = 16f;
    /* 面板最大宽度, 宽屏上不把内容拉成一行长条. */
    private static final float PANEL_MAX_WIDTH_DP = 720f;
    /* 面板顶部标题行高度与内边距. */
    private static final float TITLE_HEIGHT_DP = 56f;
    private static final float PANEL_PADDING_DP = 16f;
    /* 关闭按钮素材的设计尺寸 (36x46 像素), 按素材标尺折算. */
    private static final float CLOSE_WIDTH_PX = 36f;
    private static final float CLOSE_HEIGHT_PX = 46f;
    /* 面板九宫的四边边界, 与素材的像素边界一致. */
    private static final int PANEL_INSET = 10;

    private MobilePage() {
    }

    /*
     * 组装一页: 返回可直接 setContentView 的根视图.
     * content 放进面板内部的可滚动区域, 调用方只需构造内容本身.
     */
    public static View create(Activity activity, CharSequence title, View content) {
        final FrameLayout root = new FrameLayout(activity);
        PageBackground.apply(root);

        /*
         * 遮罩: 压暗底图并吃掉面板外的点击.
         * 只叠这一层, 底图的蒙版由 PageBackground 负责, 两层加起来才不过暗.
         */
        final View scrim = new View(activity);
        scrim.setBackgroundColor(SCRIM_COLOR);
        scrim.setClickable(true);
        root.addView(scrim, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));

        final FrameLayout panel = new FrameLayout(activity);
        final Drawable surface = panelSurface(activity);
        if (surface != null) {
            panel.setBackground(surface);
        } else {
            panel.setBackground(AccountTheme.panelBackground(activity));
        }

        final LinearLayout column = new LinearLayout(activity);
        column.setOrientation(LinearLayout.VERTICAL);
        final int padding = AccountTheme.dp(activity, PANEL_PADDING_DP);
        column.setPadding(padding, AccountTheme.dp(activity, 8f), padding,
                AccountTheme.dp(activity, 8f));
        panel.addView(column, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));

        column.addView(buildTitleRow(activity, title), new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, AccountTheme.dp(activity, TITLE_HEIGHT_DP)));
        column.addView(content, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, 0, 1f));

        final int margin = AccountTheme.dp(activity, PANEL_MARGIN_DP);
        final FrameLayout.LayoutParams panelParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.MATCH_PARENT,
                Gravity.CENTER);
        panelParams.setMargins(margin, margin, margin, margin);
        /* 宽屏上限: 用最大宽度约束, 面板不再随屏幕无限拉宽. */
        panel.setMinimumWidth(0);
        panel.setLayoutParams(panelParams);
        root.addView(panel, panelParams);
        root.post(() -> clampPanelWidth(activity, panel));
        return root;
    }

    /* 面板宽度: 屏幕宽度减两侧留白, 但不超过上限. */
    private static void clampPanelWidth(Activity activity, View panel) {
        final ViewGroup.LayoutParams params = panel.getLayoutParams();
        if (!(params instanceof FrameLayout.LayoutParams)) {
            return;
        }
        final int screenWidth = activity.getResources().getDisplayMetrics().widthPixels;
        final int margin = AccountTheme.dp(activity, PANEL_MARGIN_DP) * 2;
        final int maxWidth = AccountTheme.dp(activity, PANEL_MAX_WIDTH_DP);
        final int width = Math.min(screenWidth - margin, maxWidth);
        params.width = Math.max(width, AccountTheme.dp(activity, 240f));
        panel.setLayoutParams(params);
    }

    /* 面板底: 大面板九宫, 素材缺失返回空由调用方回落主题面板. */
    private static Drawable panelSurface(Activity activity) {
        return LobbyArt.nineSlice(activity, LobbyArt.CARD_WHITE,
                PANEL_INSET, PANEL_INSET, PANEL_INSET, PANEL_INSET);
    }

    /* 标题行: 左标题右关闭, 关闭按钮走素材, 缺失时用文字按钮. */
    private static View buildTitleRow(Activity activity, CharSequence title) {
        final FrameLayout row = new FrameLayout(activity);

        final TextView titleView = AccountTheme.title(activity, title);
        titleView.setGravity(Gravity.CENTER);
        row.addView(titleView, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT,
                Gravity.CENTER));

        final View close = buildCloseButton(activity);
        final FrameLayout.LayoutParams closeParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT,
                Gravity.END | Gravity.CENTER_VERTICAL);
        row.addView(close, closeParams);
        return row;
    }

    private static View buildCloseButton(Activity activity) {
        final Drawable art = LobbyArt.image(activity, LobbyArt.CLOSE_BTN);
        final ImageButton button = new ImageButton(activity);
        if (art != null) {
            button.setImageDrawable(art);
            button.setBackground(new ColorDrawable(0));
            button.setScaleType(android.widget.ImageView.ScaleType.FIT_CENTER);
            button.setPadding(0, 0, 0, 0);
            final int width = LobbyArt.designSize(activity, CLOSE_WIDTH_PX);
            final int height = LobbyArt.designSize(activity, CLOSE_HEIGHT_PX);
            button.setLayoutParams(new FrameLayout.LayoutParams(width, height));
        } else {
            /* 素材缺失时用系统关闭图标, 至少保证可关闭. */
            button.setImageResource(android.R.drawable.ic_menu_close_clear_cancel);
            button.setBackground(new ColorDrawable(0));
        }
        button.setContentDescription(activity.getString(R.string.action_back));
        button.setOnClickListener(view -> activity.finish());
        return button;
    }

    /*
     * 页内分区标题: 深色小标题压一条细线, 用于把面板内容分组,
     * 比整块卡片更接近移动端的列表分组观感.
     */
    public static View sectionHeader(Activity activity, CharSequence text) {
        final LinearLayout header = new LinearLayout(activity);
        header.setOrientation(LinearLayout.VERTICAL);
        final TextView label = AccountTheme.subtitle(activity, text);
        label.setTextColor(AccountTheme.COLOR_TEXT_MUTED);
        header.addView(label, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT));
        final View line = new View(activity);
        line.setBackgroundColor(0x1F000000);
        final LinearLayout.LayoutParams lineParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, AccountTheme.dp(activity, 1f));
        lineParams.topMargin = AccountTheme.dp(activity, 4f);
        header.addView(line, lineParams);
        return header;
    }

    /* 给面板内容套一层可滚动容器, 长列表在面板内部滚动. */
    public static View scrollable(Activity activity, View content) {
        final android.widget.ScrollView scroll = new android.widget.ScrollView(activity);
        scroll.setFillViewport(true);
        scroll.setClipToPadding(false);
        scroll.setVerticalScrollBarEnabled(false);
        scroll.addView(content, new android.widget.ScrollView.LayoutParams(
                android.widget.ScrollView.LayoutParams.MATCH_PARENT,
                android.widget.ScrollView.LayoutParams.WRAP_CONTENT));
        return scroll;
    }
}
