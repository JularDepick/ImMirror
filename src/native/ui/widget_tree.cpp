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

namespace {

/* 递归复制一棵子树; map 记录源帧到新帧的对应, 供锚点重定向. */
void CloneSubtree(LayoutDocument& document, const LayoutFrame& source, LayoutFrame* parent,
                  const std::string& name,
                  std::unordered_map<const LayoutFrame*, LayoutFrame*>& map) {
    LayoutFrame* copy = document.CreateFrame(name);
    copy->width = source.width;
    copy->height = source.height;
    copy->hasColor = source.hasColor;
    copy->red = source.red;
    copy->green = source.green;
    copy->blue = source.blue;
    copy->alpha = source.alpha;
    copy->texture = source.texture;
    copy->layer = source.layer;
    if (parent != nullptr) {
        parent->AddChild(copy);
    }
    map[&source] = copy;
    for (const LayoutFrame* child : source.children) {
        if (child == nullptr) {
            continue;
        }
        /* 子控件名字保持原样, 由调用方在最外层加前缀. */
        CloneSubtree(document, *child, copy, child->name == nullptr ? "" : child->name, map);
    }
    /* 锚点单独补: 相对帧可能指向子树内部, 需要换成新帧. */
    for (const LayoutAnchor& anchor : source.anchors) {
        LayoutAnchor copyAnchor = anchor;
        const auto found = anchor.relativeTo == nullptr ? map.end() : map.find(anchor.relativeTo);
        if (found != map.end()) {
            copyAnchor.relativeTo = found->second;
        }
        copy->anchors.push_back(copyAnchor);
    }
}

}  /* namespace */

Widget* WidgetTree::Clone(Widget* source, Widget* parent, const char* newName) {
    if (source == nullptr || source->frame == nullptr || newName == nullptr || newName[0] == '\0') {
        return nullptr;
    }
    if (Find(newName) != nullptr) {
        return nullptr;
    }
    std::unordered_map<const LayoutFrame*, LayoutFrame*> map;
    CloneSubtree(document_, *source->frame, parent == nullptr ? nullptr : parent->frame, newName, map);
    if (map.empty()) {
        return nullptr;
    }
    /* 根帧按源帧查回来, 不能依赖容器遍历顺序. */
    const auto rootFound = map.find(source->frame);
    if (rootFound == map.end()) {
        return nullptr;
    }
    LayoutFrame* rootFrame = rootFound->second;
    Widget* root = nullptr;
    /* 为新帧建控件: 名字用新帧自己的名字, 子孙再补上前缀. */
    for (const auto& entry : map) {
        LayoutFrame* frame = entry.second;
        widgets_.push_back(std::unique_ptr<Widget>(new Widget()));
        Widget* widget = widgets_.back().get();
        widget->frame = frame;
        std::string fullName = frame->name == nullptr ? std::string() : std::string(frame->name);
        if (frame != rootFrame && !fullName.empty()) {
            fullName = std::string(newName) + "/" + fullName;
        }
        byName_[fullName] = widget;
        if (frame == rootFrame) {
            root = widget;
        }
    }
    return root;
}

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
        /* 布局里标了隐藏的控件载入时不显示, 由脚本按需打开. */
        widget->shown = !frame->hidden;
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

void WidgetTree::SetPointTo(Widget* widget, AnchorPoint point, const char* relativeToName,
                             AnchorPoint relativePoint, float offsetX, float offsetY) {
    if (widget == nullptr) {
        return;
    }
    widget->hasAnchor = true;
    widget->anchor.point = point;
    const LayoutFrame* target = nullptr;
    if (relativeToName != nullptr && relativeToName[0] != '\0') {
        Widget* reference = Find(relativeToName);
        target = reference != nullptr ? reference->frame : nullptr;
    }
    widget->anchor.relativeTo =
        target != nullptr ? target : (widget->frame != nullptr ? widget->frame->parent : nullptr);
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

    /* virtual="true" 的帧只是模板, 自身与子树都不绘制. */
    if (frame.isVirtual) {
        return;
    }

    const bool sizeOk = frame.rect.width > 0.0f && frame.rect.height > 0.0f;
    /*
     * 按钮三态: 按下时优先用 PushedTexture, 否则用普通态;
     * 元素自身写了贴图的, 以元素为准.
     */
    /* 选中态优先于按下态: 页签被选中后应保持选中贴图. */
    const bool pressed = widget != nullptr && widget->pressed;
    const bool checked = widget != nullptr && widget->checked && frame.checkedTexture != nullptr;
    const char* stateTexture = checked ? frame.checkedTexture
                                       : (pressed ? frame.pushedTexture : frame.normalTexture);
    const char* stateSprite = checked ? frame.checkedSprite
                                      : (pressed ? frame.pushedSprite : frame.normalSprite);
    const char* baseTexture = frame.texture != nullptr ? frame.texture : stateTexture;
    const char* baseSprite = frame.sprite != nullptr ? frame.sprite : stateSprite;
    const bool hasTexture = widget != nullptr && !widget->texture.empty();
    const bool hasText = widget != nullptr && !widget->text.empty();
    /* 布局里的 text 属性是文本控件的初始内容, 运行时没有覆盖时用它. */
    const bool frameHasText = frame.text != nullptr && frame.text[0] != '\0';
    if (sizeOk && (frame.hasColor || baseTexture != nullptr || hasTexture || hasText ||
                   frameHasText)) {
        LayoutDrawCommand command;
        command.rect = frame.rect;
        command.layer = frame.layer;
        command.name = frame.name;
        command.depth = depth;
        command.texture = hasTexture ? widget->texture.c_str() : baseTexture;
        /* 精灵名决定取图集里的哪一格, 必须一并带出去, 否则只能按整图绘制. */
        command.sprite = baseSprite;
        command.fontSize = frame.fontSize;
        command.justifyH = frame.justifyH;
        command.justifyV = frame.justifyV;
        command.lineInterval = frame.lineInterval;
        /*
         * 绘制参数必须在这里一并带出去: 控件树这条路径与布局树那条路径
         * 各产出一份命令, 只在其中一条加字段会让渲染阶段拿不到参数.
         */
        command.tileOffsetX = frame.tileOffsetX;
        command.tileOffsetY = frame.tileOffsetY;
        command.tileWidth = frame.tileWidth;
        command.tileHeight = frame.tileHeight;
        command.outline = frame.outline;
        command.multiLine = frame.multiLine;
        command.additive = frame.additive;
        command.shadowRed = frame.shadowRed;
        command.shadowGreen = frame.shadowGreen;
        command.shadowBlue = frame.shadowBlue;
        command.flipU = frame.flipU;
        command.flipV = frame.flipV;
        command.rotateQuarter = frame.rotateQuarter;
        command.bold = frame.bold;
        command.strata = frame.strata;
        command.insetLeft = frame.insetLeft;
        command.insetRight = frame.insetRight;
        command.insetTop = frame.insetTop;
        command.insetBottom = frame.insetBottom;
        command.text = hasText ? widget->text.c_str() : (frameHasText ? frame.text : nullptr);
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
    /* 断行结果的存放区按帧清理, 避免长期运行累积. */
    ResetTextLinePool();
    if (root_ == nullptr) {
        return 0;
    }
    WalkVisible(*root_, byFrame_, 0, out);
    /*
     * 跨主帧层级: 遍历产出的是"按帧递归、同帧内按图层排好"的顺序,
     * 但不同主帧之间还要按层级分先后 —— 提示层必须压在全屏层之上.
     * 用稳定排序保住同层内的原有顺序.
     */
    std::stable_sort(out.begin(), out.end(),
                     [](const LayoutDrawCommand& left, const LayoutDrawCommand& right) {
                         return left.strata < right.strata;
                     });
    return static_cast<int32_t>(out.size());
}

int32_t WidgetTree::CollectDrawCommandsFrom(const char* name,
                                           std::vector<LayoutDrawCommand>& out) const {
    out.clear();
    if (name == nullptr) {
        return 0;
    }
    const Widget* target = Find(name);
    if (target == nullptr || target->frame == nullptr) {
        return 0;
    }
    ResetTextLinePool();
    WalkVisible(*target->frame, byFrame_, 0, out);
    return static_cast<int32_t>(out.size());
}

}  /* namespace immirror::ui */
