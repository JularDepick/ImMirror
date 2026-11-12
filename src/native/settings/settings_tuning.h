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
 * 镜头摇晃. 受击抖动已对齐原版语义 (受击事件触发, 快速衰减),
 * 随移动强度变化的轻微摆动仍为占位.
 */
constexpr float kCameraShakeAmplitude = 0.03f;
constexpr float kCameraShakeFrequency = 1.6f;
constexpr float kCameraShakeMotionFloor = 0.05f;

/*
 * 第三人称预设折算. 原版 camera.lua 60 至 127 行的"四套预设 8 参数"
 * 是自定义视角编辑器的选项索引组合 (背视角 1 平视角 2 俯视角 3 与自定义),
 * 不是米制距离; 真实的距离与高度在 PlayerControl::updateGameCamera 的
 * native 代码里, 逆向工程未还原出数值. 这里按视角语义折算成米制:
 *   背视角 (view=2): 过肩跟随, 后撤 4.0 米, 眼位上抬 1.2 米,
 *     俯仰偏置 12 度向下 (约肩后高位俯看, 俯视角度取常用第三人称手感);
 *   正视角 (view=3): 相机在玩家前方回望, 后撤与高度与背视角一致.
 * 视野角 fov 固定 75 度: 原版 camera.lua 417 至 437 行的 fov 滑杆
 * 属于自定义视角编辑器, 设置键 view 没有 fov 档位, 本期不做滑杆.
 */
constexpr float kThirdPersonDistance = 4.0f;
constexpr float kThirdPersonHeight = 1.2f;
/* 俯仰偏置, 弧度, 相机从高处向下俯看的角度. */
constexpr float kThirdPersonPitchOffset = 12.0f * 3.14159265358979f / 180.0f;

/*
 * 限帧档位对应的帧率. 原版 setting.lua 的 MOBILE_LIMIT_ON 为 30,
 * MOBILE_LIMIT_OFF 为 50: limiteFps=1 锁 30 帧, 0 走 50 帧上限.
 * 实际的帧门控在渲染设备内完成, 这里只做档位到帧率的换算.
 */
constexpr int32_t kFrameLimitOnFps = 30;
constexpr int32_t kFrameLimitOffFps = 50;

/*
 * 雾效密度档位. fog 取值 0 关闭, 1 淡雾, 2 浓雾;
 * 密度按指数平方衰减模型折算, 淡雾约在百米外起雾, 浓雾起始距离减半.
 * 雾色由渲染层内定为与清屏底色一致的天空色, 不经设置下发.
 */
constexpr float kFogDensityLevels[] = {0.0f, 0.006f, 0.012f};
constexpr int32_t kFogDensityLevelCount = 3;

/*
 * 亮度滑杆折算. screenbright 取值 0 到 100, 线性映射到 0.7 到 1.3 的
 * 输出颜色系数: 0 对应 0.7, 50 对应 1.0, 100 对应 1.3.
 */
constexpr float kBrightnessAtZero = 0.7f;
constexpr float kBrightnessSpan = 0.6f;
constexpr float kBrightnessFull = 100.0f;

/*
 * 水面反射与实时光影. 渲染层当前没有反射与阴影管线, reflect 与 shadow
 * 两键先只保存取值, 等待后端管线扩展后接入; 消费面审计时不要把
 * 这两键计为缺失接线, 它们是已知的等待后端项.
 */

}  /* namespace immirror::settings::tuning */
