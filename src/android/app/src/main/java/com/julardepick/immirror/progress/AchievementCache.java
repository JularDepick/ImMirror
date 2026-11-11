package com.julardepick.immirror.progress;

import android.content.Context;
import android.content.SharedPreferences;

/*
 * 服务端进度的本地缓存.
 *
 * 只缓存上一次成功响应的原始 JSON 文本, 联网失败时界面用它回落到缓存展示,
 * 不做任何二次解析, 因此缓存内容与服务端字段始终一致.
 */
public final class AchievementCache {

    private static final String PREFERENCES = "immirror_progress_cache";
    private static final String KEY_ACHIEVEMENTS = "achievements";
    private static final String KEY_DAILY = "daily";

    private AchievementCache() {
    }

    public static void saveAchievements(Context context, String json) {
        save(context, KEY_ACHIEVEMENTS, json);
    }

    public static void saveDaily(Context context, String json) {
        save(context, KEY_DAILY, json);
    }

    public static String loadAchievements(Context context) {
        return load(context, KEY_ACHIEVEMENTS);
    }

    public static String loadDaily(Context context) {
        return load(context, KEY_DAILY);
    }

    private static void save(Context context, String key, String json) {
        if (context == null || json == null || json.isEmpty()) {
            return;
        }
        SharedPreferences preferences =
                context.getApplicationContext().getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE);
        preferences.edit().putString(key, json).apply();
    }

    private static String load(Context context, String key) {
        if (context == null) {
            return "";
        }
        SharedPreferences preferences =
                context.getApplicationContext().getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE);
        return preferences.getString(key, "");
    }
}
