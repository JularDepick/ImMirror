package com.julardepick.immirror.inventory;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.view.View;

import com.julardepick.immirror.account.AccountTheme;

/*
 * 一个物品格.
 *
 * 只负责把"物品 + 数量 + 选中态"画出来, 不持有物品栏状态, 也不处理业务:
 * 数据由 InventoryGridView 在快照变化时整体下发, 点击事件由网格转发给页面.
 *
 * 绘制顺序固定为底板, 图标 (或占位色块), 数量, 选中描边, 因此选中态永远压在最上层,
 * 图标超出格子时也不会盖住描边. 像素风格不做双线性插值, 缩放保持硬边.
 */
public final class InventorySlotView extends View {

    /* 底板与描边的设计细节, 集中在这里便于整体换肤. */
    private static final int COLOR_EMPTY = 0x33FFFFFF;
    private static final int COLOR_FILLED = 0x59FFFFFF;
    private static final int COLOR_HOTBAR_TINT = 0x1FFFFFFF;
    private static final int COLOR_STROKE = 0x66000000;
    private static final int COLOR_COUNT = 0xFFFFFFFF;
    private static final int COLOR_COUNT_SHADOW = 0xCC000000;

    /* 图标相对格子的内边距比例与圆角比例. */
    private static final float ICON_INSET_RATIO = 0.14f;
    private static final float CORNER_RATIO = 0.16f;

    private final Paint backgroundPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint strokePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint iconPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint countPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF rect = new RectF();

    private int slotIndex = -1;
    private InventoryItem item = InventoryItem.EMPTY;
    private int count;
    private boolean selected;
    private boolean hotbar;

    public InventorySlotView(Context context) {
        super(context);
        strokePaint.setStyle(Paint.Style.STROKE);
        strokePaint.setStrokeWidth(AccountTheme.dp(context, 1.6f));
        strokePaint.setColor(COLOR_STROKE);
        iconPaint.setFilterBitmap(false);
        iconPaint.setDither(false);
        countPaint.setTypeface(Typeface.DEFAULT_BOLD);
        countPaint.setTextAlign(Paint.Align.RIGHT);
        countPaint.setColor(COLOR_COUNT);
        countPaint.setShadowLayer(AccountTheme.dp(context, 2f), 0f, AccountTheme.dp(context, 1f),
                COLOR_COUNT_SHADOW);
        setWillNotDraw(false);
    }

    /* 一次下发一格的完整状态; 数据未变化时不触发重绘. */
    public void bind(int slotIndex, InventoryItem item, int count, boolean selected,
            boolean hotbar) {
        this.slotIndex = slotIndex;
        this.item = item == null ? InventoryItem.EMPTY : item;
        this.count = Math.max(count, 0);
        this.selected = selected;
        this.hotbar = hotbar;
        setContentDescription(describe());
        invalidate();
    }

    public int slotIndex() {
        return slotIndex;
    }

    /* 无障碍与调试用的单行描述. */
    private String describe() {
        if (count <= 0 || item.isNone()) {
            return "空格 " + (slotIndex + 1);
        }
        return item.label() + " x" + count;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        /*
         * 格子是正方形, 边长取父容器的宽度约束, 高度约束按同一值处理;
         * 网格布局会给到精确值, 这里只兜住 wrap_content 的场景.
         */
        int size = MeasureSpec.getSize(widthMeasureSpec);
        int heightMode = MeasureSpec.getMode(heightMeasureSpec);
        int heightSize = MeasureSpec.getSize(heightMeasureSpec);
        if (heightMode == MeasureSpec.EXACTLY && heightSize > 0) {
            size = Math.min(size, heightSize);
        }
        int spec = MeasureSpec.makeMeasureSpec(Math.max(size, 0), MeasureSpec.EXACTLY);
        super.onMeasure(spec, spec);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        final float width = getWidth();
        final float height = getHeight();
        if (width <= 0f || height <= 0f) {
            return;
        }
        final float corner = Math.min(width, height) * CORNER_RATIO;
        final boolean filled = count > 0 && !item.isNone();

        rect.set(0f, 0f, width, height);
        backgroundPaint.setStyle(Paint.Style.FILL);
        backgroundPaint.setColor(filled ? COLOR_FILLED : COLOR_EMPTY);
        canvas.drawRoundRect(rect, corner, corner, backgroundPaint);
        if (hotbar) {
            backgroundPaint.setColor(COLOR_HOTBAR_TINT);
            canvas.drawRoundRect(rect, corner, corner, backgroundPaint);
        }

        if (filled) {
            drawContent(canvas, width, height);
        }

        strokePaint.setColor(selected ? AccountTheme.COLOR_ACCENT : COLOR_STROKE);
        strokePaint.setStrokeWidth(AccountTheme.dp(getContext(), selected ? 2.4f : 1.2f));
        float inset = strokePaint.getStrokeWidth() * 0.5f;
        rect.set(inset, inset, width - inset, height - inset);
        canvas.drawRoundRect(rect, corner, corner, strokePaint);
    }

    /* 图标与数量. 图标缺失时画占位色块与首字, 数量为一格时不显示数字. */
    private void drawContent(Canvas canvas, float width, float height) {
        final float inset = Math.min(width, height) * ICON_INSET_RATIO;
        final RectF target = new RectF(inset, inset, width - inset, height - inset);
        Bitmap icon = ItemIconCache.bitmap(getContext(), item,
                Math.round(Math.max(target.width(), target.height())));
        if (icon != null && !icon.isRecycled()) {
            canvas.drawBitmap(icon, null, target, iconPaint);
        } else {
            drawPlaceholder(canvas, target, corner(target));
        }
        String text = count > 1 ? Integer.toString(count) : "";
        if (!text.isEmpty()) {
            countPaint.setTextSize(Math.max(height * 0.34f, AccountTheme.dp(getContext(), 9f)));
            float baseline = height - Math.max(height * 0.06f, AccountTheme.dp(getContext(), 1f));
            float right = width - Math.max(width * 0.08f, AccountTheme.dp(getContext(), 1f));
            canvas.drawText(text, right, baseline, countPaint);
        }
    }

    private float corner(RectF rect) {
        return Math.min(rect.width(), rect.height()) * CORNER_RATIO;
    }

    /* 占位色块: 色相由物品 id 派生, 同一物品每次都是同一颜色. */
    private void drawPlaceholder(Canvas canvas, RectF target, float corner) {
        float hue = item.isNone() ? 0f : (item.placeholderSeed() % 360);
        int color = Color.HSVToColor(new float[] {hue, 0.42f, 0.86f});
        backgroundPaint.setStyle(Paint.Style.FILL);
        backgroundPaint.setColor(color);
        canvas.drawRoundRect(target, corner, corner, backgroundPaint);

        countPaint.setTextAlign(Paint.Align.CENTER);
        countPaint.setColor(Color.WHITE);
        countPaint.setShadowLayer(AccountTheme.dp(getContext(), 2f), 0f, AccountTheme.dp(getContext(), 1f),
                COLOR_COUNT_SHADOW);
        countPaint.setTextSize(Math.max(target.height() * 0.5f, AccountTheme.dp(getContext(), 11f)));
        Paint.FontMetrics metrics = countPaint.getFontMetrics();
        float centerY = target.centerY() - (metrics.ascent + metrics.descent) * 0.5f;
        canvas.drawText(item.placeholderText(), target.centerX(), centerY, countPaint);
        countPaint.setTextAlign(Paint.Align.RIGHT);
    }
}
