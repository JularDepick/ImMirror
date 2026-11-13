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

/*
 * 在指定点撒一组指定颜色的小碎块, 用于生物死亡一类的非方块事件.
 * 颜色分量取 0 到 1, 数量上限与方块破碎同一通道.
 */
void EmitColored(float red, float green, float blue, float centerX, float centerY, float centerZ,
                 int32_t count);

/* 推进生命周期: 位移, 重力, 落地, 淡出. */
void Update(double deltaSeconds);

/* 提交绘制, 由场景在区块之后调用. */
void Draw(const float* viewProjection16);

/* 当前活跃粒子数, 供调试与日志. */
int32_t Count();

/*
 * 掉落物通道: 与碎屑共用着色器与顶点缓冲, 但存活由游戏层控制.
 * 方块掉到地上时加入, 拾取或超时后移除; 颜色按方块图集格平均色取.
 */
void AddDrop(int32_t id, voxel::BlockId block, float x, float y, float z);

/*
 * 按给定代表色添加掉落物.
 * 非方块物品没有方块贴图, 由调用方按物品图标算出代表色再传进来.
 */
void AddDropColored(int32_t id, float red, float green, float blue, float x, float y, float z);
/* 掉落物落地过程中更新显示位置. */
void MoveDrop(int32_t id, float x, float y, float z);
void RemoveDrop(int32_t id);
void ClearDrops();
int32_t DropCount();

}  /* namespace particle */
}  /* namespace immirror::render */
