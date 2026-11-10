# 体素与世界脚手架

> 受理 `.vmo` (VMOF), `.vox` (MagicaVoxel), `.r` (世界区块), `.cm` (CUST),
> `.fcm` 与 `.bp` (FlatBuffers)。工程文件是单个 `world.json`, 按 `format` 分派。

## 一、二进制格式

### 1.1 VMOF 体素模型 (.vmo)

| 偏移 | 类型 | 含义 |
|:---:|:---:|:---|
| 0x00 | u8[4] | 魔数 `VMOF` |
| 0x04 | u32le | 版本 100 或 200 |
| 0x08 | u32le | 尺寸 X |
| 0x0C | u32le | 尺寸 Y |
| 0x10 | u32le | 尺寸 Z |
| 0x14 | u16le[X*Y*Z] | 每体素的调色板索引, 行优先 |
| 0x14+2*X*Y*Z | u32le | 仅版本 200: 尾块长度 LT |
| 之后 | u8[LT] | 仅版本 200: FlatBuffers 尾块, 根表 `FBSave::BluePrint` |

闭合式: 版本 100 为 `20 + X*Y*Z*2`, 版本 200 为 `20 + X*Y*Z*2 + 4 + LT`,
实测 14 个与 12 个样本全部吻合。

### 1.2 MagicaVoxel 体素文件 (.vox)

头 8 字节为魔数 `VOX ` 与 u32le 版本 (实测 150), 其后是块序列。每块头 12 字节:
4 字节块标识, u32le 内容字节数, u32le 子块总字节数, 再往后是内容与子块。

| 块 | 内容 |
|:---:|:---|
| MAIN | 内容为空, 子块容纳整个模型 |
| SIZE | 3 个 u32le: 体素网格 x y z |
| XYZI | u32le 体素数, 其后每体素 4 字节 x y z 与颜色索引 |
| RGBA | 256 项调色板, 每项 4 字节 |

### 1.3 世界区块 (.r)

| 偏移 | 类型 | 含义 |
|:---:|:---:|:---|
| 0x00 | u8[4] | 魔数 `0C 1C 07 06` |
| 0x04 | u8[11] | 坐标区, 与单字节密钥 K 逐字节异或 |
| 0x0F | u32le | 载荷字节长度 LT |
| 0x13 | u8[3] | 未定, 原样保留 |
| 0x16 | u8[13] | 常量区, 实测只有两种取值 |
| 0x23 | ... | 数据区, 尾部为 0x00 填充 |

坐标区解出 11 字节明文: `[B, 0, C, i32le X, i32le Z]`, 其中 B 与 C 是路径里的
目录级号, X 与 Z 是区块坐标; 密钥实测为 `K = A + 32 * B`, A 与 B 是路径第 1 级
与第 2 级目录名的整数值。全量 292 个样本坐标与文件名完全一致。

数据区编码未定 (逆向资料记为 LZMA1 裸流加 FlatBuffers `FBSave::ChunkSave`,
但按单字节密钥与多种压缩参数实测都解不开), 因此按原样 base64 保存。

### 1.4 CUST 自定义模型 (.cm)

外壳 4 字节魔数 `CUST`, 其后为 16 字节周期异或密文。解密式:

```
P[i] = O[i + 4] ^ key[i mod 16]
key = 7cd7cec633f2b218ced6c7aba0f9c121
```

解密后 `P` 首 4 字节为 `a9 13 06 ed` (引擎不校验), 从 `P + 4` 起是标准 FlatBuffers,
根表 `FBSave::CustomModel`。根表字段 10 是方块记录向量, 每条记录是 12 字节表:

| 表内偏移 | 宽度 | 内容 |
|:---:|:---:|:---|
| +0x00 | 4 | i32 soffset, 指向共享 vtable |
| +0x04 | 2 | 填充, 恒为 0 |
| +0x06 | 2 | u16 方块 id |
| +0x08 | 4 | u32 坐标打包: 低字节 x, 次字节 y, 第三字节 z |

文件不含顶点与索引, 几何由方块列表现算。

### 1.5 FlatBuffers 容器 (.fcm / .bp)

首 u32le 为根表偏移, 其后是标准 FlatBuffers 缓冲区。根表首 i32 为回指 vtable 的
有符号偏移, vtable 首两个 u16 为 vtable 长度与对象长度, 其后是各字段槽位偏移。
`.fcm` 根表为 `FBSave::FullyCustomModel`, 实测 8 槽位与 11 槽位两代 schema;
`.bp` 根表为 `FBSave::BluePrint`。槽位语义按符号表反推, 工程文档按字节给出全部候选解释。

## 二、明文工程文件

```
scaffolds/world/sample/
└── world.json
```

公共字段为 `format` 与 `source`; 各格式追加字段:

| 格式 | 字段 |
|:---:|:---|
| vmof | `header` (魔数, 版本, 尺寸), `voxels` (base64 加体素数与非零数), `tail` (版本 200) |
| vox | `header`, `chunks[]`: 每块 `id` 与 `size` 或 `voxels` 或 `palette`, 其余 `content` base64 |
| world-chunk | `header`, `coordinate` (key, b, c, x, z, 密文), `data` base64 |
| world-chunk-raw | `payload` base64 与 `magic_hex` |
| cust | `shell` (密钥), `flatbuffer` (根表, 字段, 字符串, 方块记录), `payload` base64 |
| flatbuffers | `root_table`, `fields[]`, `payload` base64 |

FlatBuffers 桶里同一偏移可能有多种自洽解释, `fields[].alternatives` 列出全部候选,
`type` 按 表 > 字符串 > 向量 的置信度取主解释。

## 三、编辑与回写

| 格式 | 可编辑项 | 不改动时逐字节一致 |
|:---:|:---|:---|
| vmof | `header.size`, `voxels.base64`, `tail` | 是 |
| vox | `chunks` 里的尺寸, 体素数组, 调色板, 块顺序 | 是 |
| world-chunk | `coordinate` 的 key 与 b/c/x/z, `data.base64`, 头部字段 | 是 |
| cust | `payload.base64`; 方块记录是只读视图 | 是 |
| flatbuffers | `payload.base64`; 槽位视图只读 | 是 |

注意: VMOF 改尺寸必须同步改体素数组长度 (X*Y*Z*2 字节); vox 调色板必须是 256 项;
世界区块改坐标后按新坐标重新异或, 不改数据区则其余字节不变。

## 四、样例工程

`sample/world.json` 由 `voxel/baozishuding.vmo` 导出 (200 字节, 版本 100, 6x3x5)。
回写产物与 `source.sha256` 一致。

```bash
cd single-branch/assets
python -m scaffolds world build scaffolds/world/sample -o temp/baozishuding.vmo
python -m scaffolds world verify 体素目录 世界目录 自定义模型目录
```

## 五、未覆盖项

1. 世界区块数据区编码: 未解, 只保留 base64, 不能做方块级编辑。
2. CUST 与 FlatBuffers 的结构化写回: 视图只读, 改内容需整体替换 base64。
3. VMOF 版本 200 尾块: 只原样保留, 不解析容器坐标表。
4. `.vox` 的未知块 (如 nTRN, nGRP) 只按 base64 保留, 不解释语义。
5. `.bp` 蓝图与 `.fcm` 的字段级 schema: 只给槽位候选, 不做类型确证。

## 版权信息

Copyright &copy; 2026 JularDepick。

## 许可证

本目录内容随仓库采用 Apache-2.0 许可证。
