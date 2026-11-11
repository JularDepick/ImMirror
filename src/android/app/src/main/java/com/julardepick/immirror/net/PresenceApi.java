package com.julardepick.immirror.net;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;

import com.julardepick.immirror.account.ApiException;
import com.julardepick.immirror.account.AccountSettings;
import com.julardepick.immirror.account.Session;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/*
 * 位置上报与查询. 与 ProgressApi 同一套包络与线程模型:
 * 后台线程执行请求, 主线程回调, 失败以 ApiException 回传不抛异常.
 * 上报接口一次完成"写自身 + 读他人", 轮询间隔由调用方决定.
 */
public final class PresenceApi {

    public interface Callback {
        void onSuccess(int others);

        void onFailure(ApiException error);
    }

    private static final String PATH_PREFIX = "/api/v1";
    private static final int CONNECT_TIMEOUT_MS = 5000;
    private static final int READ_TIMEOUT_MS = 5000;
    private static final ExecutorService EXECUTOR = Executors.newSingleThreadExecutor();
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final Context context;

    public PresenceApi(Context context) {
        this.context = context.getApplicationContext();
    }

    /* 上报自身位置; 成功回调带同世界其他玩家数量. */
    public void report(final Session session, final double x, final double y, final double z,
            final double yaw, final double pitch, final Callback callback) {
        if (session == null || !session.isUsable()) {
            postFailure(callback, new ApiException("AUTH_UNAUTHORIZED", "no session"));
            return;
        }
        EXECUTOR.execute(() -> {
            try {
                final JSONObject body = new JSONObject();
                body.put("x", x);
                body.put("y", y);
                body.put("z", z);
                body.put("yaw", yaw);
                body.put("pitch", pitch);
                final JSONObject data = post("/presence", body, session.token());
                final JSONArray others = data.optJSONArray("others");
                postSuccess(callback, others == null ? 0 : others.length());
            } catch (ApiException error) {
                postFailure(callback, error);
            } catch (org.json.JSONException | RuntimeException error) {
                postFailure(callback, new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error)));
            }
        });
    }

    private void postSuccess(final Callback callback, final int others) {
        mainHandler.post(() -> callback.onSuccess(others));
    }

    private void postFailure(final Callback callback, final ApiException error) {
        mainHandler.post(() -> callback.onFailure(error));
    }

    private JSONObject post(String path, JSONObject body, String token) throws ApiException,
            org.json.JSONException {
        final String baseUrl = AccountSettings.baseUrl(context);
        HttpURLConnection connection = null;
        try {
            final URL url = new URL(baseUrl + PATH_PREFIX + path);
            connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod("POST");
            connection.setConnectTimeout(CONNECT_TIMEOUT_MS);
            connection.setReadTimeout(READ_TIMEOUT_MS);
            connection.setRequestProperty("Accept", "application/json");
            connection.setRequestProperty("Authorization", "Bearer " + token);
            connection.setRequestProperty("Content-Type", "application/json");
            connection.setDoOutput(true);
            connection.getOutputStream().write(body.toString().getBytes("UTF-8"));
            final int status = connection.getResponseCode();
            final String text = readBody(connection, status);
            return parseEnvelope(text, status);
        } catch (MalformedURLException error) {
            throw new ApiException(ApiException.CODE_NETWORK, "bad url");
        } catch (IOException error) {
            throw new ApiException(ApiException.CODE_NETWORK, String.valueOf(error));
        } catch (org.json.JSONException error) {
            throw new ApiException(ApiException.CODE_PROTOCOL, "bad json " + error);
        }
    }

    private static String readBody(HttpURLConnection connection, int status) throws IOException {
        final java.io.InputStream stream =
                status >= 400 ? connection.getErrorStream() : connection.getInputStream();
        if (stream == null) {
            return "";
        }
        final BufferedReader reader = new BufferedReader(new InputStreamReader(stream, "UTF-8"));
        final StringBuilder builder = new StringBuilder();
        String line;
        while ((line = reader.readLine()) != null) {
            builder.append(line);
        }
        reader.close();
        return builder.toString();
    }

    private static JSONObject parseEnvelope(String text, int status)
            throws org.json.JSONException, ApiException {
        if (text == null || text.trim().isEmpty()) {
            throw new ApiException(ApiException.CODE_PROTOCOL, "empty response");
        }
        final JSONObject envelope = new JSONObject(text);
        if (envelope.optBoolean("ok", false)) {
            final JSONObject data = envelope.optJSONObject("data");
            return data == null ? new JSONObject() : data;
        }
        final JSONObject error = envelope.optJSONObject("error");
        final String code = error == null ? "" : error.optString("code", "");
        final String message = error == null ? "" : error.optString("message", "");
        if (code.isEmpty()) {
            throw new ApiException(ApiException.CODE_PROTOCOL, "http " + status);
        }
        throw new ApiException(code, message);
    }
}
