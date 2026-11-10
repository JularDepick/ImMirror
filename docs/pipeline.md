# 管线流程与扩展步骤

## 一. 数据流

```
客户端资源树 (原版运行时二进制)
        │
        │ forge unpack   (解码, 索引写 reports/project-index.jsonl)
        ▼
project/ (可读工程文件, 不进版本库)
        │
        │ 人工编辑 / 工具批量改写
        ▼
project/ (仍是权威数据)
        │
        ├── forge pack --target plain     (客户端产物: 解密后的载荷)
        └── forge pack --target original  (校验产物: 逐字节还原原版容器)
```

明文工程文件的逐类规约见 `docs/plain-formats.md`; 容器与字段级结论见 `docs/formats.md`。

## 二. 两种编码目标

| 目标 | 输出 | 用途 | 确定性要求 |
|:---:|:---|:---|:---|
| `plain` (默认) | 解密后的载荷 | 客户端直接消费 | 同源必须同字节 |
| `original` | 重新套回原版容器 | 校验管线未丢失信息 | 与源逐字节一致 |

`plain` 目标的文本策略 (默认 verbatim):

| 策略 | 文本处理 | 适用 |
|:---:|:---|:---|
| `verbatim` | 保持源编码与换行 | 客户端消费 |
| `utf8-lf` | 转 UTF-8 无 BOM, 换行统一 LF | 阅读与 diff, 不进客户端 |

`original` 目标的判定在 `forge pack` 内完成, 逐文件比对索引里的 `source_sha256`。

## 三. 目录约定

```
assets/
├── README.md              # 管线定位与能力总览
├── .gitignore             # 忽略工程树, 编码产物, 源树镜像与临时目录
├── docs/
│   ├── pipeline.md        # 本文件: 流程, 命令, 扩展步骤, 验收口径
│   ├── plain-formats.md   # 明文工程文件规范 (逐类素材)
│   ├── formats.md         # 各格式的容器结论与落地状态
│   ├── scaffolds.md       # 分类脚手架: 分类矩阵, 回写能力与验证结果
│   └── notes/             # 攻坚过程的取证笔记 (块结构, 像素, 存档)
├── forge/                 # 管线工具 (Python 3, 仅标准库)
│   ├── cli.py             # 命令行入口, 七个子命令
│   ├── registry.py        # 编解码器注册与选择
│   ├── project.py         # 工程树与二进制的双向转换
│   ├── source.py          # 源树定位与镜像重建
│   ├── verify.py          # 工程树与索引的一致性校验
│   ├── roundtrip.py       # 分层抽样与端到端往返验证
│   ├── ktx_png.py         # KTX 与 PNG 互转, 回写自检
│   ├── census.py          # 素材清点
│   ├── codecs/            # 各格式编解码器
│   └── util/              # 编码探测与文本工具
├── scaffolds/             # 分类脚手架 (每种素材一个子目录与样例工程)
├── reports/
│   ├── census.csv         # 逐文件清点结果 (23541 行)
│   ├── roundtrip-200.md   # 抽样往返验证报告
│   ├── roundtrip-200.csv  # 抽样逐条明细
│   └── verify.json        # 全量校验的结构化结论
└── tests/                 # 契约测试与管线测试
```

不进版本库的目录 (按需重建):

| 目录 | 内容 | 重建命令 |
|:---:|:---|:---|
| `project/` | 明文工程文件树与视图 (`_view` 结构, `_png` 像素) | `forge unpack` |
| `build/` `build-original/` | 明文产物与校验产物 | `forge pack` |
| `build-source/` | 源树镜像 (硬链接加回写) | `forge mirror` |
| `temp/` | 抽样验证工作区与临时脚本 | - |

## 四. 命令

```bash
cd single-branch/assets

python -m forge.cli formats      # 列出编解码器, 保真级别, 可读性, 扩展名
python -m forge.cli census       # 全量清点 -> reports/census.csv
python -m forge.cli mirror       # 按索引把候选源树重建到 build-source/
python -m forge.cli unpack       # 解码全树 -> project/ 与 reports/project-index.jsonl
python -m forge.cli pack         # 工程树 -> build/ (默认目标 plain)
python -m forge.cli verify       # 工程树与索引一致性 (+ --deep 全量重编码比对)
python -m forge.cli roundtrip    # 分层抽样 200 个做逐字节往返
```

| 命令 | 关键参数 | 产物 | 退出码 |
|:---:|:---|:---|:---:|
| `census` | `--root` `--out` | 逐文件 CSV | 恒 0 |
| `mirror` | `--out` `--source-root` (可重复) `--copy` `--no-rebuild` | 源树镜像 | 有缺失或回写不符时为 1 |
| `unpack` | `--root` `--project` `--index` `--include` `--limit` | 工程树加索引 | 有解码失败为 1 |
| `pack` | `--target` `--text-policy` `--include` `--out` | 产物树 | original 目标下有不一致为 1 |
| `verify` | `--deep` `--source` `--limit` `--json` | 终端结论, 可写 JSON | 任一问题为 1 |
| `roundtrip` | `--samples` `--seed` `--project` `--out` `--csv` | 报告与明细 | 有失败为 1 |

纹理像素互转走独立入口 (与主命令共用同一份工程树与索引):

```bash
python -m forge.ktx_png stats     # 源树里 KTX 的格式与尺寸分布
python -m forge.ktx_png export    # 批量导出 PNG 旁挂文件
python -m forge.ktx_png import    # 以 PNG 为准回写工程树里的 KTX
python -m forge.ktx_png check     # 回写自检, 报告像素误差
```

常用组合:

```bash
python -m forge.cli unpack --include csvdef particles    # 只解指定前缀
python -m forge.cli pack --target original --include csvdef
python -m forge.cli pack --text-policy utf8-lf --out build-plain-utf8
python -m forge.cli verify --deep --source --json reports/verify.json
python -m forge.cli roundtrip --samples 200 --seed 0
```

## 五. 解码步骤

1. 按魔数在注册表里选择编解码器, 命中顺序为加密文本族, 加密二进制族, KTX, 命名区块, 公开格式,
   最后按扩展名落到明文文本, 仍未命中则由兜底编解码器原样保留。
2. 编解码器 `decode` 出载荷: `text` 载荷给出文本与编码, `binary` 载荷给出原始字节。
3. 文本载荷以 UTF-8 写入 `project/<rel>`, 原始编码, BOM, 换行写入索引。
4. 二进制载荷原样写入 `project/<rel>`。
5. 若编解码器提供视图, 写入 `project/_view/<rel>.json`。
6. 派生像素视图由 `forge/ktx_png.py` 旁挂到 `project/_png/<rel>.png`, 路径记入 `side_files`。
7. 追加一条索引记录。

## 六. 索引字段

索引是回写参数的唯一权威, 每行一个素材:

| 字段 | 含义 |
|:---:|:---|
| `path` | 客户端资源树里的相对路径 |
| `codec` | 编解码器名, 编码时据此选回同一个实现 |
| `fidelity` | 保真级别 |
| `source_size` | 原始二进制字节数 |
| `source_sha256` | 原始二进制摘要, 编码后逐字节比对 |
| `payload_kind` | `text` 或 `binary` |
| `meta` | 编码, BOM, 换行, 容器参数, 明文长度等编码所需参数 |
| `view` | 视图文件相对路径, 可缺省 |

## 七. 校验与验收口径

| 级别 | 命令 | 判据 |
|:---:|:---|:---|
| 契约 | `python -m unittest discover -s tests` | 全部用例通过 |
| 抽样往返 | `forge roundtrip --samples 200` | 200/200 逐字节一致 |
| 全量重编码 | `forge verify --deep` | 摘要不符 0 条, 编码异常 0 条 |
| 结构一致 | `forge verify` | 缺失 0, 多余 0, 视图缺失 0 |
| 清点回归 | `forge census` | 可读性分布不回退 (见 `reports/census.csv`) |

新增或修改编解码器后的门禁顺序:

1. `unpack` 全树, 确认解码无失败;
2. `roundtrip --samples 200`, 确认抽样覆盖了改动的类别;
3. `verify --deep`, 确认全量摘要一致;
4. `census`, 确认可读性分布符合预期, 并更新 `docs/formats.md` 与 `docs/plain-formats.md` 的状态。

## 八. 源树可用性与镜像

源树可能在三个位置, `forge` 按顺序定位: 客户端资源树, 逆向工程树的 `src/assets`,
逆向工程树的 `src`。三处都找不到时, `mirror` 与 `roundtrip` 会按索引从 `project/`
把原版容器回写出来 (再与索引摘要核对), 因此校验不依赖源树副本是否完整。

`mirror` 用硬链接重建, 同卷不占额外空间; `--copy` 强制复制。
镜像目录是缓存, 删掉不影响正确性, 重新执行即可。

## 九. 扩展一个新格式

1. 在 `forge/codecs/` 下新增模块, 继承 `Codec`。
2. 声明 `name` `fidelity` `readability` `extensions` `magics`。
3. 实现 `decode`: 文本类给出 `text` 与 `meta`, 二进制类给出 `blob` 与 `meta`;
   `meta` 必须包含重新编码所需的全部参数。
4. 需要可读视图时实现 `view`, 返回可 JSON 化的字典; 视图解析失败必须降级为错误字段, 不得抛异常。
5. 登记: 公开格式与结构容器在 `forge/codecs/standard.py` 的 `STANDARD_CODECS` 里追加,
   其余在 `forge/registry.py` 的 `MAGIC_CODECS` 或 `EXT_CODECS` 里登记; 魔数路线优先于扩展名路线。
6. 在 `tests/` 补往返测试: 合成样本一份, 真实样本一份 (缺失时跳过)。
7. 在 `docs/formats.md` 记录容器结论, 在 `docs/plain-formats.md` 补充该类素材的明文规范与回写能力。
8. 跑一遍第七节的四步门禁。

约定: 编解码器不得隐式读写索引或工程树, 文件系统操作只出现在 `project.py` 与 `source.py`。

## 十. 故障排查

| 现象 | 原因 | 处理 |
|:---:|:---|:---|
| `unpack` 报源文件缺失 | 源树不在候选根里 | 加 `--source-root`, 或用 `mirror` 先重建 |
| `pack --target original` 有不一致 | 工程文件被改过, 或索引与工程树不同源 | 重跑 `unpack` 让索引与工程树对齐 |
| `verify` 报多余文件 | 工程树里有索引之外的残留 | 清空 `project/` 重新 `unpack` |
| 回写抛编码异常 | 编辑引入了原编码无法表示的字符 | 改索引 `encoding`, 或退回原字符集 |
| 视图解析报错字段 | 该样本触发了编解码器的降级路径 | 记入待攻坚项, 不影响逐字节回写 |

## 十一. 与客户端的关系

1. 客户端只消费 `build/` 里的二进制, 不直接读 `project/`。
2. 资源改名与目录映射以 `client/src/shared/naming/resource-map.tsv` 为准,
   管线保持原版相对路径, 改名在复制阶段按映射表执行。
3. 客户端自行实现资源加载时, 可以选择消费 `unpack` 出的明文, 跳过解密层。

## 十二. 分类脚手架

`scaffolds/` 与本文的管线并列, 针对单份素材提供可编辑的工程目录: `python -m scaffolds`
把一份二进制导出成 JSON 加 PNG 或 XML 的工程, 再按工程内容重组二进制。它只调用
`forge.codecs` 与 `forge.util`, 不修改 `forge/` 下任何文件, 分工与回写能力见
`docs/scaffolds.md`。管线负责全树批量与索引权威, 脚手架负责按类修改与重建。
