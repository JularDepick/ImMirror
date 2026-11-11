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
import com.julardepick.immirror.account.Session;
import com.julardepick.immirror.account.SessionStore;
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

    /* 与原生侧 SoundId 枚举顺序一致的音效资源, 按材质分族. */
    private static final String[] SOUND_FILES = {
        "sounds/dig_grass1.ogg",
        "sounds/dig_stone1.ogg",
        "sounds/dig_sand1.ogg",
        "sounds/dig_wood1.ogg",
        "sounds/dig_snow1.ogg",
        "sounds/dig_gravel1.ogg",
        "sounds/dig_cloth1.ogg",
        "sounds/dig_glass1.ogg",
        "sounds/dig_jar1.ogg",
        "sounds/place_grass1.ogg",
        "sounds/place_stone1.ogg",
        "sounds/place_sand1.ogg",
        "sounds/place_wood1.ogg",
        "sounds/place_snow1.ogg",
        "sounds/place_gravel.ogg",
        "sounds/dig_cloth1.ogg",
        "sounds/walk_grass1.ogg",
        "sounds/walk_stone1.ogg",
        "sounds/walk_sand1.ogg",
        "sounds/walk_wood1.ogg",
        "sounds/walk_snow1.ogg",
        "sounds/walk_gravel1.ogg",
        "sounds/walk_cloth1.ogg",
    };

    private static final String TAG = "ImMirrorPlay";

    /* 成就统计的上报周期. */
    private static final long PROGRESS_REPORT_INTERVAL_MS = 30000L;
    private static final long PRESENCE_INTERVAL_MS = 10000L;
    private PresenceApi presenceApi;
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
        NativeBridge.initSettings(getFilesDir().getAbsolutePath());
        if (playtestProject != null && !playtestProject.isEmpty()) {
            NativeBridge.selectWorld(playtestProject);
        }
        setupAudio();
        NativeBridge.start();
        startProgressReporting();
        startPresenceReporting();
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
        NativeBridge.stop();
        NativeBridge.flushSettings();
        NativeBridge.detachAudio();
        releaseAudio();
        super.onDestroy();
    }

    /* 音效由平台音频池播放, 原生侧只持有句柄并按事件触发. */
    private void setupAudio() {
        AudioAttributes attributes = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
        soundPool = new SoundPool.Builder()
                .setMaxStreams(4)
                .setAudioAttributes(attributes)
                .build();
        AssetManager assets = getAssets();
        int[] soundIds = new int[SOUND_FILES.length];
        for (int index = 0; index < SOUND_FILES.length; index++) {
            try {
                AssetFileDescriptor descriptor = assets.openFd(SOUND_FILES[index]);
                soundIds[index] = soundPool.load(descriptor, 1);
                descriptor.close();
            } catch (Exception error) {
                Log.w(TAG, "sound load failed: " + SOUND_FILES[index] + " " + error);
            }
        }
        NativeBridge.attachAudio(soundPool, soundIds);
    }

    private void releaseAudio() {
        if (soundPool != null) {
            soundPool.release();
            soundPool = null;
        }
    }
}