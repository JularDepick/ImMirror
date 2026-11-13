#include "entity/projectile.h"

#include <cmath>

namespace immirror::entity {
namespace {

/* 重力与寿命: 箭按真实抛物线落下, 八秒后自动回收. */
constexpr float kGravity = -18.0f;
constexpr float kMaxAge = 8.0f;
/* 单步最大推进长度, 比最薄方块还短, 因此不会穿墙. */
constexpr float kStepLength = 0.25f;
/* 命中生物的判定半径. */
constexpr float kCreatureRadius = 0.6f;

}  /* namespace */

void ProjectileSystem::Clear() {
    items_.clear();
}

uint32_t ProjectileSystem::Spawn(const core::Vec3& origin, const core::Vec3& direction,
                                 float speed, float damage) {
    Projectile projectile;
    projectile.id = nextId_++;
    projectile.position = origin;
    projectile.velocity = direction * speed;
    projectile.damage = damage;
    items_.push_back(projectile);
    return projectile.id;
}

void ProjectileSystem::Update(float seconds, void* context, SolidAt solidAt,
                              CreatureAt creatureAt, std::vector<ProjectileHit>& hits) {
    if (seconds <= 0.0f) {
        return;
    }
    size_t index = 0;
    while (index < items_.size()) {
        Projectile& projectile = items_[index];
        projectile.age += seconds;
        projectile.velocity.y += kGravity * seconds;

        const float stepLength = projectile.velocity.Length() * seconds;
        const int32_t steps =
            stepLength <= kStepLength ? 1 : static_cast<int32_t>(stepLength / kStepLength) + 1;
        const float slice = seconds / static_cast<float>(steps);
        bool consumed = false;
        for (int32_t step = 0; step < steps && !consumed; ++step) {
            projectile.position = projectile.position + projectile.velocity * slice;
            /* 先判生物: 生物比方块先被箭碰到时应当命中生物. */
            const uint32_t creatureId =
                creatureAt != nullptr ? creatureAt(context, projectile.position, kCreatureRadius) : 0;
            if (creatureId != 0) {
                ProjectileHit hit;
                hit.kind = ProjectileHit::Kind::kCreature;
                hit.creatureId = creatureId;
                hit.position = projectile.position;
                hit.damage = projectile.damage;
                hits.push_back(hit);
                consumed = true;
                break;
            }
            if (solidAt != nullptr &&
                solidAt(context, static_cast<int32_t>(std::floor(projectile.position.x)),
                        static_cast<int32_t>(std::floor(projectile.position.y)),
                        static_cast<int32_t>(std::floor(projectile.position.z)))) {
                ProjectileHit hit;
                hit.kind = ProjectileHit::Kind::kBlock;
                hit.position = projectile.position;
                hit.damage = projectile.damage;
                hits.push_back(hit);
                consumed = true;
                break;
            }
        }

        if (consumed || projectile.age >= kMaxAge || projectile.position.y < 0.0f) {
            items_.erase(items_.begin() + static_cast<long>(index));
            continue;
        }
        index += 1;
    }
}

}  /* namespace immirror::entity */
