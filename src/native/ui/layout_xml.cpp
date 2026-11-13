#include "ui/layout_xml.h"

#include <string>

#include <cstdlib>
#include <cstring>

namespace immirror::ui {
namespace {

/* 解析期暂存的锚点: 名字还是文本, 待全部帧建好后再解析成指针. */
struct RawAnchor {
    std::string point;
    std::string relativePoint;
    std::string relativeTo;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
};

/* 一个元素的属性表, 只保留解析用得到的几项. */
struct Attributes {
    std::string name;
    std::string point;
    std::string relativePoint;
    std::string relativeTo;
    std::string x;
    std::string y;
    std::string r;
    std::string g;
    std::string b;
    std::string a;
    std::string file;
    std::string level;
    /* hidden="true": 布局载入时默认不显示, 由脚本按需打开. */
    std::string hidden;
};

/* 图层名转枚举; 未知名按 artwork 处理. */
LayerLevel ParseLayerLevel(const std::string& level) {
    if (level == "background") {
        return LayerLevel::kBackground;
    }
    if (level == "border") {
        return LayerLevel::kBorders;
    }
    if (level == "artwork") {
        return LayerLevel::kArtwork;
    }
    if (level == "overlay") {
        return LayerLevel::kOverlay;
    }
    if (level == "highlight") {
        return LayerLevel::kHighlight;
    }
    return LayerLevel::kArtwork;
}

/* 结构性元素: 只承载几何与层级, 自身不是帧. */
bool IsStructural(const std::string& tag) {
    static const char* const kTags[] = {"Ui",     "Size",   "AbsDimension", "Anchors",
                                        "Anchor", "Offset", "Layers",       "Layer",
                                        "Color",  "Scripts", "Script",      "Accel",
                                        "Highlight", "Shadow", "NormalTexture"};
    for (const char* known : kTags) {
        if (tag == known) {
            return true;
        }
    }
    return false;
}

bool IsSpace(char value) {
    return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

/* 把 $parent 展开成给定的父名, 可能出现多次. */
std::string ExpandName(const std::string& raw, const std::string& parentName) {
    std::string out;
    size_t index = 0;
    while (index < raw.size()) {
        if (raw.compare(index, 7, "$parent") == 0) {
            out += parentName;
            index += 7;
            continue;
        }
        out.push_back(raw[index]);
        index += 1;
    }
    return out;
}

float ToFloat(const std::string& text) {
    if (text.empty()) {
        return 0.0f;
    }
    return static_cast<float>(std::atof(text.c_str()));
}

/* 从 <Tag a="1" b="2"> 里取属性; 只认解析需要的几个键. */
Attributes ParseAttributes(const std::string& body) {
    Attributes attributes;
    size_t index = 0;
    while (index < body.size()) {
        while (index < body.size() && IsSpace(body[index])) {
            ++index;
        }
        const size_t keyStart = index;
        while (index < body.size() && body[index] != '=' && !IsSpace(body[index])) {
            ++index;
        }
        const std::string key = body.substr(keyStart, index - keyStart);
        if (index >= body.size() || body[index] != '=') {
            continue;
        }
        ++index;
        if (index >= body.size() || body[index] != '"') {
            continue;
        }
        const size_t valueStart = ++index;
        while (index < body.size() && body[index] != '"') {
            ++index;
        }
        const std::string value = body.substr(valueStart, index - valueStart);
        if (index < body.size()) {
            ++index;
        }
        if (key == "name") {
            attributes.name = value;
        } else if (key == "point") {
            attributes.point = value;
        } else if (key == "relativePoint") {
            attributes.relativePoint = value;
        } else if (key == "relativeTo") {
            attributes.relativeTo = value;
        } else if (key == "x") {
            attributes.x = value;
        } else if (key == "y") {
            attributes.y = value;
        } else if (key == "r") {
            attributes.r = value;
        } else if (key == "g") {
            attributes.g = value;
        } else if (key == "b") {
            attributes.b = value;
        } else if (key == "a") {
            attributes.a = value;
        } else if (key == "file") {
            attributes.file = value;
        } else if (key == "level") {
            attributes.level = value;
        } else if (key == "hidden") {
            attributes.hidden = value;
        }
    }
    return attributes;
}

}  /* namespace */

bool ParseAnchorPoint(const std::string& text, AnchorPoint& out) {
    if (text == "topleft") {
        out = AnchorPoint::kTopLeft;
    } else if (text == "top") {
        out = AnchorPoint::kTop;
    } else if (text == "topright") {
        out = AnchorPoint::kTopRight;
    } else if (text == "left") {
        out = AnchorPoint::kLeft;
    } else if (text == "center") {
        out = AnchorPoint::kCenter;
    } else if (text == "right") {
        out = AnchorPoint::kRight;
    } else if (text == "bottomleft") {
        out = AnchorPoint::kBottomLeft;
    } else if (text == "bottom") {
        out = AnchorPoint::kBottom;
    } else if (text == "bottomright") {
        out = AnchorPoint::kBottomRight;
    } else {
        return false;
    }
    return true;
}

LayoutFrame* LayoutDocument::Find(const char* name) const {
    if (name == nullptr) {
        return nullptr;
    }
    for (const std::unique_ptr<LayoutFrame>& frame : pool_) {
        if (frame != nullptr && frame->name != nullptr && std::strcmp(frame->name, name) == 0) {
            return frame.get();
        }
    }
    return nullptr;
}

LayoutFrame* LayoutDocument::CreateFrame(const std::string& name) {
    names_.push_back(name);
    pool_.push_back(std::unique_ptr<LayoutFrame>(new LayoutFrame()));
    LayoutFrame* frame = pool_.back().get();
    frame->name = names_.back().c_str();
    return frame;
}

bool LayoutDocument::Parse(const char* text, size_t length) {
    names_.clear();
    textures_.clear();
    pool_.clear();
    root_ = nullptr;
    error_.clear();
    if (text == nullptr || length == 0) {
        error_ = "空文档";
        return false;
    }

    /*
     * 帧栈决定父子关系; 配对栈记录每个未闭合标签是否真的建了帧.
     * 弹栈必须与压栈严格对应: 无名的容器标签不建帧, 若也去弹帧栈,
     * 就会把父帧甚至合成根弹掉, 后续建帧时读到空栈.
     */
    std::vector<LayoutFrame*> frameStack;
    std::vector<std::pair<std::string, bool>> openTags;
    std::vector<LayerLevel> layerStack;
    std::vector<std::pair<LayoutFrame*, RawAnchor>> pending;
    std::string current;

    /*
     * 合成根: 布局里可以平铺多个顶层帧, 需要一个统一父节点把它们收在一起,
     * 否则除第一个之外的同级顶层帧会被丢掉.
     */
    names_.push_back(std::string());
    pool_.push_back(std::unique_ptr<LayoutFrame>(new LayoutFrame()));
    root_ = pool_.back().get();
    root_->name = names_.back().c_str();
    frameStack.push_back(root_);

    size_t index = 0;

    /*
     * 报错带上行号: 布局文件动辄上千行, 只说"标签未闭合"几乎无法定位.
     * 行号按当前扫描位置数换行符得到, 不需要额外维护计数.
     */
    const auto failAt = [this, text, length](size_t offset, const char* reason) {
        int32_t line = 1;
        const size_t limit = offset < length ? offset : length;
        for (size_t position = 0; position < limit; ++position) {
            if (text[position] == '\n') {
                ++line;
            }
        }
        error_ = "第 " + std::to_string(line) + " 行: " + reason;
        return false;
    };

    while (index < length) {
        if (text[index] != '<') {
            ++index;
            continue;
        }
        /* 注释与处理指令整段跳过. */
        if (index + 3 < length && text[index + 1] == '!' && text[index + 2] == '-' &&
            text[index + 3] == '-') {
            const size_t end = std::string(text + index, length - index).find("-->");
            if (end == std::string::npos) {
                return failAt(index, "注释未闭合");
            }
            index += end + 3;
            continue;
        }
        if (index + 1 < length && text[index + 1] == '?') {
            const size_t end = std::string(text + index, length - index).find("?>");
            if (end == std::string::npos) {
                return failAt(index, "处理指令未闭合");
            }
            index += end + 2;
            continue;
        }
        const size_t close = std::string(text + index, length - index).find('>');
        if (close == std::string::npos) {
            return failAt(index, "标签未闭合");
        }
        const std::string body = std::string(text + index + 1, close - 1);
        index += close + 1;

        if (!body.empty() && body[0] == '/') {
            /* 结束标签: 弹栈. */
            const std::string tag = body.substr(1);
            if (tag == "Layer" && !layerStack.empty()) {
                layerStack.pop_back();
            }
            if (!openTags.empty()) {
                /* 只有压栈时真的建过帧的标签才弹帧栈. */
                if (openTags.back().second && !frameStack.empty()) {
                    frameStack.pop_back();
                }
                openTags.pop_back();
            }
            continue;
        }

        const bool selfClosing = !body.empty() && body[body.size() - 1] == '/';
        std::string head = selfClosing ? body.substr(0, body.size() - 1) : body;
        size_t headEnd = 0;
        while (headEnd < head.size() && !IsSpace(head[headEnd])) {
            ++headEnd;
        }
        const std::string tag = head.substr(0, headEnd);
        const Attributes attributes = ParseAttributes(head.substr(headEnd));

        if (IsStructural(tag)) {
            if (tag == "Anchor" && !frameStack.empty()) {
                RawAnchor anchor;
                anchor.point = attributes.point;
                anchor.relativePoint = attributes.relativePoint;
                anchor.relativeTo = attributes.relativeTo;
                pending.push_back(std::make_pair(frameStack.back(), anchor));
            } else if (tag == "AbsDimension") {
                /* 归属看最近的 Anchor 或 Size: 前者是偏移, 后者是尺寸. */
                std::string owner;
                for (size_t i = openTags.size(); i > 0; --i) {
                    if (openTags[i - 1].first == "Anchor" || openTags[i - 1].first == "Size") {
                        owner = openTags[i - 1].first;
                        break;
                    }
                }
                if (owner == "Anchor" && !pending.empty()) {
                    pending.back().second.offsetX = ToFloat(attributes.x);
                    pending.back().second.offsetY = ToFloat(attributes.y);
                } else if (owner == "Size" && !frameStack.empty()) {
                    frameStack.back()->width = ToFloat(attributes.x);
                    frameStack.back()->height = ToFloat(attributes.y);
                }
            } else if (tag == "Color" && !frameStack.empty()) {
                /* 颜色取最近一帧; 分量按 0 到 255 给出, 转成 0 到 1. */
                LayoutFrame* painted = frameStack.back();
                painted->hasColor = true;
                painted->red = ToFloat(attributes.r) / 255.0f;
                painted->green = ToFloat(attributes.g) / 255.0f;
                painted->blue = ToFloat(attributes.b) / 255.0f;
                painted->alpha = attributes.a.empty() ? 1.0f : ToFloat(attributes.a) / 255.0f;
            } else if (tag == "Layer") {
                layerStack.push_back(ParseLayerLevel(attributes.level));
            }
            if (!selfClosing) {
                openTags.push_back(std::make_pair(tag, false));
            }
            continue;
        }

        /* 带名字的元素才成为帧; 无名的按普通容器处理, 不进入帧树. */
        if (attributes.name.empty()) {
            if (!selfClosing) {
                openTags.push_back(std::make_pair(tag, false));
            }
            continue;
        }

        const std::string parentName = frameStack.empty() ? std::string() : frameStack.back()->name;
        names_.push_back(ExpandName(attributes.name, parentName));
        pool_.push_back(std::unique_ptr<LayoutFrame>(new LayoutFrame()));
        LayoutFrame* frame = pool_.back().get();
        frame->name = names_.back().c_str();
        /* 所在图层决定绘制顺序; 贴图路径来自元素自身的 file 属性. */
        frame->layer = layerStack.empty() ? LayerLevel::kArtwork : layerStack.back();
        /* hidden="true" 的帧载入时不显示, 与脚本里的 Show/Hide 配合. */
        frame->hidden = attributes.hidden == "true";
        if (!attributes.file.empty()) {
            textures_.push_back(attributes.file);
            frame->texture = textures_.back().c_str();
        }
        if (!frameStack.empty()) {
            frameStack.back()->AddChild(frame);
        }
        if (!selfClosing) {
            openTags.push_back(std::make_pair(tag, true));
            frameStack.push_back(frame);
        }
    }

    if (root_ == nullptr || root_->children.empty()) {
        /* 只有合成根说明这份 XML 不是布局, 例如资源清单或按键表. */
        error_ = "没有帧节点";
        return false;
    }

    /* 锚点二段解析: 此时全部帧已就位, relativeTo 可以跨层级引用. */
    for (const std::pair<LayoutFrame*, RawAnchor>& entry : pending) {
        LayoutFrame* frame = entry.first;
        const RawAnchor& raw = entry.second;
        LayoutAnchor anchor;
        if (!ParseAnchorPoint(raw.point, anchor.point)) {
            anchor.point = AnchorPoint::kTopLeft;
        }
        if (!ParseAnchorPoint(raw.relativePoint, anchor.relativePoint)) {
            anchor.relativePoint = AnchorPoint::kTopLeft;
        }
        const std::string parentName =
            frame->parent == nullptr ? std::string() : std::string(frame->parent->name);
        const std::string target = ExpandName(raw.relativeTo, parentName);
        anchor.relativeTo = target.empty() ? nullptr : Find(target.c_str());
        anchor.offsetX = raw.offsetX;
        anchor.offsetY = raw.offsetY;
        frame->anchors.push_back(anchor);
    }
    return true;
}

}  /* namespace immirror::ui */
