#pragma once

namespace immirror::player {

/*
 * 视角灵敏度. 把灵敏度作为 0 至 100 的设置项, 默认 50 (setting.lua 第 9 行),
 * 但 0 至 100 到转角的换算链尚未从 native 侧反汇编出来 (差距清单 Q9).
 * 设置体系就绪之前, 所有与视角映射相关的常量集中在本文件, 控制器只从这里取值,
 * 设置层就绪后由总装把设置项折成本结构的字段, 换算链路仍然只有这一处.
 */
struct LookTuning {
    float sensitivity = 50.0f;                    /* 设置项 sensitivity, 0 至 100 */
    float degreesPerWidthAtSensitivity = 150.0f;  /* 灵敏度 50 时横向拖满屏宽的转角 */
    float degreesPerHeightAtSensitivity = 120.0f; /* 灵敏度 50 时纵向拖满屏高的转角 */
    bool invertY = false;                         /* 反转 Y 轴, 对应reversalY 设置项 */

    /* 横向拖满屏宽的转角, 以默认灵敏度为基准线性折算. */
    float DegreesPerWidth() const {
        return degreesPerWidthAtSensitivity * (sensitivity / kReferenceSensitivity);
    }

    /* 纵向拖满屏高的转角, 同样按灵敏度折算. */
    float DegreesPerHeight() const {
        return degreesPerHeightAtSensitivity * (sensitivity / kReferenceSensitivity);
    }

    /* 灵敏度滑杆的中位值, 作为折算基准. */
    static constexpr float kReferenceSensitivity = 50.0f;
};

/*
 * 浮动摇杆. 满量程半径以屏宽为 1, 锚点默认跟随按下点;
 * 经典模式把锚点固定在底座中心, 左手模式把摇杆区换到右半屏.
 * 满量程与死区数值未见 (差距清单 Q1), 半径沿用当前取值, 只收敛到本文件.
 */
struct StickTuning {
    float radius = 0.12f;
    bool fixedAnchor = false;  /* 经典模式: 锚点固定, 不跟随按下点 */
    float anchorX = 0.24f;     /* 固定底座的归一化横坐标 */
    float anchorY = 0.71f;     /* 固定底座的归一化纵坐标 */
    bool leftHanded = false;   /* 左手模式: 摇杆区换到右半屏 */
};

inline LookTuning DefaultLookTuning() {
    return LookTuning{};
}

inline StickTuning DefaultStickTuning() {
    return StickTuning{};
}

}  /* namespace immirror::player */
