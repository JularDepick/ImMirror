#include "tests/test_framework.h"

#include <algorithm>
#include <cmath>

#include "core/vec3.h"
#include "voxel/block_registry.h"
#include "voxel/chunk_column.h"
#include "voxel/chunk_mesher.h"
#include "voxel/light_field.h"
#include "world/world.h"

namespace {

using immirror::voxel::BlockId;
using immirror::voxel::BlockRegistry;
using immirror::voxel::ChunkColumn;
using immirror::voxel::LightField;
using immirror::voxel::MeshData;

/* 全量重算一列光场的简写, 计算器复用同一个实例与队列缓冲. */
void Recompute(ChunkColumn& column) {
    static LightField field;
    field.Recompute(column, BlockRegistry::Instance());
}

/* 把一整层 y 全部设为同一方块. */
void FillLayer(ChunkColumn& column, int32_t y, BlockId id) {
    for (int32_t z = 0; z < immirror::voxel::kChunkSizeZ; ++z) {
        for (int32_t x = 0; x < immirror::voxel::kChunkSizeX; ++x) {
            column.Set(x, y, z, id);
        }
    }
}

/*
 * 天光直射与洞穴扩散. 实心顶盖下的封闭空间全黑, 打开竖井后
 * 直射光沿井落进洞室, 再按每格一档向四周横向补入.
 */
TEST_CASE(SkyLightShaftAndCaveSpread) {
    ChunkColumn column;
    for (int32_t y = 0; y <= 10; ++y) {
        FillLayer(column, y, immirror::voxel::block::Stone);
    }
    /* 洞室: y 等于 9 的 x 与 z 各 2 到 5 挖空. */
    for (int32_t z = 2; z <= 5; ++z) {
        for (int32_t x = 2; x <= 5; ++x) {
            column.Set(x, 9, z, immirror::voxel::block::Air);
        }
    }
    Recompute(column);
    /* 封闭阶段: 顶盖上方满值, 顶盖本身与洞室全黑. */
    CHECK_INT(column.SkyLightAt(0, 15, 0), 15);
    CHECK_INT(column.SkyLightAt(0, 10, 0), 0);
    CHECK_INT(column.SkyLightAt(3, 9, 3), 0);
    CHECK_INT(column.SkyLightAt(5, 9, 5), 0);

    /* 开竖井后重算: 井格与井底直射满值, 横向逐格衰减. */
    column.Set(3, 10, 3, immirror::voxel::block::Air);
    Recompute(column);
    CHECK_INT(column.SkyLightAt(3, 10, 3), 15);
    CHECK_INT(column.SkyLightAt(3, 9, 3), 15);
    CHECK_INT(column.SkyLightAt(4, 9, 3), 14);
    CHECK_INT(column.SkyLightAt(5, 9, 5), 11);
    /* 洞室底面是实心方块, 自身恒为 0. */
    CHECK_INT(column.SkyLightAt(2, 8, 2), 0);
    /* 洞室外同层仍是实心石层. */
    CHECK_INT(column.SkyLightAt(7, 9, 7), 0);
}

/* 封死竖井后重算, 旧光必须被清干净. */
TEST_CASE(RecomputeClearsStaleLight) {
    ChunkColumn column;
    for (int32_t y = 0; y <= 10; ++y) {
        FillLayer(column, y, immirror::voxel::block::Stone);
    }
    for (int32_t z = 2; z <= 5; ++z) {
        for (int32_t x = 2; x <= 5; ++x) {
            column.Set(x, 9, z, immirror::voxel::block::Air);
        }
    }
    column.Set(3, 10, 3, immirror::voxel::block::Air);
    Recompute(column);
    CHECK_INT(column.SkyLightAt(3, 9, 3), 15);

    column.Set(3, 10, 3, immirror::voxel::block::Stone);
    Recompute(column);
    CHECK_INT(column.SkyLightAt(3, 10, 3), 0);
    CHECK_INT(column.SkyLightAt(3, 9, 3), 0);
    CHECK_INT(column.SkyLightAt(4, 9, 3), 0);
    CHECK_INT(column.SkyLightAt(5, 9, 5), 0);
}

/* 水体按数据列每层衰减一档, 深水底部趋暗. */
TEST_CASE(WaterAttenuatesSkyLight) {
    ChunkColumn column;
    for (int32_t y = 0; y <= 60; ++y) {
        FillLayer(column, y, immirror::voxel::block::Stone);
    }
    for (int32_t y = 61; y <= 70; ++y) {
        FillLayer(column, y, immirror::voxel::block::Water);
    }
    Recompute(column);
    CHECK_INT(column.SkyLightAt(0, 71, 0), 15);
    CHECK_INT(column.SkyLightAt(0, 70, 0), 14);
    CHECK_INT(column.SkyLightAt(0, 66, 0), 10);
    CHECK_INT(column.SkyLightAt(0, 61, 0), 5);
    CHECK_INT(column.SkyLightAt(0, 60, 0), 0);
}

/*
 * 方块光传播. 光源格取 lightSource 原值, 六方向每格减一,
 * 实心方块阻断; 数据表里存在光源方块是本用例的前提.
 */
TEST_CASE(BlockLightSpreadsFromSource) {
    const BlockRegistry& registry = BlockRegistry::Instance();
    BlockId sourceId = 0;
    int32_t bestSource = 0;
    for (uint32_t id = 0; id < registry.Count(); ++id) {
        const int32_t level = registry.Info(static_cast<BlockId>(id)).lightSource;
        if (level > bestSource) {
            bestSource = level;
            sourceId = static_cast<BlockId>(id);
        }
    }
    CHECK(sourceId != 0);
    CHECK(bestSource >= 1);

    ChunkColumn column;
    for (int32_t y = 0; y <= 5; ++y) {
        FillLayer(column, y, immirror::voxel::block::Stone);
    }
    column.Set(8, 6, 8, sourceId);
    Recompute(column);

    CHECK_INT(column.BlockLightAt(8, 6, 8), bestSource);
    CHECK_INT(column.BlockLightAt(8, 7, 8), bestSource >= 1 ? bestSource - 1 : 0);
    CHECK_INT(column.BlockLightAt(8, 6, 7), bestSource >= 1 ? bestSource - 1 : 0);
    /* 沿表面延伸三格, 逐格递减. */
    const int32_t threeSteps = bestSource >= 3 ? bestSource - 3 : 0;
    CHECK_INT(column.BlockLightAt(8, 6, 11), threeSteps);
    /* 石层阻断向下的传播. */
    CHECK_INT(column.BlockLightAt(8, 5, 8), 0);
}

/*
 * 网格明暗接入: 同一表面格在露天与封闭顶盖两种光场下的顶面
 * 明暗系数分别为 1.0 与 0.25, 洞穴变暗由顶点 shade 直接体现.
 */
TEST_CASE(MesherShadeFollowsLightField) {
    const BlockRegistry& registry = BlockRegistry::Instance();

    ChunkColumn open;
    for (int32_t y = 0; y <= 60; ++y) {
        FillLayer(open, y, immirror::voxel::block::Stone);
    }
    Recompute(open);
    MeshData openMesh;
    immirror::voxel::ChunkMesher{}.Build(open, {0, 0}, registry, openMesh);
    CHECK(!openMesh.Empty());
    bool openTopBright = true;
    for (const immirror::voxel::Vertex& vertex : openMesh.vertices) {
        if (vertex.py == 61.0f && vertex.px >= 8.0f && vertex.px <= 9.0f && vertex.pz >= 8.0f && vertex.pz <= 9.0f) {
            if (std::fabs(vertex.shade - 1.0f) > 0.01f) {
                openTopBright = false;
            }
        }
    }
    CHECK(openTopBright);

    ChunkColumn covered;
    for (int32_t y = 0; y <= 60; ++y) {
        FillLayer(covered, y, immirror::voxel::block::Stone);
    }
    FillLayer(covered, 62, immirror::voxel::block::Stone);
    Recompute(covered);
    MeshData coveredMesh;
    immirror::voxel::ChunkMesher{}.Build(covered, {0, 0}, registry, coveredMesh);
    CHECK(!coveredMesh.Empty());
    bool coveredTopDark = true;
    for (const immirror::voxel::Vertex& vertex : coveredMesh.vertices) {
        if (vertex.py == 61.0f && vertex.px >= 8.0f && vertex.px <= 9.0f && vertex.pz >= 8.0f && vertex.pz <= 9.0f) {
            if (std::fabs(vertex.shade - 0.25f) > 0.01f) {
                coveredTopDark = false;
            }
        }
    }
    CHECK(coveredTopDark);
}

/* 世界集成冒烟: 建列即带光场, 编辑后重算不破坏既有接口. */
TEST_CASE(WorldSmokeWithLightField) {
    immirror::world::World world;
    /* 每帧建列预算有限, 连续推进模拟多帧直到 3x3 视距补齐. */
    for (int32_t frame = 0; frame < 20 && world.LoadedChunkCount() < 9; ++frame) {
        world.Update(immirror::core::Vec3(8.0f, 80.0f, 8.0f), 1);
    }
    CHECK_INT(world.LoadedChunkCount(), 9);
    const int32_t height = world.HeightAt(8, 8);
    CHECK(height > 0);
    CHECK(height <= 256);

    const uint64_t versionBefore = world.Version();
    const int32_t placeY = height + 10;
    CHECK(world.SetBlock(8, placeY, 8, immirror::voxel::block::Stone));
    CHECK_INT(world.GetBlock(8, placeY, 8), immirror::voxel::block::Stone);
    CHECK(world.Version() > versionBefore);
    CHECK(world.SetBlock(8, placeY, 8, immirror::voxel::block::Air));
    CHECK_INT(world.GetBlock(8, placeY, 8), immirror::voxel::block::Air);
    CHECK(!world.VisibleChunks().empty());
}

}  /* namespace */
