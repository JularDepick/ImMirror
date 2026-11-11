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
 *
 * 硬度, 采集工具与采集材质族取自原版 blockdef.csv 的实测值:
 * 草 3.5 铲, 石 8 镐, 沙 3 铲, 雪 1.5 铲, 冰 5 镐, 木板 6 斧, 树叶 0.4 徒手.
 * 表里没有单列的方块按原版归族: 土同草, 圆石与砖同石, 原木同木板,
 * 砂砾的硬度与工具同沙 (硬度和工具回落), 音效仍取砂砾族, 因为音效枚举里
 * 已按材质分出了砾石一族, 用它与沙区分开才是原版的材质反馈.
 * 水与基岩硬度取 -1, 即不可破坏.
 */
constexpr BlockInfo kBlockTable[] = {
    /* name          solid  opaque  index  top    side   bottom   hardness  tool   material */
    {"air", false, false, 0, kAtlasFallback, kAtlasFallback, kAtlasFallback, -1.0f,
     HarvestTool::kHand, HarvestMaterial::kNone},
    {"grass", true, true, 1, 0, 1, 2, 3.5f, HarvestTool::kShovel, HarvestMaterial::kGrass},
    {"dirt", true, true, 2, kAtlasFallback, kAtlasFallback, kAtlasFallback, 3.5f,
     HarvestTool::kShovel, HarvestMaterial::kGrass},
    {"stone", true, true, 3, kAtlasFallback, kAtlasFallback, kAtlasFallback, 8.0f,
     HarvestTool::kPickaxe, HarvestMaterial::kStone},
    {"sand", true, true, 4, kAtlasFallback, kAtlasFallback, kAtlasFallback, 3.0f,
     HarvestTool::kShovel, HarvestMaterial::kSand},
    {"water", false, false, 5, kAtlasFallback, kAtlasFallback, kAtlasFallback, -1.0f,
     HarvestTool::kHand, HarvestMaterial::kNone},
    {"wood", true, true, 6, 7, 6, 7, 6.0f, HarvestTool::kAxe, HarvestMaterial::kWood},
    {"leaves", true, true, 8, kAtlasFallback, kAtlasFallback, kAtlasFallback, 0.4f,
     HarvestTool::kHand, HarvestMaterial::kGrass},
    {"bedrock", true, true, 9, kAtlasFallback, kAtlasFallback, kAtlasFallback, -1.0f,
     HarvestTool::kHand, HarvestMaterial::kNone},
    {"brick", true, true, 10, kAtlasFallback, kAtlasFallback, kAtlasFallback, 8.0f,
     HarvestTool::kPickaxe, HarvestMaterial::kStone},
    {"cobblestone", true, true, 11, kAtlasFallback, kAtlasFallback, kAtlasFallback, 8.0f,
     HarvestTool::kPickaxe, HarvestMaterial::kStone},
    {"snow", true, true, 12, kAtlasFallback, kAtlasFallback, kAtlasFallback, 1.5f,
     HarvestTool::kShovel, HarvestMaterial::kSnow},
    {"ice", true, true, 13, kAtlasFallback, kAtlasFallback, kAtlasFallback, 5.0f,
     HarvestTool::kPickaxe, HarvestMaterial::kStone},
    {"gravel", true, true, 14, kAtlasFallback, kAtlasFallback, kAtlasFallback, 3.0f,
     HarvestTool::kShovel, HarvestMaterial::kGravel},
    {"planks", true, true, 15, kAtlasFallback, kAtlasFallback, kAtlasFallback, 6.0f,
     HarvestTool::kAxe, HarvestMaterial::kWood},
};

constexpr size_t kBlockTableSize = sizeof(kBlockTable) / sizeof(kBlockTable[0]);

static_assert(kBlockTableSize == static_cast<size_t>(block::Count),
              "方块表条目数与 block 命名空间的 id 常量不一致");

/*
 * 原版 hardness 的合法范围是 -1 到 200, 绝大多数条目落在 0.5 的网格上,
 * 树叶实测为 0.4, 因此这里只校验范围, 不对步长做强制约束.
 * 整张表在编译期校验, 填入越界硬度会直接编译失败.
 */
constexpr float kHardnessMin = -1.0f;
constexpr float kHardnessMax = 200.0f;

constexpr bool HardnessTableValid() {
    for (size_t i = 0; i < kBlockTableSize; ++i) {
        const float value = kBlockTable[i].hardness;
        if (value < kHardnessMin || value > kHardnessMax) {
            return false;
        }
    }
    return true;
}

static_assert(HardnessTableValid(), "方块硬度超出原版的 -1 到 200 范围");

/* 越界 id 的回落条目, 与空气保持完全一致, 也就是不可破坏. */
constexpr BlockInfo kFallbackBlock = {"air", false, false, 0, kAtlasFallback, kAtlasFallback,
                                     kAtlasFallback, -1.0f, HarvestTool::kHand,
                                     HarvestMaterial::kNone};

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
