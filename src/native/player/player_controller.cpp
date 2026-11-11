#include "player/player_controller.h"

#include <cmath>
#include <cstdint>

#include "core/log.h"
#include "core/vec3.h"
#include "player/mining_tuning.h"
#include "voxel/block_registry.h"
#include "world/world.h"

namespace immirror::player {
namespace {

constexpr const char* kTag = "ImMirrorPlayer";

/* 角度与无量纲常量. */
constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 2.0f * kPi;
constexpr float kDegreesToRadians = kPi / 180.0f;

/* 运动参数, 单位为米与秒. */
constexpr float kGravity = -24.0f;            /* 重力加速度 */
constexpr float kMoveSpeed = 4.5f;            /* 水平移动速度 */
constexpr float kJumpSpeed = 8.0f;            /* 起跳初速度 */
constexpr float kFallSpeedLimit = -58.0f;     /* 下落速度上限, 控制单帧位移量 */

/*
 * 碰撞盒与视点高度统一取 PlayerController 的类内常量,
 * 生物接触判定与总装的相机都复用同一份尺寸, 这里不再另写一套.
 */

/* 相机与拾取参数. */
constexpr float kMaxPitch = 89.0f * kDegreesToRadians;  /* 俯仰限制 */
constexpr float kReachDistance = 6.0f;                  /* 射线最长距离 */
constexpr int32_t kMaxRaySteps = 96;                    /* DDA 步数上限, 防御异常方向 */

/*
 * 触摸映射参数, 屏幕坐标均为归一化值.
 * 分区只在指针按下的那一刻决定初始角色, 之后手指越过中线也不改判,
 * 因此边走边看不会被半屏边界打断.
 */
constexpr float kRoleSplitX = 0.5f;      /* 摇杆与视角的初始分区边界 */
constexpr float kTapMaxSeconds = 0.22f;  /* 判定点击的最长按压时长 */
constexpr float kTapMaxTravel = 0.02f;   /* 判定点击的最大累计位移 */
constexpr float kDoubleTapWindow = 0.35f; /* 双击时间窗 */

/* 碰撞推进参数. */
constexpr float kMaxSubStep = 0.2f;                   /* 单次碰撞推进的最大位移 */
constexpr int32_t kMaxSubSteps = 24;                  /* 单帧推进次数上限 */
constexpr float kSurfaceEpsilon = 1.0e-3f;            /* 贴面间隙, 避免重复判定接触 */
constexpr float kMaxFrameSeconds = 0.1;               /* 单帧时间片上限, 防止卡顿穿墙 */
constexpr float kPendingSpawnTimeout = 2.5f;          /* 等待出生列生成的秒数上限 */
constexpr float kFallbackSpawnRatio = 0.5f;           /* 超时后暂放的相对世界高度比例 */

/* 键盘调试用的安卓按键码, 与 platform 层上报的 lastKeyCode 一致. */
constexpr int32_t kKeySpace = 62;
constexpr int32_t kKeyEnter = 66;
constexpr int32_t kKeyCenter = 23;
constexpr int32_t kKeyButtonA = 96;

int32_t FloorToInt(float value) {
    return static_cast<int32_t>(std::floor(value));
}

float ClampFloat(float value, float low, float high) {
    if (value < low) {
        return low;
    }
    return value > high ? high : value;
}

/* 沿单轴判定所需的 AABB 半长: 竖直轴向上取整高, 向下取 0, 水平轴取半宽. */
float AxisHalfExtentPos(int32_t axis, float halfWidth, float height) {
    return axis == 1 ? height : halfWidth;
}

float AxisHalfExtentNeg(int32_t axis, float halfWidth) {
    return axis == 1 ? 0.0f : halfWidth;
}

/*
 * 实心判定统一走方块注册表, 水等非实心方块不参与水平阻挡与落地判定.
 * 碰撞与放置的所有方块查询都经过本函数, 属性口径只有这一处.
 */
bool IsSolidBlock(voxel::BlockId id) {
    return voxel::BlockRegistry::Instance().Info(id).solid;
}

/*
 * 挖掘所需秒数: 硬度乘比例系数, 并抬到最短时长.
 * 硬度为 -1 表示不可破坏, 返回 0 作为拒绝累计进度的信号.
 */
float MineDurationOf(float hardness) {
    if (hardness < 0.0f) {
        return 0.0f;
    }
    const float seconds = hardness * mining::kSecondsPerHardness;
    return seconds < mining::kMinMineSeconds ? mining::kMinMineSeconds : seconds;
}

}  /* namespace */

void PlayerController::Spawn(const world::World& world, float x, float z) {
    position_ = core::Vec3{x, 0.0f, z};
    velocity_ = core::Vec3{};
    onGround_ = false;
    spawned_ = true;
    spawnPending_ = true;
    pendingSeconds_ = 0.0f;

    /*
     * HeightAt 返回该列最高非空方块的上方一格, 正好是站立所需的脚底高度;
     * 返回 0 说明该列还没有区块数据, 先保持待定, 由 Update 在数据就绪后落位.
     */
    PlaceOnSurface(world, world.HeightAt(FloorToInt(x), FloorToInt(z)));
}

bool PlayerController::PlaceOnSurface(const world::World& world, int32_t surfaceY) {
    /* 任何已生成的列都有基岩, 因此高度为 0 只可能是区块尚未加载. */
    if (surfaceY <= 0) {
        return false;
    }
    const float clamped = ClampFloat(static_cast<float>(surfaceY), 0.0f,
                                     static_cast<float>(voxel::kWorldHeight - 1));
    position_.y = clamped;

    /* 该高度可能被洞穴顶板或树冠占用, 逐格上移直到碰撞盒不再重叠. */
    if (!LiftOutOfSolid(world)) {
        return false;
    }
    spawnPosition_ = position_;
    spawnPending_ = false;
    pendingSeconds_ = 0.0f;
    onGround_ = false;
    return true;
}

bool PlayerController::LiftOutOfSolid(const world::World& world) {
    for (int32_t guard = 0; guard <= voxel::kWorldHeight; ++guard) {
        if (!CollidesAt(world, position_)) {
            return true;
        }
        position_.y += 1.0f;
    }
    return false;
}

void PlayerController::Update(double dt, platform::InputSnapshot& input, const world::World& world) {
    /* 未显式出生时按当前水平坐标补一次出生, 避免玩家自由落体掉出世界. */
    if (!spawned_) {
        Spawn(world, position_.x, position_.z);
    }

    /* 卡顿后单帧时间片截断, 保证分步碰撞的位移量始终小于方块边长. */
    double clamped = dt;
    if (!(clamped > 0.0)) {
        return;
    }
    if (clamped > kMaxFrameSeconds) {
        clamped = kMaxFrameSeconds;
    }

    /*
     * 死亡等待: 期间不处理输入也不推进物理, 计时结束后回到出生点并回满血.
     * 重生放在这里而不是伤害发生的当场, 让受击与死亡有一个可感知的停顿.
     */
    if (deathSeconds_ > 0.0f) {
        deathSeconds_ -= static_cast<float>(clamped);
        if (deathSeconds_ > 0.0f) {
            return;
        }
        deathSeconds_ = 0.0f;
        Respawn();
        IM_LOGI(kTag, "player respawn at spawn point, health=%.0f",
                static_cast<double>(health_));
        return;
    }
    TickHurt(static_cast<float>(clamped));

    /* 出生列还没有区块数据时保持静止, 等地形生成后再落位, 期间不推进物理. */
    if (spawnPending_) {
        pendingSeconds_ += static_cast<float>(clamped);
        const int32_t surface = world.HeightAt(FloorToInt(position_.x), FloorToInt(position_.z));
        if (!PlaceOnSurface(world, surface)) {
            if (pendingSeconds_ < kPendingSpawnTimeout) {
                return;
            }
            /* 超时保护: 暂放到世界中空, 交给重力与虚空回退接管. */
            position_.y = static_cast<float>(voxel::kWorldHeight) * kFallbackSpawnRatio;
            spawnPosition_ = position_;
            velocity_ = core::Vec3{};
            spawnPending_ = false;
            pendingSeconds_ = 0.0f;
        }
    }

    ProcessInput(static_cast<float>(clamped), input);
    StepMotion(clamped, world);

    /* 区块晚于出生生成或外部写入方块时, 玩家可能被包在实心方块里, 逐格上抬脱困. */
    if (CollidesAt(world, position_) && !LiftOutOfSolid(world)) {
        Respawn();
    }

    /* 低于地面或高于世界顶视为脱离有效范围, 直接回出生点. */
    if (position_.y < 0.0f || position_.y > static_cast<float>(voxel::kWorldHeight)) {
        Respawn();
    }
}

void PlayerController::GetViewMatrix(float* out16) const {
    if (out16 == nullptr) {
        return;
    }
    const core::Vec3 forward = ForwardDirection();
    /* 右向只取水平分量, 与俯仰角无关, 保证翻滚角恒为零. */
    const core::Vec3 right{std::cos(yaw_), 0.0f, -std::sin(yaw_)};
    const core::Vec3 up = core::Cross(right, forward);
    const core::Vec3 eye = EyePosition();

    /* 列主序: 前三列是相机基向量, 第四列是视点平移. */
    out16[0] = right.x;
    out16[1] = up.x;
    out16[2] = -forward.x;
    out16[3] = 0.0f;
    out16[4] = right.y;
    out16[5] = up.y;
    out16[6] = -forward.y;
    out16[7] = 0.0f;
    out16[8] = right.z;
    out16[9] = up.z;
    out16[10] = -forward.z;
    out16[11] = 0.0f;
    out16[12] = -core::Dot(right, eye);
    out16[13] = -core::Dot(up, eye);
    out16[14] = core::Dot(forward, eye);
    out16[15] = 1.0f;
}

core::Vec3 PlayerController::Position() const {
    return position_;
}

core::Vec3 PlayerController::LookDirection() const {
    return ForwardDirection();
}

bool PlayerController::BreakBlock(world::World& world) {
    RayHit hit;
    if (!CastRay(world, hit)) {
        return false;
    }
    return world.SetBlock(hit.block.x, hit.block.y, hit.block.z, 0);
}

bool PlayerController::ProbeBlock(const world::World& world, voxel::BlockId& outBlock) const {
    RayHit hit;
    if (!CastRay(world, hit)) {
        return false;
    }
    outBlock = world.GetBlock(hit.block.x, hit.block.y, hit.block.z);
    return outBlock != 0;
}

bool PlayerController::PlaceBlock(world::World& world, voxel::BlockId id) {
    if (id == 0) {
        return false;
    }
    RayHit hit;
    if (!CastRay(world, hit)) {
        return false;
    }
    const voxel::BlockPos target{hit.block.x + hit.face.x, hit.block.y + hit.face.y,
                                 hit.block.z + hit.face.z};
    if (target.y < 0 || target.y >= voxel::kWorldHeight) {
        return false;
    }
    /* 只能放进空格, 且不能与玩家自身碰撞盒重叠. */
    if (world.GetBlock(target.x, target.y, target.z) != 0) {
        return false;
    }
    if (AabbHitsBlock(position_, target)) {
        return false;
    }
    return world.SetBlock(target.x, target.y, target.z, id);
}

bool PlayerController::BeginMining(world::World& world) {
    /* 重新开始挖掘时先丢弃上一段进度, 每个会话只对应一次按下. */
    CancelMining();

    RayHit hit;
    if (!CastRay(world, hit)) {
        return false;
    }
    const voxel::BlockId id = world.GetBlock(hit.block.x, hit.block.y, hit.block.z);
    const voxel::BlockInfo& info = voxel::BlockRegistry::Instance().Info(id);
    const float duration = MineDurationOf(info.hardness);
    if (duration <= 0.0f) {
        /* 硬度 -1 的水与基岩: 不累计进度, 也不响应破坏. */
        return false;
    }

    mine_.active = true;
    mine_.block = hit.block;
    mine_.id = id;
    mine_.elapsed = 0.0f;
    mine_.duration = duration;
    mine_.nextSoundSeconds = 0.0f;
    return true;
}

PlayerController::MineTick PlayerController::AdvanceMining(double dt, world::World& world) {
    MineTick tick;
    if (!mine_.active) {
        return tick;
    }
    tick.block = mine_.id;
    tick.duration = mine_.duration;

    /* 目标格被其他途径改动时结束本次挖掘, 不把进度算到别的方块上. */
    if (world.GetBlock(mine_.block.x, mine_.block.y, mine_.block.z) != mine_.id) {
        CancelMining();
        return tick;
    }

    const float seconds = dt > 0.0 ? static_cast<float>(dt) : 0.0f;
    mine_.elapsed += seconds;

    /*
     * 挖掘音按固定间隔重复: 起手立即播一次, 之后每 kDigSoundIntervalSeconds 播一次,
     * 破坏完成那一帧再补一次收尾音.
     */
    mine_.nextSoundSeconds -= seconds;
    if (mine_.nextSoundSeconds <= 0.0f) {
        tick.digSound = true;
        mine_.nextSoundSeconds += mining::kDigSoundIntervalSeconds;
    }

    tick.elapsed = mine_.elapsed;
    if (mine_.elapsed < mine_.duration) {
        tick.progress = mine_.elapsed / mine_.duration;
        return tick;
    }

    /* 累计到所需时长才算破坏, 达成后立即写入世界并结束会话. */
    tick.progress = 1.0f;
    tick.destroyed = world.SetBlock(mine_.block.x, mine_.block.y, mine_.block.z, 0);
    if (tick.destroyed) {
        tick.digSound = true;
    }
    CancelMining();
    return tick;
}

void PlayerController::CancelMining() {
    mine_.active = false;
    mine_.block = voxel::BlockPos{};
    mine_.id = 0;
    mine_.elapsed = 0.0f;
    mine_.duration = 0.0f;
    mine_.nextSoundSeconds = 0.0f;
}

float PlayerController::MiningProgress() const {
    if (!mine_.active || mine_.duration <= 0.0f) {
        return 0.0f;
    }
    const float ratio = mine_.elapsed / mine_.duration;
    if (ratio < 0.0f) {
        return 0.0f;
    }
    return ratio > 1.0f ? 1.0f : ratio;
}

void PlayerController::ProcessInput(float dt, platform::InputSnapshot& input) {
    if (doubleTapTimer_ > 0.0f) {
        doubleTapTimer_ -= dt;
        if (doubleTapTimer_ < 0.0f) {
            doubleTapTimer_ = -1.0f;
        }
    }

    /* 键盘调试通道: 按键请求与按钮, 双击共用同一条跳跃请求, 跳跃时再判是否落地. */
    if (input.keyEvents != lastKeyEvents_) {
        const int32_t code = input.lastKeyCode;
        if (code == kKeySpace || code == kKeyEnter || code == kKeyCenter || code == kKeyButtonA) {
            jumpRequested_ = true;
        }
        lastKeyEvents_ = input.keyEvents;
    }

    /* 手势角色按指针 id 绑定, 手指抬起即让出角色, 不影响另一根手指的绑定. */
    if (stickPointerId_ >= 0 && !input.IsDown(stickPointerId_)) {
        ReleaseStick();
    }
    if (lookPointerId_ >= 0 && !input.IsDown(lookPointerId_)) {
        lookPointerId_ = -1;
    }

    AssignRoles(input);
    UpdateStick(input, dt);
    UpdateLook(input);

    /* 累计位移已在本帧消费完毕, 清零后不会被下一帧重复计入. */
    input.ClearMotion();
}

void PlayerController::AssignRoles(platform::InputSnapshot& input) {
    for (int32_t i = 0; i < platform::kMaxPointers; ++i) {
        const platform::PointerState& pointer = input.pointers[i];
        if (!pointer.down || pointer.uiClaimed) {
            continue;
        }
        if (pointer.id == stickPointerId_ || pointer.id == lookPointerId_) {
            continue;
        }

        /*
         * 左半屏落点优先当摇杆, 右半屏落点优先当视角; 首选角色已被占用时
         * 让给另一个空角色. 这样第一根手指落在任意一侧都能用, 第二根手指自动补位,
         * 之后手指越过中线也不会改判, 摇杆与视角因此可以同时操作.
         */
        const bool preferStick =
            stick_.leftHanded ? (pointer.x >= kRoleSplitX) : (pointer.x < kRoleSplitX);
        PointerRole target = PointerRole::kNone;
        if (preferStick && stickPointerId_ < 0) {
            target = PointerRole::kStick;
        } else if (!preferStick && lookPointerId_ < 0) {
            target = PointerRole::kLook;
        } else if (stickPointerId_ < 0) {
            target = PointerRole::kStick;
        } else if (lookPointerId_ < 0) {
            target = PointerRole::kLook;
        }

        if (target == PointerRole::kStick) {
            stickPointerId_ = pointer.id;
            /* 摇杆模式锚点取按下点 (浮动摇杆); 经典模式锚点固定在底座中心. */
            stickAnchorX_ = stick_.fixedAnchor ? stick_.anchorX : pointer.x;
            stickAnchorY_ = stick_.fixedAnchor ? stick_.anchorY : pointer.y;
            stickValueX_ = 0.0f;
            stickValueY_ = 0.0f;
            stickSeconds_ = 0.0f;
            stickTravel_ = 0.0f;
            IM_LOGI(kTag, "stick bound pointer=%d anchor=%.3f,%.3f", pointer.id, stickAnchorX_,
                    stickAnchorY_);
        } else if (target == PointerRole::kLook) {
            lookPointerId_ = pointer.id;
            IM_LOGI(kTag, "look bound pointer=%d", pointer.id);
        }
    }
}

void PlayerController::ReleaseStick() {
    /* 短促点击计入双击窗口, 窗口内的第二次点击触发跳跃, 与界面跳跃按钮互不干扰. */
    if (stickSeconds_ <= kTapMaxSeconds && stickTravel_ <= kTapMaxTravel) {
        if (doubleTapTimer_ > 0.0f) {
            jumpRequested_ = true;
            doubleTapTimer_ = -1.0f;
        } else {
            doubleTapTimer_ = kDoubleTapWindow;
        }
    }
    stickPointerId_ = -1;
    stickValueX_ = 0.0f;
    stickValueY_ = 0.0f;
}

void PlayerController::UpdateStick(const platform::InputSnapshot& input, float dt) {
    if (stickPointerId_ < 0) {
        stickValueX_ = 0.0f;
        stickValueY_ = 0.0f;
        return;
    }
    const platform::PointerState* pointer = input.Find(stickPointerId_);
    if (pointer == nullptr) {
        stickValueX_ = 0.0f;
        stickValueY_ = 0.0f;
        return;
    }
    stickSeconds_ += dt;

    const float aspect = input.Aspect();
    const float safeAspect = aspect > 0.0001f ? aspect : 1.0f;
    /* 纵向位移折算到与横向相同的长度单位, 摇杆在屏幕上才是各向同性的圆. */
    const float deltaX = pointer->x - stickAnchorX_;
    const float deltaY = (pointer->y - stickAnchorY_) / safeAspect;
    const float motionY = pointer->motionY / safeAspect;
    stickTravel_ += std::sqrt(pointer->motionX * pointer->motionX + motionY * motionY);

    const float radius = stick_.radius > 0.0001f ? stick_.radius : 0.0001f;
    float right = deltaX / radius;
    float forward = -deltaY / radius;
    /* 裁剪进单位圆, 斜向不会因为分量的平方和而加速. */
    const float length = std::sqrt(right * right + forward * forward);
    if (length > 1.0f) {
        right /= length;
        forward /= length;
    }
    stickValueX_ = right;
    stickValueY_ = forward;
}

void PlayerController::UpdateLook(const platform::InputSnapshot& input) {
    if (lookPointerId_ < 0) {
        return;
    }
    const platform::PointerState* pointer = input.Find(lookPointerId_);
    if (pointer == nullptr) {
        return;
    }
    /* 视角指用平台层累加的整帧位移, 一帧内的多次回调与折返滑动都不会丢失转角. */
    UpdateLookDelta(pointer->motionX, pointer->motionY);
}

void PlayerController::UpdateLookDelta(float deltaWidth, float deltaHeight) {
    /* 右拖右转, 下拖低头; 角度按屏宽与屏高换算, 具体斜率集中放在 LookTuning. */
    const float verticalSign = look_.invertY ? -1.0f : 1.0f;
    yaw_ -= deltaWidth * (look_.DegreesPerWidth() * kDegreesToRadians);
    pitch_ -= verticalSign * deltaHeight * (look_.DegreesPerHeight() * kDegreesToRadians);
    pitch_ = ClampFloat(pitch_, -kMaxPitch, kMaxPitch);
    /* 偏航角回绕到正负 pi, 长时间旋转也不会累积浮点误差. */
    while (yaw_ > kPi) {
        yaw_ -= kTwoPi;
    }
    while (yaw_ < -kPi) {
        yaw_ += kTwoPi;
    }
}

platform::StickVisual PlayerController::GetStickVisual() const {
    platform::StickVisual visual;
    visual.active = stickPointerId_ >= 0;
    visual.anchorX = stickAnchorX_;
    visual.anchorY = stickAnchorY_;
    visual.valueX = stickValueX_;
    visual.valueY = stickValueY_;
    visual.radius = stick_.radius;
    return visual;
}

PointerRole PlayerController::RoleOf(int32_t pointerId) const {
    if (pointerId < 0) {
        return PointerRole::kNone;
    }
    if (pointerId == stickPointerId_) {
        return PointerRole::kStick;
    }
    if (pointerId == lookPointerId_) {
        return PointerRole::kLook;
    }
    return PointerRole::kNone;
}

void PlayerController::StepMotion(double dt, const world::World& world) {
    const float seconds = static_cast<float>(dt);

    /* 水平方向: 摇杆量投影到水平前向与右向, 模长不超过 1. */
    const float cosYaw = std::cos(yaw_);
    const float sinYaw = std::sin(yaw_);
    const core::Vec3 forward{-sinYaw, 0.0f, -cosYaw};
    const core::Vec3 right{cosYaw, 0.0f, -sinYaw};
    core::Vec3 wish = forward * stickValueY_ + right * stickValueX_;
    const float wishLength = wish.Length();
    if (wishLength > 1.0f) {
        wish = wish * (1.0f / wishLength);
    }
    velocity_.x = wish.x * kMoveSpeed;
    velocity_.z = wish.z * kMoveSpeed;

    if (jumpRequested_ && onGround_) {
        velocity_.y = kJumpSpeed;
        onGround_ = false;
        /* 跳跃真正生效的时刻打一条日志, 便于区分按钮, 双击与键盘三条来源. */
        IM_LOGI(kTag, "jump applied vy=%.2f feet=%.2f", static_cast<double>(velocity_.y),
                static_cast<double>(position_.y));
    }
    jumpRequested_ = false;

    velocity_.y += kGravity * seconds;
    if (velocity_.y < kFallSpeedLimit) {
        velocity_.y = kFallSpeedLimit;
    }

    /* 单帧位移按最大步长切片, 切片后再逐轴分离解算, 位移再大也不会穿墙. */
    const core::Vec3 delta = velocity_ * seconds;
    float longest = std::fabs(delta.x);
    if (std::fabs(delta.y) > longest) {
        longest = std::fabs(delta.y);
    }
    if (std::fabs(delta.z) > longest) {
        longest = std::fabs(delta.z);
    }
    int32_t subSteps = 1;
    if (longest > kMaxSubStep) {
        subSteps = static_cast<int32_t>(std::ceil(longest / kMaxSubStep));
        if (subSteps > kMaxSubSteps) {
            subSteps = kMaxSubSteps;
        }
    }
    const float slice = 1.0f / static_cast<float>(subSteps);

    /* 每帧先假定离地, 垂直受阻时再置回落地状态. */
    onGround_ = false;
    for (int32_t i = 0; i < subSteps; ++i) {
        if (MoveAxis(1, delta.y * slice, world)) {
            /* 向上撞顶或向下落地, 竖直速度均归零. */
            velocity_.y = 0.0f;
        }
        MoveAxis(0, delta.x * slice, world);
        MoveAxis(2, delta.z * slice, world);
    }
}

bool PlayerController::MoveAxis(int32_t axis, float amount, const world::World& world) {
    if (amount == 0.0f) {
        return false;
    }
    const float start = axis == 0 ? position_.x : (axis == 1 ? position_.y : position_.z);
    const float target = start + amount;

    core::Vec3 probe = position_;
    if (axis == 0) {
        probe.x = target;
    } else if (axis == 1) {
        probe.y = target;
    } else {
        probe.z = target;
    }

    /* 推进后的 AABB, 每侧内缩一点, 使恰好贴面的方块不计为重叠. */
    const float minX = probe.x - kHalfWidth + kSurfaceEpsilon;
    const float maxX = probe.x + kHalfWidth - kSurfaceEpsilon;
    const float minY = probe.y + kSurfaceEpsilon;
    const float maxY = probe.y + kHeight - kSurfaceEpsilon;
    const float minZ = probe.z - kHalfWidth + kSurfaceEpsilon;
    const float maxZ = probe.z + kHalfWidth - kSurfaceEpsilon;

    const int32_t x0 = FloorToInt(minX);
    const int32_t x1 = FloorToInt(maxX);
    const int32_t y0 = FloorToInt(minY);
    const int32_t y1 = FloorToInt(maxY);
    const int32_t z0 = FloorToInt(minZ);
    const int32_t z1 = FloorToInt(maxZ);

    const float leading =
        amount > 0.0f ? AxisHalfExtentPos(axis, kHalfWidth, kHeight) : AxisHalfExtentNeg(axis, kHalfWidth);
    bool blocked = false;
    float limit = target;
    for (int32_t y = y0; y <= y1; ++y) {
        for (int32_t z = z0; z <= z1; ++z) {
            for (int32_t x = x0; x <= x1; ++x) {
                if (!IsSolidBlock(world.GetBlock(x, y, z))) {
                    continue;
                }
                const float blockLow = static_cast<float>(axis == 0 ? x : (axis == 1 ? y : z));
                if (amount > 0.0f) {
                    /* 前进侧贴到方块的下侧. */
                    const float candidate = blockLow - kSurfaceEpsilon - leading;
                    if (!blocked || candidate < limit) {
                        limit = candidate;
                    }
                } else {
                    /* 后退侧贴到方块的上侧. */
                    const float candidate = blockLow + 1.0f + kSurfaceEpsilon + leading;
                    if (!blocked || candidate > limit) {
                        limit = candidate;
                    }
                }
                blocked = true;
            }
        }
    }

    if (!blocked) {
        position_ = probe;
        return false;
    }

    /* 已经嵌入方块时留在原位, 不做反向弹开, 避免被推到其他方块里. */
    float resolved = limit;
    if (amount > 0.0f) {
        resolved = limit < start ? start : limit;
    } else {
        resolved = limit > start ? start : limit;
    }
    if (axis == 0) {
        position_.x = resolved;
    } else if (axis == 1) {
        position_.y = resolved;
        if (amount < 0.0f) {
            onGround_ = true;
        }
    } else {
        position_.z = resolved;
    }
    return true;
}

bool PlayerController::CollidesAt(const world::World& world, const core::Vec3& feet) const {
    const float minX = feet.x - kHalfWidth + kSurfaceEpsilon;
    const float maxX = feet.x + kHalfWidth - kSurfaceEpsilon;
    const float minY = feet.y + kSurfaceEpsilon;
    const float maxY = feet.y + kHeight - kSurfaceEpsilon;
    const float minZ = feet.z - kHalfWidth + kSurfaceEpsilon;
    const float maxZ = feet.z + kHalfWidth - kSurfaceEpsilon;

    for (int32_t y = FloorToInt(minY); y <= FloorToInt(maxY); ++y) {
        for (int32_t z = FloorToInt(minZ); z <= FloorToInt(maxZ); ++z) {
            for (int32_t x = FloorToInt(minX); x <= FloorToInt(maxX); ++x) {
                if (IsSolidBlock(world.GetBlock(x, y, z))) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool PlayerController::AabbHitsBlock(const core::Vec3& feet, const voxel::BlockPos& block) const {
    const float maxX = feet.x + kHalfWidth;
    const float minX = feet.x - kHalfWidth;
    const float maxY = feet.y + kHeight;
    const float minY = feet.y;
    const float maxZ = feet.z + kHalfWidth;
    const float minZ = feet.z - kHalfWidth;

    return maxX > static_cast<float>(block.x) && minX < static_cast<float>(block.x + 1) &&
           maxY > static_cast<float>(block.y) && minY < static_cast<float>(block.y + 1) &&
           maxZ > static_cast<float>(block.z) && minZ < static_cast<float>(block.z + 1);
}

bool PlayerController::CastRay(const world::World& world, RayHit& hit) const {
    hit = RayHit{};
    const core::Vec3 origin = EyePosition();
    const core::Vec3 dir = ForwardDirection();

    int32_t x = FloorToInt(origin.x);
    int32_t y = FloorToInt(origin.y);
    int32_t z = FloorToInt(origin.z);
    /* 视点已经处在实心方块内部时放弃拾取, 避免在脚下误放方块. */
    if (IsSolidBlock(world.GetBlock(x, y, z))) {
        return false;
    }

    const int32_t stepX = dir.x > 0.0f ? 1 : (dir.x < 0.0f ? -1 : 0);
    const int32_t stepY = dir.y > 0.0f ? 1 : (dir.y < 0.0f ? -1 : 0);
    const int32_t stepZ = dir.z > 0.0f ? 1 : (dir.z < 0.0f ? -1 : 0);

    constexpr float kInfinity = 1.0e30f;
    /* 每跨越一个方块所需的射线参数增量, 与到下一界面的初值. */
    const float deltaX = stepX != 0 ? 1.0f / std::fabs(dir.x) : kInfinity;
    const float deltaY = stepY != 0 ? 1.0f / std::fabs(dir.y) : kInfinity;
    const float deltaZ = stepZ != 0 ? 1.0f / std::fabs(dir.z) : kInfinity;
    float nextX = stepX > 0 ? (static_cast<float>(x + 1) - origin.x) * deltaX
                           : (stepX < 0 ? (origin.x - static_cast<float>(x)) * deltaX : kInfinity);
    float nextY = stepY > 0 ? (static_cast<float>(y + 1) - origin.y) * deltaY
                           : (stepY < 0 ? (origin.y - static_cast<float>(y)) * deltaY : kInfinity);
    float nextZ = stepZ > 0 ? (static_cast<float>(z + 1) - origin.z) * deltaZ
                           : (stepZ < 0 ? (origin.z - static_cast<float>(z)) * deltaZ : kInfinity);

    for (int32_t step = 0; step < kMaxRaySteps; ++step) {
        /* 选出最先跨越的轴, 记录该轴的进入面法线. */
        float distance = 0.0f;
        voxel::BlockPos face{};
        if (nextX <= nextY && nextX <= nextZ) {
            x += stepX;
            distance = nextX;
            nextX += deltaX;
            face = {-stepX, 0, 0};
        } else if (nextY <= nextZ) {
            y += stepY;
            distance = nextY;
            nextY += deltaY;
            face = {0, -stepY, 0};
        } else {
            z += stepZ;
            distance = nextZ;
            nextZ += deltaZ;
            face = {0, 0, -stepZ};
        }
        if (distance > kReachDistance) {
            return false;
        }
        if (IsSolidBlock(world.GetBlock(x, y, z))) {
            hit.hit = true;
            hit.block = voxel::BlockPos{x, y, z};
            hit.face = face;
            hit.distance = distance;
            return true;
        }
    }
    return false;
}

float PlayerController::HealthRatio() const {
    const float ratio = health_ / kMaxHealth;
    return ratio > 1.0f ? 1.0f : ratio;
}

void PlayerController::ApplyDamage(float amount) {
    if (!(amount > 0.0f) || health_ <= 0.0f || hurtCooldown_ > 0.0f) {
        return;
    }
    health_ -= amount;
    hurtSeconds_ = kHurtSeconds;
    hurtCooldown_ = kHurtCooldown;
    if (health_ > 0.0f) {
        IM_LOGI(kTag, "player hurt amount=%.1f health=%.1f", static_cast<double>(amount),
                static_cast<double>(health_));
        return;
    }
    /* 血量归零: 清空挖掘进度并进入死亡等待, 由 Update 送回出生点. */
    health_ = 0.0f;
    deathSeconds_ = kDeathSeconds;
    CancelMining();
    IM_LOGW(kTag, "player died amount=%.1f", static_cast<double>(amount));
}

void PlayerController::TickHurt(float seconds) {
    if (hurtSeconds_ > 0.0f) {
        hurtSeconds_ -= seconds;
        if (hurtSeconds_ < 0.0f) {
            hurtSeconds_ = 0.0f;
        }
    }
    if (hurtCooldown_ > 0.0f) {
        hurtCooldown_ -= seconds;
        if (hurtCooldown_ < 0.0f) {
            hurtCooldown_ = 0.0f;
        }
    }
}

void PlayerController::Respawn() {
    position_ = spawnPosition_;
    velocity_ = core::Vec3{};
    onGround_ = false;
    jumpRequested_ = false;
    /* 回到出生点即视为脱离险境, 血量与受击状态一并复位. */
    health_ = kMaxHealth;
    hurtSeconds_ = 0.0f;
    hurtCooldown_ = 0.0f;
    deathSeconds_ = 0.0f;
    /* 回到出生点后旧目标格已经无意义, 顺带清掉挖掘进度. */
    CancelMining();
    stickPointerId_ = -1;
    lookPointerId_ = -1;
    stickValueX_ = 0.0f;
    stickValueY_ = 0.0f;
}

core::Vec3 PlayerController::EyePosition() const {
    return core::Vec3{position_.x, position_.y + kEyeHeight, position_.z};
}

core::Vec3 PlayerController::ForwardDirection() const {
    const float cosPitch = std::cos(pitch_);
    return core::Vec3{-std::sin(yaw_) * cosPitch, std::sin(pitch_), -std::cos(yaw_) * cosPitch};
}

void PlayerController::SetPose(const core::Vec3& position, float yaw, float pitch) {
    position_ = position;
    spawnPosition_ = position;
    velocity_ = core::Vec3{};
    yaw_ = yaw;
    pitch_ = pitch;
    onGround_ = false;
    spawned_ = true;
    spawnPending_ = false;
    /* 读档落位视为满状态复活, 不把上一份存档的受伤状态带进来. */
    health_ = kMaxHealth;
    hurtSeconds_ = 0.0f;
    hurtCooldown_ = 0.0f;
    deathSeconds_ = 0.0f;
}

}  /* namespace immirror::player */
