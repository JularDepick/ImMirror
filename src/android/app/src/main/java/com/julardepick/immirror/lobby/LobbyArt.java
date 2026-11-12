package com.julardepick.immirror.lobby;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.util.LruCache;

import com.julardepick.immirror.ui.NineSliceDrawable;

import java.io.IOException;
import java.io.InputStream;

/*
 * 大厅素材装载与绘制.
 *
 * 九宫切片, 三段横向切图与整图绘制; 位图按路径缓存, 缺失时返回 null.
 */
public final class LobbyArt {

    /* APK 内大厅素材根目录. */
    private static final String ROOT = "ui/lobby/";

    private static final String TAG = "ImMirrorLobbyArt";

    /* 素材合计不足 85 KiB, 解码后按四倍膨胀算也在 8 MiB 以内, 正常使用不会触发淘汰. */
    private static final int CACHE_BYTES = 8 * 1024 * 1024;

    private static final LruCache<String, Bitmap> CACHE = new LruCache<String, Bitmap>(CACHE_BYTES) {
        @Override
        protected int sizeOf(String key, Bitmap value) {
            return value.getByteCount();
        }
    };

    /* 解码失败只提示一次, 避免每次重绘都刷日志. */
    private static boolean decodeFailureLogged;

    /* 模板纹理: 名称对应规格第 2 节模板表的导出文件列. */
    public static final String TOP_BKG = ROOT + "top_bkg_template.png";
    public static final String ROLE_INFO = ROOT + "role_info_bkg_template.png";
    public static final String CARD_WHITE = ROOT + "template_big_bkg2.png";
    public static final String BOARD_DARK = ROOT + "template_bkg8.png";
    public static final String CHIP = ROOT + "template_bkg55.png";
    public static final String CLOSE_BTN = ROOT + "template_close_btn0.png";
    public static final String BUTTON_MIDDLE = ROOT + "template_btn11.png";

    /* 直接引用的补充纹理: 见规格第 6 节. */
    public static final String TITLE_HALL = ROOT + "bigtex/img_title_hall.png";
    public static final String BOARD_BOTTOM = ROOT + "bigtex/img_board_bottom.png";

    /* 大厅顶部图标 (56x56): 设置按钮使用齿轮图标. */
    public static final String ICON_HALLTOP_SET = ROOT + "main_lobby_icon_halltop_set.png";

    /* 大厅入口按钮 (minilobby 页): 开始游戏, 工坊, 联机, 资源四个入口的原尺寸按钮. */
    public static final String HALL_ORANGE = ROOT + "main_lobby_btn_orange_hall.png";
    public static final String HALL_BLUE = ROOT + "main_lobby_btn_blue_hall.png";
    public static final String HALL_GREEN = ROOT + "main_lobby_btn_lgreen_hall.png";
    public static final String HALL_PURPLE = ROOT + "main_lobby_btn_purple_hall.png";
    /* 大厅全屏底图: texture0/bigtex/main_lobby, 640x360 设计分辨率, 铺满屏幕做最底层背景. */
    public static final String MAIN_LOBBY_BG = ROOT + "bigtex/main_lobby.png";
    public static final String AVATAR_ROLE = ROOT + "extra/1.png";
    public static final String ICON_COIN = ROOT + "extra/icon_coin.png";
    public static final String ICON_DOT = ROOT + "extra/img_dot_tips.png";
    public static final String LINE_BLUE = ROOT + "extra/line_blue_hall.png";

    private LobbyArt() {
    }

    /* 一个素材像素对应的屏幕像素, 取设备密度让素材与 dp 布局同一标尺. */
    public static float artScale(Context context) {
        return context.getResources().getDisplayMetrics().density;
    }

    /* 设计像素折算屏幕像素, 与 artScale 同一标尺. */
    public static int designSize(Context context, float designPixels) {
        return Math.max(1, Math.round(designPixels * artScale(context)));
    }

    /* 读取素材位图, 缺失或解码失败返回 null. */
    public static Bitmap bitmap(Context context, String assetPath) {
        Bitmap cached = CACHE.get(assetPath);
        if (cached != null && !cached.isRecycled()) {
            return cached;
        }
        Bitmap decoded = decode(context, assetPath);
        if (decoded != null) {
            CACHE.put(assetPath, decoded);
        }
        return decoded;
    }

    /*
     * 九宫绘制器: 四边边界单位为素材像素, 四角原位绘制, 四边与中心拉伸.
     * 素材缺失返回 null.
     */
    public static Drawable nineSlice(Context context, String assetPath, int insetLeft, int insetTop,
                                     int insetRight, int insetBottom) {
        Bitmap source = bitmap(context, assetPath);
        if (source == null) {
            return null;
        }
        NineSliceDrawable drawable = new NineSliceDrawable(source, insetLeft, insetTop,
                insetRight, insetBottom);
        drawable.setArtScale(artScale(context));
        return drawable;
    }

    /*
     * 三段横向切图绘制器: 左右两段按素材像素定宽, 中段横向拉伸, 高度随控件.
     * 素材缺失返回 null.
     */
    public static Drawable centerSlice(Context context, String assetPath, int leftWidth,
                                       int middleWidth, int rightWidth) {
        Bitmap source = bitmap(context, assetPath);
        if (source == null) {
            return null;
        }
        CenterSliceDrawable drawable = new CenterSliceDrawable(source, leftWidth, middleWidth,
                rightWidth);
        drawable.setArtScale(artScale(context));
        return drawable;
    }

    /* 整图绘制器: 按控件尺寸拉伸整张位图, 素材缺失返回 null. */
    public static Drawable image(Context context, String assetPath) {
        Bitmap source = bitmap(context, assetPath);
        if (source == null) {
            return null;
        }
        BitmapDrawable drawable = new BitmapDrawable(context.getResources(), source);
        drawable.setFilterBitmap(true);
        drawable.setGravity(android.view.Gravity.FILL);
        return drawable;
    }

    private static Bitmap decode(Context context, String assetPath) {
        try (InputStream stream = context.getAssets().open(assetPath)) {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inScaled = false;
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;
            return BitmapFactory.decodeStream(stream, null, options);
        } catch (IOException error) {
            if (!decodeFailureLogged) {
                decodeFailureLogged = true;
                Log.w(TAG, "大厅素材缺失, 回落纯色绘制: " + assetPath, error);
            }
            return null;
        }
    }

    /*
     * 三段横向切图.
     *
     * DrawType 为 center 的模板只允许横向拉伸: 左右两段保持素材原始宽度, 中段
     * (素材像素 leftWidth..width-rightWidth) 拉伸填满剩余宽度, 高度随控件缩放,
     * 因此按钮两端的圆角与描边粗细不随控件宽度变化.
     */
    private static final class CenterSliceDrawable extends Drawable {

        private final Paint paint = new Paint();

        private final Bitmap bitmap;
        private final int sourceWidth;
        private final int sourceHeight;
        private final int insetLeft;
        private final int insetMiddle;
        private final int insetRight;

        private final Rect sourceRect = new Rect();
        private final Rect targetRect = new Rect();

        private float artScale = 1f;
        private int alpha = 255;

        CenterSliceDrawable(Bitmap bitmap, int insetLeft, int insetMiddle, int insetRight) {
            this.bitmap = bitmap;
            this.sourceWidth = bitmap.getWidth();
            this.sourceHeight = bitmap.getHeight();
            this.insetLeft = Math.max(0, insetLeft);
            this.insetMiddle = Math.max(0, insetMiddle);
            this.insetRight = Math.max(0, insetRight);
            /* 三段图多为带渐变圆角的按钮, 放大保留过渡, 关闭最近邻. */
            paint.setFilterBitmap(true);
            paint.setAntiAlias(false);
        }

        void setArtScale(float value) {
            artScale = value > 0f ? value : 1f;
            invalidateSelf();
        }

        @Override
        public void draw(Canvas canvas) {
            final Rect bounds = getBounds();
            if (bounds.isEmpty() || bitmap.isRecycled()) {
                return;
            }
            int leftWidth = Math.round(insetLeft * artScale);
            int rightWidth = Math.round(insetRight * artScale);
            if (leftWidth + rightWidth >= bounds.width()) {
                /* 目标宽度不足两端固定段之和时按比例压缩, 至少保留一像素中段. */
                int available = Math.max(0, bounds.width() - 1);
                int total = leftWidth + rightWidth;
                if (available == 0 || total == 0) {
                    leftWidth = 0;
                    rightWidth = 0;
                } else {
                    leftWidth = (int) Math.round((double) leftWidth * available / total);
                    leftWidth = Math.max(0, Math.min(available, leftWidth));
                    rightWidth = available - leftWidth;
                }
            }
            final int middleWidth = bounds.width() - leftWidth - rightWidth;
            final int sourceLeft = Math.min(insetLeft, sourceWidth);
            final int sourceRight = Math.max(sourceLeft,
                    sourceWidth - Math.min(insetRight, sourceWidth));

            if (leftWidth > 0 && sourceLeft > 0) {
                drawSegment(canvas, 0, sourceLeft, bounds.left, bounds.left + leftWidth,
                        bounds.top, bounds.bottom);
            }
            if (middleWidth > 0 && sourceRight > sourceLeft) {
                drawSegment(canvas, sourceLeft, sourceRight, bounds.left + leftWidth,
                        bounds.left + leftWidth + middleWidth, bounds.top, bounds.bottom);
            }
            if (rightWidth > 0 && sourceWidth > sourceRight) {
                drawSegment(canvas, sourceRight, sourceWidth, bounds.right - rightWidth,
                        bounds.right, bounds.top, bounds.bottom);
            }
        }

        private void drawSegment(Canvas canvas, int srcLeft, int srcRight, int dstLeft, int dstRight,
                                 int dstTop, int dstBottom) {
            sourceRect.set(srcLeft, 0, srcRight, sourceHeight);
            targetRect.set(dstLeft, dstTop, dstRight, dstBottom);
            canvas.drawBitmap(bitmap, sourceRect, targetRect, paint);
        }

        @Override
        public void setAlpha(int value) {
            alpha = Math.max(0, Math.min(255, value));
            paint.setAlpha(alpha);
            invalidateSelf();
        }

        @Override
        public int getAlpha() {
            return alpha;
        }

        @Override
        public void setColorFilter(ColorFilter colorFilter) {
            paint.setColorFilter(colorFilter);
            invalidateSelf();
        }

        @Override
        public int getOpacity() {
            return PixelFormat.TRANSLUCENT;
        }
    }
}
