#pragma once

#include <cstdint>

#include "core/vec3.h"
#include "entity/box_body.h"
#include "entity/entity.h"
#include "entity/monster_table.h"

namespace immirror::world {
class World;
}

namespace immirror::entity {

/*
 * 生物状态机. 待机与游荡是巡查阶段, 追击与攻击只对敌对生物出现,
 * 死亡是终态, 死亡计时走完后由实体管理器移除.
 */
enum class CreatureState : uint8_t {
    kIdle = 0,
    kWander,
    kChase,
    kAttack,
    kDeath,
};

/* 生物每帧对外的结果: 本帧对玩家造成的伤害合计. */
struct CreatureTick {
    float playerDamage = 0.0f;
};

/*
 * 玩家在本帧的可观测信息. 由总装从玩家控制器填好后交给生物,
 * 生物不反向持有玩家对象, 两者的更新顺序由总装决定.
 */
struct PlayerProbe {
    core::Vec3 feet{};
    float halfWidth = 0.3f;
    float height = 1.8f;
    bool alive = true;
};

/*
 * 生物实体. 状态机负责巡查与追击, 碰撞体负责与体素世界的推进,
 * 攻击按生物表给出的间隔结算伤害, 伤害通过返回值交给总装施加到玩家身上.
 * 只在原生主线程使用, 不做加锁.
 */
class CreatureEntity final : public Entity {
public:
    /* def 必须是 MonsterTable 持有的条目, 其生存期与进程一致. */
    CreatureEntity(EntityId id, const MonsterDef& def, const core::Vec3& position, uint32_t seed);

    core::Vec3 Position() const override { return body_.position; }
    float BoundsHalfWidth() const override { return body_.halfWidth; }
    float BoundsHeight() const override { return body_.height; }

    /* 推进 AI 与物理, 返回本帧对玩家的伤害. */
    CreatureTick Update(double dt, const world::World& world, const PlayerProbe& player);

    /* 扣血, 返回本次是否致死. 已经死亡或伤害非正时不生效. */
    bool Damage(float amount);

    const MonsterDef& Definition() const { return *def_; }
    float Health() const { return health_; }
    float HealthRatio() const;
    float Yaw() const { return yaw_; }
    const core::Vec3& Velocity() const { return body_.velocity; }
    CreatureState State() const { return state_; }
    /* 状态名, 供日志与调试输出. */
    const char* StateName() const;
    bool Dying() const { return state_ == CreatureState::kDeath; }
    /* 受伤闪白剩余秒数, 供渲染侧做受击反馈. */
    float HurtSeconds() const { return hurtSeconds_; }
    /* 死亡计时是否走完, 走完后由管理器移除. */
    bool ReadyToRemove() const;

private:
    float RandomUnit();
    float RandomBetween(float low, float high);
    void EnterState(CreatureState state);
    /* 朝指定水平方向转向, 每帧转角受转向速度限制. */
    void FaceDirection(const core::Vec3& direction, float seconds);

    const MonsterDef* def_ = nullptr;
    BoxBody body_;
    float yaw_ = 0.0f;
    float health_ = 0.0f;
    CreatureState state_ = CreatureState::kIdle;
    float stateSeconds_ = 0.0f;   /* 当前状态已持续时间 */
    float stateDuration_ = 0.0f;  /* 当前状态的切换时长, 追击与攻击为 0 */
    float attackCooldown_ = 0.0f; /* 距离下次出手的秒数 */
    float hurtSeconds_ = 0.0f;
    float deathSeconds_ = 0.0f;
    float blockedJumpCooldown_ = 0.0f;
    core::Vec3 wanderDirection_{};
    uint32_t rng_ = 1u;
    bool fleeing_ = false; /* 被动生物在感知范围内躲避玩家 */
};

}  /* namespace immirror::entity */
