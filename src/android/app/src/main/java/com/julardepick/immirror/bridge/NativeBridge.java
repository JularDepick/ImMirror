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

    /*
     * 设置系统. 原生侧持有唯一一份设置快照并负责落盘,
     * Java 侧只做展示与转发, 因此设置界面以后接入时不必再复制一套存储.
     * 全部是低频调用, 逐帧路径不经过这里.
     */

    /* 指定应用内部存储目录并载入设置; 重复调用幂等. */
    public static boolean initSettings(String dataPath) {
        if (!ensureLoaded()) {
            return false;
        }
        return nativeSettingsInit(dataPath == null ? "" : dataPath);
    }

    /* 立即落盘, 建议在 Activity 进入后台时调用. */
    public static boolean flushSettings() {
        if (!ensureLoaded()) {
            return false;
        }
        return nativeSettingsFlush();
    }

    /* 读取单项设置; 键名未知时返回 -1. */
    public static int getSetting(String key) {
        if (!ensureLoaded() || key == null) {
            return -1;
        }
        return nativeSettingsGetInt(key);
    }

    /* 写入单项设置; 返回裁剪后的落定值, 键名未知时返回 -1. */
    public static int setSetting(String key, int value) {
        if (!ensureLoaded() || key == null) {
            return -1;
        }
        return nativeSettingsSetInt(key, value);
    }

    /* 全量导出, 每行 "键=值". */
    public static String exportSettings() {
        if (!ensureLoaded()) {
            return "";
        }
        return nativeSettingsExport();
    }

    /* 键表元数据, 每行 "键|默认值|最小值|最大值". */
    public static String describeSettings() {
        if (!ensureLoaded()) {
            return "";
        }
        return nativeSettingsDescribe();
    }

    /* 恢复单项默认值; key 为空串时恢复全部. */
    public static boolean resetSetting(String key) {
        if (!ensureLoaded()) {
            return false;
        }
        return nativeSettingsReset(key == null ? "" : key);
    }

    /* 取走原生侧聚合的成就统计增量, 返回 {"break_block":..,"place_block":..,"pick_item":..}; 拉取即清零. */
    public static String takeProgressCounters() {
        if (!ensureLoaded()) {
            return "{}";
        }
        return nativeTakeProgressCounters();
    }

    private static native String nativeTakeProgressCounters();

    private static native void nativeOnDeviceProfile(String payload);

    private static native void nativeStart();

    private static native void nativeStop();

    private static native void nativeAttachAudio(SoundPool pool, int[] soundIds);

    private static native void nativeDetachAudio();

    private static native boolean nativeSettingsInit(String dataPath);

    private static native boolean nativeSettingsFlush();

    private static native int nativeSettingsGetInt(String key);

    private static native int nativeSettingsSetInt(String key, int value);

    private static native String nativeSettingsExport();

    private static native String nativeSettingsDescribe();

    private static native boolean nativeSettingsReset(String key);
}
