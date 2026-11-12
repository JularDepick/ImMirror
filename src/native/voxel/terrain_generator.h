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

private:
    /* 世界坐标处地表方块的 y, 地形填充与树木定位共用同一定义. */
    int32_t SurfaceHeight(uint32_t seed, int32_t worldX, int32_t worldZ) const;

    /*
     * 世界坐标处的群系带下标: 温度与湿度两个低频噪声各查一维,
     * 按生成表 kBiomeBands 的四元区间选带. 树木定位与表层折算共用.
     */
    int32_t BiomeAt(uint32_t seed, int32_t worldX, int32_t worldZ) const;

    /* 填充地表方块, 泥土层, 石层与低洼处的水. */
    void FillTerrain(ChunkCoord coord, ChunkColumn& column, uint32_t seed) const;

    /* 在草地上按世界坐标候选格散布树木, 包含跨越区块边界的树冠. */
    void PlantTrees(ChunkCoord coord, ChunkColumn& column, uint32_t seed,
                    uint8_t treeShape) const;
};

}  /* namespace immirror::voxel */
