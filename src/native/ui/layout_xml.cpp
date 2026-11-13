#include "ui/layout_xml.h"

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
};

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

bool LayoutDocument::Parse(const char* text, size_t length) {
    names_.clear();
    pool_.clear();
    root_ = nullptr;
    error_.clear();
    if (text == nullptr || length == 0) {
        error_ = "空文档";
        return false;
    }

    /* 帧栈与标签栈: 帧栈决定父子关系, 标签栈用来判断 AbsDimension 归属. */
    std::vector<LayoutFrame*> frameStack;
    std::vector<std::string> tagStack;
    std::vector<std::pair<LayoutFrame*, RawAnchor>> pending;
    std::string current;
    size_t index = 0;

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
                error_ = "注释未闭合";
                return false;
            }
            index += end + 3;
            continue;
        }
        if (index + 1 < length && text[index + 1] == '?') {
            const size_t end = std::string(text + index, length - index).find("?>");
            if (end == std::string::npos) {
                error_ = "处理指令未闭合";
                return false;
            }
            index += end + 2;
            continue;
        }
        const size_t close = std::string(text + index, length - index).find('>');
        if (close == std::string::npos) {
            error_ = "标签未闭合";
            return false;
        }
        const std::string body = std::string(text + index + 1, close - 1);
        index += close + 1;

        if (!body.empty() && body[0] == '/') {
            /* 结束标签: 弹栈. */
            const std::string tag = body.substr(1);
            if (!tagStack.empty()) {
                tagStack.pop_back();
            }
            if (!IsStructural(tag) && !frameStack.empty()) {
                frameStack.pop_back();
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
                for (size_t i = tagStack.size(); i > 0; --i) {
                    if (tagStack[i - 1] == "Anchor" || tagStack[i - 1] == "Size") {
                        owner = tagStack[i - 1];
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
            }
            if (!selfClosing) {
                tagStack.push_back(tag);
            }
            continue;
        }

        /* 带名字的元素才成为帧; 无名的按普通容器处理, 不进入帧树. */
        if (attributes.name.empty()) {
            if (!selfClosing) {
                tagStack.push_back(tag);
            }
            continue;
        }

        const std::string parentName = frameStack.empty() ? std::string() : frameStack.back()->name;
        names_.push_back(ExpandName(attributes.name, parentName));
        pool_.push_back(std::unique_ptr<LayoutFrame>(new LayoutFrame()));
        LayoutFrame* frame = pool_.back().get();
        frame->name = names_.back().c_str();
        if (frameStack.empty()) {
            if (root_ == nullptr) {
                root_ = frame;
            }
        } else {
            frameStack.back()->AddChild(frame);
        }
        if (!selfClosing) {
            tagStack.push_back(tag);
            frameStack.push_back(frame);
        }
    }

    if (root_ == nullptr) {
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
