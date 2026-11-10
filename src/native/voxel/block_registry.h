#pragma once

#include <cstddef>

#include "voxel/voxel_types.h"

namespace immirror::voxel {

/*
 * 方块 id 常量. 取值与注册表条目顺序一致, 0 恒为空气.
 * 世界数据与存档直接引用这些数值, 只能追加, 不得重排或复用旧值.
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
constexpr BlockId Count = 15;
}  /* namespace block */

/*
 * 方块静态属性注册表. 进程内唯一且只读, 世界与网格构建都通过它查询
 * 名称, 实心与遮挡属性以及图集格索引. 实例在首次访问时构造, 生存期与进程一致.
 * 每个方块可以分别指定顶面, 侧面与底面的图集格, 未指定 (0xFF) 的面回落到 atlasIndex.
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
