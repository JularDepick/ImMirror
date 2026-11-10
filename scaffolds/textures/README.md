# 纹理脚手架

> 受理 `.ktx` (可套二进制族外壳) 与 `.otex` (命名区块容器里的纹理)。
> 工程文件是 `manifest.json` 加 `mip0.png`, PNG 是 0 级像素的可编辑视图。

## 一、二进制格式

### 1.1 KTX 11

| 偏移 | 类型 | 含义 |
|:---:|:---:|:---|
| 0x00 | u8[12] | 魔数 `AB 4B 54 58 20 31 31 BB 0D 0A 1A 0A` |
| 0x0C | u32 | 端序标记, 小端文件为 `01 02 03 04` |
| 0x10 | u32[12] | glType, glTypeSize, glFormat, glInternalFormat, glBaseInternalFormat, 宽, 高, 深, 数组元素数, 面数, mip 级数, 键值区字节数 |
| 0x40 | u8[键值区字节数] | 键值区原样字节 |
| 之后 | ... | 每级 mip: u32 长度 加该级压缩块 |

像素格式按 `glInternalFormat` 分派, `forge.codecs.etc` 支持 `ETC1_RGB8` (0x8D64)
与 `ETC2_RGBA8` (0x9278); 全量 4368 个 KTX 实测都是 ETC1_RGB8, 4x4 块 8 字节。

### 1.2 otex

`89 67 45 23` 容器, 纹理相关两块:

| 块名 | 载荷 |
|:---:|:---|
| TextureData | 32 字节 = 8 个 u32, 其中 [1] 宽, [2] 高, [5] 像素格式号 |
| SurfaceData | 32 字节头加像素: [0] 格式号, [1] 宽, [2] 高, [4] 每像素字节, [5] 行字节, [6] 与 [7] 像素总字节 |

像素格式号实测两种: 10 为 RGB8 (每像素 3 字节), 12 为 RGBA8 (每像素 4 字节)。
像素按行优先排列, 无填充。

## 二、明文工程文件

```
scaffolds/textures/sample/
├── manifest.json
└── mip0.png
```

| 字段 | 含义 |
|:---:|:---|
| `format` | `ktx11` `ktx11-raw` 或 `otex` |
| `envelope` | 二进制族外壳参数, 无则为 `null` |
| `header` | KTX 的 13 个 u32 字段 (端序标记单独放在 `endianness`) |
| `key_value_data` | 键值区原样字节, base64 |
| `mips[]` | 每级 `level` `width` `height` `size` 与 `payload` 压缩块 base64 |
| `trailing` | 最后一级之后的残余字节, base64 |
| `pixel` | KTX 像素格式名与编解码器名 |
| `container` `blocks[]` | otex 的容器头与块序列, 与模型分类同构 |
| `texture` | otex 的宽高与像素格式 |
| `pixels` | PNG 文件名, 尺寸与 RGBA 摘要 |

`ktx11-raw` 是多面或数组 KTX 的兜底形态: 只保存整份 `payload` base64, 不做像素切分。

## 三、编辑与回写

| 操作 | 是否支持 | 说明 |
|:---:|:---|:---|
| 不改工程直接回写 | 是 | 按压缩块原样拼接, 与源逐字节一致 |
| 改 PNG 像素 | 是 | 按原像素格式重新压块, 低层 mip 由 0 级盒式降采样重算 |
| 改压缩块 | 是 | 改 `mips[].payload` 后按块回写, 此时 PNG 与块不一致会以 PNG 为准重压块 |
| 改文件头字段 | 是 | 宽高必须与 PNG 尺寸一致 |
| 改 otex 像素 | 是 | PNG 像素替换 SurfaceData 的像素区, 头部与块序列不变 |

回写判定: 从当前压缩块解出的 0 级像素与 PNG 相同则直接用压缩块, 否则重新编码;
`build` 返回的 `reencoded` 与 `pixel_diff` 记录这次重编码与像素误差。

## 四、图标集

图标集是 `ui/mobile/textureN` 下的一组 KTX 图集页, 单页格式与 `ktx11` 相同。
整目录导出时多一层 `atlas.json` 索引:

```bash
cd single-branch/assets
python -m scaffolds textures iconset 图集目录 -o 工程目录
python -m scaffolds textures iconset-build 工程目录 -o 输出目录
```

`atlas.json` 记录每页的相对路径, 工程子目录与源摘要; 回写按相对路径还原。

## 五、样例工程

`sample/` 由 `ui/dotted.ktx` 导出 (212 字节, 17x6, 5 级 mip, ETC1_RGB8)。
回写产物与 `source.sha256` 一致。

```bash
python -m scaffolds textures build scaffolds/textures/sample -o temp/dotted.ktx
```

## 六、未覆盖项

1. 多面与数组 KTX: 结构切分未做, 落到 `ktx11-raw` 整份保留。
2. PVRTC, ATC, DXT 等像素格式: 引擎资源里没有出现, 未接编码器。
3. KTX 2 容器与 Basis 压缩: 原版资源不存在, 不处理。
4. 图集命名空间与控件坐标: 属于客户端渲染层语义, 脚手架只保证文件级往返。

## 版权信息

Copyright &copy; 2026 JularDepick。

## 许可证

本目录内容随仓库采用 Apache-2.0 许可证。
