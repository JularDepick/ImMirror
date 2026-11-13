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

/* 图层顺序: 数值小的先画, 与布局里 background / artwork / overlay 一致. */
enum class LayerLevel : uint8_t {
    kBackground = 0,
    kBorders = 1,
    kArtwork = 2,
    kOverlay = 3,
    kHighlight = 4,
};

/*
 * 一帧: 名字, 父帧, 子帧, 锚点, 尺寸与外观.
 * rect 是求解结果; 颜色与贴图来自布局元素本身, 绘制阶段按图层顺序展开.
 */
struct LayoutFrame {
    const char* name = "";
    LayoutFrame* parent = nullptr;
    std::vector<LayoutFrame*> children;
    std::vector<LayoutAnchor> anchors;
    /* 未给尺寸时按 0 处理, 求解结果宽高为 0. */
    float width = 0.0f;
    float height = 0.0f;
    LayoutRect rect;
    /* 外观: 未给颜色时按不透明白处理, 贴图为空表示只画纯色. */
    bool hasColor = false;
    float red = 1.0f;
    float green = 1.0f;
    float blue = 1.0f;
    float alpha = 1.0f;
    /* 贴图相对路径, 空表示无贴图. */
    const char* texture = nullptr;
    LayerLevel layer = LayerLevel::kArtwork;
    /* 布局里标了 hidden="true": 载入时默认不显示, 由脚本按需打开. */
    bool hidden = false;
    /* 模板里的精灵名, 与 texture 一起决定取图集里的哪一格; 空表示整图. */
    const char* sprite = nullptr;

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
