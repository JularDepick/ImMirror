# 数据表脚手架

> 受理数据表与数据定义类文本: `.csv` `.csvdef` 目录下的表, `.xml` 数据定义,
> 以及 `.txt` `.toc` `.ini` `.json` `.cfg` `.dat` 同类文本。
> 工程文件是 `data.json` 加一份 UTF-8 文本文件。

## 一、二进制格式

### 1.1 文本族容器 `FF D9 FF D7`

| 偏移 | 类型 | 含义 |
|:---:|:---:|:---|
| 0x00 | u8[4] | 魔数 `FF D9 FF D7` |
| 0x04 | u32be | 解压后字节数 |
| 0x08 | ... | zlib 流与 16 字节周期密钥异或的结果 |

密钥 `d6020800f4feff3f01000000d0ca0100`, 压缩级别实测为 zlib 默认 6, 回写可逐字节还原。
336 个 csv 中 168 个套该容器, 其余是明文。

### 1.2 明文

未经加密的 csv 与 xml 直接就是明文, 编码不固定: 全量实测 UTF-8 与 GBK 混合
(GBK 只出现在加密族里), 换行 CRLF 与 LF 混排, 另有 1 个文件带 UTF-8 BOM。

编码探测顺序为 UTF-8 (含 BOM) -> GBK -> GB18030 -> Big5 -> latin-1 兜底, 由
`forge.util.textio.decode_text` 完成, 原始编码与换行记进工程文档。

## 二、明文工程文件

```
scaffolds/data/sample/
├── data.json
└── ARstylesetting.csv
```

| 字段 | 含义 |
|:---:|:---|
| `format` | 固定 `text-table` |
| `codec` | `plain-text` 或 `container-text-xor16-zlib` |
| `encoding` | 源编码, 回写时按它编码 |
| `bom` | 源是否带 UTF-8 BOM |
| `eol` | 源换行风格 `crlf` `lf` `mixed` `none` |
| `text_file` | 工程里的文本文件名, 与源同名 |
| `text_size` `line_count` `text_sha256` | 明文长度, 行数与摘要 |
| `columns` | csv 首行列名提示, 供人工对照 |

文本文件本身是 UTF-8 无 BOM 的工程文件, 保留源换行符; 表格语义 (逗号与引号)
按 RFC 4180 组织, 脚手架不做单元格级校验。

## 三、编辑与回写

| 操作 | 是否支持 | 说明 |
|:---:|:---|:---|
| 不改文本直接回写 | 是 | 按原编码与换行还原, 与源逐字节一致 |
| 改单元格或行 | 是 | 回写按 `encoding` 转回原编码, 容器与压缩级别照旧 |
| 改编码 | 是 | 改 `encoding` 字段后按新编码回写, 需保证字符可表示 |
| 改 `columns` 提示 | 否 | 只读字段, 不影响回写 |

若编辑引入了原编码无法表示的字符, 编码会抛 `UnicodeEncodeError` 而不是静默替换,
此时应改 `encoding` 或退回原字符集。

## 四、样例工程

`sample/` 由 `csvdef/ARstylesetting.csv` 导出 (180 字节, GBK, 文本族容器)。
工程里的 `ARstylesetting.csv` 是 UTF-8 明文, 回写产物与 `source.sha256` 一致。

```bash
cd single-branch/assets
python -m scaffolds data build scaffolds/data/sample -o temp/ARstylesetting.csv
python -m scaffolds data verify 数据表目录
```

## 五、未覆盖项

1. 表格语义校验: 只保证字节往返, 不检查列数一致或引号闭合。
2. csvdef 里的类型声明: 属于运行期 schema, 脚手架不解释。
3. 二进制 `.dat`: 内容不是文本的会被拒绝, 由兜底编解码器在 forge 管线里原样保留。

## 版权信息

Copyright &copy; 2026 JularDepick。

## 许可证

本目录内容随仓库采用 Apache-2.0 许可证。
