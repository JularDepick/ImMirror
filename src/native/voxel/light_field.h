#pragma once

#include <cstddef>
#include <vector>

#include "voxel/block_registry.h"
#include "voxel/chunk_column.h"

namespace immirror::voxel {

/*
 * 列光场计算器.
 * 天光自世界顶层向下直射, 空气不衰减, 穿过半透方块按传播代价扣减,
 * 触到实心方块后直射归零; 横向扩散每格再减一, 洞穴与屋檐下的亮度
 * 由已点亮格向外补入.
 * 方块光自光源格的 lightSource 起始, 六方向传播每格减一,
 * 进入目标格时按目标格的传播代价扣减, 实心方块阻断.
 *
 * 跨列: 扩散本身仍只在本列内进行, 但会把相邻列边界的亮度按同一套
 * 扣减规则播种进本列边界面, 再参与本列扩散. 这样跨区块边界的火把与
 * 洞口两侧亮度连续; 写入只发生在本列, 相邻列由世界侧按改动点触发重算.
 * 结果整块写入 ChunkColumn 的两条光向量, 网格构建经采样接口读取.
 */
class LightField {
public:
    /* 计算器无状态, 仅复用扩散队列缓冲. */
    LightField() = default;

    /*
     * 全量重算一列光场并覆盖旧值; 方块数据变更后由世界侧调用.
     * neighbors 依次为东 (+X), 西 (-X), 南 (+Z), 北 (-Z) 四列, 缺失方向传空指针.
     */
    void Recompute(ChunkColumn& column, const ChunkColumn* const neighbors[4],
                   const BlockRegistry& registry);

private:
    /*
     * 单格传播代价折算: 不透明方块封死; coverMode 0 与 1 按数据列衰减,
     * 2 半遮挡无衰减数据时至少衰减一档, 3 同材质叠层按数据列,
     * 4 自发光特殊件不衰减; 数据列取 15 的非实心方块同样封死.
     */
    static uint8_t StepCostOf(const BlockInfo& info);

    /* 复用的扩散队列, 避免每次重算重新分配内存. */
    std::vector<size_t> skyQueue_;
    std::vector<size_t> blockQueue_;
};

}  /* namespace immirror::voxel */
