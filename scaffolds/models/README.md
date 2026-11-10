# 模型与实体脚手架

> 受理 `.omod` `.emo` `.ent` `.oanim` 以及 `.omod.r119085` `.omod.r119179` 增量文件,
> 二进制族是命名区块容器 `89 67 45 23`, 可能套一层二进制族外壳 `FF D9 FF D8`。

## 一、二进制格式

### 1.1 容器头 (12 字节)

| 偏移 | 类型 | 含义 |
|:---:|:---:|:---|
| 0x00 | u8[4] | 魔数 `89 67 45 23` |
| 0x04 | u32le | 版本, 实测恒为 100 |
| 0x08 | u32le | 恒为 1, 语义未定, 原样保留 |
| 0x0C | ... | 具名区块序列, 首尾相接铺满到文件末尾 |

### 1.2 区块头

| 偏移 | 类型 | 含义 |
|:---:|:---:|:---|
| +0x00 | u16le | 类型标识, 实测取值 100 101 102 104 105 106 |
| +0x02 | u8 | 块名长度, 实测 8 至 21 |
| +0x03 | u8[名长] | ASCII 块名, 命中 21 个封闭块名 |

区块自身没有长度字段, 块边界由下一个合法块头或文件末尾决定, 细则见
`docs/notes/named-block.md`。21 个块名为 ModelData, SkeletonData, TextureData,
SurfaceData, MeshData, BoneData, EntityMotionData, AnimationData,
MotionBindElementData, EntityData, BoneTrack, SubMeshData, SkinPatch, Material,
DummyNodeData, ParamShapeData, BillboardData, ParticleEmitterData,
RibbonEmitterData, MotionAnimElementData, MaterialParamTrack。

### 1.3 外壳

535 个 `.omod` 在容器外再套 `FF D9 FF D8`: 头部 `[魔数 4][u32be 加密字节数]`,
其后该字节数范围与前缀密钥异或, 其余明文。工程文档的 `envelope` 字段记录外壳参数,
回写时按同一参数重新套壳。

## 二、明文工程文件

工程目录里只有一个 `model.json`:

```
scaffolds/models/sample/
└── model.json
```

| 字段 | 含义 |
|:---:|:---|
| `source` | 源文件名, 扩展名, 字节数, SHA256 |
| `envelope` | 外壳参数 `{container, enc_prefix}`, 无外壳时为 `null` |
| `container` | `magic`, `version`, `header_flag` |
| `blocks[]` | 块序列, 顺序即文件顺序 |
| `blocks[].name` | 块名; 以 `?` 开头表示未解析区, 回写时不写块头 |
| `blocks[].type_id` | 类型标识, 未解析区为 -1 |
| `blocks[].mode` | `closed` 闭式命中, `predicted-mismatch` 落空, `no-closed-form` 无闭式 |
| `blocks[].form` | 闭式给出的形态说明 |
| `blocks[].payload` | 区块载荷原样字节, base64 兜底 |
| `blocks[].fields` | 字段级只读视图, 由 `forge.codecs.named_block.block_fields` 生成 |
| `coverage` | 容器字节数, 块字节数, 是否铺满到末尾, 闭式命中块数 |

`fields` 是只读视图, 回写不看它; 要改块内容就改 `payload.base64`。

## 三、编辑与回写

| 操作 | 是否支持 | 说明 |
|:---:|:---|:---|
| 不改工程直接回写 | 是 | 与源逐字节一致 |
| 替换块载荷 | 是 | 块长度随载荷长度变化, 容器自动重排 |
| 增删或重排块 | 是 | 按 `blocks` 数组顺序重建 |
| 改块名或类型标识 | 是 | 块名必须是 ASCII, 长度 1 至 255 |
| 改字段视图 | 否 | `fields` 只读; 字段级写回需要块写出器, 尚未实现 |

## 四、样例工程

`sample/model.json` 由 `particles/1002.ent` 导出 (59 字节, 1 个 EntityData 块)。
回写产物与 `source.sha256` 一致。

```bash
cd single-branch/assets
python -m scaffolds models build scaffolds/models/sample -o temp/1002.ent
python -m scaffolds models export 源文件.emo -o 工程目录
```

## 五、未覆盖项

1. `fields` 驱动的字段级写回: 需要为 21 个块各写一个序列化器, 目前只做只读视图。
2. 五个节点包装族的 29 块残差: 闭式预测小于实测载荷, 只能按下一个块头回退切分。
3. `MaterialParamTrack` 子对象类型: 需运行时定案, 目前按候选式覆盖。
4. 容器头偏移 0x08 字段语义未定, 按原值保留。

## 版权信息

Copyright &copy; 2026 JularDepick。

## 许可证

本目录内容随仓库采用 Apache-2.0 许可证。
