#include "entity/sight.h"

#include <cmath>
#include <cstdint>

#include "voxel/block_registry.h"
#include "world/world.h"

namespace immirror::entity {
namespace {

/* 步进上限. 视线最长取视距量级, 单格一米, 上限只需覆盖异常输入. */
constexpr int32_t kMaxSteps = 512;
constexpr float kInfinity = 1.0e30f;

int32_t FloorToInt(float value) {
    return static_cast<int32_t>(std::floor(value));
}

bool IsSolidBlock(const world::World& world, int32_t x, int32_t y, int32_t z) {
    return voxel::BlockRegistry::Instance().Info(world.GetBlock(x, y, z)).solid;
}

}  /* namespace */

float RaySolidDistance(const world::World& world, const core::Vec3& origin,
                       const core::Vec3& direction, float maxDistance) {
    if (!(maxDistance > 0.0f)) {
        return -1.0f;
    }

    int32_t x = FloorToInt(origin.x);
    int32_t y = FloorToInt(origin.y);
    int32_t z = FloorToInt(origin.z);

    /* 起点已经在实心方块里: 整个视线被这一点挡住, 距离按 0 处理. */
    if (IsSolidBlock(world, x, y, z)) {
        return 0.0f;
    }

    const int32_t stepX = direction.x > 0.0f ? 1 : (direction.x < 0.0f ? -1 : 0);
    const int32_t stepY = direction.y > 0.0f ? 1 : (direction.y < 0.0f ? -1 : 0);
    const int32_t stepZ = direction.z > 0.0f ? 1 : (direction.z < 0.0f ? -1 : 0);

    /* 每跨越一格所需的射线参数增量, 以及到下一个边界的初值. */
    const float deltaX = stepX != 0 ? 1.0f / std::fabs(direction.x) : kInfinity;
    const float deltaY = stepY != 0 ? 1.0f / std::fabs(direction.y) : kInfinity;
    const float deltaZ = stepZ != 0 ? 1.0f / std::fabs(direction.z) : kInfinity;

    float nextX = stepX > 0 ? (static_cast<float>(x + 1) - origin.x) * deltaX
                            : (stepX < 0 ? (origin.x - static_cast<float>(x)) * deltaX : kInfinity);
    float nextY = stepY > 0 ? (static_cast<float>(y + 1) - origin.y) * deltaY
                            : (stepY < 0 ? (origin.y - static_cast<float>(y)) * deltaY : kInfinity);
    float nextZ = stepZ > 0 ? (static_cast<float>(z + 1) - origin.z) * deltaZ
                            : (stepZ < 0 ? (origin.z - static_cast<float>(z)) * deltaZ : kInfinity);

    for (int32_t step = 0; step < kMaxSteps; ++step) {
        /* 选出最先跨越的轴, 该轴的参数值就是进入下一个格子的距离. */
        float distance = 0.0f;
        if (nextX <= nextY && nextX <= nextZ) {
            distance = nextX;
            x += stepX;
            nextX += deltaX;
        } else if (nextY <= nextZ) {
            distance = nextY;
            y += stepY;
            nextY += deltaY;
        } else {
            distance = nextZ;
            z += stepZ;
            nextZ += deltaZ;
        }
        if (distance > maxDistance) {
            return -1.0f;
        }
        if (IsSolidBlock(world, x, y, z)) {
            return distance;
        }
    }
    return -1.0f;
}

bool LineOfSight(const world::World& world, const core::Vec3& from, const core::Vec3& to) {
    const core::Vec3 offset = to - from;
    const float distance = offset.Length();
    if (distance <= 1.0e-4f) {
        return true;
    }
    /* 端点本身所在的格子不参与判定: 目标与视点都可能贴着方块站着. */
    const float solid = RaySolidDistance(world, from, offset * (1.0f / distance),
                                         distance - 1.0e-3f);
    return solid < 0.0f;
}

}  /* namespace immirror::entity */
