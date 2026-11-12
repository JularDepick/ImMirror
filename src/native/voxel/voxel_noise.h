#pragma once

#include <cmath>
#include <cstdint>

namespace immirror::voxel::detail {

/*
 * 本文件提供地形生成所需的最小噪声集合, 只在本目录内部使用.
 * 取值完全由种子与整数格点坐标决定, 不含任何隐藏状态,
 * 因此同一世界坐标在不同区块里得到的采样值必然相同.
 */

/* 32 位整数雪崩混合, 用作格点取值哈希的最后一步. */
inline uint32_t MixBits(uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

/* 种子与整数格点坐标映射到 32 位哈希. */
inline uint32_t HashLattice(uint32_t seed, int32_t x, int32_t z) {
    uint32_t hash = seed * 0x9e3779b9u;
    hash ^= MixBits(static_cast<uint32_t>(x) * 0x85ebca6bu);
    hash ^= MixBits(static_cast<uint32_t>(z) * 0xc2b2ae35u + 0x27d4eb2fu);
    return MixBits(hash);
}

/* 格点取值归一化到 [-1, 1]. */
inline float LatticeValue(uint32_t seed, int32_t x, int32_t z) {
    const uint32_t bits = HashLattice(seed, x, z) & 0x00ffffffu;
    return static_cast<float>(bits) * (2.0f / 16777215.0f) - 1.0f;
}

/* 三次平滑权重, 使格点交界处一阶导连续, 避免高度出现折痕. */
inline float SmoothStep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

/* 双线性平滑值噪声, 输出范围 [-1, 1]. */
inline float ValueNoise(uint32_t seed, float x, float z) {
    const float floorX = std::floor(x);
    const float floorZ = std::floor(z);
    const int32_t cellX = static_cast<int32_t>(floorX);
    const int32_t cellZ = static_cast<int32_t>(floorZ);
    const float weightX = SmoothStep(x - floorX);
    const float weightZ = SmoothStep(z - floorZ);
    const float value00 = LatticeValue(seed, cellX, cellZ);
    const float value10 = LatticeValue(seed, cellX + 1, cellZ);
    const float value01 = LatticeValue(seed, cellX, cellZ + 1);
    const float value11 = LatticeValue(seed, cellX + 1, cellZ + 1);
    const float lower = value00 + (value10 - value00) * weightX;
    const float upper = value01 + (value11 - value01) * weightX;
    return lower + (upper - lower) * weightZ;
}

/* 种子与三维整数格点映射到 32 位哈希, 供洞穴噪声使用. */
inline uint32_t HashLattice3(uint32_t seed, int32_t x, int32_t y, int32_t z) {
    uint32_t hash = seed * 0x9e3779b9u;
    hash ^= MixBits(static_cast<uint32_t>(x) * 0x85ebca6bu);
    hash ^= MixBits(static_cast<uint32_t>(y) * 0xc2b2ae35u + 0x27d4eb2fu);
    hash ^= MixBits(static_cast<uint32_t>(z) * 0x165667b1u + 0x9e3779b9u);
    return MixBits(hash);
}

/* 三维格点取值归一化到 [-1, 1]. */
inline float LatticeValue3(uint32_t seed, int32_t x, int32_t y, int32_t z) {
    const uint32_t bits = HashLattice3(seed, x, y, z) & 0x00ffffffu;
    return static_cast<float>(bits) * (2.0f / 16777215.0f) - 1.0f;
}

/*
 * 三线性平滑值噪声, 输出范围 [-1, 1].
 * 洞穴雕刻按 |noise| < 阈值的等值面取空腔, 因此必须三维: 二维噪声只会切出竖直的墙.
 */
inline float ValueNoise3D(uint32_t seed, float x, float y, float z) {
    const float floorX = std::floor(x);
    const float floorY = std::floor(y);
    const float floorZ = std::floor(z);
    const int32_t cellX = static_cast<int32_t>(floorX);
    const int32_t cellY = static_cast<int32_t>(floorY);
    const int32_t cellZ = static_cast<int32_t>(floorZ);
    const float weightX = SmoothStep(x - floorX);
    const float weightY = SmoothStep(y - floorY);
    const float weightZ = SmoothStep(z - floorZ);

    const float v000 = LatticeValue3(seed, cellX, cellY, cellZ);
    const float v100 = LatticeValue3(seed, cellX + 1, cellY, cellZ);
    const float v010 = LatticeValue3(seed, cellX, cellY + 1, cellZ);
    const float v110 = LatticeValue3(seed, cellX + 1, cellY + 1, cellZ);
    const float v001 = LatticeValue3(seed, cellX, cellY, cellZ + 1);
    const float v101 = LatticeValue3(seed, cellX + 1, cellY, cellZ + 1);
    const float v011 = LatticeValue3(seed, cellX, cellY + 1, cellZ + 1);
    const float v111 = LatticeValue3(seed, cellX + 1, cellY + 1, cellZ + 1);

    const float lower0 = v000 + (v100 - v000) * weightX;
    const float lower1 = v010 + (v110 - v010) * weightX;
    const float upper0 = v001 + (v101 - v001) * weightX;
    const float upper1 = v011 + (v111 - v011) * weightX;
    const float front = lower0 + (lower1 - lower0) * weightY;
    const float back = upper0 + (upper1 - upper0) * weightY;
    return front + (back - front) * weightZ;
}

}  /* namespace immirror::voxel::detail */