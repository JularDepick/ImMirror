#include "ui/widget_tree.h"

#include <algorithm>
#include <cstring>

namespace immirror::ui {
namespace {

/* 深度优先收集帧指针, 保证控件顺序与布局一致. */
void CollectFrames(LayoutFrame* frame, std::vector<LayoutFrame*>& out) {
    if (frame == nullptr) {
        return;
    }
    out.push_back(frame);
    for (LayoutFrame* child : frame->children) {
        CollectFrames(child, out);
    }
}

}  /* namespace */

bool WidgetTree::Load(const char* text, size_t length) {
    widgets_.clear();
    byName_.clear();
    byFrame_.clear();
    root_ = nullptr;
    if (!document_.Parse(text, length)) {
        return false;
    }
    root_ = document_.Root();
    if (root_ == nullptr) {
        return false;
    }
    std::vector<LayoutFrame*> frames;
    CollectFrames(root_, frames);
    widgets_.reserve(frames.size());
    for (LayoutFrame* frame : frames) {
        widgets_.push_back(std::unique_ptr<Widget>(new Widget()));
        Widget* widget = widgets_.back().get();
        widget->frame = frame;
        if (frame->name != nullptr) {
            byName_[frame->name] = widget;
        }
        byFrame_[frame] = widget;
    }
    return true;
}

void WidgetTree::Resolve(const LayoutRect& screen) {
    if (root_ == nullptr) {
        return;
    }
    /* 先把运行时覆盖写回帧, 再统一解算, 保证覆盖对子帧同样有效. */
    for (const std::unique_ptr<Widget>& widget : widgets_) {
        if (widget == nullptr || widget->frame == nullptr) {
            continue;
        }
        if (widget->hasSize) {
            widget->frame->width = widget->width;
            widget->frame->height = widget->height;
        }
        if (widget->hasAnchor) {
            if (widget->frame->anchors.empty()) {
                widget->frame->anchors.push_back(widget->anchor);
            } else {
                widget->frame->anchors[0] = widget->anchor;
            }
        }
    }
    ResolveLayout(*root_, screen);
}

void WidgetTree::SetText(Widget* widget, const char* text) {
    if (widget == nullptr) {
        return;
    }
    widget->text = text == nullptr ? std::string() : std::string(text);
}

void WidgetTree::SetTextColor(Widget* widget, float red, float green, float blue) {
    if (widget == nullptr) {
        return;
    }
    widget->textRed = red;
    widget->textGreen = green;
    widget->textBlue = blue;
}

void WidgetTree::SetSize(Widget* widget, float width, float height) {
    if (widget == nullptr) {
        return;
    }
    widget->hasSize = true;
    widget->width = width;
    widget->height = height;
}

void WidgetTree::SetPoint(Widget* widget, AnchorPoint point, AnchorPoint relativePoint,
                          float offsetX, float offsetY) {
    if (widget == nullptr) {
        return;
    }
    widget->hasAnchor = true;
    widget->anchor.point = point;
    /* 相对帧留空表示相对父帧, 与布局里不写 relativeTo 的语义一致. */
    widget->anchor.relativeTo = widget->frame != nullptr ? widget->frame->parent : nullptr;
    widget->anchor.relativePoint = relativePoint;
    widget->anchor.offsetX = offsetX;
    widget->anchor.offsetY = offsetY;
}

void WidgetTree::SetTexture(Widget* widget, const char* texture) {
    if (widget == nullptr) {
        return;
    }
    widget->texture = texture == nullptr ? std::string() : std::string(texture);
}

const Widget* WidgetTree::Lookup(const LayoutFrame* frame) const {
    const auto found = byFrame_.find(frame);
    return found == byFrame_.end() ? nullptr : found->second;
}

namespace {

void WalkVisible(const LayoutFrame& frame, const std::unordered_map<const LayoutFrame*, Widget*>& map,
                 int32_t depth, std::vector<LayoutDrawCommand>& out) {
    const auto found = map.find(&frame);
    const Widget* widget = found == map.end() ? nullptr : found->second;
    if (widget != nullptr && !widget->shown) {
        /* 隐藏的控件连同子树一起跳过. */
        return;
    }

    const bool sizeOk = frame.rect.width > 0.0f && frame.rect.height > 0.0f;
    const bool hasTexture = widget != nullptr && !widget->texture.empty();
    const bool hasText = widget != nullptr && !widget->text.empty();
    if (sizeOk && (frame.hasColor || frame.texture != nullptr || hasTexture || hasText)) {
        LayoutDrawCommand command;
        command.rect = frame.rect;
        command.layer = frame.layer;
        command.name = frame.name;
        command.depth = depth;
        command.texture = hasTexture ? widget->texture.c_str() : frame.texture;
        command.text = hasText ? widget->text.c_str() : nullptr;
        if (hasText) {
            command.red = widget->textRed;
            command.green = widget->textGreen;
            command.blue = widget->textBlue;
        } else {
            command.red = frame.red;
            command.green = frame.green;
            command.blue = frame.blue;
        }
        command.alpha = frame.alpha;
        if (widget != nullptr && widget->grayed) {
            /* 置灰: 压暗并降饱和, 仍按原透明度绘制. */
            command.red *= 0.55f;
            command.green *= 0.55f;
            command.blue *= 0.55f;
        }
        out.push_back(command);
    }

    std::vector<const LayoutFrame*> ordered;
    ordered.reserve(frame.children.size());
    for (const LayoutFrame* child : frame.children) {
        if (child != nullptr) {
            ordered.push_back(child);
        }
    }
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const LayoutFrame* left, const LayoutFrame* right) {
                         return static_cast<int32_t>(left->layer) <
                                static_cast<int32_t>(right->layer);
                     });
    for (const LayoutFrame* child : ordered) {
        WalkVisible(*child, map, depth + 1, out);
    }
}

}  /* namespace */

namespace {

void HitWalk(LayoutFrame& frame, std::unordered_map<const LayoutFrame*, Widget*>& map, float x,
             float y, Widget*& found) {
    const auto entry = map.find(&frame);
    Widget* widget = entry == map.end() ? nullptr : entry->second;
    if (widget != nullptr && !widget->shown) {
        return;
    }
    /* 子帧先测, 同级按图层从顶到底, 因此用逆序. */
    std::vector<LayoutFrame*> ordered;
    ordered.reserve(frame.children.size());
    for (LayoutFrame* child : frame.children) {
        if (child != nullptr) {
            ordered.push_back(child);
        }
    }
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const LayoutFrame* left, const LayoutFrame* right) {
                         return static_cast<int32_t>(left->layer) >
                                static_cast<int32_t>(right->layer);
                     });
    for (LayoutFrame* child : ordered) {
        HitWalk(*child, map, x, y, found);
        if (found != nullptr) {
            return;
        }
    }
    if (widget == nullptr) {
        return;
    }
    const LayoutRect& rect = frame.rect;
    if (x >= rect.x && x <= rect.x + rect.width && y >= rect.y && y <= rect.y + rect.height) {
        found = widget;
    }
}

}  /* namespace */

Widget* WidgetTree::HitTest(float x, float y) {
    if (root_ == nullptr) {
        return nullptr;
    }
    Widget* found = nullptr;
    HitWalk(*root_, byFrame_, x, y, found);
    return found;
}

int32_t WidgetTree::CollectDrawCommands(std::vector<LayoutDrawCommand>& out) const {
    out.clear();
    if (root_ == nullptr) {
        return 0;
    }
    WalkVisible(*root_, byFrame_, 0, out);
    return static_cast<int32_t>(out.size());
}

}  /* namespace immirror::ui */
