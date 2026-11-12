package com.julardepick.immirror.net;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;

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
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/*
 * 方块改动同步客户端.
 *
 * 一次调用同时完成两件事: 把本地攒下的改动推给服务端, 再按序号增量拉取别人的改动.
 * 服务端是内存环形表, 同一坐标只保留最新一条; 客户端按 seq 递增拉取, 落后太多时
 * 服务端会截断, 客户端因此最多丢失旧改动, 不会卡住.
 * 结构仿 PresenceApi: 后台单线程执行, 回调投递回主线程.
 */
public final class BlockSyncApi {

    /* 回调只关心成功与失败, 具体条目由调用方交给原生侧. */
    public interface Callback {
        void onSuccess(String editsJson, long seq);

        void onFailure(ApiException error);
    }

    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final Context context;

    public BlockSyncApi(Context context) {
        this.context = context.getApplicationContext();
    }

    /* 上报本地改动并拉取序号大于 sinceSeq 的远端改动. */
    public void sync(Session session, String localEditsJson, long sinceSeq, Callback callback) {
        if (session == null || !session.isUsable()) {
            callback.onFailure(new ApiException("AUTH_UNAUTHORIZED", "会话不可用"));
            return;
        }
        final String base = AccountSettings.baseUrl(context);
        final String token = session.token();
        final String payload = localEditsJson == null || localEditsJson.isEmpty() ? "[]" : localEditsJson;
        final long since = Math.max(0L, sinceSeq);
        executor.execute(() -> {
            try {
                final long serverSeq = postEdits(base, token, payload);
                final String edits = getEdits(base, token, since);
                mainHandler.post(() -> callback.onSuccess(edits, serverSeq));
            } catch (ApiException error) {
                mainHandler.post(() -> callback.onFailure(error));
            } catch (IOException | JSONException error) {
                mainHandler.post(() -> callback.onFailure(
                        new ApiException(ApiException.CODE_NETWORK, error.getMessage())));
            }
        });
    }

    /* 上报本地改动, 返回服务端当前最大序号. */
    private long postEdits(String base, String token, String editsJson)
            throws IOException, JSONException, ApiException {
        final JSONObject body = new JSONObject();
        body.put("edits", new JSONArray(editsJson));
        final String response = request(base + "/api/v1/blocks", "POST", token, body.toString());
        final JSONObject root = new JSONObject(response);
        if (!root.optBoolean("ok", false)) {
            throw new ApiException(ApiException.CODE_PROTOCOL, root.optJSONObject("error") == null
                    ? "上报失败" : root.optJSONObject("error").optString("message", "上报失败"));
        }
        return root.optJSONObject("data") == null ? 0L : root.optJSONObject("data").optLong("seq", 0L);
    }

    /* 拉取序号大于 since 的远端改动, 返回 JSON 数组文本. */
    private String getEdits(String base, String token, long since)
            throws IOException, JSONException, ApiException {
        final String url = base + "/api/v1/blocks?since=" + since;
        final String response = request(url, "GET", token, null);
        final JSONObject root = new JSONObject(response);
        if (!root.optBoolean("ok", false)) {
            throw new ApiException(ApiException.CODE_PROTOCOL, "拉取失败");
        }
        final JSONObject data = root.optJSONObject("data");
        if (data == null) {
            return "[]";
        }
        final JSONArray edits = data.optJSONArray("edits");
        return edits == null ? "[]" : edits.toString();
    }

    /* 统一的请求封装: 只处理 JSON 与 Bearer 令牌. */
    private String request(String url, String method, String token, String body) throws IOException {
        final HttpURLConnection connection = (HttpURLConnection) new URL(url).openConnection();
        try {
            connection.setRequestMethod(method);
            connection.setConnectTimeout(8000);
            connection.setReadTimeout(8000);
            connection.setRequestProperty("Authorization", "Bearer " + token);
            if (body != null) {
                connection.setDoOutput(true);
                connection.setRequestProperty("Content-Type", "application/json");
                try (OutputStream stream = connection.getOutputStream()) {
                    stream.write(body.getBytes(StandardCharsets.UTF_8));
                }
            }
            final int status = connection.getResponseCode();
            final InputStream stream = status >= 400 ? connection.getErrorStream() : connection.getInputStream();
            if (stream == null) {
                throw new IOException("空响应 status=" + status);
            }
            try (InputStream input = stream) {
                final ByteArrayOutputStream buffer = new ByteArrayOutputStream();
                final byte[] chunk = new byte[4096];
                int read = input.read(chunk);
                while (read > 0) {
                    buffer.write(chunk, 0, read);
                    read = input.read(chunk);
                }
                return buffer.toString(StandardCharsets.UTF_8.name());
            }
        } finally {
            connection.disconnect();
        }
    }
}
