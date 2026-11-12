# 方块音效接入方案

本文档描述全量方块接入后的音效映射与缺失音效打包方案.
数据基础是生成表 game/generated/block_data_gen.h: 每个方块保留
digSound, placeSound, walkSound 三个字段, 值是
"blockd.<族>" 或 "blocks.<族>" 前缀字符串, 空串表示没有该项音效.

## 音效覆盖实测

1191 行方块中:

| 音效 | 有值行数 | 族分布 |
|:---:|:---:|:---|
| 挖掘音 | 1066 | stone 433, wood 311, grass 164, cloth 59, glass 59, jar 17, gravel 5, snow 5, sand 4, blocks.* 杂项 9 |
| 行走音 | 890 | stone 486, wood 274, cloth 60, grass 42, gravel 5, snow 5, sand 4, ladder 1, blockd.* 杂项 13 |
| 放置音 | 328 | stone 200, wood 120, splash 4, cloth 2, blocks.* 杂项 2 |

放置音缺省是常态: 863 行没有放置音值, 按对应挖掘族播放.
行走音缺省按方块类别回落. 前缀 blockd. 是破坏族, blocks. 是行走族,
两族文件同名不同音, 不能混用.

## 已打包音源盘点

客户端 src/assets/sounds/ 现有 90 个 ogg, 全部来自素材仓库
single-branch/assets/project/sounds/ 的两个子目录:

- blockd/ (34 个, 破坏族): cloth 4, glass 3, grass 4, gravel 4,
  jar 3, sand 4, snow 4, stone 4, wood 4;
- blocks/ (44 个, 行走族): cloth 5, grass 6, gravel 6, ladder 5,
  sand 5, snow 5, stone 6, wood 6;
- 另有 12 个自制合成音: dig_grass.ogg 等 6 个挖掘单音与
  place_<族>.ogg 6 个放置音 (素材仓库没有放置音源, 这是客户端自制),
  Java 侧 PlayActivity.java 的加载清单就是这 12 个.

命名折算: 素材工程的 blockd/stone1.ogg 打包为 dig_stone1.ogg,
blocks/stone1.ogg 打包为 walk_stone1.ogg, 前缀由目录名折算.

## 名称到已打包音源的映射表

音频层按"族 + 动作"两级查找, 第一级查本表, 第二级在族内随机选一个变体.
映射表是静态数据, 建议落为 native/audio/sound_map_gen.h, 由
scripts/gen_sound_map.py 生成, 也可先手写.

### 挖掘音 (DigSound -> 播放条目)

| 名称 | 客户端条目 | 音源文件 |
|:---|:---|:---|
| blockd.stone | kDigStone | dig_stone.ogg 至 dig_stone4.ogg (5 个) |
| blockd.wood | kDigWood | dig_wood.ogg 至 dig_wood4.ogg (5 个) |
| blockd.grass | kDigGrass | dig_grass.ogg 至 dig_grass4.ogg (5 个) |
| blockd.gravel | kDigGravel | dig_gravel.ogg 至 dig_gravel4.ogg (5 个) |
| blockd.sand | kDigSand | dig_sand.ogg 至 dig_sand4.ogg (5 个) |
| blockd.snow | kDigSnow | dig_snow.ogg 至 dig_snow4.ogg (5 个) |
| blockd.cloth | kDigCloth (新增) | dig_cloth1.ogg 至 dig_cloth4.ogg (4 个) |
| blockd.glass | kDigGlass (新增) | dig_glass1.ogg 至 dig_glass3.ogg (3 个) |
| blockd.jar | kDigJar (新增) | dig_jar1.ogg 至 dig_jar3.ogg (3 个) |
| blocks.wood (挖掘字段) | 回落 kDigWood | 同 blockd.wood |
| blocks.stone (挖掘字段) | 回落 kDigStone | 同 blockd.stone |
| blocks.sand (挖掘字段) | 回落 kDigSand | 同 blockd.sand |
| blocks.cloth (挖掘字段) | 回落 kDigCloth | 同 blockd.cloth |

前缀 blocks. 出现在挖掘字段是数据的笔误形态 (共 9 行),
按族名回落到破坏族同名声源, 不单独打包.

### 放置音 (PlaceSound -> 播放条目)

| 名称 | 客户端条目 | 音源文件 |
|:---|:---|:---|
| blockd.stone | kPlaceStone | place_stone.ogg (自制) |
| blockd.wood | kPlaceWood | place_wood.ogg (自制) |
| blockd.cloth | kPlaceCloth (新增) | 复用 walk_cloth1.ogg |
| blocks.wood, blocks.cloth | 同上回落 | 同名族 |
| env.splash | kEnvSplash (新增) | 素材 env/splash.ogg, 需要新打包 |

放置音缺省 (空串) 的 863 行按挖掘族回落: 有挖掘族的播
kPlace<族>, 都没有的不发声. env.splash 只被 4 个流体系方块引用,
静态水与岩浆的放置音已实测为 env.splash, 落地时随缺省回落逻辑
优先级高于族回落.

### 行走音 (WalkSound -> 播放条目)

| 名称 | 客户端条目 | 音源文件 |
|:---|:---|:---|
| blocks.stone | kWalkStone (新增) | walk_stone1.ogg 至 walk_stone6.ogg (6 个) |
| blocks.wood | kWalkWood (新增) | walk_wood1.ogg 至 walk_wood6.ogg (6 个) |
| blocks.grass | kWalkGrass (新增) | walk_grass1.ogg 至 walk_grass6.ogg (6 个) |
| blocks.cloth | kWalkCloth (新增) | walk_cloth1.ogg 至 walk_cloth5.ogg (5 个) |
| blocks.gravel | kWalkGravel (新增) | walk_gravel1.ogg 至 walk_gravel6.ogg (6 个) |
| blocks.snow | kWalkSnow (新增) | walk_snow1.ogg 至 walk_snow5.ogg (5 个) |
| blocks.sand | kWalkSand (新增) | walk_sand1.ogg 至 walk_sand5.ogg (5 个) |
| blocks.ladder | kWalkLadder (新增) | walk_ladder1.ogg 至 walk_ladder5.ogg (5 个) |
| blockd.wood (行走字段) | 回落 kWalkWood | 同 blocks.wood |
| blockd.stone (行走字段) | 回落 kWalkStone | 同 blocks.stone |

行走音源 44 个已在客户端 assets/sounds/ 里 (walk_ 前缀),
只差音频层接线; blockd. 前缀出现在行走字段共 13 行, 按族回落.

## 缺口与批量打包清单生成

音源缺口只有一处: env.splash.ogg 未打包. 其余族音全部在包内.
生成缺失清单的脚本 scripts/gen_sound_map.py 做三件事:

1. 解析生成表的三个音效字段, 汇总出现的唯一 音效名 与引用方块数;
2. 按上文映射表折算成 安装包路径 (sounds/dig_stone1.ogg 等),
 与 assets/sounds/ 现有文件比对, 输出缺失清单;
3. 产出 sound_map_gen.h: 音效名到 SoundId 枚举的 constexpr 映射,
 与族内变体文件名数组, 供 Java 侧加载与原生侧查询共用.

缺失项按素材仓库路径折算成 pack_assets.py 的 SOUND_ENTRIES 补丁,
脚本直接输出可粘贴的清单文本, 本期缺口示例:

    ("sounds/env_splash.ogg", "sounds/env_splash.ogg")

## 原生侧接线 (方案, 不在本期实现)

- voxel::HarvestMaterial 扩展 kCloth, kGlass, kJar, kLadder 四族,
 生成表的 material 字段已按 7 到 10 预留这四族取值,
  block_registry.cpp 的 MaterialOf 折算当前回落 stone, 届时改为直通;
- audio::SoundId 追加 kDigCloth, kDigGlass, kDigJar, kWalk<族> 等
 枚举, PlayActivity.java 的加载清单按 sound_map_gen.h 同步扩充;
- game_shell.cpp 的 DigSoundOf 与 PlaceSoundOf 改为查 sound_map_gen.h
 的映射函数, 族回落逻辑集中在映射表内, 不散落在调用侧;
- 行走音由 player_controller 在落地与移动事件里按脚下方块材质触发,
 播放间隔沿用挖掘音的节拍常量, 变体随机从族内选取.

## 验收口径

- 生成表 1066 个挖掘音, 890 个行走音, 328 个放置音全部有映射落点,
 没有映射的音效名进入缺失清单而不是静默无声;
- sound_map_gen.h 与生成表同源, 重新生成方块表后同步重跑即可对齐;
- 兼容段 15 种的挖掘与放置行为与现状完全一致.
