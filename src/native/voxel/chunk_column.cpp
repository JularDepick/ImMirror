#include "voxel/chunk_column.h"

#include <algorithm>

namespace immirror::voxel {

ChunkColumn::ChunkColumn() : blocks_(static_cast<size_t>(kChunkVolume), block::Air) {}

void ChunkColumn::Fill(BlockId id) {
    std::fill(blocks_.begin(), blocks_.end(), id);
    topBlockY_ = (id == block::Air) ? -1 : (kWorldHeight - 1);
    dirty_ = true;
}

void ChunkColumn::RefreshTopBlockY() {
    for (int32_t y = topBlockY_ - 1; y >= 0; --y) {
        const size_t layerBegin = Index(0, y, 0);
        const size_t layerEnd = layerBegin + static_cast<size_t>(kChunkSizeX) * static_cast<size_t>(kChunkSizeZ);
        for (size_t index = layerBegin; index < layerEnd; ++index) {
            if (blocks_[index] != block::Air) {
                topBlockY_ = y;
                return;
            }
        }
    }
    topBlockY_ = -1;
}

}  /* namespace immirror::voxel */
