package com.julardepick.immirror.progress;

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
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/*
 * 成就与每日任务接口客户端.
 *
 * 一次读取需要两个接口: 成就总览与当日任务, 两者在后台线程顺序请求后合并成一个
 * ProgressBoard, 只回调一次, 界面因此不会出现半块数据.
 * 上报增量统计由原生进度系统在游戏侧完成, 本页只读不写, 所以这里只有查询方法;
 * 复用 account 包的会话, 地址与异常类型, 不重复定义服务端地址。
 */
public final class ProgressApi {

    /* 回调只在主线程触发, 成功与失败互斥. */
    public interface Callback {
        void onSuccess(ProgressBoard board);

        void onFailure(ApiException error);
    }

    private static final String PATH_PREFIX = "/api/v1";
    private static final String USER_AGENT = "ImMirror-Android/" + BuildConfig.VERSION_NAME;
    private static final int CONNECT_TIMEOUT_MS = 8000;
    private static final int READ_TIMEOUT_MS = 10000;

    private static final ExecutorService EXECUTOR = Executors.newSingleThreadExecutor();

    private final Context context;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    public ProgressApi(Context context) {
        this.context = context.getApplicationContext();
    }

    /* 读取成就总览与当日任务; 会话不可用时直接回失败, 不发请求. */
    public void fetchBoard(final Session session, final Callback callback) {
        if (session == null || !session.isUsable()) {
            postFailure(callback, new ApiException("AUTH_UNAUTHORIZED", "no session"));
            return;
        }
        EXECUTOR.execute(() -> {
            try {
                JSONObject achievements = get("/achievements", session.token());
                JSONObject daily = get("/achievements/daily", session.token());
                postSuccess(callback, merge(achievements, daily));
            } catch (ApiException error) {
                postFailure(callback, error);
            } catch (RuntimeException error) {
                postFailure(callback, new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error)));
            }
        });
    }


    /* 上报增量统计; 服务端在 report 内直接发放镜币, 没有单独的领取接口. */
    public void reportStats(final Session session, final JSONObject stats, final Callback callback) {
        if (session == null || !session.isUsable()) {
            postFailure(callback, new ApiException("AUTH_UNAUTHORIZED", "no session"));
            return;
        }
        EXECUTOR.execute(() -> {
            try {
                JSONObject body = new JSONObject();
                body.put("stats", stats);
                JSONObject achievements = post("/achievements/report", body, session.token());
                JSONObject daily = get("/achievements/daily", session.token());
                postSuccess(callback, merge(achievements, daily));
            } catch (ApiException error) {
                postFailure(callback, error);
            } catch (org.json.JSONException | RuntimeException error) {
                postFailure(callback, new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error)));
            }
        });
    }

    private void postSuccess(final Callback callback, final ProgressBoard board) {
        if (callback == null) {
            return;
        }
        mainHandler.post(() -> callback.onSuccess(board));
    }

    private void postFailure(final Callback callback, final ApiException error) {
        if (callback == null) {
            return;
        }
        mainHandler.post(() -> callback.onFailure(error));
    }

    /* 合并两份响应; 字段缺失一律按零或空表处理, 不因为少字段就整页失败. */
    private static ProgressBoard merge(JSONObject achievements, JSONObject daily) {
        JSONObject summary = achievements.optJSONObject("summary");
        int completed = summary == null ? 0 : summary.optInt("completed", 0);
        int total = summary == null ? 0 : summary.optInt("total", 0);
        int coins = summary == null ? 0 : summary.optInt("coins", 0);
        long balance = achievements.optLong("balance", 0L);

        List<AchievementEntry> entries = new ArrayList<>();
        JSONArray rawAchievements = achievements.optJSONArray("achievements");
        if (rawAchievements != null) {
            for (int index = 0; index < rawAchievements.length(); index++) {
                AchievementEntry entry = AchievementEntry.parse(rawAchievements.optJSONObject(index));
                if (entry != null) {
                    entries.add(entry);
                }
            }
        }

        List<DailyTaskEntry> tasks = new ArrayList<>();
        JSONArray rawTasks = daily.optJSONArray("tasks");
        if (rawTasks != null) {
            for (int index = 0; index < rawTasks.length(); index++) {
                DailyTaskEntry entry = DailyTaskEntry.parse(rawTasks.optJSONObject(index));
                if (entry != null) {
                    tasks.add(entry);
                }
            }
        }
        return new ProgressBoard(balance, completed, total, coins, daily.optString("date", ""),
                daily.optInt("completed", 0), entries, tasks);
    }

    /* 执行一次 GET 并解包络; 成功返回 data 对象, 失败抛 ApiException. */
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

    /* 执行一次 POST 并解包络; 与 get 共用超时与错误处理. */
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
            connection.setRequestProperty("Content-Type", "application/json");
            connection.setDoOutput(true);
            connection.getOutputStream().write(body.toString().getBytes("UTF-8"));
            int status = connection.getResponseCode();
            String text = readBody(connection, status);
            return parseEnvelope(text, status);
        } catch (MalformedURLException error) {
            throw new ApiException(ApiException.CODE_NETWORK, "bad url " + baseUrl);
        } catch (IOException error) {
            throw new ApiException(ApiException.CODE_NETWORK, String.valueOf(error));
        } catch (org.json.JSONException error) {
            throw new ApiException(ApiException.CODE_PROTOCOL, "bad json " + error);
        }
    }


    /* 拆响应包络: ok 为真取 data, 否则把服务端错误码与消息转成 ApiException. */
    private static JSONObject parseEnvelope(String text, int status)
            throws org.json.JSONException, ApiException {
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