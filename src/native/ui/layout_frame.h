#pragma once

#include <cstdint>
#include <vector>

namespace immirror::ui {

/*
 * 帧树与锚点求解.
 *
 * 界面布局的范式是"帧 + 锚点 + 图层": 每个帧给出若干锚点, 锚点指到自己身上
 * 的一个点与另一个帧上的一个点, 加上二维偏移; 尺寸另给绝对宽高.
 * 这里只做几何求解, 不涉及绘制与贴图, 因此可以离线用数值断言验证.
 */

/* 帧上的九个参考点. */
enum class AnchorPoint : uint8_t {
    kTopLeft = 0,
    kTop,
    kTopRight,
    kLeft,
    kCenter,
    kRight,
    kBottomLeft,
    kBottom,
    kBottomRight,
};

/* 轴对齐矩形, x 与 y 是左上角, 单位与布局文件一致 (像素). */
struct LayoutRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct LayoutFrame;

/* 一条锚点: 自己的 point 对齐到 relativeTo 的 relativePoint, 再加偏移. */
struct LayoutAnchor {
    AnchorPoint point = AnchorPoint::kTopLeft;
    const LayoutFrame* relativeTo = nullptr;
    AnchorPoint relativePoint = AnchorPoint::kTopLeft;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
};

/* 一帧: 名字, 父帧, 子帧, 锚点与尺寸. rect 是求解结果. */
struct LayoutFrame {
    const char* name = "";
    LayoutFrame* parent = nullptr;
    std::vector<LayoutFrame*> children;
    std::vector<LayoutAnchor> anchors;
    /* 未给尺寸时按 0 处理, 求解结果宽高为 0. */
    float width = 0.0f;
    float height = 0.0f;
    LayoutRect rect;

    void AddChild(LayoutFrame* child) {
        child->parent = this;
        children.push_back(child);
    }
};

/* 点在矩形上的坐标. */
float PointX(const LayoutRect& rect, AnchorPoint point);
float PointY(const LayoutRect& rect, AnchorPoint point);

/*
 * 求解整棵帧树. 根帧按屏幕矩形摆放并忽略自身锚点,
 * 其余帧按第一条锚点定位; 锚点缺失时贴在父帧左上角.
 * 必须从根开始自上而下求解, 父帧未解算时子帧的结果没有意义.
 */
void ResolveLayout(LayoutFrame& root, const LayoutRect& screen);

}  /* namespace immirror::ui */
