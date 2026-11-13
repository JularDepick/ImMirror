#include "ui/layout_frame.h"

#include <cmath>

namespace immirror::ui {

float PointX(const LayoutRect& rect, AnchorPoint point) {
    switch (point) {
        case AnchorPoint::kTopLeft:
        case AnchorPoint::kLeft:
        case AnchorPoint::kBottomLeft:
            return rect.x;
        case AnchorPoint::kTop:
        case AnchorPoint::kCenter:
        case AnchorPoint::kBottom:
            return rect.x + rect.width * 0.5f;
        default:
            return rect.x + rect.width;
    }
}

float PointY(const LayoutRect& rect, AnchorPoint point) {
    switch (point) {
        case AnchorPoint::kTopLeft:
        case AnchorPoint::kTop:
        case AnchorPoint::kTopRight:
            return rect.y;
        case AnchorPoint::kLeft:
        case AnchorPoint::kCenter:
        case AnchorPoint::kRight:
            return rect.y + rect.height * 0.5f;
        default:
            return rect.y + rect.height;
    }
}

namespace {

/* 参考点在自身宽高上的比例: 左/上 0, 中 0.5, 右/下 1. */
float FractionX(AnchorPoint point) {
    switch (point) {
        case AnchorPoint::kTopLeft:
        case AnchorPoint::kLeft:
        case AnchorPoint::kBottomLeft:
            return 0.0f;
        case AnchorPoint::kTop:
        case AnchorPoint::kCenter:
        case AnchorPoint::kBottom:
            return 0.5f;
        default:
            return 1.0f;
    }
}

float FractionY(AnchorPoint point) {
    switch (point) {
        case AnchorPoint::kTopLeft:
        case AnchorPoint::kTop:
        case AnchorPoint::kTopRight:
            return 0.0f;
        case AnchorPoint::kLeft:
        case AnchorPoint::kCenter:
        case AnchorPoint::kRight:
            return 0.5f;
        default:
            return 1.0f;
    }
}

/*
 * 求解单帧: 用自己的宽高与锚点算出左上角.
 * 自身点的偏移直接按宽高比例算, 不回头读 frame.rect, 反复解算同一棵树
 * 也不会把上一次的结果带进这一次.
 */
void ResolveOne(LayoutFrame& frame, const LayoutRect& parentRect) {
    frame.rect.width = frame.width;
    frame.rect.height = frame.height;
    if (frame.anchors.empty()) {
        /* 没有锚点: 贴父帧左上角, 与布局文件缺省行为一致. */
        frame.rect.x = parentRect.x;
        frame.rect.y = parentRect.y;
        return;
    }
    const LayoutAnchor& anchor = frame.anchors.front();
    const LayoutRect& reference = anchor.relativeTo != nullptr ? anchor.relativeTo->rect : parentRect;
    const float targetX = PointX(reference, anchor.relativePoint) + anchor.offsetX;
    const float targetY = PointY(reference, anchor.relativePoint) + anchor.offsetY;
    /* 自己的 point 必须落在目标点上, 因此左上角要回退对应的分量. */
    frame.rect.x = targetX - FractionX(anchor.point) * frame.rect.width;
    frame.rect.y = targetY - FractionY(anchor.point) * frame.rect.height;
}

void ResolveChildren(LayoutFrame& frame) {
    for (LayoutFrame* child : frame.children) {
        if (child == nullptr) {
            continue;
        }
        ResolveOne(*child, frame.rect);
        ResolveChildren(*child);
    }
}

}  /* namespace */

void ResolveLayout(LayoutFrame& root, const LayoutRect& screen) {
    root.rect = screen;
    if (root.width > 0.0f) {
        root.rect.width = root.width;
    }
    if (root.height > 0.0f) {
        root.rect.height = root.height;
    }
    ResolveChildren(root);
}

}  /* namespace immirror::ui */
