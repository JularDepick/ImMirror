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
 * 原生库由 NativeActivity 依据 manifest 中的 android.app.lib_name 自行加载,
 * 因此这里不需要手动调用加载接口, 但仍通过 NativeBridge 下发设备画像.
 */
public final class PlayActivity extends NativeActivity {

    /* 与原生侧 SoundId 枚举顺序一致的音效资源. */
    private static final String[] SOUND_FILES = {
        "sounds/dig_grass.ogg",
        "sounds/dig_stone.ogg",
        "sounds/dig_sand.ogg",
        "sounds/place_grass.ogg",
        "sounds/place_stone.ogg",
        "sounds/place_sand.ogg",
    };

    private static final String TAG = "ImMirrorPlay";

    private SoundPool soundPool;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        NativeBridge.onDeviceProfile(DeviceProfile.obtain(this).toNativePayload());
        setupAudio();
        NativeBridge.start();
    }

    @Override
    protected void onDestroy() {
        NativeBridge.detachAudio();
        releaseAudio();
        NativeBridge.stop();
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