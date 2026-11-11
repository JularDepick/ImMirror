package com.julardepick.immirror.inventory;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;
import android.util.LruCache;

import java.io.IOException;
import java.io.InputStream;
import java.util.HashSet;
import java.util.Set;

/*
 * 物品图标位图缓存.
 *
 * 图标随 APK 放在 assets/items 下 (由 scripts/pack_assets.py 从素材仓库复制),
 * 一张图只解码一次, 之后按路径命中缓存. 图集图标与缺图物品没有独立位图,
 * 这里直接返回空, 由格子视图绘制占位色块, 因此缺任何一张图都不会影响整页.
 *
 * 解码尺寸按显示像素裁剪: 首次解码时按目标边长做一次降采样,
 * 避免把大图整张读进内存后又缩到几十像素.
 */
public final class ItemIconCache {

    private static final String TAG = "ImMirrorInventory";

    /* 图标合计不超过数百 KiB, 上限按 4 MiB 设, 正常使用不会触发淘汰. */
    private static final int CACHE_BYTES = 4 * 1024 * 1024;

    private static final LruCache<String, Bitmap> CACHE = new LruCache<String, Bitmap>(CACHE_BYTES) {
        @Override
        protected int sizeOf(String key, Bitmap value) {
            return value.getByteCount();
        }
    };

    /* 解析过但不存在的路径, 避免每次重绘都去打开一次 assets. */
    private static final Set<String> MISSING = new HashSet<>();

    private ItemIconCache() {
    }

    /*
     * 取物品图标. 没有独立图标, 文件缺失或解码失败都返回空,
     * 调用方据此绘制占位色块.
     */
    public static Bitmap bitmap(Context context, InventoryItem item, int targetPixels) {
        if (context == null || item == null) {
            return null;
        }
        String path = item.iconAssetPath();
        if (path.isEmpty()) {
            return null;
        }
        Bitmap cached = CACHE.get(path);
        if (cached != null && !cached.isRecycled()) {
            return cached;
        }
        if (MISSING.contains(path)) {
            return null;
        }
        Bitmap decoded = decode(context, path, targetPixels);
        if (decoded == null) {
            MISSING.add(path);
            return null;
        }
        CACHE.put(path, decoded);
        return decoded;
    }

    /* 清空缓存与缺图登记, 供界面重建或内存告急时回收. */
    public static void clear() {
        CACHE.evictAll();
        MISSING.clear();
    }

    private static Bitmap decode(Context context, String path, int targetPixels) {
        try (InputStream stream = context.getAssets().open(path)) {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inScaled = false;
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;
            options.inSampleSize = sampleSize(context, path, targetPixels);
            return BitmapFactory.decodeStream(stream, null, options);
        } catch (IOException error) {
            Log.w(TAG, "物品图标缺失: " + path);
            return null;
        }
    }

    /*
     * 按目标边长算降采样倍数. 只读图头取原始尺寸, 不分配整张位图;
     * 图头读取失败时按不降采样处理, 交给解码失败分支兜底.
     */
    private static int sampleSize(Context context, String path, int targetPixels) {
        if (targetPixels <= 0) {
            return 1;
        }
        try (InputStream stream = context.getAssets().open(path)) {
            BitmapFactory.Options bounds = new BitmapFactory.Options();
            bounds.inJustDecodeBounds = true;
            bounds.inScaled = false;
            BitmapFactory.decodeStream(stream, null, bounds);
            int longest = Math.max(bounds.outWidth, bounds.outHeight);
            int sample = 1;
            while (longest / (sample * 2) >= targetPixels) {
                sample *= 2;
            }
            return sample;
        } catch (IOException error) {
            return 1;
        }
    }
}
