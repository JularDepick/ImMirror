package com.julardepick.immirror.audio;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.MediaPlayer;
import android.util.Log;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/*
 * 环境音与背景音乐的长音频通道.
 * SoundPool 适合短音效 (挖掘/放置/行走), 环境音与音乐用 MediaPlayer 循环或顺序播放.
 * 场景切换由原生侧通过 JNI 推送触发.
 */
public final class AmbientAudioManager {

    private static final String TAG = "ImMirrorAmbient";

    /* 环境音列表: 与 assets/sounds/env_*.ogg 一致. */
    private static final String[] AMBIENT_TRACKS = {
        "sounds/env/env_Jungleday.ogg",
        "sounds/env/env_Junglenight.ogg",
        "sounds/env/env_lava.ogg",
        "sounds/env/env_rain1.ogg",
        "sounds/env/env_rain2.ogg",
        "sounds/env/env_rain3.ogg",
        "sounds/env/env_rain4.ogg",
        "sounds/env/env_portal.ogg",
    };

    /* 音乐列表: 与 assets/sounds/music_*.ogg 一致. */
    private static final String[] MUSIC_TRACKS = {
        "sounds/music/music_1.ogg",
        "sounds/music/music_2.ogg",
        "sounds/music/music_3.ogg",
    };

    private final Context context;
    private MediaPlayer ambientPlayer;
    private MediaPlayer musicPlayer;
    private final Random random = new Random();
    private int ambientIndex = -1;
    private boolean ambientEnabled = true;
    private boolean musicEnabled = true;
    private float masterVolume = 1.0f;

    public AmbientAudioManager(Context context) {
        this.context = context.getApplicationContext();
    }

    /* 播放环境音; 随机选一首循环. */
    public void playAmbient() {
        if (!ambientEnabled || ambientPlayer != null) {
            return;
        }
        try {
            final int index = random.nextInt(AMBIENT_TRACKS.length);
            ambientIndex = index;
            ambientPlayer = createPlayer(AMBIENT_TRACKS[index], true);
            applyVolume(ambientPlayer);
            ambientPlayer.start();
            Log.i(TAG, "ambient started: " + AMBIENT_TRACKS[index]);
        } catch (Exception error) {
            Log.w(TAG, "ambient play failed: " + error);
            releaseAmbient();
        }
    }

    /* 停止环境音. */
    public void stopAmbient() {
        releaseAmbient();
    }

    /* 播放背景音乐; 随机选一首. */
    public void playMusic() {
        if (!musicEnabled || musicPlayer != null) {
            return;
        }
        try {
            final int index = random.nextInt(MUSIC_TRACKS.length);
            musicPlayer = createPlayer(MUSIC_TRACKS[index], false);
            applyVolume(musicPlayer);
            musicPlayer.start();
            Log.i(TAG, "music started: " + MUSIC_TRACKS[index]);
        } catch (Exception error) {
            Log.w(TAG, "music play failed: " + error);
            releaseMusic();
        }
    }

    /* 停止音乐. */
    public void stopMusic() {
        releaseMusic();
    }

    public void setAmbientEnabled(boolean enabled) {
        ambientEnabled = enabled;
        if (!enabled) {
            stopAmbient();
        }
    }

    public void setMusicEnabled(boolean enabled) {
        musicEnabled = enabled;
        if (!enabled) {
            stopMusic();
        }
    }

    public void setMasterVolume(float volume) {
        masterVolume = volume;
        if (ambientPlayer != null) {
            applyVolume(ambientPlayer);
        }
        if (musicPlayer != null) {
            applyVolume(musicPlayer);
        }
    }

    public void releaseAll() {
        releaseAmbient();
        releaseMusic();
    }

    private MediaPlayer createPlayer(String assetPath, boolean looping) throws IOException {
        final MediaPlayer player = new MediaPlayer();
        final AudioAttributes attributes = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
        player.setAudioAttributes(attributes);
        player.setDataSource(context.getAssets().openFd(assetPath));
        player.setLooping(looping);
        player.setOnCompletionListener(mp -> {
            // 非循环播放完成后释放
            mp.release();
            if (mp == musicPlayer) {
                musicPlayer = null;
            }
        });
        player.prepare();
        return player;
    }

    private void applyVolume(MediaPlayer player) {
        final float left = masterVolume;
        final float right = masterVolume;
        player.setVolume(left, right);
    }

    private void releaseAmbient() {
        if (ambientPlayer != null) {
            try {
                ambientPlayer.stop();
                ambientPlayer.release();
            } catch (RuntimeException error) {
                Log.w(TAG, "ambient release: " + error);
            }
            ambientPlayer = null;
        }
    }

    private void releaseMusic() {
        if (musicPlayer != null) {
            try {
                musicPlayer.stop();
                musicPlayer.release();
            } catch (RuntimeException error) {
                Log.w(TAG, "music release: " + error);
            }
            musicPlayer = null;
        }
    }
}
