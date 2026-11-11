#include "editor/editor_types.h"

#include "editor/generated/editor_data_gen.h"
#include "voxel/block_registry.h"

namespace immirror::editor {

namespace {

/*
 * 调色板条目直接引用生成的参数表, 只剔除空气:
 * 空气在生成表里是第 0 条, 作为可放置方块没有意义, 因此调色板从第 1 条开始.
 * 返回的指针指向静态表, 生存期与进程一致.
 */
const gen::BlockEntry* PaletteEntry(int32_t index) {
    if (index < 0) {
        return nullptr;
    }
    const int32_t entryIndex = index + 1;
    if (entryIndex >= gen::kBlockEntryCount) {
        return nullptr;
    }
    return &gen::kBlockEntries[entryIndex];
}

}  /* namespace */

bool Region::Contains(float x, float y, float z) const {
    const float halfX = static_cast<float>(sx) * 0.5f;
    const float halfY = static_cast<float>(sy) * 0.5f;
    const float halfZ = static_cast<float>(sz) * 0.5f;
    return x >= static_cast<float>(cx) - halfX && x <= static_cast<float>(cx) + halfX &&
           y >= static_cast<float>(cy) - halfY && y <= static_cast<float>(cy) + halfY &&
           z >= static_cast<float>(cz) - halfZ && z <= static_cast<float>(cz) + halfZ;
}

void Region::Normalize() {
    if (sx < 1) {
        sx = 1;
    }
    if (sy < 1) {
        sy = 1;
    }
    if (sz < 1) {
        sz = 1;
    }
}

int32_t PaletteCount() {
    const int32_t count = gen::kBlockEntryCount - 1;
    return count > 0 ? count : 0;
}

voxel::BlockId PaletteBlock(int32_t index) {
    const gen::BlockEntry* entry = PaletteEntry(index);
    return entry != nullptr ? static_cast<voxel::BlockId>(entry->clientId) : voxel::block::Air;
}

const char* PaletteKey(int32_t index) {
    const gen::BlockEntry* entry = PaletteEntry(index);
    return entry != nullptr ? entry->key : "air";
}

const char* PaletteNameZh(int32_t index) {
    const gen::BlockEntry* entry = PaletteEntry(index);
    return entry != nullptr ? entry->nameZh : "";
}

int32_t PaletteIndexOf(voxel::BlockId block) {
    for (int32_t index = 0; index < PaletteCount(); ++index) {
        if (PaletteBlock(index) == block) {
            return index;
        }
    }
    return -1;
}

int32_t PaletteWrap(int32_t index) {
    const int32_t count = PaletteCount();
    if (count <= 0) {
        return 0;
    }
    int32_t wrapped = index % count;
    if (wrapped < 0) {
        wrapped += count;
    }
    return wrapped;
}

}  /* namespace immirror::editor */
