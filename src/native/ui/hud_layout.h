#pragma once

#include <cmath>
#include <cstdint>

namespace immirror::ui {

/* 方块栏槽位数, 槽位顺序见 hud_slots.h 的共享表. */
constexpr int32_t kHudSlotCount = 9;

/* 方块栏槽位半径, 以屏宽为 1. */
constexpr float kHudSlotHalfSize = 0.0265f;
/* 相邻槽位之间的缝隙, 以屏宽为 1. */
constexpr float kHudSlotGap = 0.005f;
/* 槽位中心纵坐标, 归一化屏幕坐标. */
constexpr float kHudSlotCenterY = 0.9f;

/* 跳跃按钮中心与半径, 中心为归一化屏幕坐标, 半径为屏宽比例. */
constexpr float kHudJumpCenterX = 0.88f;
constexpr float kHudJumpCenterY = 0.76f;
constexpr float kHudJumpHalfSize = 0.045f;

/*
 * HUD 几何布局. 设计坐标的长度单位取屏幕高度的一半, 水平方向按 高/宽 换算,
 * 因此同一个数值横向与纵向对应的像素长度相等, 圆环不会被画成椭圆.
 * 绘制与命中判定都从这里取几何, 两处不再各写一套数字.
 */
class HudLayout {
public:
    HudLayout() = default;
    HudLayout(int32_t width, int32_t height) : width_(width), height_(height) {}

    bool Valid() const { return width_ > 0 && height_ > 0; }
    int32_t Width() const { return width_; }
    int32_t Height() const { return height_; }

    /* 水平等比系数, 等于 高/宽; 传给绘制函数后横向自动按像素等比. */
    float Scale() const {
        return width_ > 0 ? static_cast<float>(height_) / static_cast<float>(width_) : 1.0f;
    }

    /* 窗口宽高比, 等于 宽/高. */
    float Aspect() const {
        return height_ > 0 ? static_cast<float>(width_) / static_cast<float>(height_) : 1.0f;
    }

    /* 归一化屏幕坐标到设计坐标, 原点在屏幕中心, 纵轴向上. */
    float DesignX(float normalizedX) const {
        const float scale = Scale();
        return scale > 0.0001f ? (normalizedX * 2.0f - 1.0f) / scale : 0.0f;
    }

    float DesignY(float normalizedY) const { return 1.0f - normalizedY * 2.0f; }

    /* 归一化屏幕长度 (以屏宽为 1) 到设计长度, 横向纵向同值. */
    float DesignLength(float normalizedWidth) const {
        const float scale = Scale();
        return scale > 0.0001f ? normalizedWidth * 2.0f / scale : 0.0f;
    }

    /* 槽位中心横坐标, 九格以屏幕水平中心对齐. */
    float SlotCenterX(int32_t slot) const {
        const float pitch = (kHudSlotHalfSize * 2.0f) + kHudSlotGap;
        const float originX = 0.5f - pitch * static_cast<float>(kHudSlotCount - 1) * 0.5f;
        return originX + static_cast<float>(slot) * pitch;
    }

    float SlotCenterY() const { return kHudSlotCenterY; }
    float SlotHalfSize() const { return kHudSlotHalfSize; }

    /* 命中判定: 输入为归一化屏幕坐标, 未命中返回 -1. */
    int32_t HitTestSlot(float normalizedX, float normalizedY) const {
        const float half = DesignLength(kHudSlotHalfSize);
        const float x = DesignX(normalizedX);
        const float y = DesignY(normalizedY);
        if (y < DesignY(kHudSlotCenterY) - half || y > DesignY(kHudSlotCenterY) + half) {
            return -1;
        }
        for (int32_t slot = 0; slot < kHudSlotCount; ++slot) {
            const float centerX = DesignX(SlotCenterX(slot));
            if (x >= centerX - half && x <= centerX + half) {
                return slot;
            }
        }
        return -1;
    }

    float JumpCenterX() const { return kHudJumpCenterX; }
    float JumpCenterY() const { return kHudJumpCenterY; }
    float JumpHalfSize() const { return kHudJumpHalfSize; }

    bool HitTestJump(float normalizedX, float normalizedY) const {
        const float half = DesignLength(kHudJumpHalfSize);
        return std::fabs(DesignX(normalizedX) - DesignX(kHudJumpCenterX)) <= half &&
               std::fabs(DesignY(normalizedY) - DesignY(kHudJumpCenterY)) <= half;
    }

private:
    int32_t width_ = 0;
    int32_t height_ = 0;
};

}  /* namespace immirror::ui */
