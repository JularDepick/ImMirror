# ImMirror 素材工程与转码管线

> 本目录只放**可读的素材工程文件**与**转码工具**, 不放客户端运行时二进制。
> 客户端消费的二进制由本目录的管线编码产出后复制进 `client/src/assets`。

## 一、定位

原版客户端把大部分素材装进了私有容器: 文本族 `FF D9 FF D7` (异或加 zlib) 包住
`.lua` `.csv` `.xml`, 二进制族 `FF D9 FF D8` 包住 `.ktx` `.ogg` `.omod`,
另有 `89 67 45 23` 命名区块容器, `VMOF` 体素模型, `CUST` 自定义模型, `0C 1C 07 06` 世界区块。
管线负责三层转换:

| 层 | 内容 | 位置 |
|:---:|:---|:---|
| 解码 | 二进制 -> 可读工程文件 | `project/` |
| 编辑 | 直接改工程文件 | `project/` |
| 编码 | 工程文件 -> 二进制 | `build/` |

## 二、契约

1. **两种编码目标**
   - `pack --target plain` (默认): 解密后的载荷, 客户端直接消费; 文本默认保持原编码与原换行
     (`--text-policy utf8-lf` 可转成 UTF-8 无 BOM 加 LF, 供阅读与 diff), 同一份源必须产出同一份字节
   - `pack --target original`: 重新套回原版容器, 与源逐字节一致, 仅作校验路径
2. **逐字节往返** `encode(decode(x)) == x`: 解码不丢信息, 编码是解码的严格逆运算
3. **索引唯一权威** `reports/project-index.jsonl`: 编码、BOM、换行、容器参数、源 SHA256
   全部记录在此, 不允许存在只存在于内存里的隐式状态; 索引属派生产物, 不进版本库
4. **保真级别**: `lossless-edit` 表示工程文件是权威数据且可人工编辑;
   `lossless-passthrough` 表示工程文件保存原始载荷, 只额外附可读视图, 不改写结构
5. **不丢素材**: 未解格式一律由兜底编解码器原样保留并记录摘要, 管线不会跳过文件
6. **版本库边界**: `project/` 与 `build*/` 按需重建, 不进 git; 入库的是工具、文档、
   测试与报告

## 三、可读性分级

```
text                 明文可读, 可直接编辑
binary-structured    有结构视图或块名视图, 字段级解析待推进
binary-standard      PNG / Ogg / ZIP / TTF / GIF / JPEG 等公开格式, 外部工具可读
binary-opaque        容器与载荷均未解, 需要继续逆向
```

原版素材树全量清点结果 (23541 个文件, 879368996 字节):

| 分级 | 文件数 | 体积 |
|:---:|:---:|:---:|
| text | 4326 | 66.3 MiB |
| binary-structured | 11369 | 609.6 MiB |
| binary-standard | 7491 | 157.9 MiB |
| binary-opaque | 355 | 4.8 MiB |

明细见 `reports/census.csv`, 每行一个素材, 记录容器族、编解码器、可读性与编码。
逐类素材的明文格式与回写能力见 `docs/plain-formats.md`。

## 四、目录结构

```
assets/
├── README.md              # 本文件
├── .gitignore             # 忽略编码产物与派生视图
├── docs/
│   ├── pipeline.md        # 管线流程, 命令, 扩展步骤, 验收口径
│   ├── plain-formats.md   # 明文工程文件规范 (逐类素材)
│   ├── formats.md         # 各格式的容器结论与推进状态
│   └── notes/             # 攻坚过程的取证笔记
├── forge/                 # 管线工具 (Python 3, 仅标准库)
│   ├── cli.py             # 命令行入口
│   ├── registry.py        # 编解码器注册与选择
│   ├── project.py         # 工程树与二进制的双向转换
│   ├── source.py          # 源树定位与镜像重建
│   ├── verify.py          # 工程树与索引的一致性校验
│   ├── roundtrip.py       # 分层抽样与端到端往返验证
│   ├── census.py          # 素材清点
│   ├── codecs/            # 各格式编解码器
│   └── util/              # 编码探测与文本工具
├── reports/               # 清点, 往返验证与全量校验报告
└── tests/                 # 编解码器契约测试与管线测试
```

`project/` `build*/` 与 `temp/` 属于按需重建的工作目录, 不进版本库。

## 五、常用命令

```bash
cd single-branch/assets

python -m forge.cli formats                  # 列出已接入的编解码器与保真级别
python -m forge.cli census                   # 全量清点, 写 reports/census.csv
python -m forge.cli mirror                   # 重建源树镜像 (硬链接加按索引回写)
python -m forge.cli unpack                   # 全量解码到 project/, 索引写 reports/
python -m forge.cli unpack --include csvdef particles   # 只解指定前缀
python -m forge.cli pack                     # 产物到 build/ (默认目标 plain)
python -m forge.cli pack --target original    # 还原原版容器, 用于逐字节校验
python -m forge.cli verify --deep --source    # 全量重编码比对加源树复查
python -m forge.cli roundtrip --samples 200   # 分层抽样 200 个做逐字节往返
python -m unittest discover -s tests         # 契约测试与管线测试
```

编码产物进入客户端:

```bash
python -m forge.cli pack --out build
# 按 client/src/shared/naming/resource-map.tsv 改名后复制进 client/src/assets
```

## 六、推进状态

| 容器或格式 | 魔数 | 状态 | 编解码器 |
|:---:|:---:|:---:|:---|
| 文本族 | `ffd9ffd7` | 已解, 逐字节往返通过 | `container-text-xor16-zlib` |
| 二进制族 | `ffd9ffd8` | 已解, 逐字节往返通过 | `container-binary-xor16-head` |
| KTX 11 纹理 | `ab4b5458` | 头字段已出视图, 像素待解 (全量为 ETC1_RGB8) | `ktx11` |
| 命名区块容器 | `89674523` | 块序列与字段已出视图, 块写出器待做 | `named-block` |
| VMOF 体素模型 | `564d4f46` | 待接入 | - |
| CUST 自定义模型 | `43555354` | 载荷为异或加 FlatBuffers, 待接入 | - |
| 世界区块 | `0c1c0706` | 载荷为 LZMA1 加 FlatBuffers, 待接入 | - |
| 二进制 Android XML | `03000800` | 589/589 解析为可读 XML 视图 | `axml` |
| 公开格式 | PNG/WebP/Ogg/ZIP/TTF/GIF/JPEG/MP3 | 已识别并出视图 | `png` `webp` `ogg` `zip` `ttf` `gif` `jpeg` `mp3` |

详细结论与证据链见 `docs/formats.md`, 原始逆向文档位于
`client/temp/original/fake-mini-BUT-true-moments/docs/formats/`。

## 版权信息

Copyright &copy; 2026 JularDepick。

## 许可证

本目录内容随仓库采用 Apache-2.0 许可证。
