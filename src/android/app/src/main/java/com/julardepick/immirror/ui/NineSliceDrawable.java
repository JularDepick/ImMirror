package com.julardepick.immirror.ui;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;

/*
 * 九宫格绘制器.
 *
 * 把一张带固定描边的素材按四边边界切成九块: 四角原位绘制, 四边单向拉伸,
 * 中心双向拉伸, 描边与内高光的粗细因此不随控件尺寸变化, 对应原版模板库
 * 里 ninesquare 与 center 两种拉伸方式的渲染规则.
 *
 * 素材像素到屏幕像素的换算由 artScale 给出, 默认一个素材像素等于一个 dp,
 * 乘上设备密度后同一素材在不同密度下的描边视觉粗细一致; 整体缩放用于按下态.
 * 上下边界为零的三片素材同样由本类绘制, 此时上下两行退化为零高, 中段整高拉伸.
 */
public final class NineSliceDrawable extends Drawable {

    /* 关闭双线性过滤, 素材放大时保持像素硬边, 与像素画风一致. */
    private final Paint paint = new Paint();

    private final Bitmap bitmap;
    private final int sourceWidth;
    private final int sourceHeight;
    private final int insetLeft;
    private final int insetTop;
    private final int insetRight;
    private final int insetBottom;

    private final Rect sourceRect = new Rect();
    private final Rect targetRect = new Rect();

    /* 一个素材像素对应的屏幕像素, 由调用方按设备密度设置. */
    private float artScale = 1f;

    /* 整体缩放, 按下态取零点九; 缩放围绕控件中心进行. */
    private float scale = 1f;

    private int alpha = 255;

    public NineSliceDrawable(Bitmap bitmap, int insetLeft, int insetTop, int insetRight,
                             int insetBottom) {
        if (bitmap == null) {
            throw new IllegalArgumentException("九宫素材不能为空");
        }
        this.bitmap = bitmap;
        this.sourceWidth = bitmap.getWidth();
        this.sourceHeight = bitmap.getHeight();
        this.insetLeft = Math.max(0, insetLeft);
        this.insetTop = Math.max(0, insetTop);
        this.insetRight = Math.max(0, insetRight);
        this.insetBottom = Math.max(0, insetBottom);
        paint.setFilterBitmap(false);
        paint.setAntiAlias(false);
    }

    /* 复制一份参数相同的绘制器, 按下态与禁用态共用同一张位图, 不重复解码. */
    public NineSliceDrawable copy() {
        NineSliceDrawable clone =
                new NineSliceDrawable(bitmap, insetLeft, insetTop, insetRight, insetBottom);
        clone.artScale = artScale;
        clone.paint.setAlpha(paint.getAlpha());
        return clone;
    }

    public void setArtScale(float value) {
        artScale = value > 0f ? value : 1f;
        invalidateSelf();
    }

    public float getArtScale() {
        return artScale;
    }

    public void setScale(float value) {
        scale = value > 0f ? value : 1f;
        invalidateSelf();
    }

    public float getScale() {
        return scale;
    }

    @Override
    public void draw(Canvas canvas) {
        Rect bounds = getBounds();
        if (bounds.isEmpty() || bitmap.isRecycled()) {
            return;
        }

        /* 整体缩放围绕中心: 先按缩放算目标矩形, 再把多出来的边距对半分. */
        int drawWidth = Math.max(1, Math.round(bounds.width() * scale));
        int drawHeight = Math.max(1, Math.round(bounds.height() * scale));
        int left = bounds.left + (bounds.width() - drawWidth) / 2;
        int top = bounds.top + (bounds.height() - drawHeight) / 2;
        int right = left + drawWidth;
        int bottom = top + drawHeight;

        float unit = artScale * scale;
        int[] horizontal = fitInsets(insetLeft, insetRight, drawWidth, unit);
        int[] vertical = fitInsets(insetTop, insetBottom, drawHeight, unit);
        int[] xs = {left, left + horizontal[0], right - horizontal[1], right};
        int[] ys = {top, top + vertical[0], bottom - vertical[1], bottom};
        int[] sourceXs = {0, clampSource(insetLeft, sourceWidth),
                sourceWidth - clampSource(insetRight, sourceWidth), sourceWidth};
        int[] sourceYs = {0, clampSource(insetTop, sourceHeight),
                sourceHeight - clampSource(insetBottom, sourceHeight), sourceHeight};

        for (int row = 0; row < 3; row++) {
            for (int column = 0; column < 3; column++) {
                int targetLeft = xs[column];
                int targetRight = xs[column + 1];
                int targetTop = ys[row];
                int targetBottom = ys[row + 1];
                int srcLeft = sourceXs[column];
                int srcRight = sourceXs[column + 1];
                int srcTop = sourceYs[row];
                int srcBottom = sourceYs[row + 1];
                if (targetRight <= targetLeft || targetBottom <= targetTop) {
                    continue;
                }
                if (srcRight <= srcLeft || srcBottom <= srcTop) {
                    continue;
                }
                sourceRect.set(srcLeft, srcTop, srcRight, srcBottom);
                targetRect.set(targetLeft, targetTop, targetRight, targetBottom);
                canvas.drawBitmap(bitmap, sourceRect, targetRect, paint);
            }
        }
    }

    /*
     * 内容矩形: 去掉四边固定区之后可以安全摆放文字与图标的区域,
     * 供调用方在自绘控件里定位内容; 控件过小时返回整块边界.
     */
    public boolean getContentRect(Rect out) {
        Rect bounds = getBounds();
        if (bounds.isEmpty() || out == null) {
            return false;
        }
        int[] horizontal = fitInsets(insetLeft, insetRight, bounds.width(), artScale);
        int[] vertical = fitInsets(insetTop, insetBottom, bounds.height(), artScale);
        out.set(bounds.left + horizontal[0], bounds.top + vertical[0],
                bounds.right - horizontal[1], bounds.bottom - vertical[1]);
        return !out.isEmpty();
    }

    /* 素材原始的九宫边界, 单位为素材像素. */
    public int getInsetLeft() {
        return insetLeft;
    }

    public int getInsetTop() {
        return insetTop;
    }

    public int getInsetRight() {
        return insetRight;
    }

    public int getInsetBottom() {
        return insetBottom;
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

    /*
     * 目标边长不足两条固定边之和时按比例压缩, 至少保留一像素中段,
     * 避免九块互相重叠把描边画花; 素材本身边界之和超过原始边长时同理.
     */
    private static int[] fitInsets(int firstInset, int secondInset, int length, float unit) {
        int first = Math.round(firstInset * unit);
        int second = Math.round(secondInset * unit);
        if (first + second < length) {
            return new int[]{first, second};
        }
        int available = Math.max(0, length - 1);
        long total = (long) first + second;
        if (available == 0 || total == 0) {
            return new int[]{0, 0};
        }
        int scaledFirst = (int) Math.round((double) first * available / total);
        scaledFirst = Math.max(0, Math.min(available, scaledFirst));
        return new int[]{scaledFirst, available - scaledFirst};
    }

    private static int clampSource(int inset, int sourceLength) {
        return Math.max(0, Math.min(sourceLength, inset));
    }
}
