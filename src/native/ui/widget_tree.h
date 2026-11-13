#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ui/layout_draw.h"
#include "ui/layout_frame.h"
#include "ui/layout_xml.h"

namespace immirror::ui {

/*
 * 控件: 帧的运行时状态.
 *
 * 布局只给出骨架, 显示与否, 文本, 尺寸与锚点都可能在运行时被改写,
 * 因此把这些可变量单独放在控件上, 帧本身保持布局里的原始数据不变.
 */
struct Widget {
    LayoutFrame* frame = nullptr;
    bool shown = true;
    bool grayed = false;
    /* 运行时文本, 空表示沿用布局里的文本 (本实现只存文本, 不做字体排版). */
    std::string text;
    /* 运行时贴图覆盖, 空表示沿用布局里的贴图. */
    std::string texture;
    float textRed = 1.0f;
    float textGreen = 1.0f;
    float textBlue = 1.0f;
    /* 显式尺寸与锚点覆盖: 有值时优先于布局. */
    bool hasSize = false;
    float width = 0.0f;
    float height = 0.0f;
    bool hasAnchor = false;
    LayoutAnchor anchor;
};

/*
 * 控件树: 布局解析结果加运行时状态, 并提供脚本侧常用的绑定动作.
 *
 * 动作集合按界面脚本的调用频次选取: 显示, 隐藏, 设文本, 设文本色, 设尺寸,
 * 设锚点, 置灰与设贴图. 未列出的动作不在这里实现, 避免做成一个空壳接口.
 */
class WidgetTree {
public:
    /* 解析布局并建好控件, 之后必须调用 Resolve 才会得到坐标. */
    bool Load(const char* text, size_t length);

    /* 按屏幕矩形解算; 运行时的尺寸与锚点覆盖会在这里生效. */
    void Resolve(const LayoutRect& screen);

    Widget* Find(const char* name) const {
        const auto found = byName_.find(name == nullptr ? std::string() : std::string(name));
        return found == byName_.end() ? nullptr : found->second;
    }

    void Show(Widget* widget) {
        if (widget != nullptr) {
            widget->shown = true;
        }
    }
    void Hide(Widget* widget) {
        if (widget != nullptr) {
            widget->shown = false;
        }
    }
    bool IsShown(const Widget* widget) const { return widget != nullptr && widget->shown; }

    void SetText(Widget* widget, const char* text);
    void SetTextColor(Widget* widget, float red, float green, float blue);
    void SetSize(Widget* widget, float width, float height);
    void SetPoint(Widget* widget, AnchorPoint point, AnchorPoint relativePoint, float offsetX,
                  float offsetY);
    void SetGray(Widget* widget, bool grayed) {
        if (widget != nullptr) {
            widget->grayed = grayed;
        }
    }
    void SetTexture(Widget* widget, const char* texture);

    /* 展开成绘制命令: 隐藏的控件连同其子树一起跳过, 置灰的控件压暗. */
    int32_t CollectDrawCommands(std::vector<LayoutDrawCommand>& out) const;

    int32_t WidgetCount() const { return static_cast<int32_t>(widgets_.size()); }
    const std::string& Error() const { return document_.Error(); }

private:
    const Widget* Lookup(const LayoutFrame* frame) const;

    LayoutDocument document_;
    std::vector<std::unique_ptr<Widget>> widgets_;
    std::unordered_map<std::string, Widget*> byName_;
    std::unordered_map<const LayoutFrame*, Widget*> byFrame_;
    LayoutFrame* root_ = nullptr;
};

}  /* namespace immirror::ui */
