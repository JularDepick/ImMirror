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

struct LayoutRenderStats {
    int32_t quads = 0;
    int32_t vertices = 0;
    /* 带贴图但未接入图集的命令数, 供统计缺口. */
    int32_t textured = 0;
    /* 超出顶点容量的命令数. */
    int32_t dropped = 0;
};

/*
 * 把绘制命令写进顶点缓冲.
 * 屏幕像素坐标按屏幕宽高折算成裁剪空间; 容量不足时停止并记入 dropped.
 * 返回写入的顶点数.
 */
int32_t EmitLayoutCommands(const std::vector<LayoutDrawCommand>& commands, float screenWidth,
                           float screenHeight, float* out, int32_t capacityVertices,
                           LayoutRenderStats* stats);

}  /* namespace immirror::ui */
