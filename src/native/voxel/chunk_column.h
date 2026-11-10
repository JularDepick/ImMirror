#pragma once

#include <cstddef>
#include <vector>

#include "voxel/block_registry.h"
#include "voxel/voxel_types.h"

namespace immirror::voxel {

/*
 * 16x256x16 的列式区块. 内部按 y 分层存放, 同一层的 16x16 数据连续,
 * 网格构建自下而上顺序扫描时访存局部性最好.
 * 所有越界访问都是安全的: 读返回空气, 写被忽略.
 */
class ChunkColumn {
public:
    /* 构造后整列为空气且处于脏状态. */
    ChunkColumn();

    BlockId Get(int32_t x, int32_t y, int32_t z) const;
    void Set(int32_t x, int32_t y, int32_t z, BlockId id);

    bool Dirty() const { return dirty_; }
    void ClearDirty() { dirty_ = false; }

    std::vector<BlockId>& Blocks() { return blocks_; }
    const std::vector<BlockId>& Blocks() const { return blocks_; }

    /* 最高非空气方块的 y, 整列为空时返回 -1; 网格构建据此截断扫描高度. */
    int32_t TopBlockY() const { return topBlockY_; }

    /* 整列填充同一方块, 用于重置或清空. */
    void Fill(BlockId id);

    static constexpr bool InRange(int32_t x, int32_t y, int32_t z) {
        return x >= 0 && x < kChunkSizeX && y >= 0 && y < kWorldHeight && z >= 0 && z < kChunkSizeZ;
    }

    static constexpr size_t Index(int32_t x, int32_t y, int32_t z) {
        return (static_cast<size_t>(y) * static_cast<size_t>(kChunkSizeZ) + static_cast<size_t>(z)) *
                   static_cast<size_t>(kChunkSizeX) +
               static_cast<size_t>(x);
    }

private:
    /* 顶部方块被挖空后自顶向下重算最高非空气层. */
    void RefreshTopBlockY();

    std::vector<BlockId> blocks_;
    int32_t topBlockY_ = -1;
    bool dirty_ = true;
};

inline BlockId ChunkColumn::Get(int32_t x, int32_t y, int32_t z) const {
    if (!InRange(x, y, z)) {
        return block::Air;
    }
    return blocks_[Index(x, y, z)];
}

inline void ChunkColumn::Set(int32_t x, int32_t y, int32_t z, BlockId id) {
    if (!InRange(x, y, z)) {
        return;
    }
    const size_t index = Index(x, y, z);
    if (blocks_[index] == id) {
        return;
    }
    blocks_[index] = id;
    dirty_ = true;
    if (id != block::Air) {
        if (y > topBlockY_) {
            topBlockY_ = y;
        }
    } else if (y == topBlockY_) {
        RefreshTopBlockY();
    }
}

}  /* namespace immirror::voxel */
