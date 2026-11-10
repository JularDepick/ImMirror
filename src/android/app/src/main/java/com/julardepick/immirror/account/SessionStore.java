package com.julardepick.immirror.account;

import android.content.Context;
import android.content.SharedPreferences;

/*
 * 会话存储. 令牌, UID 与邮箱写入独立的偏好设置文件,
 * 启动时据此判断是否需要进入登录界面, 登出或令牌失效时清除.
 */
public final class SessionStore {

    private static final String PREFERENCES_NAME = "immirror_account";
    private static final String KEY_TOKEN = "session_token";
    private static final String KEY_UID = "session_uid";
    private static final String KEY_EMAIL = "session_email";
    private static final String KEY_EXPIRES_AT = "session_expires_at";

    private SessionStore() {
    }

    public static void save(Context context, Session session) {
        if (session == null) {
            return;
        }
        SharedPreferences preferences = context.getSharedPreferences(PREFERENCES_NAME, Context.MODE_PRIVATE);
        preferences.edit()
                .putString(KEY_TOKEN, session.token())
                .putString(KEY_UID, session.uid())
                .putString(KEY_EMAIL, session.email())
                .putLong(KEY_EXPIRES_AT, session.expiresAt())
                .apply();
    }

    /* 读取已保存的会话; 未登录时返回 null. */
    public static Session load(Context context) {
        SharedPreferences preferences = context.getSharedPreferences(PREFERENCES_NAME, Context.MODE_PRIVATE);
        String token = preferences.getString(KEY_TOKEN, null);
        if (token == null || token.isEmpty()) {
            return null;
        }
        return new Session(
                token,
                preferences.getString(KEY_UID, ""),
                preferences.getString(KEY_EMAIL, ""),
                preferences.getLong(KEY_EXPIRES_AT, 0L));
    }

    /* 是否存在可直接使用的会话, 过期的会话视为不存在. */
    public static boolean hasUsableSession(Context context) {
        Session session = load(context);
        return session != null && session.isUsable();
    }

    public static void clear(Context context) {
        SharedPreferences preferences = context.getSharedPreferences(PREFERENCES_NAME, Context.MODE_PRIVATE);
        preferences.edit().clear().apply();
    }
}
