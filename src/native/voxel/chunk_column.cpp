#include "voxel/chunk_column.h"

#include <algorithm>

namespace immirror::voxel {

ChunkColumn::ChunkColumn()
    : blocks_(static_cast<size_t>(kChunkVolume), block::Air),
      /* 天光初始按全亮假设填充, 方块光清零; 正式值由 LightField 重算写入. */
      skyLight_(static_cast<size_t>(kChunkVolume), kMaxLightLevel),
      blockLight_(static_cast<size_t>(kChunkVolume), 0) {}

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


void ChunkColumn::AssignBlocks(const std::vector<BlockId>& data) {
    if (data.size() != blocks_.size()) {
        return;
    }
    blocks_ = data;
    /* 方块数据整体替换后旧光照失效, 回到全亮天光假设, 等待世界侧重算. */
    std::fill(skyLight_.begin(), skyLight_.end(), kMaxLightLevel);
    std::fill(blockLight_.begin(), blockLight_.end(), static_cast<uint8_t>(0));
    dirty_ = true;
    /* RefreshTopBlockY 从当前最高层往下扫描, 批量赋值时必须先置顶再重算. */
    topBlockY_ = kWorldHeight - 1;
    RefreshTopBlockY();
}

}  /* namespace immirror::voxel */
