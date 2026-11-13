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

    /*
     * 把音效池与音效句柄推给原生侧; 音效播放属于事件触发的低频调用.
     * soundIds 按族顺序平铺多变体句柄, familyCounts 给出各族变体数.
     */
    public static void attachAudio(SoundPool pool, int[] soundIds, int[] familyCounts) {
        if (!ensureLoaded()) {
            return;
        }
        nativeAttachAudioEx(pool, soundIds, familyCounts);
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

    /* 选择要加载的世界存档; 试玩模式由编辑器导出后传入工程名. */
    public static void selectWorld(String projectName) {
        if (!ensureLoaded()) {
            return;
        }
        nativeSelectWorld(projectName == null ? "" : projectName);
    }

    /*
     * 多世界列表: 扫描内部存储 worlds 目录下的全部存档,
     * 返回 JSON 数组 (file,name,lastPlayed,seed,playerX,playerZ), 空目录返回 [].
     */
    public static String listWorlds(String directory) {
        if (!ensureLoaded()) {
            return "[]";
        }
        return nativeListWorlds(directory == null ? "" : directory);
    }

    /* 选择游玩的世界: 载入已有存档或按名字新建; 与试玩模式的 selectWorld 互不影响. */
    public static void chooseWorld(String worldName) {
        if (!ensureLoaded()) {
            return;
        }
        nativeChooseWorld(worldName == null ? "" : worldName);
    }

    /*
     * 按指定种子选择世界: 联机房间用房间种子, 让同一房间的各方生成同一份地形.
     * 种子为 0 时退回按世界名派生, 与 chooseWorld 等价.
     */
    public static void chooseWorldWithSeed(String worldName, int seed) {
        if (!ensureLoaded()) {
            return;
        }
        nativeChooseWorldSeed(worldName == null ? "" : worldName, seed);
    }

    /* 设置房间客人模式: 客人侧不再本地生成生物, 生物由房主上报驱动. */
    public static void setGuestMode(boolean guest) {
        if (!ensureLoaded()) {
            return;
        }
        nativeSetGuestMode(guest);
    }

    /* 切换暂停菜单; 由返回键触发, 世界推进随之冻结或恢复. */
    public static void togglePauseMenu() {
        if (!ensureLoaded()) {
            return;
        }
        nativeTogglePauseMenu();
    }

    /* 取走原生界面事件: 逗号分隔的编码, 空串表示没有事件. */
    public static String takeUiEvents() {
        if (!ensureLoaded()) {
            return "";
        }
        return nativeTakeUiEvents();
    }

    /* 熔炉内容快照: 房主侧每轮上报前调用, 返回 JSON 数组文本. */
    public static String takeFurnaceSnapshot() {
        if (!ensureLoaded()) {
            return "[]";
        }
        return nativeTakeFurnaceSnapshot();
    }

    /* 远端熔炉内容: 客人侧收到房主下发的列表后调用. */
    public static void applyRemoteFurnaces(String json) {
        if (!ensureLoaded()) {
            return;
        }
        nativeApplyRemoteFurnaces(json == null ? "[]" : json);
    }

    /* 储物箱内容快照: 房主侧每轮上报前调用, 返回 JSON 数组文本. */
    public static String takeChestSnapshot() {
        if (!ensureLoaded()) {
            return "[]";
        }
        return nativeTakeChestSnapshot();
    }

    /* 远端储物箱内容: 客人侧收到房主下发的列表后调用. */
    public static void applyRemoteChests(String json) {
        if (!ensureLoaded()) {
            return;
        }
        nativeApplyRemoteChests(json == null ? "[]" : json);
    }

    /* 地面掉落物快照: 房主侧每轮上报前调用, 返回 JSON 数组文本. */
    public static String takeDropSnapshot() {
        if (!ensureLoaded()) {
            return "[]";
        }
        return nativeTakeDropSnapshot();
    }

    /* 远端掉落物: 客人侧收到房主下发的列表后调用. */
    public static void applyRemoteDrops(String json) {
        if (!ensureLoaded()) {
            return;
        }
        nativeApplyRemoteDrops(json == null ? "[]" : json);
    }

    /* 生物快照导出: 房主侧每轮上报前调用, 返回 JSON 数组文本. */
    public static String takeCreatureSnapshot() {
        if (!ensureLoaded()) {
            return "[]";
        }
        return nativeTakeCreatureSnapshot();
    }

    /* 远端生物快照: 客人侧收到房主下发的列表后调用. */
    public static void applyRemoteCreatures(String json) {
        if (!ensureLoaded()) {
            return;
        }
        nativeApplyRemoteCreatures(json == null ? "[]" : json);
    }

    /* 查询游戏内是否为白天, 供环境音按日相切换曲目. */
    public static boolean getDayPhase() {
        if (!ensureLoaded()) {
            return true;
        }
        return nativeGetDayPhase();
    }

    /*
     * 环境音场景: 0 通用, 1 雨, 2 雷暴, 3 水下, 4 岩浆.
     * 由原生侧按相机所在方块与当前天气折算, 与日相同一批轮询拉取.
     */
    public static int getAmbientContext() {
        if (!ensureLoaded()) {
            return 0;
        }
        return nativeGetAmbientContext();
    }

    /* 取玩家姿态 JSON; 供联机 presence 轮询使用. */
    public static String takePlayerPose() {
        if (!ensureLoaded()) {
            return "{}";
        }
        return nativeTakePlayerPose();
    }

    /* 取走本地方块改动 JSON, 取走后原生侧清空队列. */
    public static String takeBlockEdits() {
        if (!ensureLoaded()) {
            return "[]";
        }
        return nativeTakeBlockEdits();
    }

    /* 应用远端方块改动 JSON. */
    public static void applyBlockEdits(String json) {
        if (!ensureLoaded()) {
            return;
        }
        nativeApplyBlockEdits(json == null ? "[]" : json);
    }

    /* 接收其他玩家快照 JSON, 原生侧更新远程玩家列表. */
    public static void applyRemotePlayers(String json) {
        if (!ensureLoaded()) {
            return;
        }
        nativeApplyRemotePlayers(json);
    }

    /*
     * 环境音与音乐开关: 环境音与音乐由 Java 侧 AmbientAudioManager 直接管理,
     * 原生侧的联动接口留待场景接线时启用, 当前为预留空实现.
     */
    public static void setAmbientEnabled(boolean enabled) {
    }

    public static void setMusicEnabled(boolean enabled) {
    }

    private static native String nativeTakePlayerPose();

    private static native void nativeApplyRemotePlayers(String json);

    private static native String nativeTakeBlockEdits();

    private static native void nativeApplyBlockEdits(String json);

    private static native int nativeGetAmbientContext();

    private static native String nativeTakeProgressCounters();

    private static native void nativeSelectWorld(String projectName);

    private static native String nativeListWorlds(String directory);

    private static native void nativeChooseWorld(String worldName);

    private static native void nativeChooseWorldSeed(String worldName, int seed);

    private static native void nativeSetGuestMode(boolean guest);

    private static native void nativeTogglePauseMenu();

    private static native String nativeTakeUiEvents();

    private static native String nativeTakeFurnaceSnapshot();

    private static native void nativeApplyRemoteFurnaces(String json);

    private static native String nativeTakeChestSnapshot();

    private static native void nativeApplyRemoteChests(String json);

    private static native String nativeTakeDropSnapshot();

    private static native void nativeApplyRemoteDrops(String json);

    private static native String nativeTakeCreatureSnapshot();

    private static native void nativeApplyRemoteCreatures(String json);

    private static native boolean nativeGetDayPhase();

    private static native void nativeOnDeviceProfile(String payload);

    private static native void nativeStart();

    private static native void nativeStop();

    /*
     * 生物音效句柄银行: 原生侧按资源路径懒加载句柄后播放.
     * 与方块音效共用同一个音频池, 因此两个挂接顺序不限.
     */
    public static void attachCreatureSound(Object bank, SoundPool pool) {
        if (!ensureLoaded()) {
            return;
        }
        nativeAttachCreatureSound(bank, pool);
    }

    private static native void nativeAttachAudioEx(SoundPool pool, int[] soundIds, int[] familyCounts);

    private static native void nativeAttachCreatureSound(Object bank, SoundPool pool);

    private static native void nativeDetachAudio();

    private static native boolean nativeSettingsInit(String dataPath);

    private static native boolean nativeSettingsFlush();

    private static native int nativeSettingsGetInt(String key);

    private static native int nativeSettingsSetInt(String key, int value);

    private static native String nativeSettingsExport();

    private static native String nativeSettingsDescribe();

    private static native boolean nativeSettingsReset(String key);
}
