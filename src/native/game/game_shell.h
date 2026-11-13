#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "audio/audio_engine.h"
#include "core/vec3.h"
#include "entity/entity_manager.h"
#include "entity/projectile.h"
#include "game/ui_data_source_game.h"
#include "ui/script/widget_host.h"
#include "game/containers.h"
#include "game/container_screen.h"
#include "game/inventory.h"
#include "game/item/inventory_screen.h"
#include "platform/input_state.h"
#include "world/weather.h"
#include "world/world_time.h"
#include "ui/hud_renderer.h"
#include "voxel/block_registry.h"
#include "game/remote_player.h"
#include "render/creature_visual.h"
#include "voxel/voxel_types.h"

struct AAssetManager;

namespace immirror {

namespace platform {
struct InputSnapshot;
}

namespace render {
class ChunkRenderer;
}

namespace settings {
class Settings;
}

namespace world {
class World;
}

namespace player {
class PlayerController;
}

namespace editor {

/* 编辑器外壳, 由总装在表面就绪时按 Java 侧的请求接管世界与渲染. */
class EditorShell;

}  /* namespace editor */

namespace game {

/*
 * 游戏总装: 持有世界, 玩家与区块渲染器,
 * 把主循环的固定步长更新与每帧场景绘制接到具体玩法上.
 * 触摸按指针 id 分别跟踪: 先由界面按钮认领, 剩下的交给玩家控制器分配摇杆与视角角色,
 * 因此多根手指同时操作时各自的动作互不打断.
 * 所有方法都在原生主线程调用.
 */
class GameShell {
public:
    GameShell();
    ~GameShell();

    GameShell(const GameShell&) = delete;
    GameShell& operator=(const GameShell&) = delete;

    /* 图形上下文就绪后调用; dataPath 为应用内部存储目录, 用于世界存档与设置落盘. */
    void OnSurfaceReady(AAssetManager* assets, const char* dataPath);

    /* 图形上下文失效前调用, 释放全部 GPU 资源. */
    void OnSurfaceLost();

    void SetViewportSize(int32_t width, int32_t height);

    /* 按设备分级 (high/medium/low) 播种视距档位. */
    void SetQualityFromGrade(const char* grade);

    void Update(double deltaSeconds, const platform::InputSnapshot& input);

    /* 在图形上下文中绘制一帧场景; 由渲染设备在清屏后回调. */
    void RenderScene();

    /*
     * 联机方块同步: 取走本地改动并序列化成 JSON 数组, 同时应用远端改动.
     * 远端改动直接写世界, 不进本地改动队列, 避免来回反弹.
     */
    std::string TakeBlockEditsJson();
    void ApplyRemoteBlockEdits(const std::string& json);

    /* 手持工具对目标方块的挖掘速度系数, 1.0 为徒手基准. */
    float MiningSpeedAgainst(voxel::BlockId block) const;

    /* 饥饿低于阈值时自动吃一样可食用物品, 吃掉返回 true. */
    bool TryAutoEat();

    /* 进食判定的随机源, 只用于附加状态的万分比判定. */
    uint32_t nextRandom();

    /*
     * 当前天空底色, 按昼夜混合后写入 out 的三个分量.
     * 供渲染后端清屏使用, 与雾色分开: 雾只影响远景方块, 天空不参与雾.
     */
    void SkyColor(float* out) const;

    /* 供 JNI 桥读取背包与选中格; 装配前调用无副作用. */
    Inventory& InventoryRef() { return inventory_; }

    /* 试玩模式: 读取编辑器导出的世界存档; 装配后调用, 在下次 Update 前生效. */
    void LoadNamedWorld(const std::string& projectName);

    /*
     * 多世界: 把当前存档路径切换到 worlds 目录下指定名字的存档并立即载入;
     * 文件不存在时只切路径, 后续进入流程走地形生成, 即"新建世界".
     * 返回是否载入了已有存档.
     */
    /*
     * 选择世界: seed 为 0 时按世界名与时间派生新种子, 非 0 时直接采用,
     * 供联机房间让同一房间的各方生成同一份地形.
     */
    bool SelectWorld(const std::string& name, uint32_t seed = 0);

    /*
     * 扫描目录下的 *.world 存档, 逐个读头取元数据, 返回 JSON 数组:
     * [{"file":..,"name":..,"lastPlayed":..,"seed":..,"playerX":..,"playerZ":..},..]
     * 目录不存在或没有存档时返回空数组.
     */
    std::string ListWorlds(const std::string& directory);

    /* 联机: 输出玩家姿态 JSON, 接收远程玩家快照 JSON. */
    std::string FormatPlayerPose() const;

    /* 当前是否为游戏内白天, 供环境音日相切换. */
    bool IsDay() const;

    /*
     * 环境音场景: 0 通用, 1 雨天, 2 雷暴, 3 水下, 4 岩浆.
     * 由相机所在方块与当前天气折算, Java 侧据此切换环境音分组.
     */
    int32_t AmbientContext() const;
    void ApplyRemotePlayers(const std::string& json);

    /*
     * 重力沉降: 方块表重力方式为 1 的方块 (沙, 砂砾) 悬空时向下落,
     * 每次方块改动后按改动点上方一列处理, 只落不改其它方块.
     */
    void SettleGravityAt(int32_t x, int32_t y, int32_t z);

    /*
     * 流体流动: 改动点周围有重力方式为 2 的液体时, 液体向相邻空格蔓延,
     * 单次最多 24 格, 半径 3 格, 避免挖开一片水域时整片灌满.
     */
    void SettleFluidAt(int32_t x, int32_t y, int32_t z);

    /*
     * 生物快照导出: 把当前活体整理成 JSON 数组, 字段为
     * id, kind (定义索引), x, y, z, yaw, hp. 供房主上行给房间.
     */
    std::string TakeCreatureSnapshot() const;

    /* 本批远端生物插值进度 0 到 1, 供渲染与再下发时固化当前位置. */
    float RemoteCreatureAlpha() const;

    /*
     * 远端生物快照: 客人侧把房主下发的活体列表写入渲染队列,
     * 下一帧与本地生物同一批提交; 空数组表示清空.
     */
    void ApplyRemoteCreatures(const std::string& json);
    /* 上一轮下发的远端玩家数, 用于只在数量变化时打日志. */
    int32_t lastRemoteCount_ = -1;
    /*
     * 环境伤害: 岩浆里持续掉血, 头部长时间在水下开始溺水.
     * 溺水计时在离开水面时清零.
     */
    void UpdateEnvironmentDamage(double deltaSeconds);

    /* 连续在水下的秒数, 用于溺水判定. */
    double drownSeconds_ = 0.0;

    /*
     * 界面事件队列: 原生侧发生的、需要界面层配音的事件 (死亡, 合成成功).
     * 事件用单字节编码, 由 Java 侧低频取走并播放对应界面音.
     */
    std::string TakeUiEvents();

    /* 上一帧玩家是否存活, 用于识别死亡沿. */
    bool lastPlayerAlive_ = true;
    /* 上一帧血量, 用于识别受伤沿; 负值表示尚未采样. */
    float lastHealth_ = -1.0f;
    std::vector<uint8_t> uiEvents_;

    /*
     * 地面掉落物: 破坏方块与生物掉落先落在地上, 走近再拾取.
     * 渲染交给粒子系统的掉落物通道, 游戏层只维护位置与存活.
     */
    struct DroppedItem {
        int32_t id = 0;
        item::ItemId item = 0;
        voxel::BlockId block = 0;
        int32_t count = 0;
        core::Vec3 position;
        core::Vec3 velocity;
        double ageSeconds = 0.0;
    };
    void SpawnDrop(item::ItemId item, int32_t count, voxel::BlockId block, float x, float y,
                   float z);

    /*
     * 掉落物快照: 房主上行给房间, 客人据此在本地画一份"别人的掉落物".
     * 客人侧只做展示, 不参与拾取, 拾取仍由房主权威决定.
     */
    std::string TakeDropSnapshot() const;
    void ApplyRemoteDrops(const std::string& json);

    /*
     * 储物箱内容快照: 房主上行, 客人据此把本地箱子填成房主的内容.
     * 键沿用容器坐标编码的十进制串, 两端编码一致, 不需要额外对表.
     */
    std::string TakeChestSnapshot() const;
    void ApplyRemoteChests(const std::string& json);

    /*
     * 熔炉内容快照: 与箱子同口径, 只同步输入/燃料/产出三个格,
     * 燃烧计时与进度仍由房主自己推进, 客人只看到静态内容.
     */
    std::string TakeFurnaceSnapshot() const;
    void ApplyRemoteFurnaces(const std::string& json);
    /* 当前显示的远端掉落物标识, 用于下一轮差集删除. */
    std::vector<int32_t> remoteDropIds_;
    void UpdateDrops(double deltaSeconds);
    std::vector<DroppedItem> droppedItems_;
    int32_t nextDropId_ = 1;
    /* 掉落概率用的轻量伪随机状态, 与其它模块同一套 xorshift. */
    uint32_t dropRandom_ = 0x9e3779b9u;

    /*
     * 进度中心的逐帧驱动: 在线时长与行走距离在 Tick 里累计,
     * 缺最后一次位置时只推进时间, 不做位移累计, 避免瞬移被算成走路.
     */
    core::Vec3 lastProgressPos_;
    bool hasProgressPos_ = false;

    /* 战斗框目标: 最近攻击过的生物标识与显示截止时刻. */
    entity::EntityId battleTargetId_ = 0;
    double battleTargetUntil_ = 0.0;
    /* 战斗框上的血量数值文本, 形如 "18/20". */
    char battleTargetLabel_[24] = {};
    /* Boss 血条上的血量数值文本, 与战斗框同一格式. */
    char bossLabel_[24] = {};

    /* 飞行物: 箭一类投射物, 与生物管理器并列由总装逐帧推进. */
    entity::ProjectileSystem projectiles_;
    /* 最近一次射击后的冷却, 避免一次按键连发. */
    float shotCooldown_ = 0.0f;

    /* 发射一发箭: 手持弓且有箭时消耗一支并生成飞行物. */
    bool TryShootArrow();

    /* 生物射出的箭: 与玩家箭共用同一套飞行物, 但命中玩家而不是生物. */
    entity::ProjectileSystem hostileProjectiles_;
    /* 推进飞行物并结算命中. */
    void UpdateProjectiles(double deltaSeconds);
    /*
     * 暂停菜单: 打开期间世界时间, 天气, 世界与玩家推进全部冻结,
     * 只保留面板按钮的指针处理. 由 Java 侧返回键触发切换.
     */
    /*
     * 房间客人模式: 关掉本地自然生成并清空现有生物, 世界里的生物改由
     * 房主上报的快照驱动, 否则同一只生物会被本地与远端各画一遍.
     */
    void SetGuestMode(bool guest);

    /* 界面层给一条提示: 与内部提示同一通道, 供网络状态一类的低频事件使用. */
    void ShowTipFromUi(const char* text) { ShowTip(text); }

    /* 供界面层查询当前是否潜水: 环境音压减按它判断. */
    bool IsUnderwater() const { return wasInWater_; }

    /*
     * 客人侧拾取申报: 掉落物由房主维护, 客人只能申报"我要捡这个",
     * 由服务端裁定归属. 取走后返回逗号分隔的掉落物标识.
     */
    std::string TakePickupRequests();

    /* 服务端裁定成功后, 把物品放进本地背包. */
    void ApplyPickupResult(int32_t dropId, int32_t blockId);

    /*
     * 打开一份界面: 读布局与同名脚本, 建控件树并执行脚本.
     * 之后每帧把绘制命令交给界面渲染器, 直到关闭.
     */
    bool OpenUiScreen(const char* layoutAsset, const char* scriptAsset);
    void CloseUiScreen();
    bool UiScreenOpen() const { return uiHost_ != nullptr; }

    /* 把游戏状态接到界面宿主上; 界面脚本因此能读到背包与玩家数据. */
    void BindUiDataSource(ui::script::WidgetHost& host);

    /* 世界时间: 总秒数, 供房间同步昼夜; 客人侧写入房主的时间. */
    float WorldTimeSeconds() const;
    void SetWorldTimeSeconds(float seconds);

    /* 天气: 房主上报编号与强度, 客人侧据此对齐降水与天色. */
    int32_t WeatherKindCode() const;
    float WeatherIntensity() const;
    void SetWeatherState(int32_t kind, float intensity);

    /* 本端是否允许改动方块; 房间客人没有改方块权限, 世界内容以房主为准. */
    bool BlocksEditable() const { return blocksEditable_; }
    bool blocksEditable_ = true;
    /* 本端是否房主: 掉落物拾取与内容上报按它判断, 与编辑权限无关. */
    bool isHost_ = true;

    void TogglePauseMenu() { pauseMenuOpen_ = !pauseMenuOpen_; }
    bool PauseMenuOpen() const { return pauseMenuOpen_; }

    /* 暂停面板的指针处理: 命中"继续"关闭面板, 命中"退出"发一条退出事件. */
    void UpdatePauseMenu(platform::InputSnapshot& input);

    bool pauseMenuOpen_ = false;

    /*
     * 天气环境音: 降水期间按间隔重复播放雨声, 雷暴时另外插入雷声.
     * 间隔取一段随机区间, 避免雨声节奏过于机械.
     */
    float rainSoundSeconds_ = 0.0f;
    float thunderSoundSeconds_ = 0.0f;
    /* 上一帧是否浸水: 用于识别入水与出水的瞬间各播一次溅水音. */
    bool wasInWater_ = false;

    /* 伤害飘字: 文案与剩余显示秒数; 受设置里的"伤害数值显示"开关控制. */
    char damageText_[16] = {};
    float damagePopupSeconds_ = 0.0f;
    /* 飘字的世界锚点, 取命中生物胸口位置, 每帧投影成屏幕坐标. */
    core::Vec3 damageAnchor_;

    /*
     * 远端生物插值状态: 每只生物保存上一轮位置与目标位置, 渲染时按
     * 两次下发之间的时间做线性插值, 避免 10 秒一次的轮询节拍让形象瞬移.
     */
    struct RemoteCreatureState {
        render::creature::CreatureView from;
        render::creature::CreatureView to;
    };
    std::vector<RemoteCreatureState> remoteCreatureStates_;
    /* 与 remoteCreatureStates_ 一一对应的实体标识, 用于跨批次匹配同一只生物. */
    std::vector<uint32_t> remoteCreatureIds_;
    /* 本批插值的起始时刻, 与 elapsedSeconds_ 同一时钟. */
    double remoteCreatureBlendStart_ = 0.0;
    /* 上一轮下发的远端生物数, 只在数量变化时打日志. */
    int32_t lastRemoteCreatureCount_ = -1;
    int32_t SelectedSlot() const { return selectedSlot_; }
    void SelectSlotPublic(int32_t index) { SelectSlot(index); }

    /*
     * 成就统计计数器: 原生侧聚合, Java 侧周期拉取并上报服务端.
     * 拉取即取走并清零, 单调累计放在服务端, 这里只保存未上报的增量.
     */
    struct ProgressCounters {
        int64_t breakBlocks = 0;
        int64_t placeBlocks = 0;
        int64_t pickedItems = 0;
        int64_t walkedMeters = 0;
        int64_t crafts = 0;
        int64_t smelted = 0;
        int64_t kills = 0;
    };
    ProgressCounters TakeProgressCounters();
    void CountProgressEvent(int32_t kind);
    audio::SoundId WalkSoundForBlock(voxel::BlockId block);

private:
    /*
     * 把设置项折算成控制器参数: 灵敏度与反转 Y 进 LookTuning,
     * 左手与经典模式的固定底座进 StickTuning. 逐帧调用, 设置改变即时生效.
     */
    void ApplyControlSettings();

    /* 界面按钮与方块栏的认领与结算: 跳跃按钮按下即跳, 方块栏短按选中, 长按显示名称. */
    void UpdateHudButtons(platform::InputSnapshot& input, double deltaSeconds);

    /*
     * 容器面板 (熔炉与储物箱) 的指针结算, 与背包面板同源:
     * 命中面板交给容器状态机, 点在面板以外关闭并退回物品.
     */
    void UpdateContainerPanel(platform::InputSnapshot& input);

    /*
     * 背包面板的指针认领与结算. 面板打开时独占全部指针:
     * 命中面板交给背包界面处理, 点在面板以外则关闭面板并归还合成材料.
     */
    void UpdateInventoryPanel(platform::InputSnapshot& input);

    /* 开局物品: 快捷栏每种方块一叠, 背包里放几样常用材料, 方便直接试合成. */
    void SeedStartingItems();

    /* 选中槽位的物品变化后同步编辑用方块, 物品不可放置时回落到空气. */
    void RefreshSelectedItem();

    /*
     * 编辑手势: 右半屏按住不松手按硬度累积破坏进度, 轻点放置当前材料,
     * 与摇杆和视角各自独立.
     */
    void UpdateEditGesture(platform::InputSnapshot& input, double deltaSeconds);

    /* 对准视线命中的方块开始挖掘; 目标不可破坏时返回 false, 本次手势只能结算为放置. */
    bool BeginEditMining();

    /* 推进挖掘进度: 按节拍播放挖掘音, 挖满时破坏方块并收取掉落物. */
    void AdvanceEditMining(double deltaSeconds);

    /* 把这次破坏的掉落物放进物品栏, 堆满或没有对应槽位时只累计丢弃数. */
    /* 方块破坏后的掉落处理: 按破坏点生成地面掉落物. */
    void CollectDrop(voxel::BlockId brokenBlock, const voxel::BlockPos& position);

    /*
     * 生物与战斗: 每帧推进生物管理器, 把生物对玩家的伤害转交玩家控制器,
     * 并把死亡掉落收进物品栏. 实体层不反向依赖玩家层, 两者在这里对接.
     */
    void UpdateEntities(double deltaSeconds);

    /* 视线拾取生物并造成一次伤害; 命中返回 true, 未命中时本次手势仍按挖掘与放置走. */
    bool TryAttackCreature();

    /* 按住攻击时的连续出手结算; 目标消失后回落到挖掘. */
    void AdvanceHeldAttack(double deltaSeconds);

    /* 把生物死亡掉落的物品放进物品栏. */
    void CollectCreatureDrops();

    /* 用当前选中槽位的方块放置一次; 数量为 0 时拒绝放置并给出提示. */
    bool TryPlaceSelectedBlock();

    /* 方块栏提示: 登记提示文本并重置显示时长, 到时自动消失. */
    void ShowTip(const char* text);

    /* 方块名标签与方块栏提示的显示时长倒计时. */
    void UpdateNamePlate(double deltaSeconds);
    void UpdateTip(double deltaSeconds);

    /* 自动跳跃: 前推量足够且前方是一格高台阶时补一次跳跃请求. */
    void MaybeRequestAutoJump();

    /* 切换方块栏选中项, 同时刷新编辑所用方块. */
    void SelectSlot(int32_t slot);

    /* 设置改动后的视距与音量同步. */
    void SyncViewDistance();

    /*
     * 雾效与亮度的设置同步: 折算出密度与亮度系数缓存, 变化时经渲染器下发.
     * 血条开关与树形同样在这里取缓存, 供 RenderScene 与世界更新使用.
     */
    void SyncVisualSettings();

    /* 把视距折算成生物生成半径下发, 视距外没有区块数据, 生成会落空. */
    void SyncEntitySpawnExtent();
    void SyncAudioSettings();

    void LogStatus(double deltaSeconds);

    /* 把世界与玩家状态写入存档, 容器状态写入独立的容器文件. */
    void SaveWorld();

    /*
     * 编辑手势按下时探测视线目标是否为容器方块:
     * 熔炉或箱子按住超过长按窗口即打开对应面板, 探测失败走原有挖掘路径.
     */
    bool ProbeContainerTarget();

    /* 推进长按开面板的计时, 返回本次手势是否已被容器面板接管. */
    bool AdvanceContainerProbe(double deltaSeconds);

    /* 关闭全部容器面板并把格子里的东西退回物品栏. */
    void CloseContainerPanels();

    /* 组装存档路径, 必要时创建目录. */
    void PrepareSavePath(const char* dataPath);

    /* 由存档文件路径折算同目录的容器文件路径, 两文件始终配套. */
    void SyncContainerPath();

    /* 表面重建后复位指针快照与手势持有的状态. */
    void ResetInputState();

    std::unique_ptr<world::World> world_;
    std::unique_ptr<world::WorldTime> worldTime_;

    /*
     * 界面数据源: 把背包, 玩家, 世界与进度接到界面脚本上.
     * 用指针是因为它需要引用上述对象, 而这些对象在构造期之后才就位.
     */
    std::unique_ptr<GameUiDataSource> uiDataSource_;
    /* 当前打开的界面: 控件宿主持有布局, 脚本与控件状态. */
    std::unique_ptr<ui::script::WidgetHost> uiHost_;
    /* 资产管理器, 由 OnSurfaceReady 传入, 界面装载要用它读布局与脚本. */
    AAssetManager* assets_ = nullptr;
    /* 当前世界名, 界面显示与数据源都用它. */
    std::string worldName_ = "default";
    /* 待申报的掉落物标识, 由客人侧在走到掉落物上时填入. */
    std::vector<int32_t> pendingPickups_;
    /* 天气状态机: 按玩家所在群系的权重周期重抽, 结果下发给渲染侧. */
    world::Weather weather_;
    world::WeatherKind lastWeatherKind_ = world::WeatherKind::kClear;
    std::unique_ptr<player::PlayerController> player_;
    std::unique_ptr<render::ChunkRenderer> renderer_;
    ui::HudRenderer hud_;
    platform::InputSnapshot lastInput_;

    /* 设置单例, 构造后即绑定, 逐帧只读原子快照. */
    settings::Settings* settings_ = nullptr;

    int32_t width_ = 0;
    int32_t height_ = 0;
    bool surfaceReady_ = false;

    /* 上一帧按下的指针位图, 用于识别本帧新按下的指针. */
    uint64_t previousDownMask_ = 0ull;

    /* 已消费到的触摸事件计数, 防止同一份累计位移被两帧重复计入. */
    int64_t lastProcessedTouchEvents_ = -1;

    /* 跳跃按钮持有的指针, 按下期间保持按压反馈. */
    int32_t jumpPointerId_ = -1;
    bool jumpHeld_ = false;

    /* 方块栏持有的指针与长按计时. */
    int32_t slotPointerId_ = -1;
    int32_t slotHoldIndex_ = -1;
    double slotHoldSeconds_ = 0.0;
    bool slotNameShown_ = false;

    /* 方块名标签: 显示中的槽位与剩余秒数. */
    int32_t nameSlot_ = -1;
    double nameRemainSeconds_ = 0.0;

    /* 物品栏: 前九格与快捷栏一一对应, 放置时扣减, 破坏与生物掉落入栏. */
    Inventory inventory_;

    /* 背包界面状态与它的一帧绘制数据, 面板打开时才填充. */
    InventoryScreen inventoryScreen_;
    ui::InventoryPanelFrame inventoryFrame_;

    /*
     * 容器系统: 世界里的熔炉与储物箱按方块坐标存放,
     * 面板状态机与背包面板同源, 互斥打开.
     */
    FurnaceMap furnaces_;
    ChestMap chests_;
    FurnaceScreen furnaceScreen_;
    ChestScreen chestScreen_;
    ui::FurnacePanelFrame furnaceFrame_;
    ui::ChestPanelFrame chestFrame_;
    /* 容器存档路径, 与世界存档同目录, 后缀区分. */
    std::string containerPath_;
    /* 熔炼事件采样基线: 全部熔炉产出堆叠的上帧合计, 负值表示尚未采样. */
    int64_t smeltBaseline_ = -1;
    /* 长按开面板的探测: 编辑指针按下时的目标方块与计时. */
    voxel::BlockId containerProbeBlock_ = voxel::block::Air;
    int32_t containerProbeX_ = 0;
    int32_t containerProbeY_ = 0;
    int32_t containerProbeZ_ = 0;
    double containerProbeSeconds_ = 0.0;
    bool containerProbeOpened_ = false;

    /* 背包按钮持有的指针, 按下期间保持按压反馈. */
    int32_t bagPointerId_ = -1;
    bool bagHeld_ = false;

    /* 生物实体管理器, 持有全部生物并负责生成, 休眠, 回收与伤害结算. */
    entity::EntityManager entities_;

    /* 攻击手势状态: 本次手势是否已转为攻击, 以及是否出手过 (抬手不再放置方块). */
    bool attackHeld_ = false;
    bool editAttacked_ = false;
    double attackAccumulator_ = 0.0;

    /* 方块栏提示文本与剩余显示秒数, 空指针表示不显示. */
    const char* tipText_ = nullptr;
    double tipRemainSeconds_ = 0.0;

    /* 编辑手势持有的指针与按下点. */
    int32_t editPointerId_ = -1;
    float editDownX_ = 0.0f;
    float editDownY_ = 0.0f;
    float editLastX_ = 0.0f;
    float editLastY_ = 0.0f;
    double editDownSeconds_ = 0.0;

    /* 音频设置生效状态的缓存, 避免每帧重复下发. */
    int32_t audioVolumePercent_ = -1;
    int32_t audioSoundOpen_ = -1;
    int32_t audioMusicOpen_ = -1;

    /* 视觉设置生效状态的缓存: 雾密度, 亮度系数, 血条开关与树形档. */
    float fogDensity_ = -1.0f;
    float brightness_ = -1.0f;
    int32_t fogCachedSetting_ = -1;
    int32_t brightCachedSetting_ = -1;
    int32_t bloodSwitch_ = -1;
    int32_t treeShape_ = -1;

    /* 上次自动跳跃的时刻, 用于限制触发频率. */
    double lastAutoJumpSeconds_ = -1.0;

    /* 生物死亡起点时刻: 实体 id 到死亡登记时的 elapsedSeconds_, 供倒地渐变折算. */
    std::unordered_map<uint32_t, double> deathStartTimes_;

    double elapsedSeconds_ = 0.0;
    /* 进食附加状态判定用的轻量随机源. */
    uint32_t eatRandom_ = 0x9e3779b9u;
    /* 自动进食冷却, 单位秒. */
    float eatCooldown_ = 0.0f;
    double logAccumulator_ = 0.0;
    double saveAccumulator_ = 0.0;
    std::string savePath_;
    /*
     * 大厅在表面就绪前下发的世界名: 此时存档路径还没建立, 先记住,
     * 等 OnSurfaceReady 建好路径后再切换, 保证读档落到正确的文件.
     */
    std::string pendingWorldName_;
    /* 与延迟世界名配套的种子, 0 表示按世界名派生. */
    uint32_t pendingWorldSeed_ = 0;
    int32_t viewDistanceChunks_ = 4;
    /* 天光混合因子的平滑缓存, 每帧向目标值靠近, 10 秒完成过渡. */
    float skyBlend_ = 1.0f;
    ProgressCounters progressCounters_;
    net::RemotePlayerManager remotePlayers_;
    double walkSoundTimer_ = 0.0;
    int32_t selectedSlot_ = 0;
    voxel::BlockId selectedBlock_ = voxel::block::Grass;
    const char* qualityGrade_ = "high";
};

}  /* namespace game */
}  /* namespace immirror */