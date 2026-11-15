#include "ui/layout_frame.h"

#include <cmath>
#include <cstring>

namespace immirror::ui {

namespace {
/* 未设置时不做精灵尺寸补齐, 布局层因此可以独立使用. */
SpriteSizeFn g_spriteSize = nullptr;
/* 整图页尺寸解析, 未设置时不做补齐. */
TextureSizeFn g_textureSize = nullptr;
/* 缺失参照物计数, 由解算过程累加. */
int32_t g_missingReference = 0;
std::vector<std::string> g_missingReferenceNames;
}  /* namespace */

void SetSpriteSizeResolver(SpriteSizeFn fn) {
    g_spriteSize = fn;
}

void SetTextureSizeResolver(TextureSizeFn fn) {
    g_textureSize = fn;
}

int32_t MissingAnchorReferenceCount() {
    return g_missingReference;
}

void ResetMissingAnchorReferenceCount() {
    g_missingReference = 0;
    g_missingReferenceNames.clear();
}

const std::vector<std::string>& MissingAnchorReferenceNames() {
    return g_missingReferenceNames;
}

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
    /*
     * 相对尺寸按父帧比例折算, 绝对尺寸优先.
     * 两者都没写的容器帧隐式铺满父帧: 布局里像 GameSetFrame 这类纯容器不给 Size,
     * 子元素却按父帧比例定位, 若容器按零尺寸处理, 整棵子树的位置都会算成负值.
     */
    /*
     * 尺寸来源按优先级: 自身绝对尺寸 > 自身相对尺寸 > 双锚点距离 > 铺满父帧.
     * 双锚点必须排在铺满之前: 先铺满会让"两侧锚点定宽"那段判断失效,
     * 拉伸元素的尺寸就永远取父帧尺寸, 而不是锚点之间的距离.
     */
    frame.rect.width = frame.width > 0.0f
                           ? frame.width
                           : (frame.relWidth > 0.0f ? frame.relWidth * parentRect.width : 0.0f);
    frame.rect.height = frame.height > 0.0f
                            ? frame.height
                            : (frame.relHeight > 0.0f ? frame.relHeight * parentRect.height : 0.0f);
    /*
     * 双锚点定尺寸: 布局里常见"左边贴父帧左边, 右边贴父帧右边"的写法,
     * 此时宽高由两侧锚点的距离决定, 元素本身不给 Size.
     */
    if (frame.rect.width <= 0.0f || frame.rect.height <= 0.0f) {
        const LayoutAnchor* left = nullptr;
        const LayoutAnchor* right = nullptr;
        const LayoutAnchor* top = nullptr;
        const LayoutAnchor* bottom = nullptr;
        for (const LayoutAnchor& anchor : frame.anchors) {
            const float fx = FractionX(anchor.point);
            const float fy = FractionY(anchor.point);
            if (fx <= 0.0f && left == nullptr) {
                left = &anchor;
            }
            if (fx >= 1.0f && right == nullptr) {
                right = &anchor;
            }
            if (fy <= 0.0f && top == nullptr) {
                top = &anchor;
            }
            if (fy >= 1.0f && bottom == nullptr) {
                bottom = &anchor;
            }
        }
        if (frame.rect.width <= 0.0f && left != nullptr && right != nullptr) {
            const LayoutRect& leftRef = left->relativeTo != nullptr ? left->relativeTo->rect : parentRect;
            const LayoutRect& rightRef =
                right->relativeTo != nullptr ? right->relativeTo->rect : parentRect;
            const float x0 = PointX(leftRef, left->relativePoint) + left->offsetX;
            const float x1 = PointX(rightRef, right->relativePoint) + right->offsetX;
            if (x1 > x0) {
                frame.rect.width = x1 - x0;
            }
        }
        if (frame.rect.height <= 0.0f && top != nullptr && bottom != nullptr) {
            const LayoutRect& topRef = top->relativeTo != nullptr ? top->relativeTo->rect : parentRect;
            const LayoutRect& bottomRef =
                bottom->relativeTo != nullptr ? bottom->relativeTo->rect : parentRect;
            const float y0 = PointY(topRef, top->relativePoint) + top->offsetY;
            const float y1 = PointY(bottomRef, bottom->relativePoint) + bottom->offsetY;
            if (y1 > y0) {
                frame.rect.height = y1 - y0;
            }
        }
    }
    /* 布局不给尺寸时按精灵原始像素补齐, 这是原版 Texture 的常见写法. */
    if ((frame.rect.width <= 0.0f || frame.rect.height <= 0.0f) && frame.sprite != nullptr &&
        g_spriteSize != nullptr) {
        float spriteWidth = 0.0f;
        float spriteHeight = 0.0f;
        if (g_spriteSize(frame.sprite, &spriteWidth, &spriteHeight)) {
            if (frame.rect.width <= 0.0f) {
                frame.rect.width = spriteWidth;
            }
            if (frame.rect.height <= 0.0f) {
                frame.rect.height = spriteHeight;
            }
        }
    }
    /*
     * 文本自动尺寸: 原版的 FontString 不写 Size, 宽高由文字与字号决定.
     * 点阵字只有 ASCII, 这里按字号的半宽估算每个字符的步进.
     */
    if (frame.text != nullptr && frame.text[0] != '\0') {
        const float fontSize = frame.fontSize > 0.0f ? frame.fontSize : 20.0f;
        if (frame.rect.height <= 0.0f) {
            frame.rect.height = fontSize;
        }
        if (frame.rect.width <= 0.0f) {
            frame.rect.width = 0.5f * fontSize * static_cast<float>(std::strlen(frame.text));
        }
    }
    /*
     * 拉伸边缘: 没有显式尺寸与锚点时铺满父帧.
     * 背景蒙版、整屏遮罩这类元素只写 stretchedge="true", 靠这条规则定位.
     */
    if (frame.stretchEdge && frame.width <= 0.0f && frame.relWidth <= 0.0f &&
        frame.height <= 0.0f && frame.relHeight <= 0.0f && frame.anchors.empty()) {
        frame.rect = parentRect;
        for (LayoutFrame* child : frame.children) {
            if (child != nullptr) {
                ResolveLayout(*child, frame.rect);
            }
        }
        return;
    }

    /* 只给贴图的元素按整图像素尺寸补齐(原版整图默认按图片大小绘制). */
    if ((frame.rect.width <= 0.0f || frame.rect.height <= 0.0f) && frame.texture != nullptr &&
        frame.sprite == nullptr && g_textureSize != nullptr) {
        float textureWidth = 0.0f;
        float textureHeight = 0.0f;
        if (g_textureSize(frame.texture, &textureWidth, &textureHeight)) {
            if (frame.rect.width <= 0.0f) {
                frame.rect.width = textureWidth;
            }
            if (frame.rect.height <= 0.0f) {
                frame.rect.height = textureHeight;
            }
        }
    }
    /*
     * 跨主帧层级向下继承: 布局只在主帧上写 frameStrata, 子元素不重复写.
     * 子帧保持默认值时沿用父帧的层级, 否则提示层里的子元素会掉回中层被盖住.
     */
    if (frame.strata == 1 && frame.parent != nullptr && frame.parent->strata != 1) {
        frame.strata = frame.parent->strata;
    }
    /*
     * 所有尺寸来源都没给出时, 容器帧隐式铺满父帧.
     * 布局里像 GameSetFrame 这类纯容器不给 Size, 子元素却按父帧比例定位,
     * 若容器按零尺寸处理, 整棵子树的位置都会算成负值.
     */
    if (frame.rect.width <= 0.0f) {
        frame.rect.width = parentRect.width;
    }
    if (frame.rect.height <= 0.0f) {
        frame.rect.height = parentRect.height;
    }

    if (frame.anchors.empty()) {
        /* 没有锚点: 贴父帧左上角, 与布局文件缺省行为一致. */
        frame.rect.x = parentRect.x;
        frame.rect.y = parentRect.y;
        return;
    }
    const LayoutAnchor& anchor = frame.anchors.front();
    if (anchor.relativeTo == nullptr && anchor.relativeName != nullptr &&
        anchor.relativeName[0] != '\0') {
        /* 名字写了但没解析到控件: 记为缺失参照物, 便于转储报告. */
        g_missingReference += 1;
        if (g_missingReferenceNames.size() < 64) {
            g_missingReferenceNames.push_back(anchor.relativeName);
        }
    }
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
    /*
     * 流式布局: 子元素按常规解算拿到自身尺寸后, 由容器覆盖位置.
     * 必须放在递归之后, 否则会被子元素的常规解算覆盖掉.
     */
    if (frame.layoutStyle == nullptr || frame.layoutStyle[0] == '\0' || frame.children.empty()) {
        return;
    }
    const std::string style(frame.layoutStyle);
    float cursorX = frame.rect.x;
    float cursorY = frame.rect.y;
    for (LayoutFrame* child : frame.children) {
        if (child == nullptr) {
            continue;
        }
        if (style == "horizontal_left" || style == "horizontal_right") {
            child->rect.y = frame.rect.y;
            if (style == "horizontal_left") {
                child->rect.x = cursorX;
                cursorX += child->rect.width + frame.spaceX;
            } else {
                cursorX -= child->rect.width + frame.spaceX;
                child->rect.x = cursorX;
            }
        } else if (style == "vertical_top" || style == "vertical_bottom") {
            child->rect.x = frame.rect.x;
            if (style == "vertical_top") {
                child->rect.y = cursorY;
                cursorY += child->rect.height + frame.spaceY;
            } else {
                cursorY -= child->rect.height + frame.spaceY;
                child->rect.y = cursorY;
            }
        } else {
            if (cursorX + child->rect.width > frame.rect.x + frame.rect.width &&
                cursorX > frame.rect.x) {
                cursorX = frame.rect.x;
                cursorY += child->rect.height + frame.spaceY;
            }
            child->rect.x = cursorX;
            child->rect.y = cursorY;
            cursorX += child->rect.width + frame.spaceX;
        }
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
