#include "voxel/terrain_generator.h"

#include <cmath>

#include "core/log.h"
#include "voxel/block_registry.h"
#include "voxel/generated/biome_data_gen.h"
#include "voxel/generated/ore_data_gen.h"
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
/* 树冠水平半径, 决定需要参与本区块的候选格范围. 候选格外扩按两档中较大者计算. */
constexpr int32_t kTreeCanopyRadius = 2;
/* 标准树 (treeshape=1): 干高 4 到 6, 树冠底两层半径 2. */
constexpr int32_t kTreeMinTrunk = 4;
constexpr int32_t kTreeTrunkVariants = 3;
/* 矮树 (treeshape=0): 干高 3 到 4, 树冠底两层半径 1. */
constexpr int32_t kTreeShortMinTrunk = 3;
constexpr int32_t kTreeShortTrunkVariants = 2;
constexpr int32_t kTreeShortCanopyRadius = 1;

/*
 * 洞穴参数. 两组三维噪声取 |noise| < 阈值 的等值面, 二者相交处才是空腔,
 * 因此洞穴是隧洞状而不是成片的空层. 频率是每方块的格点增量, 洞穴横向尺度
 * 约 20 格, 纵向因接近球形格点而略高, 与常见体素洞穴的手感一致.
 */
constexpr int32_t kCaveMinY = 5;         /* 洞穴最低高度, 保留世界底部基岩层 */
constexpr int32_t kCaveRoofDepth = 3;    /* 地表以下的顶板厚度, 防止挖穿地表 */
constexpr float kCaveFreqXZ = 0.045f;
constexpr float kCaveFreqY = 0.075f;
constexpr float kCaveThreshold = 0.075f;
constexpr uint32_t kCaveSeedA = 0x51ed270bu;
constexpr uint32_t kCaveSeedB = 0x2545f491u;

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

/*
 * 群系选取: 温度与湿度两个低频值噪声, 频率取地形大陆层的四分之一,
 * 一片群系区域约上百格宽; 两值各折算到 0-100 后按生成表的四元区间
 * 顺序查带, 兜底带是森林. 海洋不占带, 由地表高度另行折算表层.
 */
int32_t TerrainGenerator::BiomeAt(uint32_t seed, int32_t worldX, int32_t worldZ) const {
    const float x = static_cast<float>(worldX);
    const float z = static_cast<float>(worldZ);
    constexpr float kBiomeFreq = 0.0030f;
    const float heat = (detail::ValueNoise(seed ^ 0x243f6a88u, x * kBiomeFreq, z * kBiomeFreq) + 1.0f) * 50.0f;
    const float humid = (detail::ValueNoise(seed ^ 0x13198a2eu, x * kBiomeFreq, z * kBiomeFreq) + 1.0f) * 50.0f;
    const int32_t heatLevel = ClampInt(RoundToInt(heat), 0, 100);
    const int32_t humidLevel = ClampInt(RoundToInt(humid), 0, 100);
    for (int32_t index = 0; index < kBiomeBandCount; ++index) {
        const BiomeBand& band = kBiomeBands[index];
        if (heatLevel >= static_cast<int32_t>(band.heatMin) &&
            heatLevel < static_cast<int32_t>(band.heatMax) &&
            humidLevel >= static_cast<int32_t>(band.humidMin) &&
            humidLevel < static_cast<int32_t>(band.humidMax)) {
            return index;
        }
    }
    return kBiomeBandCount - 1;
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
            /*
             * 表层与填充块按群系带折算: 原版 TopBlock/FillBlock 的客户端近似,
             * 沙滩与水域保持沙与水的旧语义; 草颜色本期不做顶点色.
             */
            const int32_t biome = BiomeAt(seed, worldX, worldZ);
            const BiomeBand& band = kBiomeBands[biome];
            const BlockId surfaceBlock = shore
                                             ? block::Sand
                                             : static_cast<BlockId>(band.topBlock);
            const BlockId soilBlock = shore ? block::Sand
                                            : static_cast<BlockId>(band.fillBlock);
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

void TerrainGenerator::PlantTrees(ChunkCoord coord, ChunkColumn& column, uint32_t seed,
                                  uint8_t treeShape) const {
    const int32_t baseX = coord.x * kChunkSizeX;
    const int32_t baseZ = coord.z * kChunkSizeZ;
    /* 树形档位折算: 0 矮树, 其余按标准树. 档内参数成组取用. */
    const bool shortTree = treeShape == 0;
    const int32_t canopyRadius = shortTree ? kTreeShortCanopyRadius : kTreeCanopyRadius;
    const int32_t minTrunk = shortTree ? kTreeShortMinTrunk : kTreeMinTrunk;
    const int32_t trunkVariants = shortTree ? kTreeShortTrunkVariants : kTreeTrunkVariants;
    /*
     * 本区块外扩树冠半径后覆盖的候选格范围, 保证边界树冠两侧一致.
     * 外扩按两档中较大的标准树冠计算: 矮树冠必然落在标准范围内,
     * 邻列即使树形档不同, 各自生成的树也互不缺格.
     */
    const int32_t cellXMin = FloorDiv(baseX - kTreeCanopyRadius, kTreeCell);
    const int32_t cellXMax = FloorDiv(baseX + kChunkSizeX + kTreeCanopyRadius - 1, kTreeCell);
    const int32_t cellZMin = FloorDiv(baseZ - kTreeCanopyRadius, kTreeCell);
    const int32_t cellZMax = FloorDiv(baseZ + kChunkSizeZ + kTreeCanopyRadius - 1, kTreeCell);

    for (int32_t cellZ = cellZMin; cellZ <= cellZMax; ++cellZ) {
        for (int32_t cellX = cellXMin; cellX <= cellXMax; ++cellX) {
            /*
             * 树密度按候选格中心的群系带折算: 生成表 treeChance 0-255
             * 与成树哈希同口径, 沙漠带为 0 自然无树.
             */
            const int32_t cellCenterX = cellX * kTreeCell + kTreeCell / 2;
            const int32_t cellCenterZ = cellZ * kTreeCell + kTreeCell / 2;
            const int32_t biome = BiomeAt(seed, cellCenterX, cellCenterZ);
            const uint32_t treeChance = kBiomeBands[biome].treeChance;
            const uint32_t hash = detail::HashLattice(seed ^ 0x51ed270bu, cellX, cellZ);
            if ((hash & 0xffu) >= treeChance) {
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
                minTrunk + static_cast<int32_t>((hash >> 19) % static_cast<uint32_t>(trunkVariants));
            const int32_t localTrunkX = trunkWorldX - baseX;
            const int32_t localTrunkZ = trunkWorldZ - baseZ;

            for (int32_t y = 1; y <= trunkHeight; ++y) {
                PutTreeBlock(column, localTrunkX, groundY + y, localTrunkZ, block::Wood, true);
            }

            /* 树冠自下而上三层, 顶层收成一个方块; 矮树各层半径取矮树档. */
            const int32_t topY = groundY + trunkHeight;
            for (int32_t dy = -1; dy <= 2; ++dy) {
                int32_t radius = canopyRadius;
                if (dy == 1) {
                    radius = canopyRadius > 0 ? canopyRadius - 1 : 0;
                } else if (dy >= 2) {
                    radius = 0;
                }
                for (int32_t dx = -radius; dx <= radius; ++dx) {
                    for (int32_t dz = -radius; dz <= radius; ++dz) {
                        const int32_t absX = dx < 0 ? -dx : dx;
                        const int32_t absZ = dz < 0 ? -dz : dz;
                        /* 削掉底层的四角, 树冠边缘更圆; 半径 1 的矮树没有可削的角. */
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

int32_t TerrainGenerator::CarveCaves(ChunkCoord coord, ChunkColumn& column, uint32_t seed) const {
    int32_t carved = 0;
    const int32_t baseX = coord.x * kChunkSizeX;
    const int32_t baseZ = coord.z * kChunkSizeZ;
    for (int32_t localZ = 0; localZ < kChunkSizeZ; ++localZ) {
        const int32_t worldZ = baseZ + localZ;
        for (int32_t localX = 0; localX < kChunkSizeX; ++localX) {
            const int32_t worldX = baseX + localX;
            const int32_t height = SurfaceHeight(seed, worldX, worldZ);
            const int32_t topY = height - kCaveRoofDepth;
            if (topY <= kCaveMinY) {
                continue;
            }
            const float sampleX = static_cast<float>(worldX) * kCaveFreqXZ;
            const float sampleZ = static_cast<float>(worldZ) * kCaveFreqXZ;
            for (int32_t y = kCaveMinY; y <= topY; ++y) {
                if (column.Get(localX, y, localZ) != block::Stone) {
                    continue;
                }
                const float sampleY = static_cast<float>(y) * kCaveFreqY;
                const float first = detail::ValueNoise3D(seed ^ kCaveSeedA, sampleX, sampleY, sampleZ);
                if (first < -kCaveThreshold || first > kCaveThreshold) {
                    continue;
                }
                const float second = detail::ValueNoise3D(seed ^ kCaveSeedB, sampleX, sampleY, sampleZ);
                if (second < -kCaveThreshold || second > kCaveThreshold) {
                    continue;
                }
                column.Set(localX, y, localZ, block::Air);
                ++carved;
            }
        }
    }
    return carved;
}

int32_t TerrainGenerator::PlaceOres(ChunkCoord coord, ChunkColumn& column, uint32_t seed) const {
    int32_t total = 0;
    if (gen::kOreEntryCount <= 0) {
        return total;
    }
    /* 每区块的随机序列由种子与区块坐标共同决定, 同一区块反复生成结果一致. */
    uint32_t state = detail::HashLattice(seed ^ 0x9e3779b9u, coord.x, coord.z) | 1u;
    const auto next = [&state]() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    };
    const auto unit = [&next]() {
        return static_cast<float>(next() & 0x00ffffffu) / 16777216.0f;
    };

    for (int32_t index = 0; index < gen::kOreEntryCount; ++index) {
        const gen::OreEntry& ore = gen::kOreEntries[index];
        const int32_t wantBlocks = ore.maxBlocks > 0 ? ore.maxBlocks : 1;
        for (int32_t attempt = 0; attempt < ore.lodes; ++attempt) {
            if ((next() % 10000u) >= ore.odds) {
                continue;
            }
            /*
             * 布种点取区块内的随机格: 高度按原版矿脉区间取, 横向限定在本区块,
             * 因此矿脉不跨区块边界, 相邻区块各自独立布种.
             */
            int32_t x = static_cast<int32_t>(unit() * static_cast<float>(kChunkSizeX));
            int32_t z = static_cast<int32_t>(unit() * static_cast<float>(kChunkSizeZ));
            if (x >= kChunkSizeX) {
                x = kChunkSizeX - 1;
            }
            if (z >= kChunkSizeZ) {
                z = kChunkSizeZ - 1;
            }
            const int32_t span = ore.maxY > ore.minY ? ore.maxY - ore.minY : 0;
            int32_t y = ore.minY + static_cast<int32_t>(unit() * static_cast<float>(span + 1));
            if (y > ore.maxY) {
                y = ore.maxY;
            }

            /* 随机游走生长: 步数上限取目标块数的三倍, 允许走出区块后放弃该步. */
            int32_t placed = 0;
            for (int32_t step = 0; step < wantBlocks * 3 && placed < wantBlocks; ++step) {
                if (x >= 0 && x < kChunkSizeX && z >= 0 && z < kChunkSizeZ && y >= 0 && y < kWorldHeight) {
                    if (column.Get(x, y, z) == static_cast<BlockId>(ore.replace)) {
                        column.Set(x, y, z, static_cast<BlockId>(ore.block));
                        ++placed;
                        ++total;
                    }
                }
                switch (next() % 6u) {
                    case 0: ++x; break;
                    case 1: --x; break;
                    case 2: ++y; break;
                    case 3: --y; break;
                    case 4: ++z; break;
                    default: --z; break;
                }
            }
        }
    }
    return total;
}

int32_t TerrainGenerator::PlantDecorations(ChunkCoord coord, ChunkColumn& column, uint32_t seed) const {
    int32_t planted = 0;
    if (kBiomeDecorationCount <= 0) {
        return planted;
    }
    /*
     * 装饰按区块中心所属群系带取表: 群系带噪声的特征尺度约三百格, 一个 16x16 区块
     * 绝大多数情况落在同一带内, 逐列判定会让同一簇花草被切碎.
     */
    const int32_t baseX = coord.x * kChunkSizeX;
    const int32_t baseZ = coord.z * kChunkSizeZ;
    const int32_t band = BiomeAt(seed, baseX + kChunkSizeX / 2, baseZ + kChunkSizeZ / 2);
    const uint16_t topBlock = kBiomeBands[band].topBlock;

    uint32_t state = detail::HashLattice(seed ^ 0x7f4a7c15u, coord.x, coord.z) | 1u;
    const auto next = [&state]() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    };

    for (int32_t index = 0; index < kBiomeDecorationCount; ++index) {
        const BiomeDecoration& decoration = kBiomeDecorations[index];
        if (decoration.band != static_cast<uint8_t>(band)) {
            continue;
        }
        for (int32_t attempt = 0; attempt < decoration.attempts; ++attempt) {
            const int32_t localX = static_cast<int32_t>(next() % static_cast<uint32_t>(kChunkSizeX));
            const int32_t localZ = static_cast<int32_t>(next() % static_cast<uint32_t>(kChunkSizeZ));
            const int32_t height = SurfaceHeight(seed, baseX + localX, baseZ + localZ);
            int32_t targetY = 0;
            if (height <= kSeaLevel) {
                /* 水下与水面: 水草, 珊瑚与浮叶都落在海平面那一格的水面上. */
                if (column.Get(localX, kSeaLevel, localZ) != block::Water) {
                    continue;
                }
                targetY = kSeaLevel;
            } else {
                const int32_t above = height + 1;
                if (above >= kWorldHeight || column.Get(localX, height, localZ) != topBlock) {
                    continue;
                }
                if (column.Get(localX, above, localZ) != block::Air) {
                    continue;
                }
                targetY = above;
            }
            column.Set(localX, targetY, localZ, static_cast<BlockId>(decoration.block));
            ++planted;
        }
    }
    return planted;
}

void TerrainGenerator::Generate(ChunkCoord coord, ChunkColumn& column, uint32_t seed,
                                uint8_t treeShape) const {
    FillTerrain(coord, column, seed);
    /* 先挖洞穴再布矿脉: 洞穴只挖石头, 矿脉只替换石头, 二者不会互相覆盖空腔. */
    const int32_t carved = CarveCaves(coord, column, seed);
    const int32_t ores = PlaceOres(coord, column, seed);
    PlantTrees(coord, column, seed, treeShape);
    /* 装饰最后撒: 树冠占掉的格子不再长花草. */
    const int32_t decorations = PlantDecorations(coord, column, seed);
    /*
     * 生成规模按区块输出一条调试日志: 洞穴与矿脉的体量是地形手感的直接指标,
     * 数值异常时不必进游戏就能看出生成器是否失效.
     */
    IM_LOGD("ImMirrorTerrain", "chunk %d,%d carved=%d ores=%d decor=%d", coord.x, coord.z, carved, ores,
            decorations);
}

}  /* namespace immirror::voxel */
