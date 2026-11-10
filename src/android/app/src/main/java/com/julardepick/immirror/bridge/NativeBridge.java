package com.julardepick.immirror.bridge;

import android.media.SoundPool;
import android.util.Log;

/*
 * 原生库加载入口与 JNI 声明.
 *
 * 逐帧路径不经过本类: 渲染与输入的循环完全在原生线程内完成,
 * 这里只暴露进入游戏前后各调用一次的三个低频接口.
 */
public final class NativeBridge {

    /* 共享库名, 与 manifest 中 android.app.lib_name 以及 jniLibs 内的文件名保持一致. */
    public static final String LIBRARY_NAME = "immirror_core";

    private static final String TAG = "ImMirrorBridge";

    private static volatile boolean loaded;

    private NativeBridge() {
    }

    /* 幂等加载; 失败时只记录日志, 由调用方决定是否继续. */
    public static synchronized boolean ensureLoaded() {
        if (loaded) {
            return true;
        }
        try {
            System.loadLibrary(LIBRARY_NAME);
            loaded = true;
            Log.i(TAG, "native library loaded: " + LIBRARY_NAME);
        } catch (Throwable error) {
            Log.e(TAG, "native library load failed: " + error);
        }
        return loaded;
    }

    /* 下发设备画像文本; 原生库未就绪时静默跳过. */
    public static void onDeviceProfile(String payload) {
        if (!ensureLoaded()) {
            return;
        }
        nativeOnDeviceProfile(payload == null ? "" : payload);
    }

    /* 请求原生主循环进入游戏状态. */
    public static void start() {
        if (!ensureLoaded()) {
            return;
        }
        nativeStart();
    }

    /* 请求原生主循环退出, 由胶水层完成后台线程回收. */
    public static void stop() {
        if (!ensureLoaded()) {
            return;
        }
        nativeStop();
    }

    /* 把音效池与音效句柄推给原生侧; 音效播放属于事件触发的低频调用. */
    public static void attachAudio(SoundPool pool, int[] soundIds) {
        if (!ensureLoaded()) {
            return;
        }
        nativeAttachAudio(pool, soundIds);
    }

    /* Activity 销毁前解除引用. */
    public static void detachAudio() {
        if (!ensureLoaded()) {
            return;
        }
        nativeDetachAudio();
    }

    private static native void nativeOnDeviceProfile(String payload);

    private static native void nativeStart();

    private static native void nativeStop();

    private static native void nativeAttachAudio(SoundPool pool, int[] soundIds);

    private static native void nativeDetachAudio();
}
