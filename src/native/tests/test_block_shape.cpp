#include "tests/test_framework.h"

#include <algorithm>
#include <cmath>

#include "voxel/block_registry.h"
#include "voxel/chunk_column.h"
#include "voxel/chunk_mesher.h"
#include "voxel/voxel_types.h"

namespace {

using immirror::voxel::BlockId;
using immirror::voxel::BlockRegistry;
using immirror::voxel::BlockShape;
using immirror::voxel::ChunkColumn;
using immirror::voxel::ChunkCoord;
using immirror::voxel::ChunkMesher;
using immirror::voxel::MeshData;

/* 方块注册表里挑一个指定形状的方块, 找不到返回空气. */
BlockId FindBlockOfShape(BlockShape shape) {
    const BlockRegistry& registry = BlockRegistry::Instance();
    for (BlockId id = 1; id < registry.Count(); ++id) {
        if (registry.Info(id).shape == shape) {
            return id;
        }
    }
    return 0;
}

/*
 * 在空列里只放一个方块, 取网格的竖直与水平范围.
 * 单方块列的四邻都按空气处理, 因此该方块的六个面全部生成.
 */
void MeshOfSingleBlock(BlockId id, MeshData& out, float* minY, float* maxY, float* minX, float* maxX) {
    static ChunkColumn column;
    column.Fill(0);
    column.Set(0, 64, 0, id);
    ChunkMesher mesher;
    mesher.Build(column, ChunkCoord{0, 0}, BlockRegistry::Instance(), out);
    *minY = 1.0e9f;
    *maxY = -1.0e9f;
    *minX = 1.0e9f;
    *maxX = -1.0e9f;
    for (const auto& vertex : out.vertices) {
        *minY = std::min(*minY, vertex.py);
        *maxY = std::max(*maxY, vertex.py);
        *minX = std::min(*minX, vertex.px);
        *maxX = std::max(*maxX, vertex.px);
    }
}

TEST_CASE(BlockShapeSlabIsHalfHeight) {
    const BlockId slab = FindBlockOfShape(BlockShape::kSlab);
    CHECK(slab != 0);
    MeshData mesh;
    float minY = 0.0f;
    float maxY = 0.0f;
    float minX = 0.0f;
    float maxX = 0.0f;
    MeshOfSingleBlock(slab, mesh, &minY, &maxY, &minX, &maxX);
    CHECK(!mesh.vertices.empty());
    /* 方块放在 y=64, 台阶占 64.0 到 64.5. */
    CHECK(std::fabs(minY - 64.0f) < 0.001f);
    CHECK(std::fabs(maxY - 64.5f) < 0.001f);
    /* 水平方向仍是整格. */
    CHECK(std::fabs(minX - 0.0f) < 0.001f);
    CHECK(std::fabs(maxX - 1.0f) < 0.001f);
}

TEST_CASE(BlockShapeStairReachesTop) {
    const BlockId stair = FindBlockOfShape(BlockShape::kStair);
    CHECK(stair != 0);
    MeshData mesh;
    float minY = 0.0f;
    float maxY = 0.0f;
    float minX = 0.0f;
    float maxX = 0.0f;
    MeshOfSingleBlock(stair, mesh, &minY, &maxY, &minX, &maxX);
    CHECK(!mesh.vertices.empty());
    CHECK(std::fabs(minY - 64.0f) < 0.001f);
    CHECK(std::fabs(maxY - 65.0f) < 0.001f);
    /* 两个盒子各六个面, 每面四顶点, 共 48 个顶点. */
    CHECK_INT(mesh.vertices.size(), 48);
}

TEST_CASE(BlockShapeFenceIsNarrow) {
    const BlockId fence = FindBlockOfShape(BlockShape::kFence);
    CHECK(fence != 0);
    MeshData mesh;
    float minY = 0.0f;
    float maxY = 0.0f;
    float minX = 0.0f;
    float maxX = 0.0f;
    MeshOfSingleBlock(fence, mesh, &minY, &maxY, &minX, &maxX);
    CHECK(!mesh.vertices.empty());
    /* 中心立柱占水平方向的中间四分之一. */
    CHECK(std::fabs(minX - 0.375f) < 0.001f);
    CHECK(std::fabs(maxX - 0.625f) < 0.001f);
    CHECK(std::fabs(minY - 64.0f) < 0.001f);
    CHECK(std::fabs(maxY - 65.0f) < 0.001f);
}

TEST_CASE(BlockShapeCarpetIsThin) {
    const BlockId carpet = FindBlockOfShape(BlockShape::kCarpet);
    CHECK(carpet != 0);
    MeshData mesh;
    float minY = 0.0f;
    float maxY = 0.0f;
    float minX = 0.0f;
    float maxX = 0.0f;
    MeshOfSingleBlock(carpet, mesh, &minY, &maxY, &minX, &maxX);
    CHECK(!mesh.vertices.empty());
    CHECK(std::fabs(maxY - minY - 0.0625f) < 0.001f);
}

TEST_CASE(BlockShapeCubeStaysFullSize) {
    MeshData mesh;
    float minY = 0.0f;
    float maxY = 0.0f;
    float minX = 0.0f;
    float maxX = 0.0f;
    MeshOfSingleBlock(immirror::voxel::block::Stone, mesh, &minY, &maxY, &minX, &maxX);
    CHECK(!mesh.vertices.empty());
    CHECK(std::fabs(minY - 64.0f) < 0.001f);
    CHECK(std::fabs(maxY - 65.0f) < 0.001f);
    CHECK(std::fabs(minX - 0.0f) < 0.001f);
    CHECK(std::fabs(maxX - 1.0f) < 0.001f);
}

}  /* namespace */
