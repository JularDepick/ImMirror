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
    /* 是否已经跑过 OnLoad: 生命周期脚本只在首次显示时触发一次. */
    bool loaded = false;
    /* 选中态: 页签按钮靠它切换贴图, 与临时的按下态分开. */
    bool checked = false;
    /* 按下态: 绘制时改用 PushedTexture, 由输入层设置. */
    bool pressed = false;
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

    /* 装入外部模板文档, 让跨文件的模板也能克隆子树. */
    void SetTemplateDocument(const LayoutDocument* document) {
        document_.SetTemplateDocument(document);
    }

    /* 装入参照文档: 跨文件的 relativeTo 靠它找到坐标. */
    void AddReferenceDocument(const LayoutDocument* document) {
        document_.AddReferenceDocument(document);
    }

    /* 装入模板表, 之后载入的布局会按继承属性展开外观. */
    void SetTemplates(const LayoutTemplates* templates) { document_.SetTemplates(templates); }

    /*
     * 按模板复制控件: 把模板控件的整棵子树复制到指定父控件下.
     * 新根用 newName 命名, 子孙名字为 "新根名/原子名", 因此按名字仍能取到.
     * 返回新根控件, 参数非法或名字已被占用时返回空.
     */
    Widget* Clone(Widget* source, Widget* parent, const char* newName);

    /* 全部控件的只读视图: 供统计与调试遍历, 不暴露可变句柄. */
    const std::vector<std::unique_ptr<Widget>>& Widgets() const { return widgets_; }

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
    /*
     * 带参照物的锚点设置: 脚本里的 SetPoint 五参数写法会显式给参照物名,
     * 名字为空时退回父帧, 与不写 relativeTo 的语义一致.
     */
    void SetPointTo(Widget* widget, AnchorPoint point, const char* relativeToName,
                    AnchorPoint relativePoint, float offsetX, float offsetY);
    void SetGray(Widget* widget, bool grayed) {
        if (widget != nullptr) {
            widget->grayed = grayed;
        }
    }
    void SetTexture(Widget* widget, const char* texture);

    /* 展开成绘制命令: 隐藏的控件连同其子树一起跳过, 置灰的控件压暗. */
    int32_t CollectDrawCommands(std::vector<LayoutDrawCommand>& out) const;
    /*
     * 只收集指定帧子树的绘制命令.
     * 用于核对子界面: 主帧仍打开(提供锚点参照物), 但统计只算目标子树.
     */
    int32_t CollectDrawCommandsFrom(const char* name, std::vector<LayoutDrawCommand>& out) const;

    /*
     * 命中测试: 返回覆盖该点的最上层控件, 没有命中返回空.
     * 层级顺序与绘制相反 (绘制先底后顶, 命中先顶后底), 隐藏控件的子树不参与.
     */
    Widget* HitTest(float x, float y);

    /* 按下态: 只影响绘制取哪张态贴图. */
    void SetPressed(Widget* widget, bool pressed) {
        if (widget != nullptr) {
            widget->pressed = pressed;
        }
    }

    int32_t WidgetCount() const { return static_cast<int32_t>(widgets_.size()); }

    /* 按下标取控件, 越界返回空; 供只读遍历使用. */
    Widget* WidgetAt(int32_t index) {
        return index < 0 || index >= static_cast<int32_t>(widgets_.size())
                   ? nullptr
                   : widgets_[static_cast<size_t>(index)].get();
    }
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
