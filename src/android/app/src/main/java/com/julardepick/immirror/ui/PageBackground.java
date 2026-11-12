package com.julardepick.immirror.ui;

import android.content.Context;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.view.View;

import com.julardepick.immirror.account.AccountTheme;
import com.julardepick.immirror.lobby.LobbyArt;

/*
 * 二级页共用底图.
 *
 * 原版的背包, 成就, 钱包, 设置与世界列表都铺在同一张大厅底图上, 而不是纯色底.
 * 这里直接复用大厅底图, 再压一层深色蒙版: 页面标题与落在底图上的文字仍是白字,
 * 蒙版保证对比度; 底图素材缺失时退回 AccountTheme.COLOR_PAGE 纯色, 裁剪包同样可用.
 */
public final class PageBackground {

    /* 蒙版不透明度: 保留底图纹理与层次, 同时把白字对比度提到可读范围. */
    private static final int SCRIM_ALPHA = 0xC0;

    private PageBackground() {
    }

    /* 三级页与弹层最多用到该缓存, 底图只解码一次. */
    public static Drawable drawable(Context context) {
        final Drawable art = LobbyArt.image(context, LobbyArt.MAIN_LOBBY_BG);
        if (art == null) {
            return new ColorDrawable(AccountTheme.COLOR_PAGE);
        }
        final ColorDrawable scrim = new ColorDrawable(0xFF000000);
        scrim.setAlpha(SCRIM_ALPHA);
        return new LayerDrawable(new Drawable[]{art, scrim});
    }

    /* 把底图铺到根视图上; 已有自定义背景的视图不要调用本方法. */
    public static void apply(View root) {
        if (root == null) {
            return;
        }
        root.setBackground(drawable(root.getContext()));
    }
}
