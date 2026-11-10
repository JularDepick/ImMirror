# 素材脚手架

> 本目录按素材分类把二进制资源与明文工程目录双向转换。每个分类一个子目录,
> 内含该分类的格式说明、一个 Python 模块与一份最小样例工程。

## 一、定位

`forge/` 是全树管线: 按索引批量解码整棵资源树, 产物是只读视图加原样载荷。
`scaffolds/` 是分类脚手架: 面向单份素材, 产出**可编辑的工程目录**, 回写时按工程内容
重新组装二进制。两者分工如下。

| 项 | forge 管线 | scaffolds 脚手架 |
|:---:|:---|:---|
| 单位 | 整棵资源树, 按索引批量 | 单份素材, 一次一个 |
| 工程文件 | 载荷原样加旁挂 JSON 视图 | 分类定制的 JSON 加 PNG 或 XML |
| 回写能力 | 载荷原样回写 | 按工程结构重组, 可改可增删 |
| 入口 | `python -m forge.cli` | `python -m scaffolds` |

脚手架只调用 `forge.codecs` 与 `forge.util` 的公开函数, 不修改 `forge/` 下任何文件。

## 二、目录结构

```
scaffolds/
├── README.md          # 本文件
├── __init__.py        # 分类注册与索引
├── __main__.py        # 统一命令行入口
├── util.py            # 共用工具: JSON, base64, 外壳, FlatBuffers 只读解析
├── models/            # 模型与实体: omod emo ent oanim
├── textures/          # 纹理: ktx otex 图标集页
├── ui/                # 界面: axml 与文本布局 xml
├── data/              # 数据表: csv csvdef xml 数据定义
├── audio/             # 音频: ogg
└── world/             # 体素与世界: vox VMOF .r CUST fcm bp
```

每个分类目录固定三件套: `README.md` 说明格式与字段, `__init__.py` 提供实现,
`sample/` 放 1 到 2 个文件的样例工程 (只含明文, 不含源二进制)。

## 三、统一接口

每个分类模块对外提供同样的函数, 命令行与测试都按此调用:

```python
export(源二进制路径, 工程目录)   # 二进制 -> 明文工程目录, 返回摘要字典
build(工程目录, 输出二进制路径)   # 明文工程目录 -> 二进制, 返回摘要字典
detect(源二进制路径, 字节)        # 判断是否受理, 返回格式名
```

`build` 的返回值里 `byte_identical` 表示产物与工程文档记录的源摘要是否一致,
`reencoded` 表示像素或结构被改动后重新编码过, `pixel_diff` 给出重编码的像素误差。

## 四、工程文档公共字段

每个工程目录里有一个主 JSON, 顶层字段固定:

| 字段 | 含义 |
|:---:|:---|
| `project` | 固定为 `immirror-scaffold` |
| `version` | 工程文档版本, 当前为 1 |
| `category` | 分类名, 回写时校验, 防止跨分类误用 |
| `format` | 该分类下的具体格式名 |
| `source` | 源文件名, 扩展名, 字节数, SHA256 |

二进制兜底一律用 base64, 形态为 `{"encoding": "base64", "bytes": N, "sha256": "...", "base64": "..."}`;
这类字段表示"原样字节", 可以整体替换, 但不能只改其中的元数据。

## 五、命令

```bash
cd single-branch/assets

python -m scaffolds list                          # 列出分类与受理扩展名
python -m scaffolds world export 源文件.vmo -o 工程目录
python -m scaffolds world build 工程目录 -o 产物.vmo
python -m scaffolds textures verify 源目录          # 批量导出并逐字节比对
python -m scaffolds textures iconset 图集目录 -o 工程目录
python -m scaffolds textures iconset-build 工程目录 -o 输出目录
```

分类名取 `models` `textures` `ui` `data` `audio` `world`。`export` 未给 `-o` 时落在
工作目录下的默认工程目录, `build` 未给 `-o` 时输出到工程目录的 `build/` 子目录。

## 六、分类矩阵

| 分类 | 二进制扩展名 | 明文工程文件 | 逐字节可逆 |
|:---:|:---:|:---:|:---:|
| models | omod emo ent oanim r119085 r119179 | model.json | 是 (未改载荷) |
| textures | ktx otex | manifest.json 加 mip0.png | 是 (未改 PNG) |
| ui | xml | layout.json 加 layout.xml | axml 未改 xml 时是, 文本 xml 恒是 |
| data | csv xml txt toc ini json cfg dat | data.json 加文本文件 | 是 (未改文本) |
| audio | ogg | audio.json | 是 |
| world | vmo vox r cm fcm bp | world.json | 是 (未改数组或载荷) |

完整的分类矩阵, 回写能力与验证结果见 `docs/scaffolds.md`。

## 七、样例工程

各分类的 `sample/` 是一份真实素材导出的工程目录, 只保留明文文件, 源摘要写在
主 JSON 的 `source.sha256` 里, 因此不依赖源二进制即可自测:

```bash
python -m scaffolds models build scaffolds/models/sample -o temp/models-sample.ent
```

产物与 `source.sha256` 一致即说明该分类的往返闭合。

## 版权信息

Copyright &copy; 2026 JularDepick。

## 许可证

本目录内容随仓库采用 Apache-2.0 许可证。
