package com.julardepick.immirror.account;

import android.view.View;
import android.view.animation.LinearInterpolator;
import android.widget.TextView;

/*
 * 页面内提示条控制器.
 *
 * 按界面视觉规范的动效要求: 出现时立即全不透明, 停留三秒后按线性插值在零点五秒内淡出,
 * 结束后隐藏并复位透明度. 再次提示会取消上一次淡出重新计时, 避免两条提示叠加,
 * 也让短时间连续的提示都能停留满三秒. 全部提示走页面内控件, 不使用 Toast 与系统弹窗.
 */
public final class NoticeBar {

    /* 停留时长, 对应规范第七章 tips.lua 的调用参数. */
    private static final long HOLD_MILLIS = 3000L;

    /* 淡出时长, 对应规范第七章每拍减零点一乘以十拍的节拍. */
    private static final long FADE_MILLIS = 500L;

    /* 淡出结束判定阈值, 动画末帧的浮点误差内仍视为已经透明. */
    private static final float ALPHA_EPSILON = 0.01f;

    private final TextView view;

    public NoticeBar(TextView view) {
        this.view = view;
    }

    /* 显示一条提示, 语义色由调用方传入, 这里换算成深底上的浅色. */
    public void show(CharSequence message, int semanticColor) {
        if (view == null) {
            return;
        }
        view.animate().cancel();
        view.setText(message);
        view.setTextColor(noticeColor(semanticColor));
        view.setVisibility(View.VISIBLE);
        view.setAlpha(1f);
        view.animate()
                .alpha(0f)
                .setStartDelay(HOLD_MILLIS)
                .setDuration(FADE_MILLIS)
                .setInterpolator(new LinearInterpolator())
                .withEndAction(this::settle);
    }

    /* 立即清空并隐藏, 供切换模式时收起上一条提示. */
    public void clear() {
        if (view == null) {
            return;
        }
        view.animate().cancel();
        view.setText("");
        view.setAlpha(1f);
        view.setVisibility(View.GONE);
    }

    /* 淡出结束后隐藏; 被新提示取消时透明度已被重置为一, 不隐藏. */
    private void settle() {
        if (view.getAlpha() <= ALPHA_EPSILON) {
            view.setVisibility(View.GONE);
            view.setAlpha(1f);
        }
    }

    /* 提示条底为深棕黑素材, 文字统一取浅色, 再用色相区分成功与失败. */
    private static int noticeColor(int semanticColor) {
        if (semanticColor == AccountTheme.COLOR_ERROR) {
            return AccountTheme.COLOR_NOTICE_ERROR;
        }
        if (semanticColor == AccountTheme.COLOR_SUCCESS) {
            return AccountTheme.COLOR_NOTICE_SUCCESS;
        }
        return AccountTheme.COLOR_NOTICE_TEXT;
    }
}
