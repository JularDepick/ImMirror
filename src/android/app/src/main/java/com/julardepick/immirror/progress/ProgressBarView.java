package com.julardepick.immirror.progress;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.View;

import com.julardepick.immirror.account.AccountTheme;

/*
 * 进度条.
 *
 * 只负责画一条圆角轨道与一段圆角填充, 文案由外部 TextView 显示,
 * 因此换字体或换语言都不影响进度条的绘制. 完成态整条填充换成成功色,
 * 与卡片右上角的完成标记呼应, 未完成时用强调色.
 */
public final class ProgressBarView extends View {

    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF track = new RectF();
    private final RectF fill = new RectF();

    /* 轨道高度与圆角, 与卡片的圆角风格保持一致. */
    private final float heightPx;
    private final float radiusPx;
    private final int trackColor;

    private float ratio;
    private boolean completed;

    public ProgressBarView(Context context) {
        this(context, null);
    }

    public ProgressBarView(Context context, AttributeSet attrs) {
        super(context, attrs);
        heightPx = AccountTheme.dp(context, 8f);
        radiusPx = heightPx / 2f;
        trackColor = (AccountTheme.COLOR_CARD_STROKE & 0x00FFFFFF) | 0x22000000;
    }

    /* 设置进度; ratio 裁剪到 0 到 1, completed 决定填充色. */
    public void setProgress(float ratio, boolean completed) {
        this.ratio = Math.max(0f, Math.min(1f, ratio));
        this.completed = completed;
        invalidate();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int width = MeasureSpec.getSize(widthMeasureSpec);
        setMeasuredDimension(width, Math.max(1, Math.round(heightPx)));
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        float width = getWidth();
        float height = getHeight();
        if (width <= 0f || height <= 0f) {
            return;
        }
        track.set(0f, 0f, width, height);
        paint.setStyle(Paint.Style.FILL);
        paint.setColor(trackColor);
        canvas.drawRoundRect(track, radiusPx, radiusPx, paint);

        if (ratio <= 0f) {
            return;
        }
        /* 填充至少一个圆角宽度, 极小的进度也能看见一截色块. */
        float filled = Math.max(radiusPx * 2f, width * ratio);
        fill.set(0f, 0f, Math.min(width, filled), height);
        paint.setColor(completed ? AccountTheme.COLOR_SUCCESS : AccountTheme.COLOR_ACCENT);
        canvas.drawRoundRect(fill, radiusPx, radiusPx, paint);
    }
}
