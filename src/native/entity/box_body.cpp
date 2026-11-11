#include "entity/box_body.h"

#include <cmath>

#include "entity/entity_tuning.h"
#include "voxel/block_registry.h"
#include "voxel/voxel_types.h"
#include "world/world.h"

namespace immirror::entity {
namespace {

int32_t FloorToInt(float value) {
    return static_cast<int32_t>(std::floor(value));
}

/*
 * 实心判定与玩家控制器走同一张注册表, 水等非实心方块不参与阻挡与落地判定,
 * 因此生物可以涉水而不会浮在水面上.
 */
bool IsSolidBlock(voxel::BlockId id) {
    return voxel::BlockRegistry::Instance().Info(id).solid;
}

/* 沿单轴判定所需的半长: 竖直轴向上取整个高度, 向下取 0, 水平轴取半宽. */
float AxisHalfExtentPos(const BoxBody& body, int32_t axis) {
    return axis == 1 ? body.height : body.halfWidth;
}

float AxisHalfExtentNeg(const BoxBody& body, int32_t axis) {
    return axis == 1 ? 0.0f : body.halfWidth;
}

}  /* namespace */

bool MoveBoxAxis(BoxBody& body, int32_t axis, float amount, const world::World& world) {
    if (amount == 0.0f) {
        return false;
    }
    const float start = axis == 0 ? body.position.x : (axis == 1 ? body.position.y : body.position.z);
    const float target = start + amount;

    core::Vec3 probe = body.position;
    if (axis == 0) {
        probe.x = target;
    } else if (axis == 1) {
        probe.y = target;
    } else {
        probe.z = target;
    }

    /* 推进后的包围盒, 每侧内缩一点, 使恰好贴面的方块不计为重叠. */
    const float minX = probe.x - body.halfWidth + tuning::kSurfaceEpsilon;
    const float maxX = probe.x + body.halfWidth - tuning::kSurfaceEpsilon;
    const float minY = probe.y + tuning::kSurfaceEpsilon;
    const float maxY = probe.y + body.height - tuning::kSurfaceEpsilon;
    const float minZ = probe.z - body.halfWidth + tuning::kSurfaceEpsilon;
    const float maxZ = probe.z + body.halfWidth - tuning::kSurfaceEpsilon;

    const int32_t x0 = FloorToInt(minX);
    const int32_t x1 = FloorToInt(maxX);
    const int32_t y0 = FloorToInt(minY);
    const int32_t y1 = FloorToInt(maxY);
    const int32_t z0 = FloorToInt(minZ);
    const int32_t z1 = FloorToInt(maxZ);

    const float leading = amount > 0.0f ? AxisHalfExtentPos(body, axis) : AxisHalfExtentNeg(body, axis);
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
                    const float candidate = blockLow - tuning::kSurfaceEpsilon - leading;
                    if (!blocked || candidate < limit) {
                        limit = candidate;
                    }
                } else {
                    /* 后退侧贴到方块的上侧. */
                    const float candidate = blockLow + 1.0f + tuning::kSurfaceEpsilon + leading;
                    if (!blocked || candidate > limit) {
                        limit = candidate;
                    }
                }
                blocked = true;
            }
        }
    }

    if (!blocked) {
        body.position = probe;
        return false;
    }

    /* 已经嵌入方块时留在原位, 不做反向弹开, 避免被推到其他方块里. */
    const float resolved = amount > 0.0f ? (limit < start ? start : limit)
                                         : (limit > start ? start : limit);
    if (axis == 0) {
        body.position.x = resolved;
    } else if (axis == 1) {
        body.position.y = resolved;
        if (amount < 0.0f) {
            body.onGround = true;
        }
    } else {
        body.position.z = resolved;
    }
    return true;
}

BoxStepResult StepBox(BoxBody& body, float seconds, const world::World& world) {
    BoxStepResult result;
    body.velocity.y += tuning::kGravity * seconds;
    if (body.velocity.y < tuning::kMaxFallSpeed) {
        body.velocity.y = tuning::kMaxFallSpeed;
    }

    /* 单帧位移按最大步长切片, 切片后再逐轴分离解算, 位移再大也不会穿墙. */
    const core::Vec3 delta = body.velocity * seconds;
    float longest = std::fabs(delta.x);
    if (std::fabs(delta.y) > longest) {
        longest = std::fabs(delta.y);
    }
    if (std::fabs(delta.z) > longest) {
        longest = std::fabs(delta.z);
    }
    int32_t subSteps = 1;
    if (longest > tuning::kMaxSubStep) {
        subSteps = static_cast<int32_t>(std::ceil(longest / tuning::kMaxSubStep));
        if (subSteps > tuning::kMaxSubSteps) {
            subSteps = tuning::kMaxSubSteps;
        }
    }
    const float slice = 1.0f / static_cast<float>(subSteps);

    /* 每帧先假定离地, 竖直向下受阻时再置回落地状态. */
    body.onGround = false;
    for (int32_t i = 0; i < subSteps; ++i) {
        if (MoveBoxAxis(body, 1, delta.y * slice, world)) {
            /* 向上撞顶或向下落地, 竖直速度均归零. */
            body.velocity.y = 0.0f;
            result.blockedY = true;
        }
        if (MoveBoxAxis(body, 0, delta.x * slice, world)) {
            result.blockedX = true;
        }
        if (MoveBoxAxis(body, 2, delta.z * slice, world)) {
            result.blockedZ = true;
        }
    }
    return result;
}

bool BoxCollidesAt(const BoxBody& body, const core::Vec3& feet, const world::World& world) {
    const float minX = feet.x - body.halfWidth + tuning::kSurfaceEpsilon;
    const float maxX = feet.x + body.halfWidth - tuning::kSurfaceEpsilon;
    const float minY = feet.y + tuning::kSurfaceEpsilon;
    const float maxY = feet.y + body.height - tuning::kSurfaceEpsilon;
    const float minZ = feet.z - body.halfWidth + tuning::kSurfaceEpsilon;
    const float maxZ = feet.z + body.halfWidth - tuning::kSurfaceEpsilon;

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

bool LiftBoxOutOfSolid(BoxBody& body, const world::World& world) {
    for (int32_t guard = 0; guard <= voxel::kWorldHeight; ++guard) {
        if (!BoxCollidesAt(body, body.position, world)) {
            return true;
        }
        body.position.y += 1.0f;
    }
    return false;
}

}  /* namespace immirror::entity */
