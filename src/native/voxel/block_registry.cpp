#include "voxel/block_registry.h"

#include <cstring>

namespace immirror::voxel {
namespace {

/* 图集格未指定时的回落标记, 与 BlockInfo 的约定一致. */
constexpr uint8_t kAtlasFallback = 0xFF;

/*
 * 条目顺序即方块 id. 图集格编号与渲染侧从原版资源合成的图集一致:
 * 0 草顶, 1 草侧, 2 土, 3 石, 4 沙, 5 水, 6 木侧, 7 木顶,
 * 8 叶, 9 基岩, 10 砖, 11 圆石, 12 雪, 13 冰, 14 砂砾, 15 木板.
 * 除草与木需要区分顶侧底之外, 其余方块三个面共用 atlasIndex.
 */
const BlockInfo kBlockTable[] = {
    /* name          solid  opaque  index  top    side   bottom */
    {"air", false, false, 0, kAtlasFallback, kAtlasFallback, kAtlasFallback},
    {"grass", true, true, 1, 0, 1, 2},
    {"dirt", true, true, 2, kAtlasFallback, kAtlasFallback, kAtlasFallback},
    {"stone", true, true, 3, kAtlasFallback, kAtlasFallback, kAtlasFallback},
    {"sand", true, true, 4, kAtlasFallback, kAtlasFallback, kAtlasFallback},
    {"water", false, false, 5, kAtlasFallback, kAtlasFallback, kAtlasFallback},
    {"wood", true, true, 6, 7, 6, 7},
    {"leaves", true, true, 8, kAtlasFallback, kAtlasFallback, kAtlasFallback},
    {"bedrock", true, true, 9, kAtlasFallback, kAtlasFallback, kAtlasFallback},
    {"brick", true, true, 10, kAtlasFallback, kAtlasFallback, kAtlasFallback},
    {"cobblestone", true, true, 11, kAtlasFallback, kAtlasFallback, kAtlasFallback},
    {"snow", true, true, 12, kAtlasFallback, kAtlasFallback, kAtlasFallback},
    {"ice", true, true, 13, kAtlasFallback, kAtlasFallback, kAtlasFallback},
    {"gravel", true, true, 14, kAtlasFallback, kAtlasFallback, kAtlasFallback},
    {"planks", true, true, 15, kAtlasFallback, kAtlasFallback, kAtlasFallback},
};

constexpr size_t kBlockTableSize = sizeof(kBlockTable) / sizeof(kBlockTable[0]);

static_assert(kBlockTableSize == static_cast<size_t>(block::Count),
              "方块表条目数与 block 命名空间的 id 常量不一致");

/* 越界 id 的回落条目, 与空气保持完全一致. */
const BlockInfo kFallbackBlock = {"air", false, false, 0, kAtlasFallback, kAtlasFallback, kAtlasFallback};

}  /* namespace */

BlockRegistry::BlockRegistry() = default;

const BlockRegistry& BlockRegistry::Instance() {
    static const BlockRegistry instance;
    return instance;
}

const BlockInfo& BlockRegistry::Info(BlockId id) const {
    const size_t index = static_cast<size_t>(id);
    if (index >= kBlockTableSize) {
        return kFallbackBlock;
    }
    return kBlockTable[index];
}

BlockId BlockRegistry::FindByName(const char* name) const {
    if (name == nullptr) {
        return block::Air;
    }
    for (size_t i = 0; i < kBlockTableSize; ++i) {
        if (std::strcmp(kBlockTable[i].name, name) == 0) {
            return static_cast<BlockId>(i);
        }
    }
    return block::Air;
}

size_t BlockRegistry::Count() const {
    return kBlockTableSize;
}

}  /* namespace immirror::voxel */
