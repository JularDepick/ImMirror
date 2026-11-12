#pragma once

#include <cstdint>

#include "voxel/voxel_types.h"

namespace immirror::render {
namespace particle {

/*
 * 方块碎屑粒子.
 *
 * 方块被破坏时在破坏点撒出若干小立方体碎屑, 带初速度与重力, 一秒内淡出.
 * 渲染用独立的顶点色程序: 顶点是位置加 RGBA, 混合开启, 不写深度.
 * 几何在 CPU 侧按帧重建, 粒子总量很小 (上限数十个), 不值得做实例化缓冲.
 */
bool Initialize();
void Shutdown();

/*
 * 在方块中心撒一组碎屑. 颜色按方块图集格的平均色取,
 * 因此不同材质掉出的碎屑颜色与方块本体一致.
 */
void EmitBlockBreak(voxel::BlockId block, float centerX, float centerY, float centerZ);

/* 推进生命周期: 位移, 重力, 落地, 淡出. */
void Update(double deltaSeconds);

/* 提交绘制, 由场景在区块之后调用. */
void Draw(const float* viewProjection16);

/* 当前活跃粒子数, 供调试与日志. */
int32_t Count();

}  /* namespace particle */
}  /* namespace immirror::render */
