package com.julardepick.immirror.ui;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.util.LruCache;

import com.julardepick.immirror.generated.UiResources;

import java.io.IOException;
import java.io.InputStream;

/*
 * 界面素材装载器.
 *
 * 素材随 APK 放在 assets/ui 下, 由 scripts/gen_ui_resources.py 在构建期复制,
 * 并按 ui_manifest.json 生成 UiResources 参数表, 因此这里只按枚举取路径,
 * 不再手写文件名与九宫边界. 位图按路径缓存在 LruCache 里, 同一素材只解码一次;
 * 素材缺失或解码失败时返回空, 由调用方回落到纯色绘制, 裁剪包里界面仍然可用.
 */
public final class UiTextures {

    private static final String TAG = "ImMirrorUi";

    /* 素材合计不足 54 KiB, 上限按 4 MiB 设, 正常使用不会触发淘汰. */
    private static final int CACHE_BYTES = 4 * 1024 * 1024;

    private static final LruCache<String, Bitmap> CACHE = new LruCache<String, Bitmap>(CACHE_BYTES) {
        @Override
        protected int sizeOf(String key, Bitmap value) {
            return value.getByteCount();
        }
    };

    /* 解码失败只提示一次, 避免每次重绘都刷日志. */
    private static boolean decodeFailureLogged;

    private UiTextures() {
    }

    /*
     * 一个素材像素对应的屏幕像素.
     * 取设备密度等于让一个素材像素占一个 dp, 与按 dp 布局的控件同一标尺,
     * 描边在高密度屏上不会细到看不见, 在低密度屏上也不会糊成一片.
     */
    public static float artScale(Context context) {
        return context.getResources().getDisplayMetrics().density;
    }

    /* 读取素材位图, 失败返回空. */
    public static Bitmap bitmap(Context context, UiResources.Widget widget) {
        if (widget == null) {
            return null;
        }
        String path = widget.assetPath();
        Bitmap cached = CACHE.get(path);
        if (cached != null && !cached.isRecycled()) {
            return cached;
        }
        Bitmap decoded = decode(context, path);
        if (decoded != null) {
            CACHE.put(path, decoded);
        }
        return decoded;
    }

    /* 九宫或三片素材的绘制器, 整体拉伸的素材返回空. */
    public static NineSliceDrawable nineSlice(Context context, UiResources.Widget widget) {
        if (widget == null || !widget.sliced()) {
            return null;
        }
        Bitmap bitmap = bitmap(context, widget);
        if (bitmap == null) {
            return null;
        }
        NineSliceDrawable drawable = new NineSliceDrawable(bitmap, widget.insetLeft,
                widget.insetTop, widget.insetRight, widget.insetBottom);
        drawable.setArtScale(artScale(context));
        return drawable;
    }

    /* 整体拉伸素材的绘制器, 用于图标一类不需要切片的素材. */
    public static Drawable stretched(Context context, UiResources.Widget widget) {
        Bitmap bitmap = bitmap(context, widget);
        if (bitmap == null) {
            return null;
        }
        BitmapDrawable drawable = new BitmapDrawable(context.getResources(), bitmap);
        drawable.setFilterBitmap(true);
        return drawable;
    }

    private static Bitmap decode(Context context, String path) {
        try (InputStream stream = context.getAssets().open(path)) {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inScaled = false;
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;
            return BitmapFactory.decodeStream(stream, null, options);
        } catch (IOException error) {
            if (!decodeFailureLogged) {
                decodeFailureLogged = true;
                Log.w(TAG, "界面素材缺失, 回落纯色绘制: " + path, error);
            }
            return null;
        }
    }
}
