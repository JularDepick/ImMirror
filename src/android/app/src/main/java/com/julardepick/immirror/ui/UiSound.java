package com.julardepick.immirror.ui;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.util.Log;

import java.util.HashMap;
import java.util.Map;

/*
 * 界面音效.
 *
 * 按钮点击, 成就提示与签到提示走独立的 SoundPool, 与游戏内音效池分开:
 * 界面音只在交互时触发, 并发量低, 但要求低延迟, 独立池不会被游戏内
 * 长时间的音效挤占通道.
 * 每个通道按资产路径懒加载并缓存句柄, 素材缺失时静默降级, 不影响界面功能.
 */
public final class UiSound {

    private static final String TAG = "ImMirrorUiSound";

    /* 按钮点击音在安装包内的路径, 与打包后的扁平化命名一致. */
    private static final String CLICK_ASSET = "sounds/ui_button_button_click.ogg";
    /* 成就完成音: 单条成就领取成功后播放. */
    private static final String ACHIEVEMENT_COMPLETE_ASSET = "sounds/ui_info_achievement_complete.ogg";
    /* 每日任务达成音: 任务奖励领取成功后播放. */
    private static final String ACHIEVEMENT_REACH_ASSET = "sounds/ui_info_achievement_reach.ogg";
    /* 签到音: 签到成功后播放. */
    private static final String EXPERIENCE_STAR_ASSET = "sounds/ui_info_experience_star.ogg";
    /* 死亡音: 玩家死亡时播放. */
    private static final String DEATH_ASSET = "sounds/ui_info_death.ogg";
    /* 合成成功音: 合成动作完成后播放. */
    private static final String CRAFT_SUCCESS_ASSET = "sounds/ui_info_crafting_success.ogg";
    /* 背包开合音: 打开与关闭各播一次. */
    private static final String BAG_TOGGLE_ASSET = "sounds/ui_bp_bpsound2.ogg";
    /* 附魔成功音. */
    private static final String ENCHANT_ASSET = "sounds/ui_info_book_unlock.ogg";
    /* 拾取音: 地面掉落物入包时播放. */
    private static final String PICKUP_ASSET = "sounds/ui_info_experience_star.ogg";
    /* 界面音并发上限: 连点也不会互相打断. */
    private static final int MAX_STREAMS = 4;

    private static SoundPool pool;
    private static int clickId;
    private static boolean failed;
    /* 资产路径到句柄的缓存; 加载失败记 0, 下次直接跳过. */
    private static final Map<String, Integer> handles = new HashMap<>();

    private UiSound() {
    }

    /* 首次使用时按需初始化, 之后复用; 初始化失败只记一次日志. */
    private static void ensure(Context context) {
        if (pool != null || failed) {
            return;
        }
        final Context app = context.getApplicationContext();
        try {
            final AudioAttributes attributes = new AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_GAME)
                    .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                    .build();
            final SoundPool created = new SoundPool.Builder()
                    .setMaxStreams(MAX_STREAMS)
                    .setAudioAttributes(attributes)
                    .build();
            pool = created;
            clickId = load(created, app.getAssets(), CLICK_ASSET);
        } catch (Exception error) {
            failed = true;
            Log.w(TAG, "ui sound unavailable: " + error);
        }
    }

    /* 按资产路径加载一个界面音, 失败返回 0. */
    private static int load(SoundPool target, AssetManager assets, String asset) {
        try {
            final AssetFileDescriptor descriptor = assets.openFd(asset);
            final int id = target.load(descriptor, 1);
            descriptor.close();
            return id;
        } catch (Exception error) {
            Log.w(TAG, "ui sound missing: " + asset + " " + error);
            return 0;
        }
    }

    /* 播放一次按钮点击音. 未初始化成功时静默返回. */
    public static void click(Context context) {
        play(context, CLICK_ASSET);
    }

    /* 成就完成音: 领取成功后调用. */
    public static void achievementComplete(Context context) {
        play(context, ACHIEVEMENT_COMPLETE_ASSET);
    }

    /* 每日任务达成音: 领取成功后调用. */
    public static void achievementReach(Context context) {
        play(context, ACHIEVEMENT_REACH_ASSET);
    }

    /* 签到音: 签到成功后调用. */
    public static void experienceStar(Context context) {
        play(context, EXPERIENCE_STAR_ASSET);
    }

    /* 死亡音: 玩家死亡时调用. */
    public static void death(Context context) {
        play(context, DEATH_ASSET);
    }

    /* 合成成功音: 合成动作完成后调用. */
    public static void craftSuccess(Context context) {
        play(context, CRAFT_SUCCESS_ASSET);
    }

    /* 背包开合音: 面板打开或关闭时调用. */
    public static void bagToggle(Context context) {
        play(context, BAG_TOGGLE_ASSET);
    }

    /* 附魔成功音. */
    public static void enchant(Context context) {
        play(context, ENCHANT_ASSET);
    }

    /* 拾取音: 掉落物入包时调用. */
    public static void pickup(Context context) {
        play(context, PICKUP_ASSET);
    }

    /*
     * 按资产路径播放一次界面音.
     * 点击音在初始化时预载, 其余通道首次播放时加载并缓存句柄.
     * 音量固定 1.0: 界面音由系统媒体音量控制, 不跟随游戏内音量滑杆.
     */
    private static void play(Context context, String asset) {
        if (context == null) {
            return;
        }
        ensure(context);
        if (pool == null) {
            return;
        }
        int id;
        if (CLICK_ASSET.equals(asset)) {
            id = clickId;
        } else {
            final Integer cached = handles.get(asset);
            if (cached != null) {
                id = cached;
            } else {
                id = load(pool, context.getApplicationContext().getAssets(), asset);
                handles.put(asset, id);
            }
        }
        if (id == 0) {
            return;
        }
        pool.play(id, 1.0f, 1.0f, 1, 0, 1.0f);
    }

    /* 释放音频池, 由界面退出时调用; 重复调用安全. */
    public static void release() {
        if (pool != null) {
            pool.release();
            pool = null;
            clickId = 0;
        }
        handles.clear();
    }
}
