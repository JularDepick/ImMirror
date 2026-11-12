#include "tests/test_framework.h"

#include <algorithm>
#include <set>

#include "voxel/block_registry.h"
#include "voxel/chunk_column.h"
#include "voxel/generated/biome_data_gen.h"
#include "voxel/terrain_generator.h"
#include "voxel/voxel_types.h"

namespace {

using immirror::voxel::BlockId;
using immirror::voxel::ChunkColumn;
using immirror::voxel::ChunkCoord;
using immirror::voxel::TerrainGenerator;

/*
 * 群系带表里每个带都必须给出树种与叶种, 或者两者都是 0 表示不长树.
 * 单边为 0 会让地形生成器跳过该带的树, 属于数据错误.
 */
TEST_CASE(BiomeTreeFieldsAreConsistent) {
    for (int32_t index = 0; index < immirror::voxel::kBiomeBandCount; ++index) {
        const immirror::voxel::BiomeBand& band = immirror::voxel::kBiomeBands[index];
        const bool hasWood = band.woodBlock != 0;
        const bool hasLeaves = band.leavesBlock != 0;
        CHECK_INT(static_cast<int>(hasWood), static_cast<int>(hasLeaves));
    }
}

/* 沙漠带不长树, 其余带都要有树种. */
TEST_CASE(DesertHasNoTrees) {
    for (int32_t index = 0; index < immirror::voxel::kBiomeBandCount; ++index) {
        const immirror::voxel::BiomeBand& band = immirror::voxel::kBiomeBands[index];
        const std::string key = band.key;
        if (key == "Desert") {
            CHECK_INT(static_cast<int>(band.woodBlock), 0);
        } else {
            CHECK(band.woodBlock != 0);
            CHECK(band.leavesBlock != 0);
        }
    }
}

/*
 * 树种 id 必须落在注册表范围内, 且注册表里确实存在这两个方块.
 * 取错 id 会让世界里长出不相干的方块, 这里用注册表名做交叉核对.
 */
TEST_CASE(BiomeTreeBlocksExist) {
    const auto& registry = immirror::voxel::BlockRegistry::Instance();
    for (int32_t index = 0; index < immirror::voxel::kBiomeBandCount; ++index) {
        const immirror::voxel::BiomeBand& band = immirror::voxel::kBiomeBands[index];
        if (band.woodBlock == 0) {
            continue;
        }
        CHECK(band.woodBlock < registry.Count());
        CHECK(band.leavesBlock < registry.Count());
        /* 名字里带 wood 或 log 的才算原木, 带 leaves 的才算树叶. */
        const std::string woodName = registry.Info(static_cast<BlockId>(band.woodBlock)).name;
        const std::string leavesName = registry.Info(static_cast<BlockId>(band.leavesBlock)).name;
        const bool woodOk = woodName.find("wood") != std::string::npos ||
                            woodName.find("log") != std::string::npos;
        const bool leavesOk = leavesName.find("leaves") != std::string::npos ||
                              leavesName.find("leaf") != std::string::npos;
        CHECK(woodOk);
        CHECK(leavesOk);
    }
}

}  /* namespace */