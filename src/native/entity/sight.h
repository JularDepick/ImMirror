#pragma once

#include "core/vec3.h"

namespace immirror::world {
class World;
}

namespace immirror::entity {

/*
 * 体素视线查询.
 *
 * 生物出手与玩家攻击都需要判断"视线中间有没有实心方块挡着". 全工程只在本文件
 * 实现一份判定, 各处不再各写一套采样步长: 生成表里还有十余米攻击距离的远程生物,
 * 采样步长一旦比墙厚就会漏判, 造成隔墙命中.
 * 水与空气不算遮挡, 与碰撞的实心口径一致, 全部经过方块注册表查询.
 */

/*
 * 从 origin 沿 direction 到第一个实心方块的距离, 单位米.
 * 全程无遮挡, 或 maxDistance 内没有方块时返回 -1; origin 已在实心方块内时返回 0.
 * direction 必须是单位向量. 内部按 DDA 逐格跨越方块边界, 不会跳过薄墙,
 * 返回的是射线进入该格时的距离, 与浮点采样无关.
 */
float RaySolidDistance(const world::World& world, const core::Vec3& origin,
                       const core::Vec3& direction, float maxDistance);

/*
 * 两点之间是否没有实心方块遮挡, 供远程生物出手前的视野判定使用.
 * from 与 to 重合时视为通视.
 */
bool LineOfSight(const world::World& world, const core::Vec3& from, const core::Vec3& to);

}  /* namespace immirror::entity */
