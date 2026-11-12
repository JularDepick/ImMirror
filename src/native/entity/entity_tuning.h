#pragma once

#include <cstdint>

/*
 * 生物与战斗系统的可调数值.
 *
 * 物理量沿用玩家控制器的口径: 重力, 下落限速与分步推进上限都与
 * player_controller 取值一致, 让生物与玩家的落地手感相同.
 * 生成, 休眠与战斗的数值在这里集中维护, 调整生物密度或攻击手感只改本文件.
 */
namespace immirror::entity::tuning {

/* 与 player_controller 同源的物理参数. */
constexpr float kGravity = -24.0f;
constexpr float kMaxFallSpeed = -58.0f;
constexpr float kMaxSubStep = 0.2f;      /* 单次碰撞推进的最大位移 */
constexpr int32_t kMaxSubSteps = 16;     /* 单帧推进次数上限 */
constexpr float kSurfaceEpsilon = 1.0e-3f; /* 贴面间隙, 避免重复判定接触 */
constexpr float kMaxFrameSeconds = 0.1f;   /* 单帧时间片上限, 防止卡顿穿墙 */

/*
 * 生物被一格台阶挡住时补一次跳跃. 与玩家不同, 生物不处理双击与按键,
 * 只在水平推进受阻且已落地时起跳, 冷却防止连续起跳抖动.
 */
constexpr float kJumpSpeed = 6.5f;
constexpr float kBlockedJumpCooldown = 0.8f;

/*
 * 生成与存活. 生物只在地表按概率生成, 生成环避开玩家脚下近处;
 * 超出休眠半径的生物暂停 AI, 超出移除半径的直接回收.
 */
constexpr int32_t kMaxCreatures = 10;  /* 观感平衡: 同时在场生物上限 */
/* 玩家生命上限与生成密度. 生命上限取玩家数值; 密度按观感可调. */
constexpr float kPlayerMaxHealth = 100.0f;
constexpr float kPlayerRegenPerSecond = 1.0f;    /* 脱战回血 */
constexpr float kPlayerRespawnSeconds = 3.0f;    /* 死亡后重生延迟 */
constexpr int32_t kSpawnCapPerPlayer = 8;        /* 存活生物上限 */
constexpr double kSpawnIntervalSeconds = 6.0;    /* 生成间隔: 首夜不会一次围满 */
constexpr int32_t kSpawnAttemptsPerTick = 4;
constexpr float kSpawnMinRadius = 12.0f;
constexpr float kSpawnMaxRadius = 34.0f;
constexpr float kSpawnChancePerAttempt = 0.35f;
constexpr float kSleepRadius = 40.0f;
constexpr float kDespawnRadius = 80.0f;

/* AI 状态时长与移动比例. */
constexpr float kIdleMinSeconds = 1.2f;
constexpr float kIdleMaxSeconds = 4.0f;
constexpr float kWanderMinSeconds = 2.0f;
constexpr float kWanderMaxSeconds = 5.0f;
constexpr float kWanderSpeedRatio = 0.5f;  /* 游荡速度相对生物表速度 */
constexpr float kFleeSpeedRatio = 1.0f;    /* 被动生物躲避时按全速移动 */
constexpr float kChaseLeashRatio = 1.35f;  /* 追出感知半径的该倍数即放弃 */
constexpr float kContactSlack = 0.30f;     /* 接触判定的额外余量, 米 */
constexpr float kTurnSpeed = 9.0f;         /* 朝向插值速度, 弧度每秒 */

/* 战斗. */
constexpr float kPlayerReach = 4.5f;        /* 攻击生物的视线长度 */
constexpr float kAttackHoldInterval = 0.45f; /* 按住连续攻击的间隔 */
/* 生物伤害按原表 attack 值折算的比例, 原表数值面向血量体系. */
constexpr float kDamageScale = 0.5f;
constexpr float kFistDamage = 5.0f;          /* 空手单次伤害 */
constexpr float kHeldBlockBaseDamage = 3.0f; /* 手持方块的基础伤害 */
constexpr float kHeldBlockHardnessScale = 1.5f;
constexpr float kHeldDamageMax = 14.0f;
/*
 * 攻击距离超过该值的生物才做视线遮挡判定. 近身生物的贴脸攻击常发生在一格高差上,
 * 直线采样容易误判成隔墙, 因此近程出手不看遮挡.
 */
constexpr float kSightCheckDistance = 3.0f;
constexpr float kDeathSeconds = 0.6f;   /* 死亡后保留实体的秒数, 留给渲染播死亡 */
constexpr float kHurtFlashSeconds = 0.25f; /* 生物受击闪白残留秒数 */

/*
 * 手持物到单次伤害的换算: 空手取拳击伤害, 手持方块按方块硬度加成并封顶.
 * 工具与武器物品接入后由调用方改传武器攻击力, 公式不变.
 */
inline float HeldAttackDamage(bool holding, float hardness) {
    if (!holding || hardness < 0.0f) {
        return kFistDamage;
    }
    const float damage = kHeldBlockBaseDamage + hardness * kHeldBlockHardnessScale;
    return damage > kHeldDamageMax ? kHeldDamageMax : damage;
}

}  /* namespace immirror::entity::tuning */