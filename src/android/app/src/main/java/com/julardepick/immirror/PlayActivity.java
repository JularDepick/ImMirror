package com.julardepick.immirror;

import android.app.NativeActivity;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.julardepick.immirror.account.ApiClient;
import com.julardepick.immirror.lobby.MainLobbyActivity;
import com.julardepick.immirror.account.Session;
import com.julardepick.immirror.account.SessionStore;
import com.julardepick.immirror.audio.AmbientAudioManager;
import com.julardepick.immirror.editor.EditorBridge;
import com.julardepick.immirror.net.PresenceApi;
import com.julardepick.immirror.progress.ProgressApi;
import com.julardepick.immirror.progress.ProgressBoard;
import android.view.WindowManager;

import com.julardepick.immirror.bridge.NativeBridge;
import com.julardepick.immirror.device.DeviceProfile;

/*
 * 游戏界面入口. 继承 NativeActivity, 由 android_native_app_glue 在独立线程上
 * 驱动 C++ 主循环; Java 侧只在进入与退出时各介入一次, 不参与逐帧逻辑.
 *
 * 每次进入游戏都是一个新的 Activity 实例与一条新的原生线程, 但进程与原生库
 * 的全局状态是复用的: 因此启动与停止请求只描述本次会话的玩法开关,
 * 原生主循环的存活只跟随 Activity 的销毁.
 *
 * 原生库由 NativeActivity 依据 manifest 中的 android.app.lib_name 自行加载,
 * 因此这里不需要手动调用加载接口, 但仍通过 NativeBridge 下发设备画像.
 */
public final class PlayActivity extends NativeActivity {

    /*
     * 与原生侧 SoundId 枚举顺序一致的音效资源, 按材质分族加载.
     * 每族按 "族名, 变体数, 别名族" 描述: 变体数大于 1 时按 assets 内
     * 1 起始的连续编号取文件 (挖掘族 1..4 号, 行走族取 1..6 号中的前 4 个);
     * 变体数为 1 的放置族在 APK 内是无编号单文件, 按裸名取文件.
     * 别名族不加载新文件, 直接复用目标族已加载的句柄:
     * 原版放置音没有布族的独立音源, place_cloth 复用 dig_cloth 变体.
     * 原版 jar 族没有独立行走音, walk 侧不设 jar 档; 原版 glass 族没有放置音,
     * place 侧不设 glass 档, 两处都由原生侧按近亲材质回落.
     */
    private static final String[][] SOUND_FAMILIES = {
        {"dig_grass", "4", null},
        {"dig_stone", "4", null},
        {"dig_sand", "4", null},
        {"dig_wood", "4", null},
        {"dig_snow", "4", null},
        {"dig_gravel", "4", null},
        {"dig_cloth", "4", null},
        {"dig_glass", "3", null},
        {"dig_jar", "3", null},
        {"place_grass", "1", null},
        {"place_stone", "1", null},
        {"place_sand", "1", null},
        {"place_wood", "1", null},
        {"place_snow", "1", null},
        {"place_gravel", "1", null},
        {"place_cloth", "4", "dig_cloth"},
        {"walk_grass", "4", null},
        {"walk_stone", "4", null},
        {"walk_sand", "4", null},
        {"walk_wood", "4", null},
        {"walk_snow", "4", null},
        {"walk_gravel", "4", null},
        {"walk_cloth", "4", null},
        {"walk_ladder", "4", null},
    };

    private static final String TAG = "ImMirrorPlay";

    /* 启动附加字段: 要进入或新建的世界名, 与主菜单的世界列表配套. */
    public static final String EXTRA_WORLD = "world";

    /* 成就统计的上报周期. */
    private static final long PROGRESS_REPORT_INTERVAL_MS = 30000L;
    private static final long PRESENCE_INTERVAL_MS = 10000L;
    private PresenceApi presenceApi;
    private AmbientAudioManager ambientManager;
    private Handler presenceHandler;
    private Runnable presenceRunnable;
    private Session presenceSession;

    private SoundPool soundPool;
    private Handler progressHandler;
    private Runnable progressRunnable;
    private ProgressApi progressApi;
    private Session progressSession;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        /* 设置目录与存档目录同源, 都取应用内部存储根目录; 先初始化再启动主循环. */
        NativeBridge.initSettings(getFilesDir().getAbsolutePath());
        NativeBridge.onDeviceProfile(DeviceProfile.obtain(this).toNativePayload());
        /* 试玩参数: 从编辑器进入时携带工程名, 原生侧据此加载导出的世界存档. */
        final String playtestProject = getIntent().getStringExtra(EditorBridge.EXTRA_PLAYTEST);
        if (playtestProject != null && !playtestProject.isEmpty()) {
            NativeBridge.selectWorld(playtestProject);
        } else {
            /*
             * 大厅世界选择: 从世界列表进入时携带世界名, 原生侧据此切换存档路径,
             * 名字在存档目录下没有对应文件时按新建处理. 试玩优先, 两条通道互斥.
             */
            final String lobbyWorld = getIntent().getStringExtra(EXTRA_WORLD);
            if (lobbyWorld != null && !lobbyWorld.isEmpty()) {
                NativeBridge.chooseWorld(lobbyWorld);
            }
        }
        setupAudio();
        NativeBridge.start();
        startProgressReporting();
        startPresenceReporting();
        /*
         * 环境音与背景音乐: 进游戏自动播放, 退出时释放.
         * 音量与音乐开关从设置层取值 (音量为 0 到 100 的滑杆值), 长音频通道
         * 不再使用固定音量; 音乐开关关闭时通道保持静默, 打开时进入正常轮播.
         */
        ambientManager = new AmbientAudioManager(this);
        final int volumeSetting = NativeBridge.getSetting("volume");
        final int musicSetting = NativeBridge.getSetting("musicopen");
        final float musicVolume = volumeSetting > 0 ? volumeSetting / 100.0f : 0.8f;
        ambientManager.syncSettings(musicVolume, musicSetting != 0);
        ambientManager.playAmbient();
    }

    @Override
    protected void onResume() {
        super.onResume();
        /*
         * 每次回到前台都重新接管玩法: 进程内可能先后存在多个实例
         * (例如连点"进入游戏"), 后一个实例销毁时会关闭全局的玩法开关,
         * 前一个实例恢复前台后需要重新打开, 否则画面停在上一次交换的缓冲上.
         */
        NativeBridge.start();
    }

    @Override
    protected void onPause() {
        /* 设置由原生侧异步落盘, 进入后台时强制写一次, 避免进程被回收丢改动. */
        NativeBridge.flushSettings();
        super.onPause();
    }

    /* 成就统计上报: 原生聚合增量, 周期取走上报服务端, 服务端按事件累计不重复发币. */
    private void startProgressReporting() {
        progressSession = SessionStore.load(this);
        if (progressSession == null || !progressSession.isUsable()) {
            return;
        }
        if (progressApi == null) {
            progressApi = new ProgressApi(this);
        }
        if (progressHandler == null) {
            progressHandler = new Handler(Looper.getMainLooper());
        }
        progressRunnable = new Runnable() {
            @Override
            public void run() {
                reportProgressOnce();
                progressHandler.postDelayed(this, PROGRESS_REPORT_INTERVAL_MS);
            }
        };
        progressHandler.postDelayed(progressRunnable, PROGRESS_REPORT_INTERVAL_MS);
    }

    /* 位置轮询: 上报自身并拉取他人, 10 秒周期. */
    private void startPresenceReporting() {
        presenceSession = SessionStore.load(this);
        if (presenceSession == null || !presenceSession.isUsable()) {
            return;
        }
        if (presenceApi == null) {
            presenceApi = new PresenceApi(this);
        }
        if (presenceHandler == null) {
            presenceHandler = new Handler(Looper.getMainLooper());
        }
        presenceRunnable = new Runnable() {
            @Override
            public void run() {
                presenceTick(presenceSession);
                presenceHandler.postDelayed(this, PRESENCE_INTERVAL_MS);
            }
        };
        presenceHandler.postDelayed(presenceRunnable, PRESENCE_INTERVAL_MS);
    }

    private void stopPresenceReporting() {
        if (presenceHandler != null && presenceRunnable != null) {
            presenceHandler.removeCallbacks(presenceRunnable);
        }
        presenceRunnable = null;
    }

    private void presenceTick(final Session session) {
        /* 日相与环境音: 顺带查询游戏内昼夜, 切换昼/夜环境音轨. */
        if (ambientManager != null) {
            ambientManager.setDayPhase(NativeBridge.getDayPhase());
        }
        final String pose = NativeBridge.takePlayerPose();
        if (pose == null || pose.length() <= 2) {
            return;
        }
        try {
            final org.json.JSONObject self = new org.json.JSONObject(pose);
            presenceApi.reportAndFetch(session,
                    (float) self.optDouble("x", 0),
                    (float) self.optDouble("y", 0),
                    (float) self.optDouble("z", 0),
                    (float) self.optDouble("yaw", 0),
                    (float) self.optDouble("pitch", 0),
                    new PresenceApi.Callback() {
                        @Override
                        public void onSuccess(java.util.List<com.julardepick.immirror.net.PresenceApi.Snapshot> others,
                                              long serverTime) {
                            Log.d(TAG, "presence ok, others=" + others.size());
                        }

                        @Override
                        public void onFailure(com.julardepick.immirror.account.ApiException error) {
                            Log.w(TAG, "presence failed: " + error);
                        }
                    });
        } catch (org.json.JSONException | RuntimeException error) {
            Log.w(TAG, "presence tick failed: " + error);
        }
    }

    private void stopProgressReporting() {
        if (progressHandler != null && progressRunnable != null) {
            progressHandler.removeCallbacks(progressRunnable);
        }
        progressRunnable = null;
    }

    private void reportProgressOnce() {
        final String payload = NativeBridge.takeProgressCounters();
        if (payload == null || payload.length() <= 2) {
            return;
        }
        try {
            final org.json.JSONObject stats = new org.json.JSONObject(payload);
            if (stats.length() == 0) {
                return;
            }
            progressApi.reportStats(progressSession, stats, new ProgressApi.Callback() {
                @Override
                public void onSuccess(ProgressBoard board) {
                    Log.i(TAG, "progress reported");
                }

                @Override
                public void onFailure(com.julardepick.immirror.account.ApiException error) {
                    Log.w(TAG, "progress report failed: " + error);
                }
            });
        } catch (org.json.JSONException | RuntimeException error) {
            Log.w(TAG, "progress parse failed: " + error);
        }
    }

    @Override
    protected void onDestroy() {
        /*
         * 顺序固定: 先结束本次会话的玩法并落盘, 再释放音频引用, 最后交给父类.
         * 父类会等待原生线程返回, 而原生主循环要一直跑事件泵才能让这次等待结束,
         * 因此这里只表达“离开游戏”, 不表达“关闭进程内的原生线程”.
         */
        stopProgressReporting();
        stopPresenceReporting();
        if (ambientManager != null) {
            ambientManager.releaseAll();
        }
        NativeBridge.stop();
        NativeBridge.flushSettings();
        NativeBridge.detachAudio();
        releaseAudio();
        super.onDestroy();
    }

    /*
     * 音效由平台音频池播放, 原生侧只持有句柄并按事件触发.
     * 每族按变体数展开加载: 句柄数组按 [族1变体1, 族1变体2, ..., 族2变体1, ...]
     * 平铺, 计数数组给出各族实际加载到的变体数, 原生侧按同一布局切开.
     * 别名族直接复用目标族的句柄数组, 不产生新的加载.
     * 脚步与挖掘可能并发, 并发上限提到 6, 避免脚步被挖掘挤掉.
     */
    private void setupAudio() {
        AudioAttributes attributes = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
        soundPool = new SoundPool.Builder()
                .setMaxStreams(6)
                .setAudioAttributes(attributes)
                .build();
        final AssetManager assets = getAssets();
        /* 先按族加载, 再平铺成原生侧期望的布局: 每族至少占一个槽位,
         * 整族加载失败时保留一个 0 占位槽, 播放侧对 0 句柄静默跳过. */
        int[][] familyIds = new int[SOUND_FAMILIES.length][];
        for (int family = 0; family < SOUND_FAMILIES.length; family++) {
            final String name = SOUND_FAMILIES[family][0];
            final int variantCount = Integer.parseInt(SOUND_FAMILIES[family][1]);
            final String alias = SOUND_FAMILIES[family][2];
            if (alias != null) {
                int aliasIndex = -1;
                for (int probe = 0; probe < SOUND_FAMILIES.length; probe++) {
                    if (SOUND_FAMILIES[probe][0].equals(alias)) {
                        aliasIndex = probe;
                        break;
                    }
                }
                familyIds[family] = (aliasIndex >= 0 && familyIds[aliasIndex] != null)
                        ? familyIds[aliasIndex]
                        : new int[0];
                continue;
            }
            int[] ids = new int[variantCount];
            int loaded = 0;
            for (int variant = 1; variant <= variantCount; variant++) {
                /* 单文件族 (放置族) 在 APK 内不带编号, 多变体族按 1 起始编号展开. */
                final String path = variantCount == 1
                        ? "sounds/" + name + ".ogg"
                        : "sounds/" + name + variant + ".ogg";
                try {
                    AssetFileDescriptor descriptor = assets.openFd(path);
                    final int id = soundPool.load(descriptor, 1);
                    descriptor.close();
                    if (id == 0) {
                        throw new IllegalStateException("load returned 0");
                    }
                    ids[loaded] = id;
                    loaded++;
                } catch (Exception error) {
                    Log.w(TAG, "sound load failed: " + path + " " + error);
                    break;
                }
            }
            familyIds[family] = new int[loaded];
            System.arraycopy(ids, 0, familyIds[family], 0, loaded);
        }
        int total = 0;
        for (int[] ids : familyIds) {
            total += ids.length > 0 ? ids.length : 1;
        }
        int[] soundIds = new int[total];
        int[] familyCounts = new int[SOUND_FAMILIES.length];
        int cursor = 0;
        for (int family = 0; family < SOUND_FAMILIES.length; family++) {
            final int[] ids = familyIds[family];
            familyCounts[family] = ids.length > 0 ? ids.length : 1;
            if (ids.length == 0) {
                cursor++;
                continue;
            }
            System.arraycopy(ids, 0, soundIds, cursor, ids.length);
            cursor += ids.length;
        }
        NativeBridge.attachAudio(soundPool, soundIds, familyCounts);
    }

    private void releaseAudio() {
        if (soundPool != null) {
            soundPool.release();
            soundPool = null;
        }
    }
}