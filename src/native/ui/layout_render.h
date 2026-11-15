#pragma once

#include <cstdint>
#include <vector>

#include "ui/layout_draw.h"

namespace immirror::ui {

/*
 * 布局绘制命令转顶点.
 *
 * 输出与 HUD 同一套顶点格式: 每个顶点 8 个浮点 (x, y, u, v, r, g, b, a),
 * 坐标已折算到裁剪空间 [-1, 1]. 贴图通道暂时留 -1 表示纯色,
 * 因此本函数不依赖图集, 可以离线断言顶点数与坐标.
 */
constexpr int32_t kLayoutFloatsPerVertex = 8;
/* 纯色顶点的 uv 约定, 与 HUD 一致. */
constexpr float kLayoutSolidUv = -1.0f;

/*
 * 按素材名查界面图集: 传入贴图路径, 取文件名去掉扩展名后在图集里找同名精灵.
 * 找到时写入归一化 uv 并返回 true; 找不到返回 false, 调用方回落到纯色.
 */
bool UiSpriteUv(const char* texturePath, float* u0, float* v0, float* u1, float* v1);

/* 按子图集里的精灵名取归一化矩形; 名字带扩展名时自动去掉. */
bool UiSpriteRectUv(const char* spriteName, float* u0, float* v0, float* u1, float* v1);

/*
 * 按精灵名取原始像素宽高.
 * 布局里大量 Texture 不给 Size, 尺寸取自图集精灵本身, 解算时用这个补齐.
 */
bool UiSpriteSize(const char* spriteName, float* width, float* height);

/* 按贴图页路径取整图像素尺寸; 用于没有精灵名的整图元素. */
bool UiPageSize(const char* texturePath, float* width, float* height);

/*
 * 文本绘制原点: 按绘制命令的对齐方式, 把文字放进命令矩形.
 * 字号由 pixelSize 给出(单个点阵像素的边长), 宽度用 5x7 点阵的度量.
 * 返回文本占用的宽高, 供调用方排版与断言.
 */
void LayoutTextOrigin(const LayoutDrawCommand& command, float pixelSize, float* x, float* y,
                      float* width, float* height);

struct LayoutRenderStats {
    int32_t quads = 0;
    int32_t vertices = 0;
    /* 带贴图但未接入图集的命令数, 供统计缺口. */
    int32_t textured = 0;
    /* 超出顶点容量的命令数. */
    int32_t dropped = 0;
    /* 文字被容器宽度裁掉的字符数: 比例过高说明字号或容器宽度有问题. */
    int32_t clippedChars = 0;
    /* 有切片参数却无法切片的元素数: 内边距超过元素尺寸时会退化为整体拉伸. */
    int32_t sliceFallback = 0;
};

/*
 * 把绘制命令写进顶点缓冲.
 * 屏幕像素坐标按屏幕宽高折算成裁剪空间; 容量不足时停止并记入 dropped.
 * 贴图 uv 的朝向与 HUD 四边形一致, 但**不含界面图集的通道偏移**,
 * 提交方需要给 u 加上该偏移, 与 PushUiQuad 的处理一致.
 * 返回写入的顶点数.
 */
int32_t EmitLayoutCommands(const std::vector<LayoutDrawCommand>& commands, float screenWidth,
                           float screenHeight, float* out, int32_t capacityVertices,
                           LayoutRenderStats* stats);

}  /* namespace immirror::ui */
