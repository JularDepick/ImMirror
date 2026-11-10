package com.julardepick.immirror.account;

import android.os.Handler;
import android.os.Looper;

/*
 * 验证码发送倒计时.
 *
 * 冷却秒数由服务端返回, 这里只负责按秒回调剩余时间并在归零时收尾;
 * 发送按钮据此禁用并显示剩余秒数, 界面层不重复写计时逻辑.
 */
public final class CodeCountdown {

    public interface Listener {
        void onTick(int remainingSeconds);

        void onFinish();
    }

    private final Handler handler = new Handler(Looper.getMainLooper());

    private final Runnable ticker = new Runnable() {
        @Override
        public void run() {
            if (listener == null) {
                return;
            }
            remaining -= 1;
            if (remaining <= 0) {
                Listener finished = listener;
                listener = null;
                finished.onFinish();
                return;
            }
            listener.onTick(remaining);
            handler.postDelayed(this, 1000L);
        }
    };

    private int remaining;
    private Listener listener;

    public void start(int seconds, Listener target) {
        cancel();
        remaining = Math.max(1, seconds);
        listener = target;
        listener.onTick(remaining);
        handler.postDelayed(ticker, 1000L);
    }

    public void cancel() {
        handler.removeCallbacks(ticker);
        listener = null;
        remaining = 0;
    }

    public boolean isRunning() {
        return listener != null;
    }
}
