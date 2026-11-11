#pragma once

#include <cstdint>

#include "core/vec3.h"

namespace immirror::world {
class World;
}

namespace immirror::entity {

/*
 * 轴对齐碰撞体. 做法与玩家控制器一致: 位置取脚底中心, 由半宽与高度定盒,
 * 逐轴分离推进, 单帧位移按最大步长切片, 推进时每侧内缩一点,
 * 恰好贴面的方块不算重叠.
 * 生物与后续所有可移动实体共用本实现; 玩家控制器也可以迁移到这一份,
 * 迁移前两边是同一套算法各自持有.
 */
struct BoxBody {
    core::Vec3 position{};  /* 脚底中心 */
    core::Vec3 velocity{};
    float halfWidth = 0.3f;
    float height = 1.8f;
    bool onGround = false;
};

/* 一次推进的受阻情况, 供 AI 判断是否被墙或台阶挡住. */
struct BoxStepResult {
    bool blockedX = false;
    bool blockedZ = false;
    bool blockedY = false;
};

/* 沿单轴推进 amount, 遇到实心方块贴面停下; 返回是否受阻. */
bool MoveBoxAxis(BoxBody& body, int32_t axis, float amount, const world::World& world);

/*
 * 重力积分与分步碰撞推进. seconds 由调用方先裁剪到单帧上限,
 * 水平速度由调用方在调用前写好, 竖直速度由本函数累加重力.
 */
BoxStepResult StepBox(BoxBody& body, float seconds, const world::World& world);

/* 盒体放在给定脚底位置时是否与实心方块重叠. */
bool BoxCollidesAt(const BoxBody& body, const core::Vec3& feet, const world::World& world);

/* 逐格上抬直到不再与实心方块重叠; 抬到世界顶部仍重叠时返回 false. */
bool LiftBoxOutOfSolid(BoxBody& body, const world::World& world);

}  /* namespace immirror::entity */
