package com.julardepick.immirror.audio;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.util.Log;

import java.util.HashMap;
import java.util.Map;

/*
 * 生物音效句柄银行.
 *
 * 生物音效按生物种类分目录, 一个世界可能同时出现多种生物, 全部预加载会占用
 * 大量内存与解码时间, 因此改为首次播放时按需加载并缓存句柄.
 * 句柄 0 表示加载失败, 播放侧跳过, 不影响战斗逻辑.
 *
 * 调用方向是原生侧发起: 原生在受击, 死亡, 攻击时按生物种类查表得到资源路径,
 * 通过 JNI 调本类的 handleFor 拿到句柄后自行播放.
 */
public final class CreatureSoundBank {

    private static final String TAG = "ImMirrorCreatureSound";

    /* 生物音效并发上限: 混战时多只生物同时发声也不互相打断. */
    private static final int MAX_STREAMS = 6;

    private static SoundPool pool;
    private static AssetManager assets;
    private static final Map<String, Integer> HANDLES = new HashMap<>();

    private CreatureSoundBank() {
    }

    /* 由 Activity 创建时调用, 建立音频池并保存资源管理器. 重复调用幂等. */
    public static void initialize(Context context) {
        if (pool != null || context == null) {
            return;
        }
        final Context app = context.getApplicationContext();
        try {
            final AudioAttributes attributes = new AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_GAME)
                    .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                    .build();
            pool = new SoundPool.Builder()
                    .setMaxStreams(MAX_STREAMS)
                    .setAudioAttributes(attributes)
                    .build();
            assets = app.getAssets();
        } catch (Exception error) {
            Log.w(TAG, "creature sound pool unavailable: " + error);
            pool = null;
        }
    }

    /* 释放音频池, 界面销毁时调用. 重复调用安全. */
    public static void release() {
        if (pool != null) {
            pool.release();
            pool = null;
        }
        assets = null;
        HANDLES.clear();
    }

    /* 当前音频池, 供原生侧播放时复用. */
    public static SoundPool pool() {
        return pool;
    }

    /*
     * 按资源路径取句柄, 首次调用时加载. 加载失败返回 0 并记住失败结果,
     * 避免每次受击都重复尝试打开同一个缺失文件.
     */
    public static int handleFor(String assetPath) {
        if (assetPath == null || assetPath.isEmpty() || pool == null || assets == null) {
            return 0;
        }
        final Integer cached = HANDLES.get(assetPath);
        if (cached != null) {
            return cached;
        }
        int handle = 0;
        try {
            final AssetFileDescriptor descriptor = assets.openFd(assetPath);
            handle = pool.load(descriptor, 1);
            descriptor.close();
        } catch (Exception error) {
            Log.w(TAG, "creature sound load failed: " + assetPath + " " + error);
            handle = 0;
        }
        HANDLES.put(assetPath, handle);
        return handle;
    }
}
