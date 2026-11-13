#pragma once

#include <cstdint>

namespace immirror::player {

/*
 * 生存属性调参. 数值全部照抄const.lua
 * (temp/original/fake-mini-BUT-true-moments/src/lua/luascript/res/const.lua),
 * 每一项注明键名与行号; 未给出的恢复与扣血节奏在块内注明为自定值.
 *
 * 语义: 体力是动作的即时资源, 体力消耗按 food_beilv 折算成饥饿推进,
 * 饥饿是长期资源, 饥饿高低反过来驱动回血与饿死.
 * 调整手感只改本文件, 控制器与总装都从这里取值, 不存在第二份系数.
 */
namespace survival {

/* const.lua 第 4 行: hpmax = 100, 最大血量, 与 kMaxHealth 一致. */
constexpr float kMaxHealth = 100.0f;
/* const.lua 第 5 行: foodlevel = 100, 初始饥饿值. */
constexpr float kHungerMax = 100.0f;
constexpr float kHungerInitial = 100.0f;
/*
 * const.lua 第 6 行: food_beilv = 5, 注释为"食物的倍率, 20 -> 100 了,
 * 所以也翻了 5 倍", 即食物数值放大倍率, 并非体力折算饥饿的除数.
 * 早期实现把体力消耗直接除以它推进饥饿, 受伤一次 (360 体力) 就扣 72 饥饿,
 * 几次攻击即饿光, 与节奏不符. 这里保留常量本义供食物系统使用.
 */
constexpr float kFoodBeilv = 5.0f;

/*
 * 体力消耗推进饥饿的独立折算 (未给出明确口径, 自定并注明):
 * 按体力消耗点数的 2% 推进, 跳跃 195 点约扣 3.9 饥饿, 走路每秒 15 点约扣 0.3,
 * 与"体力是即时资源, 饥饿是长期资源"的语义一致, 不会几下动作就见底.
 */
constexpr float kStaminaToHungerRatio = 0.02f;

/*
 * 体力上限. const.lua 未单列体力上限键, 但同一文件给出的动作消耗在
 * 100 到 3600 之间 (tili_action_jump=195, tili_action_attack=360,
 * tili_action_food2hp=3600), 说明体力与饥饿不是同一量纲, 体力池远大于 100.
 * 这里按最大一次性动作 (跳跃 195) 与恢复节奏 (每秒 8 到 12) 取 1000,
 * 使连续跳跃约 5 次后见底, 静止约 80 秒回满; 原值 100 会让一次跳跃直接扣光.
 */
constexpr float kStaminaMax = 1000.0f;
constexpr float kStaminaInitial = 1000.0f;

/*
 * 移动速度倍率, const.lua 第 17 到 24 行.
 * 蜂蜜 (0.1) 与物理冲刺 (2.3) 依赖流体与生物系统, 本期无接入点, 仅保留数据.
 */
constexpr float kMoveBackwardRatio = 0.8f;   /* 第 17 行 houtui_yidong_beilv */
constexpr float kMoveSneakRatio = 0.5f;      /* 第 18 行 sneaking_yidong_beilv */
constexpr float kMoveSprintRatio = 1.3f;     /* 第 19 行 chongci_yidong_beilv */
constexpr float kMoveSwimRatio = 0.25f;      /* 第 20 行 swimming_yidong_beilv */
constexpr float kMoveHoneyRatio = 0.1f;      /* 第 21 行 swimming_honey_yidong_beilv */
constexpr float kMoveChargeRatio = 0.5f;     /* 第 22 行 xuli_yidong_beilv, 挖掘中 */
constexpr float kMovePhysSprintRatio = 2.3f; /* 第 24 行 chongci_phys_yidong_beilv, 未接入 */

/* 倍率叠加后的输出钳制区间, 防止多倍率相乘后越界 (自定, 注明). */
constexpr float kMoveRatioMin = 0.1f;
constexpr float kMoveRatioMax = 2.5f;

/*
 * 冲刺触发: 摇杆前向量超过该阈值视为冲刺意图 (简化触发, 注明).
 * 为双击前进或满量持续, 摇杆量 0.95 以上等价于几乎推满, 手感接近.
 */
constexpr float kSprintStickThreshold = 0.95f;

/*
 * 体力消耗, const.lua 第 26 到 34 行 tili_action_* 键.
 * 单位为"每次动作"的体力点数, 未给出按时间的折算口径,
 * 本实现按秒折算持续动作 (走路与游泳), 按次扣一次性动作 (跳跃, 破坏, 攻击).
 */
constexpr float kStaminaWalkPerSecond = 15.0f;        /* 第 26 行 tili_action_walk */
constexpr float kStaminaSprintPerSecond = 75.0f;      /* 第 27 行 tili_action_sprint */
constexpr float kStaminaSwimPerSecond = 25.0f;        /* 第 28 行 tili_action_swim */
constexpr float kStaminaJump = 195.0f;                /* 第 29 行 tili_action_jump */
constexpr float kStaminaSprintJump = 960.0f;          /* 第 30 行 tili_action_sprintjump */
constexpr float kStaminaDestroyBlock = 100.0f;        /* 第 31 行 tili_action_destroyblock */
constexpr float kStaminaAttack = 360.0f;              /* 第 32 行 tili_action_attack */
constexpr float kStaminaHurt = 360.0f;                /* 第 33 行 tili_action_hurt */
constexpr float kStaminaFoodToHp = 3600.0f;           /* 第 34 行 tili_action_food2hp */

/*
 * 体力恢复节奏 (未给出, 自定, 注明): 移动中每秒回 8,
 * 静止每秒回 12, 鼓励停下来喘气但不惩罚正常行进.
 */
constexpr float kStaminaRegenPerSecond = 80.0f;
constexpr float kStaminaRegenStillPerSecond = 120.0f;
/* 体力低于该值时禁止冲刺与跳跃 (按上限 1000 同比例放大). */
constexpr float kStaminaActionFloor = 100.0f;

/*
 * 饥饿驱动的血量节奏 (未给出每秒扣回比例, 自定, 注明):
 * 饥饿归零每秒扣 5% 最大血量 (饿死), 饥饿高于 80 每秒回 2% (吃饱回血),
 * 与food2hp 的"吃饱才回血"语义一致.
 */
/*
 * 被动饥饿 (未给出, 自定, 注明): 站着不动也会缓慢变饿, 每秒 0.4 点,
 * 满饥饿约 250 秒见底; 动作消耗在体力折算里另计.
 */
constexpr float kHungerDrainPerSecond = 0.4f;

constexpr float kStarveDamagePerSecond = 0.05f;
constexpr float kWellFedHealPerSecond = 0.02f;
constexpr float kWellFedHungerFloor = 80.0f;

}  /* namespace survival */

}  /* namespace immirror::player */
