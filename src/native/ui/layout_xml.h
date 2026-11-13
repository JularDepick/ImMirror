#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
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
class LayoutDocument {
public:
    /* 解析一段布局文本; 失败时返回 false 并给出原因. */
    bool Parse(const char* text, size_t length);

    /* 解析结果: 根帧, 帧数量, 失败原因. 根帧在失败时为空. */
    LayoutFrame* Root() { return root_; }
    const LayoutFrame* Root() const { return root_; }
    int32_t FrameCount() const { return static_cast<int32_t>(pool_.size()); }
    const std::string& Error() const { return error_; }

    /* 按展开后的名字查帧, 找不到返回空指针. */
    LayoutFrame* Find(const char* name) const;

private:
    /* 帧与名字都放在容器里, 保证节点地址与名字指针在整个文档生命周期内稳定. */
    std::deque<std::string> names_;
    std::vector<std::unique_ptr<LayoutFrame>> pool_;
    LayoutFrame* root_ = nullptr;
    std::string error_;
};

/* 把布局里的锚点名 (小写, 如 topleft) 转成枚举; 未知返回 false. */
bool ParseAnchorPoint(const std::string& text, AnchorPoint& out);

}  /* namespace immirror::ui */
