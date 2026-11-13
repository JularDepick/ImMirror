/* 飞行物的离线断言: 推进, 重力, 方块命中, 生物命中与超时回收. */
#include <cmath>
#include <cstdio>
#include <vector>

#include "entity/projectile.h"

using namespace immirror::entity;
namespace core = immirror::core;

static int failures = 0;

static void check(const char* label, bool ok) {
    std::printf(ok ? "ok   %s\n" : "FAIL %s\n", label);
    if (!ok) {
        failures += 1;
    }
}

static void near(const char* label, float got, float want, float tolerance) {
    if (std::fabs(got - want) > tolerance) {
        std::printf("FAIL %-22s got %.3f want %.3f\n", label, got, want);
        failures += 1;
    } else {
        std::printf("ok   %-22s %.3f\n", label, got);
    }
}

struct Context {
    float solidZ = 1.0e9f;   /* 该 z 之后的方块视为实心 */
    float creatureX = 1.0e9f; /* 距该 x 以内视为命中生物 */
};

static bool SolidAt(void* context, int32_t x, int32_t y, int32_t z) {
    (void)x;
    (void)y;
    auto* ctx = static_cast<Context*>(context);
    return static_cast<float>(z) >= ctx->solidZ;
}

static uint32_t CreatureAt(void* context, const immirror::core::Vec3& point, float radius) {
    auto* ctx = static_cast<Context*>(context);
    if (point.z >= ctx->creatureX && point.z <= ctx->creatureX + radius) {
        return 77u;
    }
    return 0u;
}

int main() {
    /* 自由飞行: 位置前进, 竖直速度受重力变小. */
    {
        ProjectileSystem system;
        system.Spawn(core::Vec3{0.0f, 10.0f, 0.0f}, core::Vec3{0.0f, 0.0f, 1.0f}, 20.0f, 12.0f);
        check("发射后有一发", system.Count() == 1);
        Context context;
        std::vector<ProjectileHit> hits;
        system.Update(0.1f, &context, SolidAt, CreatureAt, hits);
        check("无碰撞不产生命中", hits.empty());
        check("仍在飞行", system.Count() == 1);
        near("前进了两米", system.At(0).position.z, 2.0f, 0.2f);
        check("重力使高度下降", system.At(0).position.y < 10.0f);
    }
    /* 方块命中: 前方有实心方块时应当停下并记一次命中. */
    {
        ProjectileSystem system;
        system.Spawn(core::Vec3{0.0f, 10.0f, 0.0f}, core::Vec3{0.0f, 0.0f, 1.0f}, 30.0f, 9.0f);
        Context context;
        context.solidZ = 1.0f;
        std::vector<ProjectileHit> hits;
        system.Update(0.2f, &context, SolidAt, CreatureAt, hits);
        check("命中方块", hits.size() == 1 && hits[0].kind == ProjectileHit::Kind::kBlock);
        check("方块命中带走伤害值", !hits.empty() && hits[0].damage > 8.0f);
        check("命中后移除", system.Count() == 0);
    }
    /* 生物命中: 生物优先于方块判定. */
    {
        ProjectileSystem system;
        system.Spawn(core::Vec3{0.0f, 10.0f, 0.0f}, core::Vec3{0.0f, 0.0f, 1.0f}, 30.0f, 15.0f);
        Context context;
        context.solidZ = 1.0e9f;
        context.creatureX = 1.0f;
        std::vector<ProjectileHit> hits;
        system.Update(0.2f, &context, SolidAt, CreatureAt, hits);
        check("命中生物", hits.size() == 1 && hits[0].kind == ProjectileHit::Kind::kCreature);
        check("命中生物标识正确", !hits.empty() && hits[0].creatureId == 77u);
    }
    /* 超时回收: 长时间飞行后自动消失. */
    {
        ProjectileSystem system;
        system.Spawn(core::Vec3{0.0f, 100.0f, 0.0f}, core::Vec3{0.0f, 1.0f, 0.0f}, 1.0f, 5.0f);
        Context context;
        std::vector<ProjectileHit> hits;
        for (int32_t step = 0; step < 90; ++step) {
            system.Update(0.1f, &context, SolidAt, CreatureAt, hits);
        }
        check("超时后回收", system.Count() == 0);
    }

    std::printf("failures=%d\n", failures);
    return failures == 0 ? 0 : 1;
}
