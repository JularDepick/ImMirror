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
    /* 跨主帧层级: 绘制阶段按它做全局排序. */
    int32_t strata = 1;
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
    /* 文本字号: 0 表示按缺省 20. */
    float fontSize = 0.0f;
    /* 文本排版: 0 左/上, 1 中, 2 右/下; 行距为 0 时按字号. */
    int32_t justifyH = 0;
    int32_t justifyV = 0;
    float lineInterval = 0.0f;
    /* 文字描边: 渲染时先在四周各画一遍深色文字. */
    bool outline = false;
    /* 文字加粗: 同色再画一遍并右移一个点阵像素. */
    bool bold = false;
    /* 多行文本: 超过帧宽时按宽度断行. */
    bool multiLine = false;
    /* 加法混合: 渲染时改用叠加混合. */
    bool additive = false;
    /* 文字阴影色: 描边用这个颜色. */
    float shadowRed = 0.0f;
    float shadowGreen = 0.0f;
    float shadowBlue = 0.0f;
    /* 贴图翻转. */
    bool flipU = false;
    bool flipV = false;
    /* 直角旋转: 1 表示顺时针九十度, 3 表示二百七十度. */
    int32_t rotateQuarter = 0;
    /* 九宫切片的内边距(像素), 全零表示整体拉伸. */
    /* 平铺起始偏移. */
    float tileOffsetX = 0.0f;
    float tileOffsetY = 0.0f;
    /* 平铺块尺寸: 大于 0 时按这个尺寸重复贴图. */
    float tileWidth = 0.0f;
    float tileHeight = 0.0f;
    float insetLeft = 0.0f;
    float insetRight = 0.0f;
    float insetTop = 0.0f;
    float insetBottom = 0.0f;
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

/*
 * 断行结果的存放区: 绘制命令只存指针, 由这里保活.
 * 每次收集前清空, 否则每帧断行都会往里追加, 长期运行会一直涨.
 * 调用方必须保证上一批命令已经用完再收集下一批(渲染完再收集即满足).
 */
void ResetTextLinePool();

}  /* namespace immirror::ui */
