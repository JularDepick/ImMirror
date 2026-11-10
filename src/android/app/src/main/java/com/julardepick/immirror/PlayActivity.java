package com.julardepick.immirror;

import android.app.NativeActivity;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.os.Bundle;
import android.util.Log;
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
        "sounds/dig_grass.ogg",
        "sounds/dig_stone.ogg",
        "sounds/dig_sand.ogg",
        "sounds/dig_wood.ogg",
        "sounds/dig_snow.ogg",
        "sounds/dig_gravel.ogg",
        "sounds/place_grass.ogg",
        "sounds/place_stone.ogg",
        "sounds/place_sand.ogg",
        "sounds/place_wood.ogg",
        "sounds/place_snow.ogg",
        "sounds/place_gravel.ogg",
    };

    private static final String TAG = "ImMirrorPlay";

    private SoundPool soundPool;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        /* 设置目录与存档目录同源, 都取应用内部存储根目录; 先初始化再启动主循环. */
        NativeBridge.initSettings(getFilesDir().getAbsolutePath());
        NativeBridge.onDeviceProfile(DeviceProfile.obtain(this).toNativePayload());
        setupAudio();
        NativeBridge.start();
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

    @Override
    protected void onDestroy() {
        /*
         * 顺序固定: 先结束本次会话的玩法并落盘, 再释放音频引用, 最后交给父类.
         * 父类会等待原生线程返回, 而原生主循环要一直跑事件泵才能让这次等待结束,
         * 因此这里只表达“离开游戏”, 不表达“关闭进程内的原生线程”.
         */
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