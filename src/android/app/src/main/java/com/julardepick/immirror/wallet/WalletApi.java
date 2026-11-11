package com.julardepick.immirror.wallet;

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
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/*
 * 钱包接口客户端.
 *
 * 封装服务端钱包与签到三个接口: 余额流水查询, 本月签到状态查询与当日签到,
 * 请求在单线程后台池顺序执行, 回调统一投递回主线程, 界面层不做线程处理.
 * 服务端地址每次请求时从 AccountSettings 读取, 修改地址立即生效.
 * 进入钱包页一次需要余额流水与签到状态两份数据, fetchOverview 在后台把它们
 * 请求完再合并成一个 WalletBoard 回调, 避免界面出现半块数据.
 */
public final class WalletApi {

    /* 回调只在主线程触发, 成功与失败互斥. */
    public interface Callback {
        void onSuccess(WalletBoard board);

        void onFailure(ApiException error);
    }

    /* 单接口回调, 供签到等只关心一份响应的调用使用. */
    public interface ResultCallback<T> {
        void onSuccess(T value);

        void onFailure(ApiException error);
    }

    private static final String PATH_PREFIX = "/api/v1";
    private static final String USER_AGENT = "ImMirror-Android/" + BuildConfig.VERSION_NAME;
    private static final int CONNECT_TIMEOUT_MS = 8000;
    private static final int READ_TIMEOUT_MS = 10000;

    private static final ExecutorService EXECUTOR = Executors.newSingleThreadExecutor();

    private final Context context;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    public WalletApi(Context context) {
        this.context = context.getApplicationContext();
    }

    /* 读取余额流水与签到状态, 两份接口都成功才回调成功. */
    public void fetchOverview(final Session session, final Callback callback) {
        if (session == null || !session.isUsable()) {
            postFailure(callback, new ApiException("AUTH_UNAUTHORIZED", "no session"));
            return;
        }
        EXECUTOR.execute(() -> {
            try {
                JSONObject wallet = get("/wallet", session.token());
                JSONObject checkin = get("/wallet/checkin", session.token());
                postSuccess(callback, merge(wallet, checkin));
            } catch (ApiException error) {
                postFailure(callback, error);
            } catch (RuntimeException error) {
                postFailure(callback, new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error)));
            }
        });
    }

    /* 执行当日签到; 已签过时服务端返回 WALLET_ALREADY_CHECKED_IN, 原样抛给界面处理. */
    public void checkin(final Session session, final ResultCallback<CheckinResult> callback) {
        if (session == null || !session.isUsable()) {
            postFailure(callback, new ApiException("AUTH_UNAUTHORIZED", "no session"));
            return;
        }
        EXECUTOR.execute(() -> {
            try {
                JSONObject data = post("/wallet/checkin", new JSONObject(), session.token());
                postSuccess(callback, CheckinResult.parse(data));
            } catch (ApiException error) {
                postFailure(callback, error);
            } catch (JSONException error) {
                postFailure(callback, new ApiException(ApiException.CODE_PROTOCOL, "bad json " + error));
            } catch (IOException error) {
                postFailure(callback, new ApiException(ApiException.CODE_NETWORK, String.valueOf(error)));
            } catch (RuntimeException error) {
                postFailure(callback, new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error)));
            }
        });
    }

    /* 两份响应按字段聚合; 流水条数由服务端控制, 字段缺失时按零值兜底. */
    private static WalletBoard merge(JSONObject wallet, JSONObject checkin) {
        CheckinStatus status = CheckinStatus.parse(checkin);
        long balance = wallet.optLong("balance", 0L);
        long updatedAt = wallet.optLong("updatedAt", 0L);
        List<LedgerEntry> entries = new ArrayList<>();
        JSONArray rawLedger = wallet.optJSONArray("ledger");
        if (rawLedger != null) {
            for (int index = 0; index < rawLedger.length(); index++) {
                LedgerEntry entry = LedgerEntry.parse(rawLedger.optJSONObject(index));
                if (entry != null) {
                    entries.add(entry);
                }
            }
        }
        return new WalletBoard(balance, updatedAt, status, entries);
    }

    private void postSuccess(final Callback callback, final WalletBoard board) {
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

    private <T> void postSuccess(final ResultCallback<T> callback, final T value) {
        if (callback == null) {
            return;
        }
        mainHandler.post(() -> callback.onSuccess(value));
    }

    private <T> void postFailure(final ResultCallback<T> callback, final ApiException error) {
        if (callback == null) {
            return;
        }
        mainHandler.post(() -> callback.onFailure(error));
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
    private JSONObject post(String path, JSONObject body, String token) throws ApiException, IOException, JSONException {
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
            connection.setDoOutput(true);
            connection.setFixedLengthStreamingMode(payload.length);
            try (OutputStream output = connection.getOutputStream()) {
                output.write(payload);
            }
            int status = connection.getResponseCode();
            String text = readBody(connection, status);
            return parseEnvelope(text, status);
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
