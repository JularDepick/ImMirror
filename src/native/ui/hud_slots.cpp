#include "ui/hud_slots.h"

namespace immirror::ui {
namespace {

/* 图集格未指定时的回落标记, 与 BlockInfo 的约定一致. */
constexpr uint8_t kAtlasFallback = 0xFF;

/*
 * 槽位顺序即方块栏从左到右的顺序. 新增槽位只需在这张表里追加,
 * 界面图标与编辑材质同时生效, 不存在第二处需要同步的硬编码.
 */
constexpr voxel::BlockId kSlotBlocks[kBlockSlotCount] = {
    voxel::block::Grass, voxel::block::Dirt,  voxel::block::Stone,
    voxel::block::Sand,  voxel::block::Wood,  voxel::block::Leaves,
    voxel::block::Brick, voxel::block::Cobblestone, voxel::block::Snow,
};

int32_t ClampSlot(int32_t slot) {
    if (slot < 0) {
        return 0;
    }
    if (slot >= kBlockSlotCount) {
        return kBlockSlotCount - 1;
    }
    return slot;
}

}  /* namespace */

voxel::BlockId BlockSlotId(int32_t slot) {
    return kSlotBlocks[ClampSlot(slot)];
}

const char* BlockSlotName(int32_t slot) {
    return voxel::BlockRegistry::Instance().Info(BlockSlotId(slot)).name;
}

int32_t BlockSlotAtlasCell(int32_t slot) {
    const voxel::BlockInfo& info = voxel::BlockRegistry::Instance().Info(BlockSlotId(slot));
    return info.atlasTop == kAtlasFallback ? static_cast<int32_t>(info.atlasIndex)
                                           : static_cast<int32_t>(info.atlasTop);
}

int32_t BlockSlotIndexOf(voxel::BlockId id) {
    for (int32_t slot = 0; slot < kBlockSlotCount; ++slot) {
        if (kSlotBlocks[slot] == id) {
            return slot;
        }
    }
    return -1;
}

}  /* namespace immirror::ui */
