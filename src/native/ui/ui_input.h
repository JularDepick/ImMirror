#pragma once

#include "ui/widget_tree.h"

namespace immirror::ui {

/*
 * 界面输入: 屏幕坐标到设计画布的映射, 以及按下/移动/抬起的命中状态机.
 *
 * 布局按 1280x720 的设计画布写死坐标, 实际屏幕比例不同时按等比缩放并居中,
 * 因此两侧或上下会留黑边; 映射与命中都必须用同一套换算, 否则点击会偏.
 */
class UiInput {
public:
    /* 设计画布尺寸, 默认 1280x720. */
    void SetCanvas(float width, float height);

    /* 实际屏幕尺寸(像素). */
    void SetScreen(float width, float height);

    /* 屏幕坐标转画布坐标; 落在黑边上时返回 false. */
    bool ToCanvas(float screenX, float screenY, float* canvasX, float* canvasY) const;

    /* 按下: 命中控件即置按下态并返回它; 未命中返回空. */
    Widget* Down(WidgetTree& tree, float screenX, float screenY);

    /* 移动: 指针离开按下控件时清除按下态. */
    void Move(WidgetTree& tree, float screenX, float screenY);

    /*
     * 抬起: 仍在按下控件上时返回该控件(调用方据此派发点击), 否则返回空.
     * 无论是否命中, 按下态都会清除.
     */
    Widget* Up(WidgetTree& tree, float screenX, float screenY);

    Widget* Pressed() const { return pressed_; }

    /* 画布在屏幕上的可见矩形(像素), 供黑边绘制与断言. */
    void VisibleRect(float* x, float* y, float* width, float* height) const;

private:
    float canvasWidth_ = 1280.0f;
    float canvasHeight_ = 720.0f;
    float screenWidth_ = 1280.0f;
    float screenHeight_ = 720.0f;
    Widget* pressed_ = nullptr;
};

}  /* namespace immirror::ui */
