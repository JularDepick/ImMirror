#pragma once

#include "core/vec3.h"
#include "player/input_tuning.h"
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
 * 第一人称玩家控制. 负责输入到运动的映射, 与体素世界的碰撞,
 * 以及视线射线拾取. 该对象只在原生主线程上使用.
 */
class PlayerController {
public:
    void Spawn(const world::World& world, float x, float z);

    /*
     * 输入快照按引用传入而不是只读: 控制器消费完累计位移后会清零,
     * 保证同一段位移不会被两帧重复计入.
     */
    void Update(double dt, platform::InputSnapshot& input, const world::World& world);

    /* 列主序 4x4 视图矩阵. */
    void GetViewMatrix(float* out16) const;

    /* 返回脚底中心的世界坐标, 相机视点在此之上 kEyeHeight 处. */
    core::Vec3 Position() const;

    /* 视线单位方向. */
    core::Vec3 LookDirection() const;

    /* 当前朝向, 读档与存档时需要. */
    float Yaw() const { return yaw_; }
    float Pitch() const { return pitch_; }

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

    /* 在命中面的外侧放置方块. */
    bool PlaceBlock(world::World& world, voxel::BlockId id);

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
};

}  /* namespace immirror::player */
