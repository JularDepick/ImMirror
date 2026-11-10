#include "player/player_controller.h"

#include <cmath>
#include <cstdint>

#include "core/vec3.h"
#include "voxel/block_registry.h"
#include "world/world.h"

namespace immirror::player {
namespace {

/* 角度与无量纲常量. */
constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 2.0f * kPi;
constexpr float kDegreesToRadians = kPi / 180.0f;

/* 运动参数, 单位为米与秒. */
constexpr float kGravity = -24.0f;            /* 重力加速度 */
constexpr float kMoveSpeed = 4.5f;            /* 水平移动速度 */
constexpr float kJumpSpeed = 8.0f;            /* 起跳初速度 */
constexpr float kFallSpeedLimit = -58.0f;     /* 下落速度上限, 控制单帧位移量 */

/* 玩家碰撞盒尺寸, 全部相对脚底中心. */
constexpr float kPlayerHalfWidth = 0.3f;
constexpr float kPlayerHeight = 1.8f;
constexpr float kEyeHeight = 1.62f;

/* 相机与拾取参数. */
constexpr float kMaxPitch = 89.0f * kDegreesToRadians;  /* 俯仰限制 */
constexpr float kReachDistance = 6.0f;                  /* 射线最长距离 */
constexpr int32_t kMaxRaySteps = 96;                    /* DDA 步数上限, 防御异常方向 */

/* 触摸映射参数, 屏幕坐标均为归一化值. */
constexpr float kHalfScreenSplit = 0.5f;              /* 左右半屏分界 */
constexpr float kStickRadius = 0.12f;                 /* 摇杆满量程半径 */
constexpr float kLookDegreesPerWidth = 150.0f;        /* 横向拖满屏宽的转角 */
constexpr float kLookDegreesPerHeight = 120.0f;       /* 纵向拖满屏高的转角 */
constexpr float kPointerJumpThreshold = 0.35f;        /* 指针跳变阈值, 超过则重锚 */
constexpr float kTapMaxSeconds = 0.22f;               /* 判定点击的最长按压时长 */
constexpr float kTapMaxTravel = 0.02f;                /* 判定点击的最大累计位移 */
constexpr float kDoubleTapWindow = 0.35f;             /* 双击时间窗 */

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

/* 沿单轴判定所需的 AABB 半宽与半高. */
float AxisHalfExtentPos(int32_t axis) {
    return axis == 1 ? kPlayerHeight : kPlayerHalfWidth;
}

float AxisHalfExtentNeg(int32_t axis) {
    return axis == 1 ? 0.0f : kPlayerHalfWidth;
}

/*
 * 实心判定统一走方块注册表, 水等非实心方块不参与水平阻挡与落地判定.
 * 碰撞与放置的所有方块查询都经过本函数, 属性口径只有这一处.
 */
bool IsSolidBlock(voxel::BlockId id) {
    return voxel::BlockRegistry::Instance().Info(id).solid;
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

void PlayerController::Update(double dt, const platform::InputSnapshot& input, const world::World& world) {
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

    ProcessInput(clamped, input);
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

void PlayerController::ProcessInput(double dt, const platform::InputSnapshot& input) {
    const float seconds = static_cast<float>(dt);

    if (doubleTapTimer_ > 0.0f) {
        doubleTapTimer_ -= seconds;
        if (doubleTapTimer_ < 0.0f) {
            doubleTapTimer_ = -1.0f;
        }
    }
    if (gesture_ != Gesture::kNone) {
        gestureSeconds_ += seconds;
    }

    /* 键盘调试通道: 按键请求与双击共用同一条跳跃请求, 跳跃时再判是否落地. */
    if (input.keyEvents != lastKeyEvents_) {
        const int32_t code = input.lastKeyCode;
        if (code == kKeySpace || code == kKeyEnter || code == kKeyCenter || code == kKeyButtonA) {
            jumpRequested_ = true;
        }
        lastKeyEvents_ = input.keyEvents;
    }

    /*
     * 输入快照只上报单指针流: pointerCount 反映触点总数, 位置取其中一个触点.
     * 因此手势以首次落点的半屏位置归属, 左右同时按下时以先落下的手指为准.
     */
    const bool down = input.pointerDown && input.pointerCount > 0;
    const float pointerX = input.pointerX;
    const float pointerY = input.pointerY;

    if (down && !pointerWasDown_) {
        /* 新手势: 左半屏进入摇杆, 右半屏进入视角拖动. */
        gesture_ = pointerX < kHalfScreenSplit ? Gesture::kStick : Gesture::kLook;
        anchorX_ = pointerX;
        anchorY_ = pointerY;
        lastPointerX_ = pointerX;
        lastPointerY_ = pointerY;
        gestureSeconds_ = 0.0f;
        gestureTravel_ = 0.0f;
        stickX_ = 0.0f;
        stickY_ = 0.0f;
    } else if (down && pointerWasDown_ && gesture_ != Gesture::kNone) {
        float deltaX = pointerX - lastPointerX_;
        float deltaY = pointerY - lastPointerY_;

        /*
         * 触点数变化意味着快照里的指针可能换了一根, 位置会跳到另一根手指上;
         * 单帧位移异常大时按同样方式处理. 两种情况都重设锚点而不是产生突兀转头,
         * 也不让摇杆瞬间顶到满量程.
         */
        const bool pointerSwitched = input.pointerCount != lastPointerCount_;
        const bool jumpedTooFar = std::fabs(deltaX) > kPointerJumpThreshold ||
                                  std::fabs(deltaY) > kPointerJumpThreshold;
        if (pointerSwitched || jumpedTooFar) {
            anchorX_ = pointerX;
            anchorY_ = pointerY;
            stickX_ = 0.0f;
            stickY_ = 0.0f;
            deltaX = 0.0f;
            deltaY = 0.0f;
        }
        gestureTravel_ += std::fabs(deltaX) + std::fabs(deltaY);

        if (gesture_ == Gesture::kStick) {
            /* 摇杆量取锚点到当前触点的位移, 并裁剪进单位圆, 斜向不加速. */
            float stickRight = (pointerX - anchorX_) / kStickRadius;
            float stickForward = -(pointerY - anchorY_) / kStickRadius;
            const float length = std::sqrt(stickRight * stickRight + stickForward * stickForward);
            if (length > 1.0f) {
                stickRight /= length;
                stickForward /= length;
            }
            stickX_ = stickRight;
            stickY_ = stickForward;
        } else {
            UpdateLookDelta(deltaX, deltaY);
        }
        lastPointerX_ = pointerX;
        lastPointerY_ = pointerY;
    } else if (!down && pointerWasDown_) {
        /* 抬手: 左半屏的短促点击计入双击窗口, 第二次点击触发跳跃. */
        if (gesture_ == Gesture::kStick && gestureSeconds_ <= kTapMaxSeconds &&
            gestureTravel_ <= kTapMaxTravel) {
            if (doubleTapTimer_ > 0.0f) {
                jumpRequested_ = true;
                doubleTapTimer_ = -1.0f;
            } else {
                doubleTapTimer_ = kDoubleTapWindow;
            }
        }
        gesture_ = Gesture::kNone;
        stickX_ = 0.0f;
        stickY_ = 0.0f;
    } else if (!down) {
        /* 无触点时清空摇杆, 松开手指立刻停止水平移动. */
        gesture_ = Gesture::kNone;
        stickX_ = 0.0f;
        stickY_ = 0.0f;
    }

    pointerWasDown_ = down;
    lastPointerCount_ = input.pointerCount;
}

void PlayerController::UpdateLookDelta(float deltaX, float deltaY) {
    /* 右拖右转, 下拖低头; 角度按屏幕尺寸比例换算, 满屏宽度约 150 度. */
    yaw_ -= deltaX * (kLookDegreesPerWidth * kDegreesToRadians);
    pitch_ -= deltaY * (kLookDegreesPerHeight * kDegreesToRadians);
    pitch_ = ClampFloat(pitch_, -kMaxPitch, kMaxPitch);
    /* 偏航角回绕到正负 pi, 长时间旋转也不会累积浮点误差. */
    while (yaw_ > kPi) {
        yaw_ -= kTwoPi;
    }
    while (yaw_ < -kPi) {
        yaw_ += kTwoPi;
    }
}

void PlayerController::StepMotion(double dt, const world::World& world) {
    const float seconds = static_cast<float>(dt);

    /* 水平方向: 摇杆量投影到水平前向与右向, 模长不超过 1. */
    const float cosYaw = std::cos(yaw_);
    const float sinYaw = std::sin(yaw_);
    const core::Vec3 forward{-sinYaw, 0.0f, -cosYaw};
    const core::Vec3 right{cosYaw, 0.0f, -sinYaw};
    core::Vec3 wish = forward * stickY_ + right * stickX_;
    const float wishLength = wish.Length();
    if (wishLength > 1.0f) {
        wish = wish * (1.0f / wishLength);
    }
    velocity_.x = wish.x * kMoveSpeed;
    velocity_.z = wish.z * kMoveSpeed;

    if (jumpRequested_ && onGround_) {
        velocity_.y = kJumpSpeed;
        onGround_ = false;
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
    const float minX = probe.x - kPlayerHalfWidth + kSurfaceEpsilon;
    const float maxX = probe.x + kPlayerHalfWidth - kSurfaceEpsilon;
    const float minY = probe.y + kSurfaceEpsilon;
    const float maxY = probe.y + kPlayerHeight - kSurfaceEpsilon;
    const float minZ = probe.z - kPlayerHalfWidth + kSurfaceEpsilon;
    const float maxZ = probe.z + kPlayerHalfWidth - kSurfaceEpsilon;

    const int32_t x0 = FloorToInt(minX);
    const int32_t x1 = FloorToInt(maxX);
    const int32_t y0 = FloorToInt(minY);
    const int32_t y1 = FloorToInt(maxY);
    const int32_t z0 = FloorToInt(minZ);
    const int32_t z1 = FloorToInt(maxZ);

    const float leading = amount > 0.0f ? AxisHalfExtentPos(axis) : AxisHalfExtentNeg(axis);
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
    const float minX = feet.x - kPlayerHalfWidth + kSurfaceEpsilon;
    const float maxX = feet.x + kPlayerHalfWidth - kSurfaceEpsilon;
    const float minY = feet.y + kSurfaceEpsilon;
    const float maxY = feet.y + kPlayerHeight - kSurfaceEpsilon;
    const float minZ = feet.z - kPlayerHalfWidth + kSurfaceEpsilon;
    const float maxZ = feet.z + kPlayerHalfWidth - kSurfaceEpsilon;

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
    const float maxX = feet.x + kPlayerHalfWidth;
    const float minX = feet.x - kPlayerHalfWidth;
    const float maxY = feet.y + kPlayerHeight;
    const float minY = feet.y;
    const float maxZ = feet.z + kPlayerHalfWidth;
    const float minZ = feet.z - kPlayerHalfWidth;

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

void PlayerController::Respawn() {
    position_ = spawnPosition_;
    velocity_ = core::Vec3{};
    onGround_ = false;
    jumpRequested_ = false;
    stickX_ = 0.0f;
    stickY_ = 0.0f;
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
}

}  /* namespace immirror::player */
