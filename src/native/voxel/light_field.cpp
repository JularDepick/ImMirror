#include "voxel/light_field.h"

#include <algorithm>

namespace immirror::voxel {
namespace {

/* 六方向的坐标增量, 传播扩散按此顺序遍历邻居. */
constexpr int32_t kOffsetX[6] = {1, -1, 0, 0, 0, 0};
constexpr int32_t kOffsetY[6] = {0, 0, 1, -1, 0, 0};
constexpr int32_t kOffsetZ[6] = {0, 0, 0, 0, 1, -1};

/* 一维下标还原成列内坐标, 三个分量与 ChunkColumn::Index 的打包顺序一致. */
inline int32_t CoordXOf(size_t index) {
    return static_cast<int32_t>(index % static_cast<size_t>(kChunkSizeX));
}

inline int32_t CoordZOf(size_t index) {
    return static_cast<int32_t>((index / static_cast<size_t>(kChunkSizeX)) % static_cast<size_t>(kChunkSizeZ));
}

inline int32_t CoordYOf(size_t index) {
    return static_cast<int32_t>(index / (static_cast<size_t>(kChunkSizeX) * static_cast<size_t>(kChunkSizeZ)));
}

/* 下标是否落在列内, 扩散时用于拒绝越界邻居. */
inline bool InColumn(int32_t x, int32_t y, int32_t z) {
    return x >= 0 && x < kChunkSizeX && y >= 0 && y < kWorldHeight && z >= 0 && z < kChunkSizeZ;
}

}  /* namespace */

uint8_t LightField::StepCostOf(const BlockInfo& info) {
    if (info.opaque) {
        return kMaxLightLevel;
    }
    switch (info.coverMode) {
        case 4:
            return 0;
        case 2:
            return info.lightAtten > 0 ? info.lightAtten : static_cast<uint8_t>(1);
        default:
            return info.lightAtten;
    }
}

void LightField::Recompute(ChunkColumn& column, const ChunkColumn* const neighbors[4],
                          const BlockRegistry& registry) {
    std::vector<uint8_t>& sky = column.SkyLightData();
    std::vector<uint8_t>& block = column.BlockLightData();
    /* 容量守卫: 正常构造与批量赋值后两条向量都是全列长度, 异常时按全亮回落. */
    if (sky.size() != static_cast<size_t>(kChunkVolume)) {
        sky.assign(static_cast<size_t>(kChunkVolume), kMaxLightLevel);
    }
    if (block.size() != static_cast<size_t>(kChunkVolume)) {
        block.assign(static_cast<size_t>(kChunkVolume), 0);
    }

    const int32_t topY = column.TopBlockY();
    const std::vector<BlockId>& blocks = column.Blocks();
    if (topY < 0) {
        /* 整列为空: 天光保持满值, 方块光为零, 无需扩散. */
        std::fill(sky.begin(), sky.end(), kMaxLightLevel);
        std::fill(block.begin(), block.end(), static_cast<uint8_t>(0));
        return;
    }

    skyQueue_.clear();
    blockQueue_.clear();

    /*
     * 第一段: 自顶向下按垂直列折算天光直射分量.
     * 空气代价为零, 直射光一路保持满值; 穿过水与树叶这类半透方块时
     * 按传播代价逐层扣减; 触到实心方块后直射归零, 下方改由横向扩散补光.
     * 同一轮顺带登记方块光源, 光源格取值与自身是否遮挡无关.
     */
    for (int32_t z = 0; z < kChunkSizeZ; ++z) {
        for (int32_t x = 0; x < kChunkSizeX; ++x) {
            uint8_t running = kMaxLightLevel;
            for (int32_t y = kWorldHeight - 1; y >= 0; --y) {
                const size_t index = ChunkColumn::Index(x, y, z);
                const BlockInfo& info = registry.Info(blocks[index]);
                const uint8_t cost = StepCostOf(info);
                if (cost >= kMaxLightLevel) {
                    sky[index] = 0;
                    running = 0;
                } else {
                    running = running > cost ? static_cast<uint8_t>(running - cost) : static_cast<uint8_t>(0);
                    sky[index] = running;
                }
                if (info.lightSource > 0) {
                    block[index] = info.lightSource;
                    blockQueue_.push_back(index);
                }
            }
        }
    }

    /* 第二段: 登记天光扩散源, 只扫最高方块以下的区域, 其上恒为满值. */
    for (int32_t z = 0; z < kChunkSizeZ; ++z) {
        for (int32_t x = 0; x < kChunkSizeX; ++x) {
            for (int32_t y = 0; y <= topY; ++y) {
                const size_t index = ChunkColumn::Index(x, y, z);
                if (sky[index] >= 2) {
                    skyQueue_.push_back(index);
                }
            }
        }
    }

    /*
     * 第三段: 跨列播种.
     * 相邻列边界格的亮度按同一套规则 (源值减一再减本格代价) 播种进本列边界面,
     * 只抬高不降低, 因此不会把本列自己的直射光算丢. 写入只发生在本列,
     * 相邻列由世界侧按改动点触发各自重算, 两侧最终收敛到同一结果.
     */
    for (int32_t dir = 0; dir < 4; ++dir) {
        const ChunkColumn* neighbor = neighbors[dir];
        if (neighbor == nullptr) {
            continue;
        }
        for (int32_t y = 0; y < kWorldHeight; ++y) {
            for (int32_t lateral = 0; lateral < kChunkSizeX; ++lateral) {
                int32_t x = 0;
                int32_t z = 0;
                int32_t nx = 0;
                int32_t nz = 0;
                if (dir == 0) {
                    x = kChunkSizeX - 1;
                    z = lateral;
                    nx = 0;
                    nz = lateral;
                } else if (dir == 1) {
                    x = 0;
                    z = lateral;
                    nx = kChunkSizeX - 1;
                    nz = lateral;
                } else if (dir == 2) {
                    x = lateral;
                    z = kChunkSizeZ - 1;
                    nx = lateral;
                    nz = 0;
                } else {
                    x = lateral;
                    z = 0;
                    nx = lateral;
                    nz = kChunkSizeZ - 1;
                }
                const size_t here = ChunkColumn::Index(x, y, z);
                const uint8_t cost = StepCostOf(registry.Info(blocks[here]));
                if (cost >= kMaxLightLevel) {
                    continue;
                }
                const int32_t skyCandidate =
                    static_cast<int32_t>(neighbor->SkyLightAt(nx, y, nz)) - 1 - cost;
                if (skyCandidate > static_cast<int32_t>(sky[here])) {
                    sky[here] = static_cast<uint8_t>(skyCandidate);
                    skyQueue_.push_back(here);
                }
                const int32_t blockCandidate =
                    static_cast<int32_t>(neighbor->BlockLightAt(nx, y, nz)) - 1 - cost;
                if (blockCandidate > static_cast<int32_t>(block[here])) {
                    block[here] = static_cast<uint8_t>(blockCandidate);
                    blockQueue_.push_back(here);
                }
            }
        }
    }

    /*
     * 第四段: 广度优先扩散. 每步候选值为源值减一再减目标格代价,
     * 只有能抬高目标亮度时才写入并续排; 封死方块的代价让扩散自然止步.
     * 天光与方块光两个通道独立成队, 互不干扰.
     */
    size_t head = 0;
    while (head < skyQueue_.size()) {
        const size_t index = skyQueue_[head++];
        const int32_t value = sky[index];
        if (value <= 1) {
            continue;
        }
        const int32_t x = CoordXOf(index);
        const int32_t y = CoordYOf(index);
        const int32_t z = CoordZOf(index);
        for (int32_t dir = 0; dir < 6; ++dir) {
            const int32_t nx = x + kOffsetX[dir];
            const int32_t ny = y + kOffsetY[dir];
            const int32_t nz = z + kOffsetZ[dir];
            if (!InColumn(nx, ny, nz)) {
                continue;
            }
            const size_t next = ChunkColumn::Index(nx, ny, nz);
            const uint8_t cost = StepCostOf(registry.Info(blocks[next]));
            const int32_t candidate = value - 1 - cost;
            if (candidate > sky[next]) {
                sky[next] = static_cast<uint8_t>(candidate);
                skyQueue_.push_back(next);
            }
        }
    }

    head = 0;
    while (head < blockQueue_.size()) {
        const size_t index = blockQueue_[head++];
        const int32_t value = block[index];
        if (value <= 1) {
            continue;
        }
        const int32_t x = CoordXOf(index);
        const int32_t y = CoordYOf(index);
        const int32_t z = CoordZOf(index);
        for (int32_t dir = 0; dir < 6; ++dir) {
            const int32_t nx = x + kOffsetX[dir];
            const int32_t ny = y + kOffsetY[dir];
            const int32_t nz = z + kOffsetZ[dir];
            if (!InColumn(nx, ny, nz)) {
                continue;
            }
            const size_t next = ChunkColumn::Index(nx, ny, nz);
            const uint8_t cost = StepCostOf(registry.Info(blocks[next]));
            const int32_t candidate = value - 1 - cost;
            if (candidate > block[next]) {
                block[next] = static_cast<uint8_t>(candidate);
                blockQueue_.push_back(next);
            }
        }
    }
}

}  /* namespace immirror::voxel */
