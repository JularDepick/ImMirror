#include "entity/entity.h"

#include <cmath>

namespace immirror::entity {
namespace {

/* 平行于某个轴时该轴的进出参数视为无穷, 由另外两轴决定交点. */
constexpr float kRayInfinity = 1.0e30f;

}  /* namespace */

Entity::Entity(EntityId id, EntityKind kind) : id_(id), kind_(kind) {}

void Entity::Bounds(core::Vec3& minCorner, core::Vec3& maxCorner) const {
    const core::Vec3 feet = Position();
    const float halfWidth = BoundsHalfWidth();
    const float height = BoundsHeight();
    minCorner = core::Vec3{feet.x - halfWidth, feet.y, feet.z - halfWidth};
    maxCorner = core::Vec3{feet.x + halfWidth, feet.y + height, feet.z + halfWidth};
}

float RayHitsBox(const core::Vec3& origin, const core::Vec3& direction,
                 const core::Vec3& minCorner, const core::Vec3& maxCorner) {
    float enter = 0.0f;
    float exit = kRayInfinity;
    const float originAxis[3] = {origin.x, origin.y, origin.z};
    const float directionAxis[3] = {direction.x, direction.y, direction.z};
    const float minAxis[3] = {minCorner.x, minCorner.y, minCorner.z};
    const float maxAxis[3] = {maxCorner.x, maxCorner.y, maxCorner.z};

    for (int32_t axis = 0; axis < 3; ++axis) {
        const float dir = directionAxis[axis];
        const float start = originAxis[axis];
        if (std::fabs(dir) < 1.0e-6f) {
            /* 射线平行于该轴: 起点不在区间内就不可能相交. */
            if (start < minAxis[axis] || start > maxAxis[axis]) {
                return -1.0f;
            }
            continue;
        }
        float near = (minAxis[axis] - start) / dir;
        float far = (maxAxis[axis] - start) / dir;
        if (near > far) {
            const float swap = near;
            near = far;
            far = swap;
        }
        if (near > enter) {
            enter = near;
        }
        if (far < exit) {
            exit = far;
        }
        if (enter > exit) {
            return -1.0f;
        }
    }
    return exit < 0.0f ? -1.0f : enter;
}

}  /* namespace immirror::entity */
