package com.julardepick.immirror.net;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;

import com.julardepick.immirror.BuildConfig;
import com.julardepick.immirror.account.AccountSettings;
import com.julardepick.immirror.account.ApiException;
import com.julardepick.immirror.account.Session;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/*
 * 位置上报与查询接口客户端.
 *
 * 联机骨架 M9 采用 HTTP 轮询: 每个周期调用一次 reportAndFetch,
 * 一个请求同时完成自身位置上报与其他玩家快照拉取, 服务端按 30 秒超时剔除离线者.
 * 复用 account 包的会话, 地址与异常类型, 不重复定义服务端地址;
 * 结构仿 ProgressApi: 后台单线程执行, 回调统一投递回主线程.
 * 轮询属于低频调用, 不触碰逐帧路径, 位置快照交给原生侧管理.
 */
public final class PresenceApi {

    /* 一次轮询结果: 其他玩家的最新位置快照列表. */
    public static final class Snapshot {
        public final String uid;
        public final float x;
        public final float y;
        public final float z;
        public final float yaw;
        public final float pitch;

        Snapshot(String uid, float x, float y, float z, float yaw, float pitch) {
            this.uid = uid;
            this.x = x;
            this.y = y;
            this.z = z;
            this.yaw = yaw;
            this.pitch = pitch;
        }
    }

    /* 回调只在主线程触发, 成功与失败互斥. */
    public interface Callback {
        void onSuccess(List<Snapshot> others, long serverTime);

        void onFailure(ApiException error);
    }

    private static final String PATH_PREFIX = "/api/v1";
    private static final String USER_AGENT = "ImMirror-Android/" + BuildConfig.VERSION_NAME;
    private static final int CONNECT_TIMEOUT_MS = 5000;
    private static final int READ_TIMEOUT_MS = 6000;

    private static final ExecutorService EXECUTOR = Executors.newSingleThreadExecutor();

    private final Context context;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    public PresenceApi(Context context) {
        this.context = context.getApplicationContext();
    }

    /*
     * 上报自身位置并拉取同世界其他玩家.
     * 会话不可用时直接回失败, 不发请求; 坐标为世界连续坐标, 单位方块.
     */
    public void reportAndFetch(final Session session, final float x, final float y, final float z,
                               final float yaw, final float pitch, final Callback callback) {
        if (session == null || !session.isUsable()) {
            postFailure(callback, new ApiException("AUTH_UNAUTHORIZED", "no session"));
            return;
        }
        EXECUTOR.execute(() -> {
            try {
                JSONObject body = new JSONObject();
                body.put("x", (double) x);
                body.put("y", (double) y);
                body.put("z", (double) z);
                body.put("yaw", (double) yaw);
                body.put("pitch", (double) pitch);
                JSONObject data = post("/presence", body, session.token());
                postSuccess(callback, parseOthers(data), data.optLong("serverTime", 0L));
            } catch (ApiException error) {
                postFailure(callback, error);
            } catch (JSONException | RuntimeException error) {
                postFailure(callback, new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error)));
            }
        });
    }

    /* 只读拉取其他玩家, 不上报自身位置; 用于进世界后先看一眼再决定是否上报. */
    public void fetchOnly(final Session session, final Callback callback) {
        if (session == null || !session.isUsable()) {
            postFailure(callback, new ApiException("AUTH_UNAUTHORIZED", "no session"));
            return;
        }
        EXECUTOR.execute(() -> {
            try {
                JSONObject data = get("/presence", session.token());
                postSuccess(callback, parseOthers(data), data.optLong("serverTime", 0L));
            } catch (ApiException error) {
                postFailure(callback, error);
            } catch (JSONException | RuntimeException error) {
                postFailure(callback, new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error)));
            }
        });
    }

    /* 拆 others 数组; 字段缺失一律按零处理, 不因为少字段就整轮失败. */
    private static List<Snapshot> parseOthers(JSONObject data) throws JSONException {
        List<Snapshot> others = new ArrayList<>();
        JSONArray raw = data.optJSONArray("others");
        if (raw != null) {
            for (int index = 0; index < raw.length(); index++) {
                JSONObject entry = raw.optJSONObject(index);
                if (entry == null) {
                    continue;
                }
                others.add(new Snapshot(entry.optString("uid", ""),
                        (float) entry.optDouble("x", 0.0),
                        (float) entry.optDouble("y", 0.0),
                        (float) entry.optDouble("z", 0.0),
                        (float) entry.optDouble("yaw", 0.0),
                        (float) entry.optDouble("pitch", 0.0)));
            }
        }
        return others;
    }

    private void postSuccess(final Callback callback, final List<Snapshot> others,
                             final long serverTime) {
        if (callback == null) {
            return;
        }
        lastServerTime = serverTime;
        mainHandler.post(() -> callback.onSuccess(others, serverTime));
    }

    private void postFailure(final Callback callback, final ApiException error) {
        if (callback == null) {
            return;
        }
        mainHandler.post(() -> callback.onFailure(error));
    }

    /* 最近一次响应的服务端时间戳, 毫秒; 0 表示尚无成功响应. */
    private volatile long lastServerTime = 0L;

    public long lastServerTime() {
        return lastServerTime;
    }

    /* 执行一次 GET 并拆包络; 成功返回 data 对象, 失败抛 ApiException. */
    private JSONObject get(String path, String token) throws ApiException {
        String baseUrl = AccountSettings.baseUrl(context);
        HttpURLConnection connection = null;
        try {
            URL url = new URL(baseUrl + PATH_PREFIX + path);
            connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod("GET");
            connection.setConnectTimeout(CONNECT_TIMEOUT_MS);
            connection.setReadTimeout(READ_TIMEOUT_MS);
            connection.setRequestProperty("Accept", "application/json");
            connection.setRequestProperty("User-Agent", USER_AGENT);
            connection.setRequestProperty("Authorization", "Bearer " + token);
            int status = connection.getResponseCode();
            String text = readBody(connection, status);
            return parseEnvelope(text, status);
        } catch (MalformedURLException error) {
            throw new ApiException(ApiException.CODE_NETWORK, "bad url " + baseUrl);
        } catch (IOException error) {
            throw new ApiException(ApiException.CODE_NETWORK, String.valueOf(error));
        } catch (JSONException error) {
            throw new ApiException(ApiException.CODE_PROTOCOL, "invalid json");
        } finally {
            if (connection != null) {
                connection.disconnect();
            }
        }
    }

    /* 执行一次 POST 并拆包络; 与 get 共用超时与错误处理. */
    private JSONObject post(String path, JSONObject body, String token) throws ApiException {
        String baseUrl = AccountSettings.baseUrl(context);
        HttpURLConnection connection = null;
        try {
            URL url = new URL(baseUrl + PATH_PREFIX + path);
            connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod("POST");
            connection.setConnectTimeout(CONNECT_TIMEOUT_MS);
            connection.setReadTimeout(READ_TIMEOUT_MS);
            connection.setRequestProperty("Accept", "application/json");
            connection.setRequestProperty("User-Agent", USER_AGENT);
            connection.setRequestProperty("Authorization", "Bearer " + token);
            connection.setRequestProperty("Content-Type", "application/json; charset=utf-8");
            byte[] payload = body.toString().getBytes(StandardCharsets.UTF_8);
            connection.setFixedLengthStreamingMode(payload.length);
            connection.setDoOutput(true);
            try (OutputStream output = connection.getOutputStream()) {
                output.write(payload);
            }
            int status = connection.getResponseCode();
            String text = readBody(connection, status);
            return parseEnvelope(text, status);
        } catch (MalformedURLException error) {
            throw new ApiException(ApiException.CODE_NETWORK, "bad url " + baseUrl);
        } catch (IOException error) {
            throw new ApiException(ApiException.CODE_NETWORK, String.valueOf(error));
        } catch (JSONException error) {
            throw new ApiException(ApiException.CODE_PROTOCOL, "bad json " + error);
        } finally {
            if (connection != null) {
                connection.disconnect();
            }
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

    /* 拆响应包络: ok 为真取 data, 否则把服务端错误码与消息转成 ApiException. */
    private static JSONObject parseEnvelope(String text, int status)
            throws JSONException, ApiException {
        if (text == null || text.trim().isEmpty()) {
            throw new ApiException(ApiException.CODE_PROTOCOL, "empty response");
        }
        JSONObject envelope = new JSONObject(text);
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
    }
}
