package com.julardepick.immirror.audio;

import android.content.Context;
import android.media.AudioAttributes;
import android.media.MediaPlayer;
import android.util.Log;

import java.io.IOException;
import java.util.Locale;
import java.util.Random;

/*
 * 环境音与背景音乐的长音频通道.
 * SoundPool 适合短音效 (挖掘/放置/行走), 环境音与音乐用 MediaPlayer 循环或顺序播放.
 * 场景切换由原生侧通过 JNI 推送触发; 曲表按playmain.lua 的昼夜分组语义组织,
 * 分组未接线时音乐从通用表随机取曲.
 */
public final class AmbientAudioManager {

    private static final String TAG = "ImMirrorAmbient";

    /*
     * 环境音分组常量: 每组内的曲目按场景语义取用.
     * jungle 是背景循环组 (白天与黑夜各一首), rain 含四段强度与渐入和雷声,
     * water 含入水花与游泳四段和水下与水面, lava 是岩浆循环与冒泡,
     * portal 与 transfer 是传送类单发音源, 按同名分组名取用.
     */
    public static final String AMBIENT_GROUP_JUNGLE = "jungle";
    public static final String AMBIENT_GROUP_RAIN = "rain";
    public static final String AMBIENT_GROUP_WATER = "water";
    public static final String AMBIENT_GROUP_LAVA = "lava";

    /*
     * 环境音分组曲表: 19 首与 assets/sounds/env 一致.
     * rainfadein 与 thunder 属于雨组 (雷雨同源于降水), splash/swim/water/underwater
     * 属于水组, 其余按字面语义归组.
     */
    private static final String[] AMBIENT_TRACKS_JUNGLE = {
        "sounds/env/env_Jungleday.ogg",
        "sounds/env/env_Junglenight.ogg",
    };
    private static final String[] AMBIENT_TRACKS_RAIN = {
        "sounds/env/env_rain1.ogg",
        "sounds/env/env_rain2.ogg",
        "sounds/env/env_rain3.ogg",
        "sounds/env/env_rain4.ogg",
        "sounds/env/env_rainfadein.ogg",
        "sounds/env/env_thunder.ogg",
    };
    private static final String[] AMBIENT_TRACKS_WATER = {
        "sounds/env/env_splash.ogg",
        "sounds/env/env_swim1.ogg",
        "sounds/env/env_swim2.ogg",
        "sounds/env/env_swim3.ogg",
        "sounds/env/env_swim4.ogg",
        "sounds/env/env_underwater.ogg",
        "sounds/env/env_water.ogg",
    };
    private static final String[] AMBIENT_TRACKS_LAVA = {
        "sounds/env/env_lava.ogg",
        "sounds/env/env_lavapop.ogg",
    };
    private static final String[] AMBIENT_TRACKS_PORTAL = {
        "sounds/env/env_portal.ogg",
    };
    private static final String[] AMBIENT_TRACKS_TRANSFER = {
        "sounds/env/env_transfer.ogg",
    };

    /*
     * 背景音乐曲表, 按playmain.lua 的昼夜分组语义组织.
     * 昼表与夜表只在对应时段播放; 通用表对齐MusicFrequency 的常驻曲池,
     * 分组未接线或未命中时段表时从通用表随机取曲.
     */
    private static final String[] MUSIC_TRACKS_DAY = {
        "sounds/music/music_bgm_day_1.ogg",
        "sounds/music/music_bgm_day_2.ogg",
        "sounds/music/music_bgm_day_3.ogg",
        "sounds/music/music_bgm_day_4.ogg",
    };
    private static final String[] MUSIC_TRACKS_NIGHT = {
        "sounds/music/music_bgm_night_1.ogg",
        "sounds/music/music_bgm_night_2.ogg",
        "sounds/music/music_bgm_night_3.ogg",
        "sounds/music/music_bgm_night_4.ogg",
    };
    private static final String[] MUSIC_TRACKS_COMMON = {
        "sounds/music/music_bgm1.ogg",
        "sounds/music/music_bgm2.ogg",
        "sounds/music/music_bgm3.ogg",
        "sounds/music/music_bgm4.ogg",
        "sounds/music/music_bgm5.ogg",
        "sounds/music/music_bgm6.ogg",
        "sounds/music/music_bgm7.ogg",
        "sounds/music/music_bgm8.ogg",
        "sounds/music/music_theme1.ogg",
        /*
         * 星球与星空曲目并入通用池: 客户端还没有维度划分, 这些曲子
         * 只作为氛围变化参与轮播; 首领曲 (earthcore_boss, star_bossmusic_final)
         * 不进通用池, 等战斗场景接线后再单独成组.
         */
        "sounds/music/music_bgm_planet_day_1.ogg",
        "sounds/music/music_bgm_planet_day_2.ogg",
        "sounds/music/music_bgm_planet_day_3.ogg",
        "sounds/music/music_bgm_planet_land.ogg",
        "sounds/music/music_bgm_planet_night_1.ogg",
        "sounds/music/music_bgm_planet_night_2.ogg",
        "sounds/music/music_bgm_planet_night_3.ogg",
        "sounds/music/music_star_bgm.ogg",
    };

    private final Context context;
    private MediaPlayer ambientPlayer;
    private MediaPlayer musicPlayer;
    private final Random random = new Random();
    private int ambientIndex = -1;
    private boolean ambientEnabled = true;
    private boolean musicEnabled = true;
    private float masterVolume = 1.0f;
    /*
     * 水下压减: 潜入水中时环境音与音乐整体压到三成, 与画面上的水下冷色滤镜配套,
     * 出水后恢复. 用系数而不是改主音量, 因此设置里的滑杆仍然生效.
     */
    private boolean underwater = false;
    /*
     * 昼夜相位: true 昼表, false 夜表; 默认取昼.
     * 相位接线标记: 场景侧首次调用 setDayPhase 即视为相位源已接入,
     * 之后音乐按时段表取曲; 未接线时保持通用表, 避免单侧接线导致曲风单一.
     */
    private boolean dayPhase = true;
    private boolean dayPhaseWired = false;
    /* 当前环境音分组, 未指定时播放普通循环环境音. */
    private String ambientGroup = null;

    public AmbientAudioManager(Context context) {
        this.context = context.getApplicationContext();
    }

    /* 昼夜相位切换: 在播音乐保持不动, 下一轮换曲时按新相位选表. */
    public void setDayPhase(boolean day) {
        dayPhase = day;
        dayPhaseWired = true;
    }

    /* 环境音分组切换: 空串或 null 取消分组, 回到普通循环; 在播曲目立即换组重播. */
    public void setAmbientGroup(String group) {
        if (group == null || group.length() == 0) {
            if (ambientGroup != null) {
                ambientGroup = null;
                restartAmbient();
            }
            return;
        }
        /* 组名按 ROOT locale 折小写, 避免默认 locale 影响标识符比较. */
        final String normalized = group.toLowerCase(Locale.ROOT);
        if (normalized.equals(ambientGroup)) {
            return;
        }
        ambientGroup = normalized;
        /* 分组切换是环境音唯一的状态变化点, 打一行日志便于实机核对. */
        Log.i(TAG, "ambient group -> " + normalized);
        restartAmbient();
    }

    /* 按 String 分组名取对应曲表, 未收录的组名回落 null. */
    private static String[] ambientTracksFor(String group) {
        if (group == null) {
            return null;
        }
        switch (group) {
            case AMBIENT_GROUP_JUNGLE:
                return AMBIENT_TRACKS_JUNGLE;
            case AMBIENT_GROUP_RAIN:
                return AMBIENT_TRACKS_RAIN;
            case AMBIENT_GROUP_WATER:
                return AMBIENT_TRACKS_WATER;
            case AMBIENT_GROUP_LAVA:
                return AMBIENT_TRACKS_LAVA;
            case "portal":
                return AMBIENT_TRACKS_PORTAL;
            case "transfer":
                return AMBIENT_TRACKS_TRANSFER;
            default:
                return null;
        }
    }

    /* 按当前昼夜相位取音乐曲表: 相位未接线时返回通用表. */
    private String[] musicTracksForPhase() {
        if (!dayPhaseWired) {
            return MUSIC_TRACKS_COMMON;
        }
        return dayPhase ? MUSIC_TRACKS_DAY : MUSIC_TRACKS_NIGHT;
    }

    /* 播放环境音; 随机选一首循环, 分组生效时从该组取曲. */
    public void playAmbient() {
        if (!ambientEnabled || ambientPlayer != null) {
            return;
        }
        try {
            String[] tracks = ambientTracksFor(ambientGroup);
            if (tracks == null) {
                tracks = AMBIENT_TRACKS_JUNGLE;
            }
            final int index = random.nextInt(tracks.length);
            ambientIndex = index;
            ambientPlayer = createPlayer(tracks[index], true, null);
            applyVolume(ambientPlayer);
            ambientPlayer.start();
            Log.i(TAG, "ambient started: " + tracks[index]);
        } catch (Exception error) {
            Log.w(TAG, "ambient play failed: " + error);
            releaseAmbient();
        }
    }

    /* 停止环境音. */
    public void stopAmbient() {
        releaseAmbient();
    }

    /* 播放背景音乐; 随机选一首, 播完由完成回调续下一首. */
    public void playMusic() {
        if (!musicEnabled || musicPlayer != null) {
            return;
        }
        try {
            final String[] tracks = musicTracksForPhase();
            final int index = random.nextInt(tracks.length);
            musicPlayer = createPlayer(tracks[index], false, new MusicAdvanceListener());
            applyVolume(musicPlayer);
            musicPlayer.start();
            Log.i(TAG, "music started: " + tracks[index]);
        } catch (Exception error) {
            Log.w(TAG, "music play failed: " + error);
            releaseMusic();
        }
    }

    /*
     * 音乐播完回调: 释放旧播放器后自动续下一首随机曲目,
     * 对齐MusicFrequency 播完换曲的循环行为.
     */
    private final class MusicAdvanceListener implements MediaPlayer.OnCompletionListener {
        @Override
        public void onCompletion(MediaPlayer mp) {
            releaseMusic();
            if (musicEnabled) {
                playMusic();
            }
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

    /*
     * 音乐通道开关. 关闭时立即停曲并一并停掉环境音氛围通道,
     * 打开时只恢复开关状态, 是否立刻起播由调用方决定 (syncSettings 里按需重播).
     */
    public void setMusicEnabled(boolean enabled) {
        musicEnabled = enabled;
        if (!enabled) {
            stopMusic();
            stopAmbient();
        }
    }

    /*
     * 设置层联动入口: 音量与音乐开关一次同步到位.
     * 音量取 0 到 1 的线性系数, 音乐开关关闭时立即停曲, 打开时按需重播;
     * 环境音作为氛围通道跟随音乐开关, 与设置界面"音乐开关"单一语义保持一致.
     */
    /* 潜水状态变化时调用: 只改系数, 下一次调音量时自然生效. */
    public void setUnderwater(boolean value) {
        underwater = value;
    }

    public void syncSettings(float volume, boolean musicOn) {
        setMasterVolume(volume);
        if (!musicOn) {
            setMusicEnabled(false);
            return;
        }
        setMusicEnabled(true);
        playMusic();
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

    private MediaPlayer createPlayer(String assetPath, boolean looping,
                                     MediaPlayer.OnCompletionListener listener) throws IOException {
        final MediaPlayer player = new MediaPlayer();
        final AudioAttributes attributes = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
        player.setAudioAttributes(attributes);
        player.setDataSource(context.getAssets().openFd(assetPath));
        player.setLooping(looping);
        if (listener != null) {
            player.setOnCompletionListener(listener);
        }
        player.prepare();
        return player;
    }

    private void applyVolume(MediaPlayer player) {
        final float left = masterVolume;
        final float right = masterVolume;
        /* 水下时整体压到三成, 出水恢复. */
        final float muffle = underwater ? 0.3f : 1.0f;
        player.setVolume(left * muffle, right * muffle);
    }

    /* 释放当前环境音并按当前分组重播; 分组切换路径专用. */
    private void restartAmbient() {
        releaseAmbient();
        if (ambientEnabled) {
            playAmbient();
        }
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
