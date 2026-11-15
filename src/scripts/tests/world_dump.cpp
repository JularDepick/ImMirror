/*
 * 世界地形转储: 给定种子与区块范围, 输出群系带分布, 高度图与方块统计.
 * 用法: world_dump <seed> <radius> [--tree-shape N]
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "voxel/chunk_column.h"
#include "voxel/generated/biome_data_gen.h"
#include "voxel/terrain_generator.h"
#include "voxel/voxel_types.h"

using namespace immirror::voxel;

int main(int argc, char** argv) {
    uint32_t seed = 20260101u;
    int32_t radius = 2;
    uint8_t treeShape = 1;
    bool seedSet = false;
    bool radiusSet = false;
    for (int32_t i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--tree-shape") == 0 && i + 1 < argc) {
            treeShape = static_cast<uint8_t>(std::atoi(argv[++i]));
        } else if (std::strcmp(argv[i], "--biome-scan") == 0) {
            /* 该开关在下方单独处理 */
        } else if (!seedSet) {
            seed = static_cast<uint32_t>(std::strtoul(argv[i], nullptr, 10));
            seedSet = true;
        } else if (!radiusSet) {
            radius = std::atoi(argv[i]);
            radiusSet = true;
        }
    }

    /* --biome-scan: 只扫群系带, 不生成地形, 用于看噪声覆盖范围 */
    bool biomeScan = false;
    for (int32_t i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--biome-scan") == 0) {
            biomeScan = true;
        }
    }
    TerrainGenerator generator;
    if (biomeScan) {
        std::map<int32_t, int32_t> scan;
        const int32_t span = 20000;
        const int32_t step = 200;
        int32_t samples = 0;
        for (int32_t z = -span; z <= span; z += step) {
            for (int32_t x = -span; x <= span; x += step) {
                scan[generator.BiomeAt(seed, x, z)] += 1;
                samples += 1;
            }
        }
        std::printf("群系扫描 | 种子 %u | 范围 +-%d 步长 %d | 样本 %d\n", seed, span, step, samples);
        for (int32_t i = 0; i < kBiomeBandCount; ++i) {
            const auto found = scan.find(i);
            const int32_t n = found == scan.end() ? 0 : found->second;
            std::printf("  带 %d %-10s %6d (%.1f%%)\n", i, kBiomeBands[i].key, n,
                        100.0 * n / samples);
        }
        return 0;
    }
    std::map<int32_t, int32_t> biomeCount;
    std::map<uint16_t, int64_t> blockCount;
    int64_t totalBlocks = 0;
    int32_t surfaceMin = kWorldHeight;
    int32_t surfaceMax = 0;
    int64_t surfaceSum = 0;
    int32_t columns = 0;
    int64_t undergroundAir = 0;   /* 地表以下的空气格: 洞穴指标 */
    int64_t undergroundSolid = 0; /* 地表以下的实心格 */
    int64_t waterBlocks = 0;      /* 液体格 */
    std::map<int32_t, int64_t> layerAir; /* 每 16 层的空气格数 */

    for (int32_t cz = -radius; cz <= radius; ++cz) {
        for (int32_t cx = -radius; cx <= radius; ++cx) {
            ChunkCoord coord;
            coord.x = cx;
            coord.z = cz;
            ChunkColumn column;
            generator.Generate(coord, column, seed, treeShape);
            for (int32_t lx = 0; lx < kChunkSizeX; ++lx) {
                for (int32_t lz = 0; lz < kChunkSizeZ; ++lz) {
                    const int32_t worldX = cx * kChunkSizeX + lx;
                    const int32_t worldZ = cz * kChunkSizeZ + lz;
                    const int32_t band = generator.BiomeAt(seed, worldX, worldZ);
                    biomeCount[band] += 1;
                    int32_t surface = -1;
                    for (int32_t y = kWorldHeight - 1; y >= 0; --y) {
                        const BlockId id = column.Get(lx, y, lz);
                        if (id != 0) {
                            blockCount[id] += 1;
                            totalBlocks += 1;
                            if (surface < 0) {
                                surface = y;
                            }
                        }
                    }
                    /* 地表以下: 空气格即洞穴, 液体格单独计 */
                    for (int32_t y = 0; y < surface; ++y) {
                        const BlockId id = column.Get(lx, y, lz);
                        if (id == 0) {
                            undergroundAir += 1;
                            layerAir[y / 16] += 1;
                        } else {
                            undergroundSolid += 1;
                            if (column.FluidLevel(lx, y, lz) > 0 || id == 8 || id == 9) {
                                waterBlocks += 1;
                            }
                        }
                    }
                    if (surface >= 0) {
                        if (surface < surfaceMin) {
                            surfaceMin = surface;
                        }
                        if (surface > surfaceMax) {
                            surfaceMax = surface;
                        }
                        surfaceSum += surface;
                        columns += 1;
                    }
                }
            }
        }
    }

    std::printf("世界转储 | 种子 %u | 半径 %d | 树形 %u\n", seed, radius, treeShape);
    std::printf("区块数 %d | 列数 %d | 总方块 %lld\n",
                (radius * 2 + 1) * (radius * 2 + 1), columns,
                static_cast<long long>(totalBlocks));
    std::printf("表面高度 最低 %d 最高 %d 平均 %.1f\n", surfaceMin, surfaceMax,
                columns > 0 ? static_cast<double>(surfaceSum) / columns : 0.0);
    std::printf("== 群系带分布\n");
    for (int32_t i = 0; i < kBiomeBandCount; ++i) {
        const auto found = biomeCount.find(i);
        const int32_t n = found == biomeCount.end() ? 0 : found->second;
        std::printf("  带 %d %-14s %-10s %6d 列 (%.1f%%)\n", i, kBiomeBands[i].key,
                    kBiomeBands[i].nameZh, n,
                    columns > 0 ? 100.0 * n / columns : 0.0);
    }
    std::printf("== 方块统计(前 20)\n");
    std::vector<std::pair<uint16_t, int64_t>> sorted(blockCount.begin(), blockCount.end());
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    for (size_t i = 0; i < sorted.size() && i < 20; ++i) {
        std::printf("  方块 %5u  %8lld 个\n", sorted[i].first,
                    static_cast<long long>(sorted[i].second));
    }
    std::printf("方块种类 %zu\n", sorted.size());
    const int64_t undergroundTotal = undergroundAir + undergroundSolid;
    std::printf("== 地下结构\n");
    std::printf("  地下格 %lld, 空气 %lld (%.2f%%), 实心 %lld\n",
                static_cast<long long>(undergroundTotal),
                static_cast<long long>(undergroundAir),
                undergroundTotal > 0 ? 100.0 * undergroundAir / undergroundTotal : 0.0,
                static_cast<long long>(undergroundSolid));
    std::printf("  液体格 %lld\n", static_cast<long long>(waterBlocks));
    std::printf("== 每 16 层空气格\n");
    for (const auto& item : layerAir) {
        std::printf("  y %3d-%3d  %8lld\n", item.first * 16, item.first * 16 + 15,
                    static_cast<long long>(item.second));
    }
    return 0;
}
