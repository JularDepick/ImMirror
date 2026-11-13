package com.julardepick.immirror.account;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;

import com.julardepick.immirror.BuildConfig;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/*
 * 账号接口客户端.
 *
 * 只使用标准库 HttpURLConnection 与 org.json: 请求在后台线程池执行,
 * 回调统一投递回主线程, 界面层不需要自行处理线程切换.
 * 服务端地址每次请求时从 AccountSettings 读取, 因此改地址立即生效.
 */
public final class ApiClient {

    /* 接口回调, 成功与失败都在主线程回调. */
    public interface Callback<T> {
        void onSuccess(T value);

        void onFailure(ApiException error);
    }

    /* 后台任务抽象, 便于统一异常处理. */
    private interface Task<T> {
        T run() throws ApiException;
    }

    private static final String PATH_PREFIX = "/api/v1";
    private static final String USER_AGENT = "ImMirror-Android/" + BuildConfig.VERSION_NAME;
    private static final int CONNECT_TIMEOUT_MS = 8000;
    private static final int READ_TIMEOUT_MS = 10000;

    private static final ExecutorService EXECUTOR = Executors.newCachedThreadPool();

    private final Context context;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    public ApiClient(Context context) {
        this.context = context.getApplicationContext();
    }

    public void sendRegisterCode(String email, Callback<CodeDelivery> callback) {
        submit(callback, () -> parseDelivery(send("POST", "/auth/register/code", makeBody("email", email), null)));
    }

    public void register(String email, String code, String password, Callback<Session> callback) {
        submit(callback, () -> parseSession(
                send("POST", "/auth/register", makeBody("email", email, "code", code, "password", password), null)));
    }

    public void login(String identifier, String password, Callback<Session> callback) {
        submit(callback, () -> parseSession(
                send("POST", "/auth/login", makeBody("identifier", identifier, "password", password), null)));
    }

    public void sendLoginCode(String email, Callback<CodeDelivery> callback) {
        submit(callback, () -> parseDelivery(send("POST", "/auth/login/code", makeBody("email", email), null)));
    }

    public void loginWithCode(String email, String code, Callback<Session> callback) {
        submit(callback, () -> parseSession(
                send("POST", "/auth/login/code/verify", makeBody("email", email, "code", code), null)));
    }

    public void sendResetCode(String email, Callback<CodeDelivery> callback) {
        submit(callback, () -> parseDelivery(
                send("POST", "/auth/password/reset/code", makeBody("email", email), null)));
    }

    public void resetPassword(String email, String code, String password, Callback<Void> callback) {
        submit(callback, () -> {
            send("POST", "/auth/password/reset", makeBody("email", email, "code", code, "password", password), null);
            return null;
        });
    }

    public void fetchProfile(Session session, Callback<AccountProfile> callback) {
        submit(callback, () -> parseProfile(send("GET", "/me", null, session == null ? null : session.token())));
    }

    /*
     * 成就与任务接口.
     * 响应统一交回原文 JSON 对象, 由进度界面自行解析: 服务端补充字段时
     * 账号层不需要跟着改解析结构, 两个模块的接口演进互不牵连.
     */
    public void fetchAchievements(Session session, Callback<JSONObject> callback) {
        submit(callback, () -> send("GET", "/achievements", null, tokenOf(session)));
    }

    public void fetchDailyTasks(Session session, Callback<JSONObject> callback) {
        submit(callback, () -> send("GET", "/achievements/daily", null, tokenOf(session)));
    }

    /* 领取单条成就奖励; 服务端若已自动发放, 会按幂等处理返回当前状态. */
    public void claimAchievement(Session session, int achievementId, Callback<JSONObject> callback) {
        submit(callback, () -> send("POST", "/achievements/claim",
                makeBody("id", String.valueOf(achievementId)), tokenOf(session)));
    }

    /* 批量领取: 一次领完所有已完成未领取的成就与当日任务. */
    public void claimAllRewards(Session session, Callback<JSONObject> callback) {
        submit(callback, () -> send("POST", "/achievements/claim-all", null, tokenOf(session)));
    }

    /* 领取单条每日任务奖励. */
    public void claimDailyTask(Session session, int slot, Callback<JSONObject> callback) {
        submit(callback, () -> send("POST", "/achievements/daily/claim",
                makeBody("slot", String.valueOf(slot)), tokenOf(session)));
    }

    private static String tokenOf(Session session) {
        return session == null ? null : session.token();
    }

    /*
     * 换绑邮箱接口.
     * 两步流程: 先向新邮箱发送验证码, 再凭验证码与当前密码提交换绑;
     * 成功后服务端作废旧会话, 调用方需清理本地会话并回登录页.
     */
    public void requestRebindCode(Session session, String newEmail, Callback<CodeDelivery> callback) {
        submit(callback, () -> parseDelivery(send("POST", "/auth/email/rebind/code",
                makeBody("newEmail", newEmail), tokenOf(session))));
    }

    /* 换绑成功时服务端返回 { uid, email }, 这里把新邮箱回传给界面展示. */
    public void rebindEmail(Session session, String newEmail, String code, String password,
                            Callback<String> callback) {
        submit(callback, () -> {
            JSONObject data = send("POST", "/auth/email/rebind",
                    makeBody("newEmail", newEmail, "code", code, "password", password), tokenOf(session));
            return data.optString("email", newEmail);
        });
    }

    public void logout(Session session, Callback<Void> callback) {
        submit(callback, () -> {
            if (session != null) {
                send("POST", "/auth/logout", new JSONObject(), session.token());
            }
            return null;
        });
    }

    /* 查询协议版本, 用于校验服务端地址是否可达. */
    public void ping(Callback<String> callback) {
        submit(callback, () -> {
            JSONObject data = send("GET", "/meta/version", null, null);
            return data.optString("apiVersion", "");
        });
    }

    private <T> void submit(final Callback<T> callback, final Task<T> task) {
        EXECUTOR.execute(() -> {
            try {
                T value = task.run();
                postSuccess(callback, value);
            } catch (ApiException error) {
                postFailure(callback, error);
            } catch (RuntimeException error) {
                postFailure(callback, new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error)));
            }
        });
    }

    private <T> void postSuccess(final Callback<T> callback, final T value) {
        if (callback == null) {
            return;
        }
        mainHandler.post(() -> callback.onSuccess(value));
    }

    private <T> void postFailure(final Callback<T> callback, final ApiException error) {
        if (callback == null) {
            return;
        }
        mainHandler.post(() -> callback.onFailure(error));
    }

    /* 执行一次请求并解包络; 失败统一抛 ApiException. */
    private JSONObject send(String method, String path, JSONObject body, String token) throws ApiException {
        String baseUrl = AccountSettings.baseUrl(context);
        HttpURLConnection connection = null;
        try {
            URL url = new URL(baseUrl + PATH_PREFIX + path);
            connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod(method);
            connection.setConnectTimeout(CONNECT_TIMEOUT_MS);
            connection.setReadTimeout(READ_TIMEOUT_MS);
            connection.setRequestProperty("Accept", "application/json");
            connection.setRequestProperty("User-Agent", USER_AGENT);
            if (token != null && !token.isEmpty()) {
                connection.setRequestProperty("Authorization", "Bearer " + token);
            }
            if (body != null) {
                byte[] payload = body.toString().getBytes(StandardCharsets.UTF_8);
                connection.setDoOutput(true);
                connection.setRequestProperty("Content-Type", "application/json; charset=utf-8");
                connection.setFixedLengthStreamingMode(payload.length);
                try (OutputStream output = connection.getOutputStream()) {
                    output.write(payload);
                }
            }
            int status = connection.getResponseCode();
            String text = readBody(connection, status);
            JSONObject envelope = parseEnvelope(text);
            if (envelope.optBoolean("ok", false)) {
                JSONObject data = envelope.optJSONObject("data");
                return data == null ? new JSONObject() : data;
            }
            JSONObject error = envelope.optJSONObject("error");
            String code = error == null ? "" : error.optString("code", "");
            String message = error == null ? "" : error.optString("message", "");
            if (code.isEmpty()) {
                throw new ApiException(ApiException.CODE_PROTOCOL, "http " + status);
            }
            throw new ApiException(code, message);
        } catch (java.net.MalformedURLException error) {
            throw new ApiException(ApiException.CODE_NETWORK, "bad url " + baseUrl);
        } catch (IOException error) {
            throw new ApiException(ApiException.CODE_NETWORK, String.valueOf(error));
        }
    }

    private static String readBody(HttpURLConnection connection, int status) throws IOException {
        InputStream stream = status >= 400 ? connection.getErrorStream() : connection.getInputStream();
        if (stream == null) {
            return "";
        }
        try (InputStream input = stream; ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[4096];
            int read;
            while ((read = input.read(buffer)) != -1) {
                output.write(buffer, 0, read);
            }
            return new String(output.toByteArray(), StandardCharsets.UTF_8);
        }
    }

    /* 响应必须是 JSON 包络; 解析失败说明地址或服务不对. */
    private static JSONObject parseEnvelope(String text) throws ApiException {
        if (text == null || text.trim().isEmpty()) {
            throw new ApiException(ApiException.CODE_PROTOCOL, "empty response");
        }
        try {
            JSONObject envelope = new JSONObject(text);
            if (!envelope.has("ok")) {
                throw new ApiException(ApiException.CODE_PROTOCOL, "missing ok field");
            }
            return envelope;
        } catch (JSONException error) {
            throw new ApiException(ApiException.CODE_PROTOCOL, "invalid json");
        }
    }

    private static JSONObject makeBody(String... keyValues) throws ApiException {
        JSONObject body = new JSONObject();
        try {
            for (int index = 0; index + 1 < keyValues.length; index += 2) {
                body.put(keyValues[index], keyValues[index + 1] == null ? "" : keyValues[index + 1]);
            }
        } catch (JSONException error) {
            throw new ApiException(ApiException.CODE_PROTOCOL, "build body failed");
        }
        return body;
    }

    private static Session parseSession(JSONObject data) throws ApiException {
        String token = data.optString("token", "");
        String uid = data.optString("uid", "");
        if (token.isEmpty() || uid.isEmpty()) {
            throw new ApiException(ApiException.CODE_PROTOCOL, "session fields missing");
        }
        return new Session(token, uid, data.optString("email", ""), data.optLong("expiresAt", 0L));
    }

    private static CodeDelivery parseDelivery(JSONObject data) {
        return new CodeDelivery(data.optInt("ttl", 600), data.optInt("cooldown", 60));
    }

    private static AccountProfile parseProfile(JSONObject data) {
        JSONObject wallet = data.optJSONObject("wallet");
        long balance = wallet == null ? 0L : wallet.optLong("balance", 0L);
        return new AccountProfile(data.optString("uid", ""), data.optString("email", ""), balance);
    }
}
