package com.julardepick.immirror.ui;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.util.Log;

/*
 * 界面音效.
 *
 * 按钮点击与面板开合走独立的 SoundPool, 与游戏内音效池分开:
 * 界面音只在触摸时触发, 并发量低, 但要求低延迟, 独立池不会被游戏内
 * 长时间的音效挤占通道.
 * 素材缺失时静默降级, 不影响任何界面功能.
 */
public final class UiSound {

    private static final String TAG = "ImMirrorUiSound";

    /* 按钮点击音在安装包内的路径, 与打包后的扁平化命名一致. */
    private static final String CLICK_ASSET = "sounds/ui_button_button_click.ogg";
    /* 界面音并发上限: 连点也不会互相打断. */
    private static final int MAX_STREAMS = 4;

    private static SoundPool pool;
    private static int clickId;
    private static boolean failed;

    private UiSound() {
    }

    /* 首次使用时按需初始化, 之后复用; 初始化失败只记一次日志. */
    private static void ensure(Context context) {
        if (pool != null || failed) {
            return;
        }
        final Context app = context.getApplicationContext();
        try {
            final AudioAttributes attributes = new AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_GAME)
                    .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                    .build();
            final SoundPool created = new SoundPool.Builder()
                    .setMaxStreams(MAX_STREAMS)
                    .setAudioAttributes(attributes)
                    .build();
            final AssetManager assets = app.getAssets();
            final AssetFileDescriptor descriptor = assets.openFd(CLICK_ASSET);
            final int id = created.load(descriptor, 1);
            descriptor.close();
            pool = created;
            clickId = id;
        } catch (Exception error) {
            failed = true;
            Log.w(TAG, "ui sound unavailable: " + error);
        }
    }

    /* 播放一次按钮点击音. 未初始化成功时静默返回. */
    public static void click(Context context) {
        if (context == null) {
            return;
        }
        ensure(context);
        if (pool == null || clickId == 0) {
            return;
        }
        /* 音量固定 1.0: 界面音由系统媒体音量控制, 不跟随游戏内音量滑杆. */
        pool.play(clickId, 1.0f, 1.0f, 1, 0, 1.0f);
    }

    /* 释放音频池, 由界面退出时调用; 重复调用安全. */
    public static void release() {
        if (pool != null) {
            pool.release();
            pool = null;
            clickId = 0;
        }
    }
}
