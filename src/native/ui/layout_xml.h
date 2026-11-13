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
};

/* 模板表: 名字到条目的映射, 由调用方在解析前装入. */
using LayoutTemplates = std::unordered_map<std::string, LayoutTemplate>;

/*
 * 解析模板文件, 把带精灵名的条目收进表里.
 * 模板文件本身也是布局格式, 这里只取外观相关字段, 不建帧树.
 */
bool ParseTemplates(const char* text, size_t length, LayoutTemplates& out);

class LayoutDocument {
public:
    /* 装入模板表; 指针须在文档解析与使用期间保持有效, 传空表示不展开继承. */
    void SetTemplates(const LayoutTemplates* templates) { templates_ = templates; }

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
    std::vector<std::unique_ptr<LayoutFrame>> pool_;
    LayoutFrame* root_ = nullptr;
    std::string error_;
    const LayoutTemplates* templates_ = nullptr;
};

/* 把布局里的锚点名 (小写, 如 topleft) 转成枚举; 未知返回 false. */
bool ParseAnchorPoint(const std::string& text, AnchorPoint& out);

}  /* namespace immirror::ui */
