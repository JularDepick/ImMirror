#include "ui/layout_draw.h"

#include <algorithm>

namespace immirror::ui {
namespace {

void Emit(const LayoutFrame& frame, int32_t depth, std::vector<LayoutDrawCommand>& out) {
    if (frame.rect.width <= 0.0f || frame.rect.height <= 0.0f) {
        return;
    }
    /* 既没有颜色也没有贴图的帧只是容器, 不产生图元. */
    if (!frame.hasColor && frame.texture == nullptr) {
        return;
    }
    LayoutDrawCommand command;
    command.rect = frame.rect;
    command.layer = frame.layer;
    command.texture = frame.texture;
    command.sprite = frame.sprite;
    command.name = frame.name;
    command.red = frame.red;
    command.green = frame.green;
    command.blue = frame.blue;
    command.alpha = frame.alpha;
    command.depth = depth;
    out.push_back(command);
}

void Walk(const LayoutFrame& frame, int32_t depth, std::vector<LayoutDrawCommand>& out) {
    Emit(frame, depth, out);

    /* 子帧按图层稳定排序: 同层保持布局里的先后, 不因排序打乱. */
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
        Walk(*child, depth + 1, out);
    }
}

}  /* namespace */

int32_t CollectDrawCommands(const LayoutFrame& root, std::vector<LayoutDrawCommand>& out) {
    out.clear();
    Walk(root, 0, out);
    return static_cast<int32_t>(out.size());
}

}  /* namespace immirror::ui */
