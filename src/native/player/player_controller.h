#pragma once

#include <vector>

#include "core/vec3.h"
#include "player/buff_state.h"
#include "player/input_tuning.h"
#include "player/survival_tuning.h"
#include "platform/input_state.h"
#include "settings/settings_tuning.h"
#include "voxel/block_registry.h"
#include "voxel/voxel_types.h"

namespace immirror::world {
class World;
}

namespace immirror::player {

/*
 * 视线射线拾取结果. face 是射线进入命中方块时穿过的那个面的外法线,
 * 分量取 -1, 0, 1, 放置方块时沿该法线向外偏移一格.
 */
struct RayHit {
    bool hit = false;
    voxel::BlockPos block{};
    voxel::BlockPos face{};
    float distance = 0.0f;
};

/*
 * 指针手势角色. 每根手指在按下时分配一个角色, 之后一直保持,
 * 中途越过屏幕中线也不切换, 因此摇杆指与视角指可以各自独立地同时工作,
 * 这正是边走边看所依赖的多点并发.
 */
enum class PointerRole {
    kNone = 0,
    kStick = 1,
    kLook = 2,
};

/*
 * 相机模式. 与设置键 view 的三个档位一一对应 (setting.lua 1666 至 1724 行):
 * 1 主视角为第一人称, 2 背视角为过肩第三人称 (相机在玩家身后跟随),
 * 3 正视角为面向玩家的第三人称 (相机在玩家前方回望).
 */
enum class CameraMode {
    kFirstPerson = 0,
    kThirdBack = 1,
    kThirdFront = 2,
};

/*
 * 第一人称玩家控制. 负责输入到运动的映射, 与体素世界的碰撞,
 * 以及视线射线拾取. 该对象只在原生主线程上使用.
 */
class PlayerController {
public:
    /*
     * 碰撞盒与视点高度. 生物接触判定, 攻击射线起点与总装的相机都取这一份,
     * 三处不再各写一套数字.
     */
    static constexpr float kHalfWidth = 0.3f;
    static constexpr float kHeight = 1.8f;
    static constexpr float kEyeHeight = 1.62f;

    /* 生命值上限, 只用于把当前血量折算成血条比例. */
    static constexpr float kMaxHealth = 100.0f;
    /* 受击反馈持续秒数, 总装据此叠加镜头摇晃. */
    static constexpr float kHurtSeconds = 0.35f;
    /* 受击后的短暂免伤窗口, 避免多只生物同帧叠加伤害直接击倒玩家. */
    static constexpr float kHurtCooldown = 0.35f;
    /* 死亡到回出生点的等待秒数. */
    static constexpr float kDeathSeconds = 1.5f;

    void Spawn(const world::World& world, float x, float z);

    /*
     * 输入快照按引用传入而不是只读: 控制器消费完累计位移后会清零,
     * 保证同一段位移不会被两帧重复计入.
     */
    void Update(double dt, platform::InputSnapshot& input, const world::World& world);

    /* 列主序 4x4 视图矩阵. 第三人称时视点按当前预设后撤与上抬. */
    void GetViewMatrix(float* out16) const;

    /*
     * 当前相机视点世界坐标: 第一人称即眼位, 第三人称是后撤回收后的轨点,
     * 总装的镜头摇晃以它为基准, 与视图矩阵共用同一视点.
     */
    core::Vec3 CameraPosition() const;

    /* 切换相机模式, 与设置键 view 的档位直接对应. */
    void SetCameraMode(CameraMode mode) { cameraMode_ = mode; }
    CameraMode GetCameraMode() const { return cameraMode_; }

    /* 返回脚底中心的世界坐标, 相机视点在此之上 kEyeHeight 处. */
    core::Vec3 Position() const;

    /* 视线单位方向. */
    core::Vec3 LookDirection() const;

    /* 当前朝向, 读档与存档时需要. */
    float Yaw() const { return yaw_; }
    float Pitch() const { return pitch_; }
    /* 脚下方块 id, 供脚步声按材质选择; 站在空气或液体时返回 Air. */
    voxel::BlockId GroundBlockId() const { return groundBlockId_; }
    /* 本帧是否在地面上移动 (触发脚步声的条件). */
    bool WalkingOnGround() const { return onGround_ && (velocity_.x != 0.0f || velocity_.z != 0.0f); }

    /*
     * 生命值. 血条按 HealthRatio 取长度, 上限见 kMaxHealth.
     * 受到伤害后进入受击反馈, 血量归零后等待 kDeathSeconds 再回出生点并回满血.
     */
    float Health() const { return health_; }
    float HealthRatio() const;
    bool Alive() const { return health_ > 0.0f; }

    /*
     * 生存属性查询, 供 HUD 下一波接入: 饥饿与体力各取绝对值与 0 到 1 比例,
     * 冲刺与潜行状态供界面高亮与音效分支使用.
     */
    float Hunger() const { return hunger_; }
    float HungerRatio() const;
    float Stamina() const { return stamina_; }
    float StaminaRatio() const;
    bool IsSprinting() const { return sprinting_; }
    bool IsSneaking() const { return sneaking_; }

    /*
     * 体力消耗的公共入口: 挖掘与跳跃在控制器内部结算, 攻击结算在总装侧,
     * 因此对总装开放这一个窄接口; 体力到饥饿的折算在内部统一完成.
     */
    void ConsumeStaminaPublic(float amount) { ConsumeStamina(amount); }

    /*
     * 本地方块改动: 挖掘与放置成功后记一条, 由总装取走并上报给服务端.
     * 联机同步只关心"哪一格变成了什么", 不关心是谁改的, 来源由服务端补.
     */
    struct BlockEdit {
        int32_t x = 0;
        int32_t y = 0;
        int32_t z = 0;
        voxel::BlockId block = 0;
    };
    /* 取走累计的改动, 取走后列表清空. */
    std::vector<BlockEdit> TakeBlockEdits();

    /* 受击反馈剩余秒数, 0 表示没有受击; 供总装叠加镜头摇晃. */
    float HurtSeconds() const { return hurtSeconds_; }
    /* 是否处于死亡等待中. */
    bool Dying() const { return deathSeconds_ > 0.0f; }
    /* 死亡等待的剩余秒数, 供界面画复活倒计时; 未死亡时为 0. */
    float DeathRemaining() const { return deathSeconds_ > 0.0f ? deathSeconds_ : 0.0f; }

    /*
     * 受到伤害. 伤害非正, 已死亡或仍在免伤窗口内时直接忽略;
     * 血量归零时清空挖掘进度并进入死亡等待, 由 Update 送回出生点.
     */
    void ApplyDamage(float amount);

    /* 读档后恢复位置与朝向, 跳过延迟落位. */
    void SetPose(const core::Vec3& position, float yaw, float pitch);

    /* 请求一次跳跃, 由界面按钮或键盘调用, 落地那一帧生效. */
    void RequestJump() { jumpRequested_ = true; }

    /* 浮动摇杆的视觉状态, 界面层据此绘制摇杆, 与输入共用同一锚点与半径. */
    platform::StickVisual GetStickVisual() const;

    /* 查询某路指针当前承担的角色, 界面层与总装据此判断点击归属. */
    PointerRole RoleOf(int32_t pointerId) const;

    /* 视角与摇杆的映射参数, 设置体系就绪后从这里注入. */
    void SetLookTuning(const LookTuning& tuning) { look_ = tuning; }
    const LookTuning& Look() const { return look_; }
    void SetStickTuning(const StickTuning& tuning) { stick_ = tuning; }
    const StickTuning& Stick() const { return stick_; }

    /* 破坏视线命中的方块; 返回是否命中并修改了世界. */
    bool BreakBlock(world::World& world);

    /*
     * 只读查询视线命中的方块 id, 命中非空气方块时返回 true.
     * 破坏前先取一次, 用于按方块查掉落物; 不修改世界, 不改动破坏路径的返回值.
     */
    bool ProbeBlock(const world::World& world, voxel::BlockId& outBlock) const;

    /* 在命中面的外侧放置方块. */
    bool PlaceBlock(world::World& world, voxel::BlockId id);

    /*
     * 一次挖掘会话的本帧结果. 控制器只汇报进度与事件, 音效, 进度环与日志
     * 由总装按这份结果处理, 控制器本身不触碰音频与界面.
     */
    struct MineTick {
        bool digSound = false;    /* 本帧应播放一次挖掘音, 起手与完成各算一次 */
        bool destroyed = false;   /* 本帧完成破坏, 方块已经从世界移除 */
        voxel::BlockId block = 0; /* 本次挖掘的目标方块 id, 供音效与日志取材质 */
        voxel::BlockPos blockPos{}; /* 目标方块的世界坐标, 供碎屑与特效定位 */
        float progress = 0.0f;    /* 当前进度, 0 到 1 */
        float elapsed = 0.0f;     /* 已累计的挖掘秒数 */
        float duration = 0.0f;    /* 该方块所需的总秒数, 由硬度换算 */
    };

    /*
     * 对准视线命中的方块开始挖掘. 未命中, 或目标硬度为 -1 (水, 基岩) 时返回 false,
     * 此时既不累计进度也不响应破坏.
     */
    bool BeginMining(world::World& world);

    /* 推进挖掘进度; 按住期间逐帧调用, 累计到所需时长时当场破坏方块. */
    MineTick AdvanceMining(double dt, world::World& world);

    /* 放弃当前挖掘, 进度清零; 松手或被取消时调用. */
    void CancelMining();

    /* 当前是否处于挖掘中. */
    bool Mining() const { return mine_.active; }

    /* 当前挖掘进度, 0 到 1; 未挖掘时为 0, 界面据此在归零时隐藏进度环. */
    float MiningProgress() const;

    /* 当前挖掘目标的方块 id, 未挖掘时为空气. */
    voxel::BlockId MiningBlock() const { return mine_.active ? mine_.id : 0; }

    /* 最近一次成功放置的方块坐标, 供总装做重力沉降等后处理. */
    voxel::BlockPos LastPlacedPos() const { return lastPlacedPos_; }

    /* 当前挖掘所需的总秒数, 未挖掘时为 0. */
    float MiningDuration() const { return mine_.active ? mine_.duration : 0.0f; }

    /*
     * 挖掘速度系数: 1.0 表示徒手, 大于 1 更快, 小于 1 更慢.
     * 由总装按手持工具与目标方块折算后下发, 控制器本身不查物品表.
     */
    void SetMiningSpeed(float multiplier);

    /*
     * 状态效果: 由总装在玩家吃到带效果的食物或药水时挂上.
     * 控制器在移动, 挖掘, 跳跃与回血四处消费, 自身不查效果表.
     */
    /*
     * 进食: 恢复饥饿度与生命, 生命不超过上限, 饥饿不超过满值.
     * 附加状态由调用方另行挂上, 控制器只负责数值.
     */
    void Eat(float food, float heal);

    void ApplyBuff(uint32_t buffId) { buffs_.Apply(buffId); }
    void ClearBuffs() { buffs_.Clear(); }
    const BuffState& Buffs() const { return buffs_; }

    /*
     * 按当前速度系数重算本次挖掘的总时长, 进度按比例保留.
     * 总装先起手拿到目标方块, 再按手持工具算速度, 然后调用本方法校正时长.
     */
    void RecomputeMiningDuration();

private:
    /* 把多指针快照翻译成摇杆量, 视角增量与跳跃请求. */
    void ProcessInput(float dt, platform::InputSnapshot& input);

    /* 为尚未认领的指针分配角色, 左半屏优先摇杆, 右半屏优先视角. */
    void AssignRoles(platform::InputSnapshot& input);

    /* 释放摇杆角色, 顺带做点击与双击跳跃判定. */
    void ReleaseStick();

    /* 依据锚点与触点位置更新摇杆量, 并累计手势位移. */
    void UpdateStick(const platform::InputSnapshot& input, float dt);

    /* 消费视角指的累计位移. */
    void UpdateLook(const platform::InputSnapshot& input);

    /* 应用一次视角拖动增量, 俯仰角在此裁剪到正负 89 度. */
    void UpdateLookDelta(float deltaWidth, float deltaHeight);

    /* 重力和跳跃积分, 以及逐轴分步的碰撞推进. */
    void StepMotion(double dt, const world::World& world);

    /* 沿单轴推进 amount, 遇到实心方块则贴面停下; 返回是否受阻. */
    bool MoveAxis(int32_t axis, float amount, const world::World& world);

    /* 以脚底中心构造 AABB, 判断是否与任意实心方块重叠. */
    bool CollidesAt(const world::World& world, const core::Vec3& feet) const;

    /* 判断 AABB 是否与指定方块格重叠. */
    bool AabbHitsBlock(const core::Vec3& feet, const voxel::BlockPos& block) const;

    /* DDA 体素步进, 返回视线命中的方块与进入面. */
    bool CastRay(const world::World& world, RayHit& hit) const;

    /* 依据地表高度把玩家放到可站立位置; 该列尚无方块数据时返回 false 保持待定. */
    bool PlaceOnSurface(const world::World& world, int32_t surfaceY);

    /* 逐格上抬直到碰撞盒不再与实心方块重叠, 用于离开被包住的处境. */
    bool LiftOutOfSolid(const world::World& world);

    /* 推进受击反馈与免伤窗口的计时. */
    void TickHurt(float seconds);

    /*
     * 生存属性结算: 消耗体力并按食物倍率折算成饥饿推进,
     * 体力不足时消耗被忽略 (动作照常执行, 只是不扣资源以外的惩罚本期不做).
     */
    /*
     * 消耗体力. advanceHunger 为真时按独立系数推进饥饿 (主动动作),
     * 受伤这类被动损失只扣体力, 不消耗长期饱食度.
     */
    void ConsumeStamina(float amount, bool advanceHunger = true);

    /*
     * 逐帧生存结算: 体力恢复, 冲刺与移动的持续消耗, 饥饿的回血与饿死.
     * 在 Update 末尾统一调用, 血量增减直接改 health_, 不走 ApplyDamage,
     * 避免与受击免伤窗口互锁.
     */
    void TickSurvival(float seconds);

    /* 判定脚部或身体是否浸在水中 (游泳倍率与游泳消耗的共用判定). */
    bool InWater(const world::World& world) const;

    /* 判定身体中段或脚部是否浸在蜂蜜里 (蜂蜜移动倍率的判定). */
    bool InHoney(const world::World& world) const;

    /* 回到出生点并清空运动状态. */
    void Respawn();

    core::Vec3 EyePosition() const;
    core::Vec3 ForwardDirection() const;

    /*
     * 第三人称视点: 从眼位沿视线反方向后撤预设距离并上抬预设高度,
     * 路径逐 0.1 米步进回收, 终点落在实心方块内时缩短到不碰撞.
     * 正视角的视线反向由 ForwardDirection 的取反封装处理.
     */
    core::Vec3 ThirdPersonEyePosition() const;

    /*
     * 一次挖掘会话: 目标格, 方块 id, 已累计秒数与总时长.
     * 进度只由硬度换算, 目标格固定不变, 抬手即清零, 与按住这个手势一一对应.
     */
    struct MineSession {
        bool active = false;
        voxel::BlockPos block{};
        voxel::BlockId id = 0;
        float elapsed = 0.0f;
        float duration = 0.0f;
        float nextSoundSeconds = 0.0f; /* 下一次挖掘音的剩余间隔 */
    };

    core::Vec3 position_{0.0f, 0.0f, 0.0f};  /* 脚底中心 */
    core::Vec3 velocity_{0.0f, 0.0f, 0.0f};
    core::Vec3 spawnPosition_{0.0f, 0.0f, 0.0f};
    float yaw_ = 0.0f;    /* 偏航角, 弧度, 0 表示朝向 -Z */
    float pitch_ = 0.0f;  /* 俯仰角, 弧度, 正值抬头 */
    bool onGround_ = false;
    voxel::BlockId groundBlockId_ = voxel::block::Air;
    bool spawned_ = false;

    /* 生命值状态: 当前血量, 受击反馈计时, 免伤窗口与死亡等待计时. */
    float health_ = kMaxHealth;
    /* 摔落伤害: 空中记录最高点, 落地按落差结算. */
    float fallPeakY_ = 0.0f;
    float hurtSeconds_ = 0.0f;
    float hurtCooldown_ = 0.0f;
    float deathSeconds_ = 0.0f;

    /* 生存属性: 饥饿与体力, 冲刺与潜行状态. 初始值取 survival_tuning. */
    float hunger_ = survival::kHungerInitial;
    float stamina_ = survival::kStaminaInitial;
    bool sprinting_ = false;
    bool sneaking_ = false;   /* 本期无潜行按键, 恒为 false, 字段先留 */
    bool spawnPending_ = false;   /* 出生列尚无区块数据, 等待地形生成后落位 */
    float pendingSeconds_ = 0.0f; /* 已等待落位的秒数, 用于超时保护 */

    /* 手势角色绑定, 按指针 id 区分, 同一时刻各自最多绑定一路指针. */
    int32_t stickPointerId_ = -1;
    int32_t lookPointerId_ = -1;

    /* 浮动摇杆: 锚点即摇杆指的按下点, 位置为归一化屏幕坐标. */
    float stickAnchorX_ = 0.0f;
    float stickAnchorY_ = 0.0f;
    float stickValueX_ = 0.0f;   /* 摇杆水平量, 向右为正 */
    float stickValueY_ = 0.0f;   /* 摇杆前向量, 向前为正 */
    float stickSeconds_ = 0.0f;  /* 摇杆指已按住的秒数 */
    float stickTravel_ = 0.0f;   /* 摇杆指累计位移, 用于区分点击与拖动 */

    float doubleTapTimer_ = -1.0f; /* 双击窗口剩余秒数, 负值表示未开启 */
    bool jumpRequested_ = false;
    int64_t lastKeyEvents_ = 0;    /* 已处理的累计按键事件数 */

    LookTuning look_;
    StickTuning stick_;
    MineSession mine_;
    /* 当前挂着的状态效果, 移动/挖掘/跳跃/回血四处消费. */
    BuffState buffs_;
    /* 待上报的方块改动, 有上限防止长时间离线时无限增长. */
    std::vector<BlockEdit> blockEdits_;

    /* 记一条待上报的方块改动, 内部用. */
    void QueueBlockEdit(int32_t x, int32_t y, int32_t z, voxel::BlockId block);
    /* 挖掘速度系数, 1.0 为徒手基准. */
    float miningSpeed_ = 1.0f;
    /* 最近一次成功放置的方块坐标, 默认零值表示尚未放置. */
    voxel::BlockPos lastPlacedPos_{};
    CameraMode cameraMode_ = CameraMode::kFirstPerson;
    /* 最近一次 Update 的世界, 相机后撤的碰撞回收在渲染期按同帧数据判定. */
    const world::World* worldRef_ = nullptr;
};

}  /* namespace immirror::player */