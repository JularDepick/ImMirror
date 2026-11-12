# 方块图集扩充方案

## 现状

| 项 | 值 |
|:---:|:---|
| 当前图集 | 2048x2048, 16x16 格, 每格 128px, 容纳 256 格 |
| 当前用途 | 15 种兼容方块的贴图 + 空余格 |
| 贴图 | assets/project/blocks/ 1538 张 PNG (39 MB) |
| blockdef 贴图引用 | 753 个唯一贴图名 |
| 有独立 PNG 源 | 647 个 |
| 无独立源 | 106 个 (grass/farmland 等多面组合贴图, 需按面拆分或用兼容段已有贴图回落) |

## 目标

| 项 | 值 |
|:---:|:---|
| 新图集 | 4096x4096, 64x64 格, 每格 64px, 容纳 4096 格 |
| 容量利用率 | 753 / 4096 = 18.4%, 余量充足 |
| 显存 | 4096x4096x4 = 64 MiB RGBA, 设备分级 high/mid 可承受, low 回落 2048 图集 |

## 分配规则

1. 格 0 到 15: 兼容段 15 种方块的现有贴图 (存档兼容, 图集索引不变)
2. 格 16 起: 按 blockdef 的贴图名字典序分配, 每个唯一贴图名占一格
3. 无独立源的 106 个: 用兼容段已有贴图回落 (grass 用 grass_top, farmland 用 dirt 等),
 映射表写死在生成器里, 保证渲染不出现空洞
4. 多面贴图 (木头顶/侧/底): 按 blockdef 的贴图 1/2 列分别分配格位

## 构建流程

1. scripts/build_atlas.py 扩展: 读 blockdef 全量贴图名 -> 从 assets/project/blocks 找 PNG -> 贴入图集
2. 产出三件套: atlas.rgba (裸像素) + atlas.json (格位映射) + atlas_index_gen.h (C++ 格位常量)
3. block_registry 运行时按贴图名查格位, 不再手写 atlasIndex
4. 兼容段 15 种的图集索引由生成器自动保持与旧版一致

## 音效映射 (同步做)

blockdef 的挖/放/走音效名 (blockd.stone 等) 与已打包 90 个 ogg 的映射表:
blockd.stone -> sounds/dig_stone1.ogg (随机变体 1-4), 由 AudioEngine 按材质族播放.

## 未接入的贴图

106 个无独立源贴图的明细与回落映射, 见生成器输出的 atlas.json 缺源清单.
