#pragma once

#include <cstdint>

/*
 * 设置值到玩法数值的换算常量.
 *
 * 原版 setting.lua 只给出档位取值, 档位到引擎参数的换算在 native 层内部完成,
 * 没有可读的证据. 这里按"默认档位保持现实现手感"的原则重建换算关系,
 * 所有可调的量集中在本文件, 调整手感只需改这里, 不必改游戏逻辑.
 */
namespace immirror::settings::tuning {

/*
 * 视角速度. 灵敏度 50 时横向拖满屏宽转 150 度, 纵向拖满屏高转 120 度,
 * 与 player_controller.cpp 原有的固定值一致, 保证默认设置下操作手感不变.
 */
constexpr float kLookDegreesPerScreenWidthAtDefault = 150.0f;
constexpr float kLookDegreesPerScreenHeightAtDefault = 120.0f;

/*
 * 灵敏度 0 到 100 线性映射到视角速度系数 0.2 到 1.8,
 * 即 灵敏度 0 对应 0.2 倍, 50 对应 1.0 倍, 100 对应 1.8 倍.
 * 下限取 0.2 而不是 0, 避免滑到最左后完全无法转向.
 */
constexpr float kLookFactorAtZeroSensitivity = 0.2f;
constexpr float kLookFactorSpan = 1.6f;
constexpr float kSensitivityFull = 100.0f;

/*
 * 视角改写时单帧允许的最大轨迹增量, 单位是归一化屏幕比例.
 * 该上限低于 player_controller 的触点跳变重锚阈值 0.35,
 * 放大灵敏度后也不会被误判成换指而重新落锚.
 */
constexpr float kLookMaxStepPerFrame = 0.30f;

/*
 * 视野距离档位. 原版 5 档取值 1 2 3 5 8, 这里直接当作区块列半径,
 * 与原版"近 中 远 更远 最远"五级描述一致.
 */
constexpr int32_t kViewDistanceLevels[] = {1, 2, 3, 5, 8};
constexpr int32_t kViewDistanceLevelCount = 5;

/* 设备分级首次运行时的视距档位播种值, 与 game_shell 原有的 2 到 4 列分级对应. */
constexpr int32_t kViewDistanceHighGrade = 3;
constexpr int32_t kViewDistanceMediumGrade = 2;
constexpr int32_t kViewDistanceLowGrade = 1;

/* 音量滑杆满量程, 原版取值 0 到 100 的百分比. */
constexpr float kVolumeFull = 100.0f;

/*
 * 经典模式的固定摇杆: 中心与半径均为归一化屏幕坐标.
 * 中心与 hud_renderer 绘制的摇杆底座位置对齐 (0.24, 0.71),
 * 半径沿用 player_controller 的摇杆满量程 0.12.
 */
constexpr float kPadCenterX = 0.24f;
constexpr float kPadCenterY = 0.71f;
constexpr float kPadRadius = 0.12f;

/*
 * 自动跳跃. 前推量超过阈值且视线前方一格处存在恰好一格高的台阶时补一次跳跃,
 * 两次触发的最小间隔避免连跳.
 */
constexpr float kAutoJumpForwardThreshold = 0.35f;
constexpr float kAutoJumpProbeDistance = 0.6f;
constexpr double kAutoJumpIntervalSeconds = 0.30;

/*
 * 镜头摇晃. 原版由受击等事件触发, 现阶段没有伤害事件,
 * 先用随移动强度变化的轻微摆动占位.
 */
constexpr float kCameraShakeAmplitude = 0.03f;
constexpr float kCameraShakeFrequency = 1.6f;
constexpr float kCameraShakeMotionFloor = 0.05f;

}  /* namespace immirror::settings::tuning */
