#pragma once

#include "core/vec3.h"
#include "platform/input_state.h"
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
 * 第一人称玩家控制. 负责输入到运动的映射, 与体素世界的碰撞,
 * 以及视线射线拾取. 该对象只在原生主线程上使用.
 */
class PlayerController {
public:
    void Spawn(const world::World& world, float x, float z);

    void Update(double dt, const platform::InputSnapshot& input, const world::World& world);

    /* 列主序 4x4 视图矩阵. */
    void GetViewMatrix(float* out16) const;

    /* 返回脚底中心的世界坐标, 相机视点在此之上 kEyeHeight 处. */
    core::Vec3 Position() const;

    /* 视线单位方向. */
    core::Vec3 LookDirection() const;

    /* 破坏视线命中的方块; 返回是否命中并修改了世界. */
    bool BreakBlock(world::World& world);

    /* 在命中面的外侧放置方块. */
    bool PlaceBlock(world::World& world, voxel::BlockId id);

private:
    /* 触摸手势归属, 由落点所在的半屏决定. */
    enum class Gesture {
        kNone = 0,
        kStick = 1,
        kLook = 2,
    };

    /* 把输入快照翻译成摇杆量, 视角增量与跳跃请求. */
    void ProcessInput(double dt, const platform::InputSnapshot& input);

    /* 应用一次视角拖动增量, 俯仰角在此裁剪到正负 89 度. */
    void UpdateLookDelta(float deltaX, float deltaY);

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

    /* 回到出生点并清空运动状态. */
    void Respawn();

    core::Vec3 EyePosition() const;
    core::Vec3 ForwardDirection() const;

    core::Vec3 position_{0.0f, 0.0f, 0.0f};  /* 脚底中心 */
    core::Vec3 velocity_{0.0f, 0.0f, 0.0f};
    core::Vec3 spawnPosition_{0.0f, 0.0f, 0.0f};
    float yaw_ = 0.0f;    /* 偏航角, 弧度, 0 表示朝向 -Z */
    float pitch_ = 0.0f;  /* 俯仰角, 弧度, 正值抬头 */
    bool onGround_ = false;
    bool spawned_ = false;
    bool spawnPending_ = false;      /* 出生列尚无区块数据, 等待地形生成后落位 */
    float pendingSeconds_ = 0.0f;    /* 已等待落位的秒数, 用于超时保护 */

    /* 触摸手势状态, 逐帧由输入快照驱动. */
    Gesture gesture_ = Gesture::kNone;
    bool pointerWasDown_ = false;
    float anchorX_ = 0.0f;         /* 摇杆锚点, 归一化屏幕坐标 */
    float anchorY_ = 0.0f;
    float lastPointerX_ = 0.0f;    /* 上一帧上报的触点位置 */
    float lastPointerY_ = 0.0f;
    int32_t lastPointerCount_ = 0; /* 上一帧的触点数量, 用于识别指针切换 */
    float stickX_ = 0.0f;          /* 摇杆水平量, 向右为正 */
    float stickY_ = 0.0f;          /* 摇杆前向量, 向前为正 */
    float gestureSeconds_ = 0.0f;  /* 当前手势已持续的秒数 */
    float gestureTravel_ = 0.0f;   /* 手势累计位移, 用于区分点击与拖动 */
    float doubleTapTimer_ = -1.0f; /* 双击窗口剩余秒数, 负值表示未开启 */
    bool jumpRequested_ = false;
    int64_t lastKeyEvents_ = 0;    /* 已处理的累计按键事件数 */
};

}  /* namespace immirror::player */
