#pragma once

#include <cstdint>

#include "voxel/voxel_types.h"

namespace immirror::voxel {

class ChunkColumn;

/*
 * 值噪声地形生成器. 结果只由种子与该列覆盖的世界坐标决定:
 * 相邻区块共享同一套高度采样, 边界必然连续; 树木按世界坐标的
 * 候选格决定位置, 跨区块的树冠能在两侧拼合成同一棵树.
 */
class TerrainGenerator {
public:
    TerrainGenerator() = default;

    /*
     * 生成一列区块, 覆盖 column 内原有的全部方块数据.
     * treeShape 为树形档: 1 标准树 (默认), 0 矮树.
     * 档位由调用方传入, 体素层不读设置, 保持模块无反向依赖.
     */
    void Generate(ChunkCoord coord, ChunkColumn& column, uint32_t seed,
                  uint8_t treeShape = 1) const;

    /*
     * 世界坐标处的群系带下标: 温度与湿度两个低频噪声各查一维,
     * 按生成表 kBiomeBands 的四元区间选带. 天气按玩家所在带取参数, 因此公开.
     */
    int32_t BiomeAt(uint32_t seed, int32_t worldX, int32_t worldZ) const;

private:
    /* 世界坐标处地表方块的 y, 地形填充与树木定位共用同一定义. */
    int32_t SurfaceHeight(uint32_t seed, int32_t worldX, int32_t worldZ) const;

    /* 填充地表方块, 泥土层, 石层与低洼处的水. */
    void FillTerrain(ChunkCoord coord, ChunkColumn& column, uint32_t seed) const;

    /*
     * 洞穴雕刻: 两组三维噪声取等值面, 只在表层以下留出顶板,
     * 避免把地表挖穿; 只挖石头, 水与表层方块不受影响.
     */
    int32_t CarveCaves(ChunkCoord coord, ChunkColumn& column, uint32_t seed) const;

    /*
     * 矿脉: 按生成表的每区块尝试次数与命中概率随机布种,
     * 每条矿脉以随机游走替换目标方块, 上限取 MaxNum.
     */
    int32_t PlaceOres(ChunkCoord coord, ChunkColumn& column, uint32_t seed) const;

    /*
     * 群系装饰: 按 biomedef 的 Chunk* 列在区块内撒花草, 南瓜, 甘蔗, 蘑菇,
     * 仙人掌, 水草与浮叶; 只落在空位或水面上, 不破坏地表与洞穴.
     */
    int32_t PlantDecorations(ChunkCoord coord, ChunkColumn& column, uint32_t seed) const;

    /* 在草地上按世界坐标候选格散布树木, 包含跨越区块边界的树冠. */
    void PlantTrees(ChunkCoord coord, ChunkColumn& column, uint32_t seed,
                    uint8_t treeShape) const;
};

}  /* namespace immirror::voxel */
