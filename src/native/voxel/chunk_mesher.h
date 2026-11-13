#pragma once

#include <cstdint>

#include "voxel/voxel_types.h"

namespace immirror::voxel {

class BlockRegistry;
class ChunkColumn;

/*
 * 相邻列引用. 指针为空表示该方向没有已加载的区块, 此时区块边界上的面全部保留,
 * 由相邻区块自身的几何遮挡; 提供相邻列后这些隐藏面会被剔除.
 */
struct NeighborColumns {
    const ChunkColumn* east = nullptr;  /* +X 方向 */
    const ChunkColumn* west = nullptr;  /* -X 方向 */
    const ChunkColumn* south = nullptr; /* +Z 方向 */
    const ChunkColumn* north = nullptr; /* -Z 方向 */
};

/*
 * 面剔除网格构建. 只保留与空气或透明方块相邻的面, 每个面是 4 顶点 6 索引,
 * 顶点逆时针环绕 (从面外侧看), 与启用背面剔除的右手系渲染一致.
 * 顶点坐标是区块局部坐标 (水平 0 到 16, 竖直 0 到 256), 渲染侧按区块原点平移;
 * 三角形法线严格等于该面的外法线, shade 由朝向决定.
 */
class ChunkMesher {
public:
    /*
     * 没有相邻列信息时的构建: 区块四周的边界面全部生成.
     * tint 为群系草色 (RGB, 0 到 1), 空指针表示不染色.
     * waterTint 为群系水色, 液体面按它着色; 空指针表示不染色.
     */
    void Build(const ChunkColumn& column, ChunkCoord coord, const BlockRegistry& registry,
               MeshData& out, const float* tint = nullptr,
               const float* waterTint = nullptr) const;

    /* 已知相邻列时的构建: 与相邻区块相接且被遮挡的面被剔除, 顶点量通常降到四分之一以下. */
    void Build(const ChunkColumn& column, ChunkCoord coord, const BlockRegistry& registry,
               const NeighborColumns& neighbors, MeshData& out, const float* tint = nullptr,
               const float* waterTint = nullptr) const;

private:
    /* 两个重载共用的构建实现, neighbors 按 +X -X +Z -Z 排列. */
    void BuildWithNeighbors(const ChunkColumn& column, ChunkCoord coord, const BlockRegistry& registry,
                            const ChunkColumn* const* neighbors, MeshData& out,
                            const float* tint, const float* waterTint) const;
};

}  /* namespace immirror::voxel */
