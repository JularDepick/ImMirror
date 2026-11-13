#include "entity/creature_entity.h"

#include "entity/sight.h"

#include <cmath>

#include "entity/entity_tuning.h"
#include "voxel/block_registry.h"
#include "world/world.h"

namespace immirror::entity {
namespace {

constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 2.0f * kPi;

/* 角度回绕到正负 pi, 长时间转动不会累积浮点误差. */
float WrapAngle(float angle) {
    float wrapped = angle;
    while (wrapped > kPi) {
        wrapped -= kTwoPi;
    }
    while (wrapped < -kPi) {
        wrapped += kTwoPi;
    }
    return wrapped;
}

}  /* namespace */

CreatureEntity::CreatureEntity(EntityId id, const MonsterDef& def, const core::Vec3& position,
                               uint32_t seed)
    : Entity(id, EntityKind::kCreature), def_(&def), rng_(seed != 0u ? seed : 1u) {
    body_.position = position;
    body_.halfWidth = def.HalfWidth();
    body_.height = def.height;
    health_ = def.life;
    /* 生成朝向随机, 避免整群生物同向站立. */
    yaw_ = RandomUnit() * kTwoPi - kPi;
    EnterState(CreatureState::kIdle);
}

CreatureTick CreatureEntity::Update(double dt, const world::World& world,
                                    const PlayerProbe& player) {
    CreatureTick tick;
    float seconds = dt > 0.0 ? static_cast<float>(dt) : 0.0f;
    if (seconds > tuning::kMaxFrameSeconds) {
        seconds = tuning::kMaxFrameSeconds;
    }

    /* 死亡计时期间不再思考与移动, 只等管理器回收. */
    if (state_ == CreatureState::kDeath) {
        deathSeconds_ += seconds;
        return tick;
    }

    if (hurtSeconds_ > 0.0f) {
        hurtSeconds_ -= seconds;
        if (hurtSeconds_ < 0.0f) {
            hurtSeconds_ = 0.0f;
        }
    }
    if (attackCooldown_ > 0.0f) {
        attackCooldown_ -= seconds;
    }
    if (blockedJumpCooldown_ > 0.0f) {
        blockedJumpCooldown_ -= seconds;
    }
    stateSeconds_ += seconds;

    /* 水平距离决定感知与接触; 生物不处理垂直方向的追击差异. */
    const core::Vec3 toPlayer{player.feet.x - body_.position.x, 0.0f,
                              player.feet.z - body_.position.z};
    const float distance = toPlayer.Length();
    const bool sensed = player.alive && distance <= def_->viewDistance;
    const float contactRange =
        def_->attackDistance + body_.halfWidth + player.halfWidth + tuning::kContactSlack;
    const bool inContact = player.alive && distance <= contactRange;

    fleeing_ = false;
    if (def_->hostile) {
        if (sensed) {
            const CreatureState wanted = inContact ? CreatureState::kAttack : CreatureState::kChase;
            if (state_ != wanted) {
                EnterState(wanted);
            }
        } else if ((state_ == CreatureState::kChase || state_ == CreatureState::kAttack) &&
                   distance > def_->viewDistance * tuning::kChaseLeashRatio) {
            /* 追出感知半径的加成就放弃, 回到待机重新巡查. */
            EnterState(CreatureState::kIdle);
        }
    } else if (sensed) {
        /*
         * 被动生物不还手: 感知范围内转身远离玩家, 仍是游荡状态,
         * 只是方向改为背离玩家并按躲避速度移动.
         */
        fleeing_ = true;
        if (state_ != CreatureState::kWander) {
            EnterState(CreatureState::kWander);
        } else {
            /* 已经在游荡时续期, 避免躲避途中被时长切回待机而停在原地. */
            stateDuration_ = tuning::kWanderMaxSeconds;
        }
    }

    /* 待机与游荡按时长互相切换, 追击与攻击由感知结果驱动. */
    if (state_ == CreatureState::kIdle && stateSeconds_ >= stateDuration_) {
        EnterState(CreatureState::kWander);
    }
    if (state_ == CreatureState::kWander && stateSeconds_ >= stateDuration_) {
        EnterState(CreatureState::kIdle);
    }

    /* 期望移动方向: 追击朝玩家, 躲避背离玩家, 游荡沿随机方向, 其余原地. */
    core::Vec3 wish{};
    float speed = def_->speed;
    switch (state_) {
        case CreatureState::kChase:
            wish = toPlayer.Normalized();
            break;
        case CreatureState::kWander:
            if (fleeing_) {
                wish = core::Vec3{-toPlayer.x, 0.0f, -toPlayer.z}.Normalized();
                speed = def_->speed * tuning::kFleeSpeedRatio;
            } else {
                wish = wanderDirection_;
                speed = def_->speed * tuning::kWanderSpeedRatio;
            }
            break;
        case CreatureState::kIdle:
        case CreatureState::kAttack:
        case CreatureState::kDeath:
            break;
    }

    body_.velocity.x = wish.x * speed;
    body_.velocity.z = wish.z * speed;
    if (wish.Length() > 1.0e-4f) {
        FaceDirection(wish, seconds);
    }

    const BoxStepResult step = StepBox(body_, seconds, world);

    /* 被一格台阶挡住时补一次跳跃, 生物因此不会卡在小坡前. */
    if ((step.blockedX || step.blockedZ) && body_.onGround && blockedJumpCooldown_ <= 0.0f &&
        (state_ == CreatureState::kChase || state_ == CreatureState::kWander)) {
        body_.velocity.y = tuning::kJumpSpeed;
        blockedJumpCooldown_ = tuning::kBlockedJumpCooldown;
    }

    /* 掉出世界底部视为消失, 直接进入死亡回收, 不产出掉落. */
    if (body_.position.y < 0.0f) {
        EnterState(CreatureState::kDeath);
        deathSeconds_ = tuning::kDeathSeconds;
        return tick;
    }

    /* 攻击: 命中距离内按生物表的间隔出手, 伤害交给总装施加到玩家身上. */
    if (state_ == CreatureState::kAttack && inContact && attackCooldown_ <= 0.0f) {
        const core::Vec3 monsterEye{body_.position.x, body_.position.y + body_.height * 0.9f,
                                    body_.position.z};
        const core::Vec3 playerEye{player.feet.x, player.feet.y + player.height * 0.9f,
                                   player.feet.z};
        /* 远程生物隔墙不得命中; 近身攻击不做遮挡判定, 避免一格高差下打空. */
        const bool sightOk = def_->attackDistance <= tuning::kSightCheckDistance ||
                             LineOfSight(world, monsterEye, playerEye);
        if (sightOk) {
            attackCooldown_ = def_->attackInterval;
            tick.playerDamage = def_->attack * tuning::kDamageScale;
            /* 攻击方式 0 近程, 1 远程, 2 爆炸, 与伤害类型的序号对应. */
            tick.playerDamageType = def_->attackType == 1
                                        ? 2
                                        : (def_->attackType == 2 ? 3 : 1);
            FaceDirection(toPlayer, 1.0f);
        }
    }
    return tick;
}

bool CreatureEntity::Damage(float amount) {
    if (state_ == CreatureState::kDeath || !(amount > 0.0f)) {
        return false;
    }
    health_ -= amount;
    hurtSeconds_ = tuning::kHurtFlashSeconds;
    if (health_ > 0.0f) {
        return false;
    }
    health_ = 0.0f;
    EnterState(CreatureState::kDeath);
    return true;
}

float CreatureEntity::HealthRatio() const {
    const float maximum = def_->life > 0.0f ? def_->life : 1.0f;
    const float ratio = health_ / maximum;
    return ratio < 0.0f ? 0.0f : (ratio > 1.0f ? 1.0f : ratio);
}

const char* CreatureEntity::StateName() const {
    switch (state_) {
        case CreatureState::kIdle:
            return "idle";
        case CreatureState::kWander:
            return "wander";
        case CreatureState::kChase:
            return "chase";
        case CreatureState::kAttack:
            return "attack";
        case CreatureState::kDeath:
            return "death";
    }
    return "idle";
}

bool CreatureEntity::ReadyToRemove() const {
    return state_ == CreatureState::kDeath && deathSeconds_ >= tuning::kDeathSeconds;
}

float CreatureEntity::RandomUnit() {
    /* xorshift32: 生成布局只需要可复现的伪随机, 不引入标准库随机设备. */
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    return static_cast<float>(rng_ & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
}

float CreatureEntity::RandomBetween(float low, float high) {
    return low + RandomUnit() * (high - low);
}

void CreatureEntity::EnterState(CreatureState state) {
    state_ = state;
    stateSeconds_ = 0.0f;
    fleeing_ = false;
    switch (state) {
        case CreatureState::kIdle:
            stateDuration_ = RandomBetween(tuning::kIdleMinSeconds, tuning::kIdleMaxSeconds);
            break;
        case CreatureState::kWander: {
            stateDuration_ = RandomBetween(tuning::kWanderMinSeconds, tuning::kWanderMaxSeconds);
            const float angle = RandomUnit() * kTwoPi;
            wanderDirection_ = core::Vec3{std::cos(angle), 0.0f, std::sin(angle)};
            break;
        }
        case CreatureState::kChase:
        case CreatureState::kAttack:
            stateDuration_ = 0.0f;
            break;
        case CreatureState::kDeath:
            stateDuration_ = 0.0f;
            deathSeconds_ = 0.0f;
            body_.velocity = core::Vec3{};
            /* 死亡后不再出手, 冷却置满即可让攻击结算自然失效. */
            attackCooldown_ = def_->attackInterval;
            break;
    }
}

void CreatureEntity::FaceDirection(const core::Vec3& direction, float seconds) {
    if (std::fabs(direction.x) < 1.0e-5f && std::fabs(direction.z) < 1.0e-5f) {
        return;
    }
    /* 朝向约定与玩家一致: 偏航 0 时前向为 -Z. */
    const float target = std::atan2(-direction.x, -direction.z);
    float difference = WrapAngle(target - yaw_);
    const float step = tuning::kTurnSpeed * seconds;
    if (difference > step) {
        difference = step;
    } else if (difference < -step) {
        difference = -step;
    }
    yaw_ = WrapAngle(yaw_ + difference);
}

}  /* namespace immirror::entity */