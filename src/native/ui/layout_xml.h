#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ui/layout_frame.h"

namespace immirror::ui {

/*
 * 布局文档: 把一份界面布局解析成帧树, 并接管全部存储.
 *
 * 布局的元素分两类: 结构性元素 (Size, Anchors, Anchor, Offset, Layer 等) 只
 * 承载几何与层级信息; 带 name 的元素才成为一个帧. 名字里的 $parent 递归展开
 * 成父帧的名字, 锚点的 relativeTo 也按同一套名字解析.
 *
 * 解析只做结构, 不做绘制; 因此可以离线对真实布局文件做断言.
 */
/* 模板条目: 元素用 inherits 指向它, 取走外观信息. */
struct LayoutTemplate {
    std::string texture;
    std::string sprite;
    float width = 0.0f;
    float height = 0.0f;
    /*
     * 拉伸方式: 0 整体拉伸, 1 三片(左右固定, 中间拉伸), 2 九宫.
     * 三片用 insetLeft/insetRight, 九宫再加 insetTop/insetBottom 与中心块尺寸.
     */
    int32_t drawType = 0;
    float insetLeft = 0.0f;
    float insetRight = 0.0f;
    float insetTop = 0.0f;
    float insetBottom = 0.0f;
    /* 平铺起始偏移: 决定平铺图案与容器边界如何对齐. */
    float tileOffsetX = 0.0f;
    float tileOffsetY = 0.0f;
    float tileWidth = 0.0f;
    float tileHeight = 0.0f;
    /*
     * 按钮三态贴图: 模板里用 <NormalTexture> / <PushedTexture> / <CheckedTexture> 给出.
     * 这类标签是结构性元素, 不建帧, 只把贴图写到模板上, 实例继承后才有外观.
     */
    std::string normalTexture;
    std::string normalSprite;
    std::string pushedTexture;
    std::string pushedSprite;
    std::string checkedTexture;
    std::string checkedSprite;
    /* 模板自带的点击脚本: 实例没有自己的 OnClick 时继承它. */
    std::string onClick;
    /* 模板之间的继承: 例如页签模板继承通用按钮模板. */
    std::string inherits;
};

/* 模板表: 名字到条目的映射, 由调用方在解析前装入. */
using LayoutTemplates = std::unordered_map<std::string, LayoutTemplate>;

/*
 * 解析模板文件, 把带精灵名的条目收进表里.
 * 模板文件本身也是布局格式, 这里只取外观相关字段, 不建帧树.
 */
bool ParseTemplates(const char* text, size_t length, LayoutTemplates& out);

/*
 * 展开模板之间的继承链: 子模板没给的字段从父模板取.
 * 允许多层, 最多展开 8 层; 出现环时停止, 不报错.
 */
void ResolveTemplateInheritance(LayoutTemplates& templates);

class LayoutDocument {
public:
    /* 装入模板表; 指针须在文档解析与使用期间保持有效, 传空表示不展开继承. */
    void SetTemplates(const LayoutTemplates* templates);

    /*
     * 装入外部模板文档(通常是 uitemplate.xml 解析出的帧树).
     * 模板定义在别的文件里时, 只有它能提供模板子树供克隆.
     */
    void SetTemplateDocument(const LayoutDocument* document) { templateDocument_ = document; }

    /*
     * 装入参照文档(已解算的布局帧树), 例如游戏内 HUD 布局.
     * 锚点的 relativeTo 指向别的布局文件里的帧时, 只有它能提供坐标.
     */
    void AddReferenceDocument(const LayoutDocument* document) {
        if (document != nullptr) {
            referenceDocuments_.push_back(document);
        }
    }

    /* 解析一段布局文本; 失败时返回 false 并给出原因. */
    bool Parse(const char* text, size_t length);

    /* 解析结果: 根帧, 帧数量, 失败原因. 根帧在失败时为空. */
    LayoutFrame* Root() { return root_; }
    const LayoutFrame* Root() const { return root_; }
    int32_t FrameCount() const { return static_cast<int32_t>(pool_.size()); }
    const std::string& Error() const { return error_; }

    /* 按展开后的名字查帧, 找不到返回空指针. */
    LayoutFrame* Find(const char* name) const;

    /*
     * 运行时新增一个空帧, 名字与贴图都复制进文档容器, 保证指针长期有效.
     * 供界面脚本按模板复制控件时使用.
     */
    LayoutFrame* CreateFrame(const std::string& name);

private:
    /* 帧与名字都放在容器里, 保证节点地址与名字指针在整个文档生命周期内稳定. */
    std::deque<std::string> names_;
    /* 贴图路径同样放在容器里, 保证帧里的指针长期有效. */
    std::deque<std::string> textures_;
    /* 精灵名同理. */
    std::deque<std::string> sprites_;
    /* 文本控件的文字内容, 与帧同生命周期. */
    std::deque<std::string> texts_;
    /* 继承的模板名, 与帧同生命周期. */
    std::deque<std::string> inherits_;
    /* 锚点参照物名字, 与帧同生命周期. */
    std::deque<std::string> anchorNames_;
    /* 流式布局样式名, 与帧同生命周期. */
    std::deque<std::string> layoutStyles_;
    /* 标签名, 与帧同生命周期. */
    std::deque<std::string> tags_;
    /* 外部模板文档: 只读引用, 生命周期由调用方保证. */
    const LayoutDocument* templateDocument_ = nullptr;
    /* 外部参照文档: 供跨文件的 relativeTo 查找, 生命周期由调用方保证. */
    std::vector<const LayoutDocument*> referenceDocuments_;
    /* OnClick 等脚本片段, 与帧同生命周期. */
    std::deque<std::string> snippets_;
    /* 精灵名到模板的索引: 直接写 file+uvname 的元素据此拿到切片参数. */
    std::unordered_map<std::string, const LayoutTemplate*> bySprite_;
    std::vector<std::unique_ptr<LayoutFrame>> pool_;
    LayoutFrame* root_ = nullptr;
    std::string error_;
    const LayoutTemplates* templates_ = nullptr;
};

/* 把布局里的锚点名 (小写, 如 topleft) 转成枚举; 未知返回 false. */
bool ParseAnchorPoint(const std::string& text, AnchorPoint& out);

}  /* namespace immirror::ui */
