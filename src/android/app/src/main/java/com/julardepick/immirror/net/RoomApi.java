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
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/*
 * 房间接口客户端.
 *
 * 联机大厅的列表, 建房, 按房间号加入与离房走这里; 房内实时状态与位置, 方块改动
 * 共用一次 sync 调用完成上报与拉取, 与服务端房间通道一一对应.
 * 结构仿 PresenceApi 与 BlockSyncApi: 后台单线程执行, 回调投递回主线程.
 * 服务端按房主权威处理方块改动: 客人的改动会被计入 rejectedEdits 而不生效.
 */
public final class RoomApi {

    /* 房间摘要, 与服务端返回字段一一对应. */
    public static final class Room {
        public final String id;
        public final String code;
        public final String name;
        public final String ownerUid;
        public final String worldName;
        public final int maxPlayers;
        public final int memberCount;
        public final long seed;
        /* 是否有房间密码: 有密码的房间加入前要先输密码. */
        public final boolean hasPassword;

        Room(String id, String code, String name, String ownerUid, String worldName,
             int maxPlayers, int memberCount, long seed, boolean hasPassword) {
            this.id = id;
            this.code = code;
            this.name = name;
            this.ownerUid = ownerUid;
            this.worldName = worldName;
            this.maxPlayers = maxPlayers;
            this.memberCount = memberCount;
            this.seed = seed;
            this.hasPassword = hasPassword;
        }
    }

    /* 一次同步的结果: 序号, 被拒绝的改动数, 其他玩家与远端改动. */
    public static final class State {
        public final long seq;
        public final int rejectedEdits;
        public final String playersJson;
        public final String editsJson;
        /* 房主上报的生物快照与版本号; 客人据此重绘远端生物. */
        public final String creaturesJson;
        public final long creatureSeq;
        public final int rejectedCreatures;
        /* 房主上报的地面掉落物与储物箱内容; 客人只做展示. */
        public final String dropsJson;
        public final String chestsJson;
        public final String furnacesJson;
        /* 房主的世界时间总秒数; 0 表示尚未上报. */
        public final float timeOfDay;
        /* 房主的天气编号与强度. */
        public final int weatherKind;
        public final float weatherIntensity;
        /* 内容版本与是否变化: 没变化时四份内容数组为 null, 调用方跳过应用. */
        public final long contentSeq;
        public final boolean contentChanged;
        /* 当前请求者是否房主: 房主移交后据此切换上报身份. */
        public final boolean owner;

        State(long seq, int rejectedEdits, String playersJson, String editsJson,
              String creaturesJson, long creatureSeq, int rejectedCreatures, String dropsJson,
              String chestsJson, String furnacesJson, float timeOfDay, int weatherKind,
              float weatherIntensity, long contentSeq, boolean contentChanged, boolean owner) {
            this.seq = seq;
            this.rejectedEdits = rejectedEdits;
            this.playersJson = playersJson;
            this.editsJson = editsJson;
            this.creaturesJson = creaturesJson;
            this.creatureSeq = creatureSeq;
            this.rejectedCreatures = rejectedCreatures;
            this.dropsJson = dropsJson;
            this.chestsJson = chestsJson;
            this.furnacesJson = furnacesJson;
            this.timeOfDay = timeOfDay;
            this.weatherKind = weatherKind;
            this.weatherIntensity = weatherIntensity;
            this.contentSeq = contentSeq;
            this.contentChanged = contentChanged;
            this.owner = owner;
        }
    }

    /* 回调只在主线程触发, 成功与失败互斥. */
    public interface ListCallback {
        void onSuccess(List<Room> rooms);

        void onFailure(ApiException error);
    }

    public interface RoomCallback {
        void onSuccess(Room room);

        void onFailure(ApiException error);
    }

    public interface JoinCallback {
        void onSuccess(Room room, long seq);

        void onFailure(ApiException error);
    }

    public interface StateCallback {
        void onSuccess(State state);

        void onFailure(ApiException error);
    }

    public interface SimpleCallback {
        void onSuccess();

        void onFailure(ApiException error);
    }

    private static final String PATH_PREFIX = "/api/v1";
    private static final int CONNECT_TIMEOUT_MS = 5000;
    private static final int READ_TIMEOUT_MS = 6000;

    private final ExecutorService executor = Executors.newSingleThreadExecutor();
    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final Context context;

    public RoomApi(Context context) {
        this.context = context.getApplicationContext();
    }

    /* 房间列表; 大厅进入与刷新各调用一次. */
    public void list(final Session session, final ListCallback callback) {
        if (!usable(session)) {
            callback.onFailure(new ApiException("AUTH_UNAUTHORIZED", "no session"));
            return;
        }
        executor.execute(() -> {
            try {
                final JSONObject data = request("GET", "/rooms", session, null);
                final JSONArray raw = data.optJSONArray("rooms");
                final List<Room> rooms = new ArrayList<>();
                if (raw != null) {
                    for (int index = 0; index < raw.length(); index++) {
                        final JSONObject entry = raw.optJSONObject(index);
                        if (entry != null) {
                            rooms.add(parseRoom(entry));
                        }
                    }
                }
                mainHandler.post(() -> callback.onSuccess(rooms));
            } catch (ApiException error) {
                mainHandler.post(() -> callback.onFailure(error));
            } catch (RuntimeException error) {
                mainHandler.post(() -> callback.onFailure(
                        new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error))));
            }
        });
    }

    /* 建房; 房主即创建者, 房间号由服务端分配. */
    public void create(final Session session, final String name, final int maxPlayers,
                       final RoomCallback callback) {
        create(session, name, maxPlayers, "", callback);
    }

    /* 建房并设置房间密码; 密码为空表示无密码房间. */
    public void create(final Session session, final String name, final int maxPlayers,
                       final String password, final RoomCallback callback) {
        if (!usable(session)) {
            callback.onFailure(new ApiException("AUTH_UNAUTHORIZED", "no session"));
            return;
        }
        executor.execute(() -> {
            try {
                final JSONObject body = new JSONObject();
                body.put("name", name == null ? "" : name);
                body.put("maxPlayers", maxPlayers);
                if (password != null && !password.isEmpty()) {
                    body.put("password", password);
                }
                final JSONObject data = request("POST", "/rooms", session, body);
                final JSONObject raw = data.optJSONObject("room");
                if (raw == null) {
                    throw new ApiException(ApiException.CODE_PROTOCOL, "missing room");
                }
                final Room room = parseRoom(raw);
                mainHandler.post(() -> callback.onSuccess(room));
            } catch (ApiException error) {
                mainHandler.post(() -> callback.onFailure(error));
            } catch (JSONException | RuntimeException error) {
                mainHandler.post(() -> callback.onFailure(
                        new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error))));
            }
        });
    }

    /* 按房间号加入; 房间号不区分大小写. */
    public void join(final Session session, final String code, final JoinCallback callback) {
        join(session, code, "", callback);
    }

    /* 按房间号加入带密码的房间; 无密码房间传空串. */
    public void join(final Session session, final String code, final String password,
                     final JoinCallback callback) {
        if (!usable(session)) {
            callback.onFailure(new ApiException("AUTH_UNAUTHORIZED", "no session"));
            return;
        }
        executor.execute(() -> {
            try {
                final JSONObject body = new JSONObject();
                body.put("code", code == null ? "" : code.trim());
                if (password != null && !password.isEmpty()) {
                    body.put("password", password);
                }
                final JSONObject data = request("POST", "/rooms/join", session, body);
                final JSONObject raw = data.optJSONObject("room");
                if (raw == null) {
                    throw new ApiException(ApiException.CODE_PROTOCOL, "missing room");
                }
                final Room room = parseRoom(raw);
                final long seq = data.optLong("seq", 0L);
                mainHandler.post(() -> callback.onSuccess(room, seq));
            } catch (ApiException error) {
                mainHandler.post(() -> callback.onFailure(error));
            } catch (JSONException | RuntimeException error) {
                mainHandler.post(() -> callback.onFailure(
                        new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error))));
            }
        });
    }

    /* 房主改名或改人数上限; 两者都为空时不下发请求. */
    public void update(final Session session, final String roomId, final String name,
                       final int maxPlayers, final RoomCallback callback) {
        if (!usable(session) || roomId == null || roomId.isEmpty()) {
            callback.onFailure(new ApiException("AUTH_UNAUTHORIZED", "no session"));
            return;
        }
        executor.execute(() -> {
            try {
                final JSONObject body = new JSONObject();
                if (name != null && !name.isEmpty()) {
                    body.put("name", name);
                }
                if (maxPlayers > 0) {
                    body.put("maxPlayers", maxPlayers);
                }
                final JSONObject data = request("POST", "/rooms/" + roomId + "/update", session, body);
                final JSONObject raw = data.optJSONObject("room");
                if (raw == null) {
                    throw new ApiException(ApiException.CODE_PROTOCOL, "missing room");
                }
                final Room room = parseRoom(raw);
                mainHandler.post(() -> callback.onSuccess(room));
            } catch (ApiException error) {
                mainHandler.post(() -> callback.onFailure(error));
            } catch (JSONException | RuntimeException error) {
                mainHandler.post(() -> callback.onFailure(
                        new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error))));
            }
        });
    }

    /* 掉落物拾取申报: 服务端裁定归属, 先到先得. */
    public void pickup(final Session session, final String roomId, final int dropId,
                       final PickupCallback callback) {
        if (!usable(session) || roomId == null || roomId.isEmpty()) {
            callback.onFailure(new ApiException("AUTH_UNAUTHORIZED", "no session"));
            return;
        }
        executor.execute(() -> {
            try {
                final JSONObject body = new JSONObject();
                body.put("dropId", dropId);
                final JSONObject data = request("POST", "/rooms/" + roomId + "/pickup", session, body);
                final boolean picked = data.optBoolean("picked", false);
                final int block = data.optInt("block", 0);
                mainHandler.post(() -> callback.onResult(picked, block));
            } catch (ApiException error) {
                mainHandler.post(() -> callback.onFailure(error));
            } catch (JSONException | RuntimeException error) {
                mainHandler.post(() -> callback.onFailure(
                        new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error))));
            }
        });
    }

    /* 拾取回调: 成功时 picked 表示是否抢到, block 为掉落方块编号. */
    public interface PickupCallback {
        void onResult(boolean picked, int block);

        void onFailure(ApiException error);
    }

    /* 离房; 房主离开时服务端把房主身份移交给最早加入的在线成员. */
    public void leave(final Session session, final String roomId, final SimpleCallback callback) {
        if (!usable(session) || roomId == null || roomId.isEmpty()) {
            callback.onFailure(new ApiException("AUTH_UNAUTHORIZED", "no session"));
            return;
        }
        executor.execute(() -> {
            try {
                request("POST", "/rooms/" + roomId + "/leave", session, new JSONObject());
                mainHandler.post(callback::onSuccess);
            } catch (ApiException error) {
                mainHandler.post(() -> callback.onFailure(error));
            } catch (RuntimeException error) {
                mainHandler.post(() -> callback.onFailure(
                        new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error))));
            }
        });
    }

    /*
     * 房内一次同步: 上报自身位置与本地改动, 拉取序号大于 sinceSeq 的远端改动.
     * 位置与改动同一请求完成, 与位置通道的节拍语义一致.
     */
    public void sync(final Session session, final String roomId, final float x, final float y,
                     final float z, final float yaw, final float pitch, final String localEditsJson,
                     final String creaturesJson, final String dropsJson,
                     final String chestsJson, final String furnacesJson,
                     final float worldTimeSeconds, final int weatherKind,
                     final float weatherIntensity, final long contentSeq, final long sinceSeq,
                     final StateCallback callback) {
        if (!usable(session) || roomId == null || roomId.isEmpty()) {
            callback.onFailure(new ApiException("AUTH_UNAUTHORIZED", "no session"));
            return;
        }
        executor.execute(() -> {
            try {
                final JSONObject body = new JSONObject();
                body.put("x", (double) x);
                body.put("y", (double) y);
                body.put("z", (double) z);
                body.put("yaw", (double) yaw);
                body.put("pitch", (double) pitch);
                body.put("since", Math.max(0L, sinceSeq));
                /* 带上上次看到的内容版本, 版本没变时服务端不下发四份内容数组. */
                body.put("content", Math.max(0L, contentSeq));
                final String edits = localEditsJson == null || localEditsJson.isEmpty()
                        ? "[]" : localEditsJson;
                body.put("edits", new JSONArray(edits));
                /* 只有房主会上报生物快照, 客人传空数组, 服务端按拒绝计数. */
                if (creaturesJson != null && creaturesJson.length() > 2) {
                    body.put("creatures", new JSONArray(creaturesJson));
                }
                if (dropsJson != null && dropsJson.length() > 2) {
                    body.put("drops", new JSONArray(dropsJson));
                }
                if (chestsJson != null && chestsJson.length() > 2) {
                    body.put("chests", new JSONArray(chestsJson));
                }
                if (furnacesJson != null && furnacesJson.length() > 2) {
                    body.put("furnaces", new JSONArray(furnacesJson));
                }
                if (worldTimeSeconds > 0.0f) {
                    body.put("timeOfDay", (double) worldTimeSeconds);
                }
                /* 天气与时间同一条路: 只有房主的设定会被服务端采纳. */
                if (worldTimeSeconds > 0.0f) {
                    body.put("weatherKind", weatherKind);
                    body.put("weatherIntensity", (double) weatherIntensity);
                }
                final JSONObject data = request("POST", "/rooms/" + roomId + "/state", session, body);
                final State state = new State(data.optLong("seq", 0L),
                        data.optInt("rejectedEdits", 0),
                        arrayText(data, "players"),
                        arrayText(data, "edits"),
                        arrayText(data, "creatures"),
                        data.optLong("creatureSeq", 0L),
                        data.optInt("rejectedCreatures", 0),
                        arrayText(data, "drops"),
                        data.has("chests") ? arrayText(data, "chests") : null,
                        data.has("furnaces") ? arrayText(data, "furnaces") : null,
                        (float) data.optDouble("timeOfDay", 0.0),
                        data.optInt("weatherKind", 0),
                        (float) data.optDouble("weatherIntensity", 0.0),
                        data.optLong("contentSeq", 0L),
                        data.optBoolean("contentChanged", true),
                        data.optBoolean("owner", false));
                mainHandler.post(() -> callback.onSuccess(state));
            } catch (ApiException error) {
                mainHandler.post(() -> callback.onFailure(error));
            } catch (JSONException | RuntimeException error) {
                mainHandler.post(() -> callback.onFailure(
                        new ApiException(ApiException.CODE_PROTOCOL, String.valueOf(error))));
            }
        });
    }

    private static boolean usable(Session session) {
        return session != null && session.isUsable();
    }

    private static Room parseRoom(JSONObject raw) {
        return new Room(raw.optString("id", ""),
                raw.optString("code", ""),
                raw.optString("name", ""),
                raw.optString("ownerUid", ""),
                raw.optString("worldName", ""),
                raw.optInt("maxPlayers", 0),
                raw.optInt("memberCount", 0),
                raw.optLong("seed", 0L),
                raw.optBoolean("hasPassword", false));
    }

    /* 取数组字段的文本形式; 缺失时给空数组, 避免调用方判空. */
    private static String arrayText(JSONObject data, String key) {
        final JSONArray array = data.optJSONArray(key);
        return array == null ? "[]" : array.toString();
    }

    /* 执行一次请求并解包络; 成功返回 data 对象, 失败抛 ApiException. */
    private JSONObject request(String method, String path, Session session, JSONObject body)
            throws ApiException {
        final String baseUrl = AccountSettings.baseUrl(context);
        HttpURLConnection connection = null;
        try {
            final URL url = new URL(baseUrl + PATH_PREFIX + path);
            connection = (HttpURLConnection) url.openConnection();
            connection.setRequestMethod(method);
            connection.setConnectTimeout(CONNECT_TIMEOUT_MS);
            connection.setReadTimeout(READ_TIMEOUT_MS);
            connection.setRequestProperty("Accept", "application/json");
            connection.setRequestProperty("Authorization", "Bearer " + session.token());
            if (body != null) {
                final byte[] payload = body.toString().getBytes(StandardCharsets.UTF_8);
                connection.setRequestProperty("Content-Type", "application/json; charset=utf-8");
                connection.setFixedLengthStreamingMode(payload.length);
                connection.setDoOutput(true);
                try (OutputStream output = connection.getOutputStream()) {
                    output.write(payload);
                }
            }
            final int status = connection.getResponseCode();
            return parseEnvelope(readBody(connection, status), status);
        } catch (java.net.MalformedURLException error) {
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
        final InputStream stream = status >= 400 ? connection.getErrorStream() : connection.getInputStream();
        if (stream == null) {
            return "";
        }
        try (InputStream input = stream; ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            final byte[] buffer = new byte[4096];
            int read;
            while ((read = input.read(buffer)) != -1) {
                output.write(buffer, 0, read);
            }
            return new String(output.toByteArray(), StandardCharsets.UTF_8);
        }
    }

    private static JSONObject parseEnvelope(String text, int status)
            throws JSONException, ApiException {
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
