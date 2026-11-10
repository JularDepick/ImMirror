package com.julardepick.immirror.account;

import android.content.Context;
import android.content.SharedPreferences;
import android.net.Uri;

import java.util.Locale;

/*
 * 服务端地址设置.
 *
 * 默认值按模拟器访问宿主机的写法 (10.0.2.2) 给出; 改用 127.0.0.1 时,
 * 模拟器内需要先执行 adb reverse tcp:61110 tcp:61110 把端口反向映射到宿主机.
 * 地址保存在独立的偏好设置里, 账号会话不与之混用.
 */
public final class AccountSettings {

    /* 默认服务端地址, 与 server 端默认端口 61110 保持一致. */
    public static final String DEFAULT_BASE_URL = "http://10.0.2.2:61110";

    /* 本机回环地址, 模拟器内需配合 adb reverse tcp:61110 tcp:61110 使用. */
    public static final String LOOPBACK_BASE_URL = "http://127.0.0.1:61110";

    private static final String PREFERENCES_NAME = "immirror_settings";
    private static final String KEY_BASE_URL = "api_base_url";

    private AccountSettings() {
    }

    /* 读取当前服务端地址, 未设置或设置损坏时回落到默认值. */
    public static String baseUrl(Context context) {
        SharedPreferences preferences = context.getSharedPreferences(PREFERENCES_NAME, Context.MODE_PRIVATE);
        String stored = normalize(preferences.getString(KEY_BASE_URL, null));
        return stored == null ? DEFAULT_BASE_URL : stored;
    }

    /* 规范化地址: 去空白, 去尾部斜杠, 校验协议与主机名; 非法返回 null. */
    public static String normalize(String raw) {
        if (raw == null) {
            return null;
        }
        String value = raw.trim();
        while (value.endsWith("/")) {
            value = value.substring(0, value.length() - 1);
        }
        if (value.isEmpty()) {
            return null;
        }
        String lower = value.toLowerCase(Locale.ROOT);
        if (!lower.startsWith("http://") && !lower.startsWith("https://")) {
            return null;
        }
        String host = Uri.parse(value).getHost();
        if (host == null || host.isEmpty()) {
            return null;
        }
        return value;
    }

    /* 保存地址, 非法地址不写入并返回 false. */
    public static boolean saveBaseUrl(Context context, String raw) {
        String normalized = normalize(raw);
        if (normalized == null) {
            return false;
        }
        SharedPreferences preferences = context.getSharedPreferences(PREFERENCES_NAME, Context.MODE_PRIVATE);
        preferences.edit().putString(KEY_BASE_URL, normalized).apply();
        return true;
    }
}
