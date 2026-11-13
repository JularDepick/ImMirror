#pragma once

#include <cstdint>
#include <vector>

#include "ui/layout_frame.h"

namespace immirror::ui {

/*
 * 一条绘制命令: 一块矩形, 一个图层, 颜色与可选贴图.
 * 贴图为空时按纯色填充; 颜色未在布局里给出时取不透明白.
 */
struct LayoutDrawCommand {
    LayoutRect rect;
    LayerLevel layer = LayerLevel::kArtwork;
    const char* texture = nullptr;
    /* 图集里的精灵名, 空表示按整张图绘制. */
    const char* sprite = nullptr;
    /* 文本内容: 只有文本控件才会带, 空表示不画文字. */
    const char* text = nullptr;
    const char* name = "";
    float red = 1.0f;
    float green = 1.0f;
    float blue = 1.0f;
    float alpha = 1.0f;
    /* 在帧树里的深度, 供调试与断言使用. */
    int32_t depth = 0;
};

/*
 * 把帧树展开成绘制命令序列.
 *
 * 顺序规则: 先画自己再画子帧; 同一父帧下的子帧按图层从底到顶稳定排序.
 * 尺寸为零的帧跳过, 避免生成看不见的图元. 返回生成的命令条数.
 * 调用前必须先解算布局, 否则矩形全是零.
 */
int32_t CollectDrawCommands(const LayoutFrame& root, std::vector<LayoutDrawCommand>& out);

}  /* namespace immirror::ui */
