#include "voxel/terrain_generator.h"

#include <cmath>

#include "voxel/block_registry.h"
#include "voxel/chunk_column.h"
#include "voxel/voxel_noise.h"

namespace immirror::voxel {
namespace {

/* 高度范围取 M4 规格建议的 56 到 120, 海平面在低洼区间. */
constexpr int32_t kMinHeight = 56;
constexpr int32_t kMaxHeight = 120;
constexpr int32_t kSeaLevel = 71;
/* 海平面以上一层仍是沙滩, 让岸线有过渡. */
constexpr int32_t kBeachTop = kSeaLevel + 1;
/* 地表以下的泥土层厚度. */
constexpr int32_t kSoilDepth = 3;

/*
 * 地形噪声参数. 频率是每方块的格点增量, 倒数约等于该层的特征尺度;
 * 四层叠加后再加一层平方的脊状噪声形成山体, 总幅度略超出高度范围,
 * 极端值由 ClampInt 截断成平台, 出现概率很低.
 */
constexpr float kBaseHeight = 84.0f;
constexpr float kContinentWeight = 18.0f;
constexpr float kContinentFreq = 0.0125f;
constexpr float kHillWeight = 13.0f;
constexpr float kHillFreq = 0.0310f;
constexpr float kDetailWeight = 4.5f;
constexpr float kDetailFreq = 0.0620f;
constexpr float kGrainWeight = 1.6f;
constexpr float kGrainFreq = 0.1400f;
constexpr float kRidgeWeight = 10.0f;
constexpr float kRidgeFreq = 0.0175f;

/* 树木候选格边长, 每格最多一棵树, 是否成树由格哈希决定. */
constexpr int32_t kTreeCell = 6;
/* 树冠水平半径, 决定需要参与本区块的候选格范围. */
constexpr int32_t kTreeCanopyRadius = 2;
constexpr int32_t kTreeMinTrunk = 4;
constexpr int32_t kTreeTrunkVariants = 3;
/* 每格成树概率, 以 256 分之一为单位, 数值越大树越密. */
constexpr uint32_t kTreeChance = 90u;

/* 向下取整的整数除法, 保证负坐标附近与正坐标行为对称. */
int32_t FloorDiv(int32_t value, int32_t divisor) {
    const int32_t quotient = value / divisor;
    const int32_t remainder = value % divisor;
    if (remainder != 0 && ((remainder < 0) != (divisor < 0))) {
        return quotient - 1;
    }
    return quotient;
}

int32_t RoundToInt(float value) {
    return static_cast<int32_t>(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

int32_t ClampInt(int32_t value, int32_t low, int32_t high) {
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

/* 写入方块, 只覆盖空气; overwriteLeaves 用于树干压过更早生成的树叶. */
void PutTreeBlock(ChunkColumn& column, int32_t localX, int32_t y, int32_t localZ, BlockId id,
                  bool overwriteLeaves) {
    if (localX < 0 || localX >= kChunkSizeX || localZ < 0 || localZ >= kChunkSizeZ) {
        return;
    }
    if (y < 0 || y >= kWorldHeight) {
        return;
    }
    const BlockId current = column.Get(localX, y, localZ);
    if (current == block::Air || (overwriteLeaves && current == block::Leaves)) {
        column.Set(localX, y, localZ, id);
    }
}

}  /* namespace */

int32_t TerrainGenerator::SurfaceHeight(uint32_t seed, int32_t worldX, int32_t worldZ) const {
    const float x = static_cast<float>(worldX);
    const float z = static_cast<float>(worldZ);
    float height = kBaseHeight;
    height += kContinentWeight * detail::ValueNoise(seed ^ 0x1f9d3b4du, x * kContinentFreq, z * kContinentFreq);
    height += kHillWeight * detail::ValueNoise(seed ^ 0x6a09e667u, x * kHillFreq, z * kHillFreq);
    height += kDetailWeight * detail::ValueNoise(seed ^ 0xbb67ae85u, x * kDetailFreq, z * kDetailFreq);
    height += kGrainWeight * detail::ValueNoise(seed ^ 0x3c6ef372u, x * kGrainFreq, z * kGrainFreq);
    const float ridge = 1.0f - std::fabs(detail::ValueNoise(seed ^ 0xa54ff53au, x * kRidgeFreq, z * kRidgeFreq));
    height += kRidgeWeight * ridge * ridge;
    return ClampInt(RoundToInt(height), kMinHeight, kMaxHeight);
}

void TerrainGenerator::FillTerrain(ChunkCoord coord, ChunkColumn& column, uint32_t seed) const {
    column.Fill(block::Air);
    const int32_t baseX = coord.x * kChunkSizeX;
    const int32_t baseZ = coord.z * kChunkSizeZ;
    for (int32_t localZ = 0; localZ < kChunkSizeZ; ++localZ) {
        const int32_t worldZ = baseZ + localZ;
        for (int32_t localX = 0; localX < kChunkSizeX; ++localX) {
            const int32_t worldX = baseX + localX;
            const int32_t height = SurfaceHeight(seed, worldX, worldZ);
            const bool shore = height <= kBeachTop;
            const BlockId surfaceBlock = shore ? block::Sand : block::Grass;
            const BlockId soilBlock = shore ? block::Sand : block::Dirt;
            for (int32_t y = 0; y <= height; ++y) {
                BlockId id = block::Stone;
                if (y == 0) {
                    id = block::Bedrock; /* 世界底层的基岩层, 与深层石头区分 */
                } else if (y == height) {
                    id = surfaceBlock;
                } else if (y >= height - kSoilDepth) {
                    id = soilBlock;
                }
                column.Set(localX, y, localZ, id);
            }
            /* 地表低于海平面的洼地补满水, 水面高度一致. */
            for (int32_t y = height + 1; y <= kSeaLevel; ++y) {
                column.Set(localX, y, localZ, block::Water);
            }
        }
    }
}

void TerrainGenerator::PlantTrees(ChunkCoord coord, ChunkColumn& column, uint32_t seed) const {
    const int32_t baseX = coord.x * kChunkSizeX;
    const int32_t baseZ = coord.z * kChunkSizeZ;
    /* 本区块外扩树冠半径后覆盖的候选格范围, 保证边界树冠两侧一致. */
    const int32_t cellXMin = FloorDiv(baseX - kTreeCanopyRadius, kTreeCell);
    const int32_t cellXMax = FloorDiv(baseX + kChunkSizeX + kTreeCanopyRadius - 1, kTreeCell);
    const int32_t cellZMin = FloorDiv(baseZ - kTreeCanopyRadius, kTreeCell);
    const int32_t cellZMax = FloorDiv(baseZ + kChunkSizeZ + kTreeCanopyRadius - 1, kTreeCell);

    for (int32_t cellZ = cellZMin; cellZ <= cellZMax; ++cellZ) {
        for (int32_t cellX = cellXMin; cellX <= cellXMax; ++cellX) {
            const uint32_t hash = detail::HashLattice(seed ^ 0x51ed270bu, cellX, cellZ);
            if ((hash & 0xffu) >= kTreeChance) {
                continue;
            }
            const int32_t trunkWorldX =
                cellX * kTreeCell + static_cast<int32_t>((hash >> 8) % static_cast<uint32_t>(kTreeCell));
            const int32_t trunkWorldZ =
                cellZ * kTreeCell + static_cast<int32_t>((hash >> 13) % static_cast<uint32_t>(kTreeCell));
            const int32_t groundY = SurfaceHeight(seed, trunkWorldX, trunkWorldZ);
            /* 沙滩, 水面与陡坎下方不种树, 避免浮空或泡水的树. */
            if (groundY <= kBeachTop + 1) {
                continue;
            }
            const int32_t trunkHeight =
                kTreeMinTrunk + static_cast<int32_t>((hash >> 19) % static_cast<uint32_t>(kTreeTrunkVariants));
            const int32_t localTrunkX = trunkWorldX - baseX;
            const int32_t localTrunkZ = trunkWorldZ - baseZ;

            for (int32_t y = 1; y <= trunkHeight; ++y) {
                PutTreeBlock(column, localTrunkX, groundY + y, localTrunkZ, block::Wood, true);
            }

            /* 树冠自下而上三层, 顶层收成一个方块. */
            const int32_t topY = groundY + trunkHeight;
            for (int32_t dy = -1; dy <= 2; ++dy) {
                int32_t radius = 2;
                if (dy == 1) {
                    radius = 1;
                } else if (dy >= 2) {
                    radius = 0;
                }
                for (int32_t dx = -radius; dx <= radius; ++dx) {
                    for (int32_t dz = -radius; dz <= radius; ++dz) {
                        const int32_t absX = dx < 0 ? -dx : dx;
                        const int32_t absZ = dz < 0 ? -dz : dz;
                        /* 削掉 5x5 两层的四角, 树冠边缘更圆. */
                        if (radius == 2 && absX == 2 && absZ == 2) {
                            continue;
                        }
                        PutTreeBlock(column, localTrunkX + dx, topY + dy, localTrunkZ + dz, block::Leaves, false);
                    }
                }
            }
        }
    }
}

void TerrainGenerator::Generate(ChunkCoord coord, ChunkColumn& column, uint32_t seed) const {
    FillTerrain(coord, column, seed);
    PlantTrees(coord, column, seed);
}

}  /* namespace immirror::voxel */
