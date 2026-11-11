#pragma once

#include <cstddef>
#include <cstdint>

#include "voxel/voxel_types.h"

namespace immirror::voxel {

/*
 * 方块 id 常量. 兼容段 0 到 14 的取值与注册表条目顺序一致, 0 恒为空气;
 * 世界数据与存档直接引用这些数值, 兼容段的取值只能保持, 不得重排或复用.
 * 15 起是全量扩展段, 客户端 id 由 scripts/gen_block_data.py 按类别分段分配,
 * 段区间与原版 ID 的对照见 game/generated/block_data_gen.h 的生成说明.
 */
namespace block {
constexpr BlockId Air = 0;
constexpr BlockId Grass = 1;
constexpr BlockId Dirt = 2;
constexpr BlockId Stone = 3;
constexpr BlockId Sand = 4;
constexpr BlockId Water = 5;
constexpr BlockId Wood = 6;
constexpr BlockId Leaves = 7;
constexpr BlockId Bedrock = 8;
constexpr BlockId Brick = 9;
constexpr BlockId Cobblestone = 10;
constexpr BlockId Snow = 11;
constexpr BlockId Ice = 12;
constexpr BlockId Gravel = 13;
constexpr BlockId Planks = 14;

/*
 * 兼容段方块数, 也是扩展段客户端 id 的起点.
 * 生成表与注册表都以它为界, 数值改动会破坏存档兼容.
 */
constexpr BlockId LegacyCount = 15;

/*
 * 全量方块数, 与生成表 kBlockEntryCount 一致, 由 block_registry.cpp
 * 编译期校验. 扩展段方块在运行时只经 Info() 查询, 不再有独立常量.
 */
constexpr BlockId Count = 1191;

/*
 * 兼容段的英文键, 与旧注册表完全一致, 供存档迁移与调试引用.
 */
constexpr const char* kLegacyNames[LegacyCount] = {
    "air", "grass", "dirt", "stone", "sand", "water", "wood", "leaves",
    "bedrock", "brick", "cobblestone", "snow", "ice", "gravel", "planks",
};

}  /* namespace block */

/*
 * 方块静态属性注册表. 进程内唯一且只读, 世界与网格构建都通过它查询
 * 名称, 实心与遮挡属性, 图集格索引, 硬度, 采集工具与材质族.
 * 数据分两段:
 *   兼容段 0 到 LegacyCount - 1: 手写表, 与旧版逐字段一致, 保证存档兼容;
 *   扩展段 LegacyCount 起: 引用 game/generated/block_data_gen.h 的
 *   kRegistryEntries, 由生成器从原版 blockdef.csv 全量折算生成.
 * 每个方块可以分别指定顶面, 侧面与底面的图集格, 未指定 (0xFF) 的面回落到
 * atlasIndex; 扩展段统一回落, 图集格索引后续随图集扩充接入.
 */
class BlockRegistry {
public:
    static const BlockRegistry& Instance();

    /* 越界 id 一律回落到空气条目, 调用方不必判空. */
    const BlockInfo& Info(BlockId id) const;

    /* 按名称查找, 未知名称与空指针返回空气 id. */
    BlockId FindByName(const char* name) const;

    /* 已注册条目数量, 也是合法 id 的上界. */
    size_t Count() const;

private:
    /* 只允许 Instance() 构造, 避免出现第二份表. */
    BlockRegistry();
};

}  /* namespace immirror::voxel */
