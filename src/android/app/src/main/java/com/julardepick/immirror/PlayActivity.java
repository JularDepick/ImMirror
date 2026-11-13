package com.julardepick.immirror;

import android.app.NativeActivity;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.media.AudioAttributes;
import android.media.SoundPool;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.julardepick.immirror.account.ApiClient;
import com.julardepick.immirror.lobby.MainLobbyActivity;
import com.julardepick.immirror.account.Session;
import com.julardepick.immirror.account.SessionStore;
import com.julardepick.immirror.audio.AmbientAudioManager;
import com.julardepick.immirror.audio.CreatureSoundBank;
import com.julardepick.immirror.editor.EditorBridge;
import com.julardepick.immirror.net.BlockSyncApi;
import com.julardepick.immirror.net.PresenceApi;
import com.julardepick.immirror.net.RoomApi;
import com.julardepick.immirror.progress.ProgressApi;
import com.julardepick.immirror.progress.ProgressBoard;
import android.view.WindowManager;

import com.julardepick.immirror.bridge.NativeBridge;
import com.julardepick.immirror.device.DeviceProfile;

/*
 * 游戏界面入口. 继承 NativeActivity, 由 android_native_app_glue 在独立线程上
 * 驱动 C++ 主循环; Java 侧只在进入与退出时各介入一次, 不参与逐帧逻辑.
 *
 * 每次进入游戏都是一个新的 Activity 实例与一条新的原生线程, 但进程与原生库
 * 的全局状态是复用的: 因此启动与停止请求只描述本次会话的玩法开关,
 * 原生主循环的存活只跟随 Activity 的销毁.
 *
 * 原生库由 NativeActivity 依据 manifest 中的 android.app.lib_name 自行加载,
 * 因此这里不需要手动调用加载接口, 但仍通过 NativeBridge 下发设备画像.
 */
public final class PlayActivity extends NativeActivity {

    /*
     * 与原生侧 SoundId 枚举顺序一致的音效资源, 按材质分族加载.
     * 每族按 "族名, 变体数, 别名族" 描述: 变体数大于 1 时按 assets 内
     * 1 起始的连续编号取文件 (挖掘族 1..4 号, 行走族取 1..6 号中的前 4 个);
     * 变体数为 1 的放置族在 APK 内是无编号单文件, 按裸名取文件.
     * 别名族不加载新文件, 直接复用目标族已加载的句柄:
     * 放置音没有布族的独立音源, place_cloth 复用 dig_cloth 变体.
     * jar 族没有独立行走音, walk 侧不设 jar 档; glass 族没有放置音,
     * place 侧不设 glass 档, 两处都由原生侧按近亲材质回落.
     */
    private static final String[][] SOUND_FAMILIES = {
        {"dig_grass", "4", null},
        {"dig_stone", "4", null},
        {"dig_sand", "4", null},
        {"dig_wood", "4", null},
        {"dig_snow", "4", null},
        {"dig_gravel", "4", null},
        {"dig_cloth", "4", null},
        {"dig_glass", "3", null},
        {"dig_jar", "3", null},
        {"place_grass", "1", null},
        {"place_stone", "1", null},
        {"place_sand", "1", null},
        {"place_wood", "1", null},
        {"place_snow", "1", null},
        {"place_gravel", "1", null},
        {"place_cloth", "4", "dig_cloth"},
        {"walk_grass", "4", null},
        {"walk_stone", "4", null},
        {"walk_sand", "4", null},
        {"walk_wood", "4", null},
        {"walk_snow", "4", null},
        {"walk_gravel", "4", null},
        {"walk_cloth", "4", null},
        {"walk_ladder", "4", null},
        {"env_rain", "4", null},
        {"env_thunder", "1", null},
    };

    private static final String TAG = "ImMirrorPlay";

    /* 启动附加字段: 要进入或新建的世界名, 与主菜单的世界列表配套. */
    public static final String EXTRA_WORLD = "world";

    /* 联机房间附加字段: 房间 id, 房间号与房间种子, 由联机大厅下发. */
    public static final String EXTRA_ROOM_ID = "room_id";
    public static final String EXTRA_ROOM_CODE = "room_code";
    public static final String EXTRA_ROOM_SEED = "room_seed";
    /* 本端是否为房主: 房主上行生物快照, 客人下行应用. */
    public static final String EXTRA_ROOM_OWNER = "room_owner";

    /* 成就统计的上报周期. */
    private static final long PROGRESS_REPORT_INTERVAL_MS = 30000L;
    private static final long PRESENCE_INTERVAL_MS = 10000L;
    /* 界面事件轮询周期: 死亡与合成成功这类提示音要跟得上手感, 取半秒. */
    private static final long UI_EVENT_INTERVAL_MS = 500L;
    private PresenceApi presenceApi;
    /* 方块改动同步: 与位置轮询同一节拍, 一次调用完成上报与拉取. */
    private BlockSyncApi blockSyncApi;
    private long blockSeq;
    /* 房间通道: 房内用一次请求完成位置上报与改动同步, 与位置通道互斥. */
    private RoomApi roomApi;
    private String roomId;
    private String roomCode;
    private long roomSeq;
    /* 上次看到的内容版本: 版本没变时服务端不下发四份内容数组. */
    private long roomContentSeq;
    /*
     * 断线重连: 连续失败到阈值时进入重连状态并在界面提示,
     * 恢复后自动退出; 房间已不存在时直接结束本局.
     */
    private int roomSyncFailures;
    private boolean roomReconnecting;
    private static final int ROOM_RECONNECT_THRESHOLD = 3;
    /* 已应用的生物快照版本号, 与位置通道的序号分开记. */
    private long roomCreatureSeq = -1L;
    /* 本端是否房主: 房主上行生物快照, 客人应用房主下发的快照. */
    private boolean roomOwner;
    private AmbientAudioManager ambientManager;
    private Handler presenceHandler;
    private Runnable presenceRunnable;
    /* 界面事件轮询: 原生侧产生的死亡与合成事件, 由 Java 侧配音. */
    private Handler uiEventHandler;
    private Runnable uiEventRunnable;
    private Session presenceSession;

    private SoundPool soundPool;
    private Handler progressHandler;
    private Runnable progressRunnable;
    private ProgressApi progressApi;
    private Session progressSession;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        /* 设置目录与存档目录同源, 都取应用内部存储根目录; 先初始化再启动主循环. */
        NativeBridge.initSettings(getFilesDir().getAbsolutePath());
        NativeBridge.onDeviceProfile(DeviceProfile.obtain(this).toNativePayload());
        /* 试玩参数: 从编辑器进入时携带工程名, 原生侧据此加载导出的世界存档. */
        final String playtestProject = getIntent().getStringExtra(EditorBridge.EXTRA_PLAYTEST);
        final String roomExtraId = getIntent().getStringExtra(EXTRA_ROOM_ID);
        if (roomExtraId != null && !roomExtraId.isEmpty()) {
            /*
             * 联机房间: 世界名按房间号生成, 种子取房间种子, 同一房间的各方因此生成
             * 同一份地形; 房内位置与方块改动由房间通道同步, 房主权威.
             */
            roomId = roomExtraId;
            roomCode = getIntent().getStringExtra(EXTRA_ROOM_CODE);
            roomOwner = getIntent().getBooleanExtra(EXTRA_ROOM_OWNER, false);
            /* 客人侧关闭本地自然生成: 生物以房主上报的快照为准. */
            NativeBridge.setGuestMode(!roomOwner);
            final String roomWorld = "room_" + (roomCode == null || roomCode.isEmpty()
                    ? roomExtraId : roomCode);
            NativeBridge.chooseWorldWithSeed(roomWorld, getIntent().getIntExtra(EXTRA_ROOM_SEED, 0));
        } else if (playtestProject != null && !playtestProject.isEmpty()) {
            NativeBridge.selectWorld(playtestProject);
        } else {
            /*
             * 大厅世界选择: 从世界列表进入时携带世界名, 原生侧据此切换存档路径,
             * 名字在存档目录下没有对应文件时按新建处理. 试玩优先, 两条通道互斥.
             */
            final String lobbyWorld = getIntent().getStringExtra(EXTRA_WORLD);
            if (lobbyWorld != null && !lobbyWorld.isEmpty()) {
                NativeBridge.chooseWorld(lobbyWorld);
            }
        }
        setupAudio();
        NativeBridge.start();
        startProgressReporting();
        startPresenceReporting();
        startUiEventPolling();
        /*
         * 环境音与背景音乐: 进游戏自动播放, 退出时释放.
         * 音量与音乐开关从设置层取值 (音量为 0 到 100 的滑杆值), 长音频通道
         * 不再使用固定音量; 音乐开关关闭时通道保持静默, 打开时进入正常轮播.
         */
        ambientManager = new AmbientAudioManager(this);
        final int volumeSetting = NativeBridge.getSetting("volume");
        final int musicSetting = NativeBridge.getSetting("musicopen");
        final float musicVolume = volumeSetting > 0 ? volumeSetting / 100.0f : 0.8f;
        ambientManager.syncSettings(musicVolume, musicSetting != 0);
        ambientManager.playAmbient();
    }

    @Override
    protected void onResume() {
        super.onResume();
        /*
         * 每次回到前台都重新接管玩法: 进程内可能先后存在多个实例
         * (例如连点"进入游戏"), 后一个实例销毁时会关闭全局的玩法开关,
         * 前一个实例恢复前台后需要重新打开, 否则画面停在上一次交换的缓冲上.
         */
        NativeBridge.start();
    }

    /*
     * 返回键: 交给原生侧的暂停面板, 连续按两次即开与关.
     * 真正退出对局走面板里的退出按钮, 避免误触直接丢局.
     */
    @Override
    public void onBackPressed() {
        NativeBridge.togglePauseMenu();
        drainUiEvents();
    }

    @Override
    protected void onPause() {
        /* 设置由原生侧异步落盘, 进入后台时强制写一次, 避免进程被回收丢改动. */
        NativeBridge.flushSettings();
        super.onPause();
    }

    /* 成就统计上报: 原生聚合增量, 周期取走上报服务端, 服务端按事件累计不重复发币. */
    private void startProgressReporting() {
        progressSession = SessionStore.load(this);
        if (progressSession == null || !progressSession.isUsable()) {
            return;
        }
        if (progressApi == null) {
            progressApi = new ProgressApi(this);
        }
        if (progressHandler == null) {
            progressHandler = new Handler(Looper.getMainLooper());
        }
        progressRunnable = new Runnable() {
            @Override
            public void run() {
                reportProgressOnce();
                progressHandler.postDelayed(this, PROGRESS_REPORT_INTERVAL_MS);
            }
        };
        progressHandler.postDelayed(progressRunnable, PROGRESS_REPORT_INTERVAL_MS);
    }

    /* 位置轮询: 上报自身并拉取他人, 10 秒周期. */
    private void startPresenceReporting() {
        presenceSession = SessionStore.load(this);
        if (presenceSession == null || !presenceSession.isUsable()) {
            Log.w(TAG, "presence not started, session unavailable");
            return;
        }
        Log.i(TAG, "presence started, interval=" + PRESENCE_INTERVAL_MS);
        if (presenceApi == null) {
            presenceApi = new PresenceApi(this);
        }
        if (blockSyncApi == null) {
            blockSyncApi = new BlockSyncApi(this);
        }
        if (roomApi == null) {
            roomApi = new RoomApi(this);
        }
        if (presenceHandler == null) {
            presenceHandler = new Handler(Looper.getMainLooper());
        }
        presenceRunnable = new Runnable() {
            @Override
            public void run() {
                presenceTick(presenceSession);
                presenceHandler.postDelayed(this, PRESENCE_INTERVAL_MS);
            }
        };
        presenceHandler.postDelayed(presenceRunnable, PRESENCE_INTERVAL_MS);
    }

    /* 界面事件轮询: 半秒一次取走原生事件队列并播放对应提示音. */
    private void startUiEventPolling() {
        if (uiEventHandler == null) {
            uiEventHandler = new Handler(Looper.getMainLooper());
        }
        uiEventRunnable = new Runnable() {
            @Override
            public void run() {
                drainUiEvents();
                uiEventHandler.postDelayed(this, UI_EVENT_INTERVAL_MS);
            }
        };
        uiEventHandler.postDelayed(uiEventRunnable, UI_EVENT_INTERVAL_MS);
    }

    private void stopUiEventPolling() {
        if (uiEventHandler != null && uiEventRunnable != null) {
            uiEventHandler.removeCallbacks(uiEventRunnable);
        }
        uiEventRunnable = null;
    }

    /* 事件编码: 0 死亡, 1 合成成功; 未知编码忽略, 只记一条日志. */
    private void drainUiEvents() {
        final String events = NativeBridge.takeUiEvents();
        if (events == null || events.isEmpty()) {
            return;
        }
        for (String part : events.split(",")) {
            final String code = part.trim();
            if (code.isEmpty()) {
                continue;
            }
            if ("0".equals(code)) {
                com.julardepick.immirror.ui.UiSound.death(this);
            } else if ("1".equals(code)) {
                com.julardepick.immirror.ui.UiSound.craftSuccess(this);
            } else if ("2".equals(code) || "3".equals(code)) {
                com.julardepick.immirror.ui.UiSound.bagToggle(this);
            } else if ("4".equals(code)) {
                com.julardepick.immirror.ui.UiSound.enchant(this);
            } else if ("5".equals(code)) {
                com.julardepick.immirror.ui.UiSound.pickup(this);
            } else if ("6".equals(code)) {
                com.julardepick.immirror.ui.UiSound.craftFailed(this);
            } else if ("7".equals(code)) {
                com.julardepick.immirror.ui.UiSound.eat(this);
            } else if ("8".equals(code)) {
                com.julardepick.immirror.ui.UiSound.hurt(this);
            } else if ("9".equals(code)) {
                /* 暂停面板的退出按钮: 结束本局, 回到大厅. */
                Log.i(TAG, "pause menu exit requested");
                finish();
            } else {
                Log.w(TAG, "unknown ui event " + code);
            }
        }
    }

    private void stopPresenceReporting() {
        if (presenceHandler != null && presenceRunnable != null) {
            presenceHandler.removeCallbacks(presenceRunnable);
        }
        presenceRunnable = null;
    }

    /*
     * 环境音场景到曲表分组的折算: 水下与岩浆优先于天气, 雨天与雷暴共用雨组,
     * 其余保持通用组, 与 AmbientAudioManager 的 19 首曲表对应.
     */
    private static String ambientGroupOf(int context) {
        switch (context) {
            case 1:
            case 2:
                return AmbientAudioManager.AMBIENT_GROUP_RAIN;
            case 3:
                return AmbientAudioManager.AMBIENT_GROUP_WATER;
            case 4:
                return AmbientAudioManager.AMBIENT_GROUP_LAVA;
            default:
                return AmbientAudioManager.AMBIENT_GROUP_JUNGLE;
        }
    }

    /*
     * 方块同步节拍: 取走本地改动推给服务端, 同时拉取别人的改动并应用.
     * 与位置轮询分开成独立方法, 便于单独调周期而不牵连位置上报.
     */
    private void blockSyncTick(final Session session) {
        if (blockSyncApi == null || session == null) {
            return;
        }
        final String localEdits = NativeBridge.takeBlockEdits();
        blockSyncApi.sync(session, localEdits, blockSeq, new BlockSyncApi.Callback() {
            @Override
            public void onSuccess(String editsJson, long seq) {
                if (editsJson != null && editsJson.length() > 2) {
                    NativeBridge.applyBlockEdits(editsJson);
                }
                blockSeq = seq;
            }

            @Override
            public void onFailure(com.julardepick.immirror.account.ApiException error) {
                Log.w(TAG, "block sync failed: " + error.code() + " " + error.detail());
            }
        });
    }

    /* 房间玩家数组的长度, 解析失败按零计, 只用于日志与诊断. */
    private static int countPlayers(String playersJson) {
        if (playersJson == null || playersJson.length() <= 2) {
            return 0;
        }
        try {
            return new org.json.JSONArray(playersJson).length();
        } catch (org.json.JSONException error) {
            return 0;
        }
    }

    /*
     * 房内同步节拍: 一次请求上报自身位置与本地改动, 同时拉取其他玩家与远端改动.
     * 远端玩家交给原生侧画成体素人形, 远端改动直接写世界, 不再回灌本地队列.
     */
    private void roomSyncTick(final Session session) {
        if (roomApi == null || roomId == null || roomId.isEmpty()) {
            return;
        }
        final String localEdits = NativeBridge.takeBlockEdits();
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float yaw = 0.0f;
        float pitch = 0.0f;
        final String pose = NativeBridge.takePlayerPose();
        if (pose != null && pose.length() > 2) {
            try {
                final org.json.JSONObject self = new org.json.JSONObject(pose);
                x = (float) self.optDouble("x", 0.0);
                y = (float) self.optDouble("y", 0.0);
                z = (float) self.optDouble("z", 0.0);
                yaw = (float) self.optDouble("yaw", 0.0);
                pitch = (float) self.optDouble("pitch", 0.0);
            } catch (org.json.JSONException error) {
                Log.w(TAG, "room pose parse failed: " + error);
            }
        }
        /* 房主每轮导出本地生物, 客人传空数组只拉取. */
        final String creatures = roomOwner ? NativeBridge.takeCreatureSnapshot() : "[]";
        final String drops = roomOwner ? NativeBridge.takeDropSnapshot() : "[]";
        final String chests = roomOwner ? NativeBridge.takeChestSnapshot() : "[]";
        final String furnaces = roomOwner ? NativeBridge.takeFurnaceSnapshot() : "[]";
        /* 房主上报世界时间, 客人不报, 由服务端把房主的时间下发回来. */
        final float worldTime = roomOwner ? NativeBridge.worldTimeSeconds() : 0.0f;
        final int weatherKind = roomOwner ? NativeBridge.weatherKind() : 0;
        final float weatherIntensity = roomOwner ? NativeBridge.weatherIntensity() : 0.0f;
        roomApi.sync(session, roomId, x, y, z, yaw, pitch, localEdits, creatures, drops, chests,
                furnaces, worldTime, weatherKind, weatherIntensity, roomContentSeq, roomSeq,
                new RoomApi.StateCallback() {
                    @Override
                    public void onSuccess(RoomApi.State state) {
                        roomSeq = state.seq;
                        roomContentSeq = state.contentSeq;
                        /* 任何一次成功都视为链路恢复, 清掉失败计数与提示. */
                        roomSyncFailures = 0;
                        if (roomReconnecting) {
                            roomReconnecting = false;
                            NativeBridge.showTip("RECONNECTED");
                            Log.i(TAG, "room sync recovered");
                        }
                        Log.i(TAG, "room sync ok, seq=" + state.seq + " players="
                                + countPlayers(state.playersJson) + " edits="
                                + state.editsJson.length() + " rejected=" + state.rejectedEdits);
                        if (state.editsJson != null && state.editsJson.length() > 2) {
                            NativeBridge.applyBlockEdits(state.editsJson);
                        }
                        try {
                            final org.json.JSONObject payload = new org.json.JSONObject();
                            payload.put("others", new org.json.JSONArray(state.playersJson));
                            NativeBridge.applyRemotePlayers(payload.toString());
                        } catch (org.json.JSONException error) {
                            Log.w(TAG, "room players apply failed: " + error);
                        }
                        if (state.rejectedEdits > 0) {
                            Log.w(TAG, "room rejected local edits=" + state.rejectedEdits);
                        }
                        if (state.rejectedCreatures > 0) {
                            Log.w(TAG, "room rejected local creatures=" + state.rejectedCreatures);
                        }
                        /* 客人应用房主下发的生物快照; 版本号变化时打一条便于核对. */
                        if (!roomOwner) {
                            /* 内容未变化时四份数组为 null, 直接跳过, 省掉重复解析. */
                            if (state.contentChanged) {
                                NativeBridge.applyRemoteCreatures(state.creaturesJson);
                                NativeBridge.applyRemoteDrops(state.dropsJson);
                                NativeBridge.applyRemoteChests(state.chestsJson);
                                NativeBridge.applyRemoteFurnaces(state.furnacesJson);
                            }
                            /* 客人跟房主的昼夜走, 两边看到的天色一致. */
                            if (state.timeOfDay > 0.0f) {
                                NativeBridge.setWorldTimeSeconds(state.timeOfDay);
                                /* 天气跟房主走, 两边的雨雪与天色一致. */
                                NativeBridge.setWeatherState(state.weatherKind,
                                        state.weatherIntensity);
                            }
                        }
                        if (state.creatureSeq != roomCreatureSeq) {
                            roomCreatureSeq = state.creatureSeq;
                            Log.i(TAG, "room creatures seq=" + state.creatureSeq + " count="
                                    + countPlayers(state.creaturesJson));
                        }
                    }

                    @Override
                    public void onFailure(com.julardepick.immirror.account.ApiException error) {
                        Log.w(TAG, "room sync failed: " + error.code() + " " + error.detail());
                        /*
                         * 房间已被关闭或自己已不在房内: 没有恢复可能, 直接结束本局.
                         * 其余错误 (网络抖动, 服务重启) 累加失败次数, 到阈值后提示重连.
                         */
                        if ("ROOM_NOT_FOUND".equals(error.code())
                                || "AUTH_UNAUTHORIZED".equals(error.code())) {
                            NativeBridge.showTip("ROOM CLOSED");
                            finish();
                            return;
                        }
                        roomSyncFailures += 1;
                        if (!roomReconnecting && roomSyncFailures >= ROOM_RECONNECT_THRESHOLD) {
                            roomReconnecting = true;
                            NativeBridge.showTip("RECONNECTING");
                            Log.w(TAG, "room sync reconnecting after " + roomSyncFailures
                                    + " failures");
                        }
                    }
                });
    }

    private void presenceTick(final Session session) {
        /* 日相与环境音: 顺带查询游戏内昼夜, 切换昼/夜环境音轨. */
        if (ambientManager != null) {
            ambientManager.setDayPhase(NativeBridge.getDayPhase());
            ambientManager.setAmbientGroup(ambientGroupOf(NativeBridge.getAmbientContext()));
        }
        /*
         * 房内走房间通道: 位置与方块改动在同一请求里完成, 不再走全局位置通道与
         * 全局改动通道, 两条通道互斥, 避免房内改动写进全局世界表.
         */
        if (roomId != null && !roomId.isEmpty()) {
            roomSyncTick(session);
            return;
        }
        /* 位置与方块同一节拍推进, 两条通道互不阻塞. */
        blockSyncTick(session);
        final String pose = NativeBridge.takePlayerPose();
        if (pose == null || pose.length() <= 2) {
            return;
        }
        try {
            final org.json.JSONObject self = new org.json.JSONObject(pose);
            presenceApi.reportAndFetch(session,
                    (float) self.optDouble("x", 0),
                    (float) self.optDouble("y", 0),
                    (float) self.optDouble("z", 0),
                    (float) self.optDouble("yaw", 0),
                    (float) self.optDouble("pitch", 0),
                    new PresenceApi.Callback() {
                        @Override
                        public void onSuccess(java.util.List<com.julardepick.immirror.net.PresenceApi.Snapshot> others,
                                              long serverTime) {
                            try {
                                org.json.JSONArray array = new org.json.JSONArray();
                                for (com.julardepick.immirror.net.PresenceApi.Snapshot other : others) {
                                    org.json.JSONObject item = new org.json.JSONObject();
                                    item.put("uid", other.uid);
                                    item.put("x", other.x);
                                    item.put("y", other.y);
                                    item.put("z", other.z);
                                    item.put("yaw", other.yaw);
                                    item.put("pitch", other.pitch);
                                    array.put(item);
                                }
                                org.json.JSONObject payload = new org.json.JSONObject();
                                payload.put("others", array);
                                NativeBridge.applyRemotePlayers(payload.toString());
                            } catch (org.json.JSONException | RuntimeException error) {
                                Log.w(TAG, "presence apply failed: " + error);
                            }
                        }

                        @Override
                        public void onFailure(com.julardepick.immirror.account.ApiException error) {
                            Log.w(TAG, "presence failed: " + error);
                        }
                    });
        } catch (org.json.JSONException | RuntimeException error) {
            Log.w(TAG, "presence tick failed: " + error);
        }
    }

    private void stopProgressReporting() {
        if (progressHandler != null && progressRunnable != null) {
            progressHandler.removeCallbacks(progressRunnable);
        }
        progressRunnable = null;
    }

    private void reportProgressOnce() {
        final String payload = NativeBridge.takeProgressCounters();
        if (payload == null || payload.length() <= 2) {
            return;
        }
        try {
            final org.json.JSONObject stats = new org.json.JSONObject(payload);
            if (stats.length() == 0) {
                return;
            }
            progressApi.reportStats(progressSession, stats, new ProgressApi.Callback() {
                @Override
                public void onSuccess(ProgressBoard board) {
                    Log.i(TAG, "progress reported");
                }

                @Override
                public void onFailure(com.julardepick.immirror.account.ApiException error) {
                    Log.w(TAG, "progress report failed: " + error);
                }
            });
        } catch (org.json.JSONException | RuntimeException error) {
            Log.w(TAG, "progress parse failed: " + error);
        }
    }

    @Override
    protected void onDestroy() {
        /*
         * 顺序固定: 先结束本次会话的玩法并落盘, 再释放音频引用, 最后交给父类.
         * 父类会等待原生线程返回, 而原生主循环要一直跑事件泵才能让这次等待结束,
         * 因此这里只表达“离开游戏”, 不表达“关闭进程内的原生线程”.
         */
        stopProgressReporting();
        stopPresenceReporting();
        stopUiEventPolling();
        if (ambientManager != null) {
            ambientManager.releaseAll();
        }
        /* 房内退出时主动离房, 服务端据此尽快把房主身份移交出去. */
        if (roomApi != null && roomId != null && !roomId.isEmpty() && presenceSession != null) {
            roomApi.leave(presenceSession, roomId, new RoomApi.SimpleCallback() {
                @Override
                public void onSuccess() {
                    Log.i(TAG, "room left, id=" + roomId);
                }

                @Override
                public void onFailure(com.julardepick.immirror.account.ApiException error) {
                    Log.w(TAG, "room leave failed: " + error.code() + " " + error.detail());
                }
            });
        }
        NativeBridge.stop();
        NativeBridge.flushSettings();
        NativeBridge.detachAudio();
        releaseAudio();
        super.onDestroy();
    }

    /*
     * 音效由平台音频池播放, 原生侧只持有句柄并按事件触发.
     * 每族按变体数展开加载: 句柄数组按 [族1变体1, 族1变体2, ..., 族2变体1, ...]
     * 平铺, 计数数组给出各族实际加载到的变体数, 原生侧按同一布局切开.
     * 别名族直接复用目标族的句柄数组, 不产生新的加载.
     * 脚步与挖掘可能并发, 并发上限提到 6, 避免脚步被挖掘挤掉.
     */
    private void setupAudio() {
        AudioAttributes attributes = new AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_GAME)
                .setContentType(AudioAttributes.CONTENT_TYPE_SONIFICATION)
                .build();
        soundPool = new SoundPool.Builder()
                .setMaxStreams(6)
                .setAudioAttributes(attributes)
                .build();
        final AssetManager assets = getAssets();
        /* 先按族加载, 再平铺成原生侧期望的布局: 每族至少占一个槽位,
         * 整族加载失败时保留一个 0 占位槽, 播放侧对 0 句柄静默跳过. */
        int[][] familyIds = new int[SOUND_FAMILIES.length][];
        for (int family = 0; family < SOUND_FAMILIES.length; family++) {
            final String name = SOUND_FAMILIES[family][0];
            final int variantCount = Integer.parseInt(SOUND_FAMILIES[family][1]);
            final String alias = SOUND_FAMILIES[family][2];
            if (alias != null) {
                int aliasIndex = -1;
                for (int probe = 0; probe < SOUND_FAMILIES.length; probe++) {
                    if (SOUND_FAMILIES[probe][0].equals(alias)) {
                        aliasIndex = probe;
                        break;
                    }
                }
                familyIds[family] = (aliasIndex >= 0 && familyIds[aliasIndex] != null)
                        ? familyIds[aliasIndex]
                        : new int[0];
                continue;
            }
            int[] ids = new int[variantCount];
            int loaded = 0;
            for (int variant = 1; variant <= variantCount; variant++) {
                /* 单文件族 (放置族) 在 APK 内不带编号, 多变体族按 1 起始编号展开. */
                final String path = variantCount == 1
                        ? "sounds/" + name + ".ogg"
                        : "sounds/" + name + variant + ".ogg";
                try {
                    AssetFileDescriptor descriptor = assets.openFd(path);
                    final int id = soundPool.load(descriptor, 1);
                    descriptor.close();
                    if (id == 0) {
                        throw new IllegalStateException("load returned 0");
                    }
                    ids[loaded] = id;
                    loaded++;
                } catch (Exception error) {
                    Log.w(TAG, "sound load failed: " + path + " " + error);
                    break;
                }
            }
            familyIds[family] = new int[loaded];
            System.arraycopy(ids, 0, familyIds[family], 0, loaded);
        }
        int total = 0;
        for (int[] ids : familyIds) {
            total += ids.length > 0 ? ids.length : 1;
        }
        int[] soundIds = new int[total];
        int[] familyCounts = new int[SOUND_FAMILIES.length];
        int cursor = 0;
        for (int family = 0; family < SOUND_FAMILIES.length; family++) {
            final int[] ids = familyIds[family];
            familyCounts[family] = ids.length > 0 ? ids.length : 1;
            if (ids.length == 0) {
                cursor++;
                continue;
            }
            System.arraycopy(ids, 0, soundIds, cursor, ids.length);
            cursor += ids.length;
        }
        NativeBridge.attachAudio(soundPool, soundIds, familyCounts);
        /* 生物音效: 独立句柄银行按资源路径懒加载, 与方块音效共用音频池. */
        CreatureSoundBank.initialize(this);
        NativeBridge.attachCreatureSound(CreatureSoundBank.class, CreatureSoundBank.pool());
    }

    private void releaseAudio() {
        if (soundPool != null) {
            soundPool.release();
            soundPool = null;
        }
    }
}