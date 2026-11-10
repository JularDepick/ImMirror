# 界面脚手架

> 受理二进制 Android XML `.axml` 形态的 `.xml` (魔数 `03 00 08 00`) 与文本布局 XML。
> 工程文件是 `layout.json` 加 `layout.xml`。

## 一、二进制格式 (axml)

| 偏移 | 类型 | 含义 |
|:---:|:---:|:---|
| 0x00 | u16le | 文件类型, 固定 0x0003 (RES_XML) |
| 0x02 | u16le | 文件头长度, 固定 8 |
| 0x04 | u32le | 文件总字节数 |
| 0x08 | ... | 块序列, 每块首尾相接铺满到末尾 |

每块头 8 字节: u16 类型, u16 块头长度, u32 块总长度。常见块类型:

| 类型 | 含义 |
|:---:|:---|
| 0x0001 | RES_STRING_POOL 字符串池 |
| 0x0180 | RES_XML_RESOURCE_MAP 资源映射 |
| 0x0100 0x0101 | 命名空间开始与结束 |
| 0x0102 0x0103 | 元素开始与结束 |
| 0x0104 | 文本节点 |

字符串池分 UTF-8 与 UTF-16 两种编码, 由池头标志位决定; 可读 XML 由
`forge.codecs.axml` 解析后写入 `layout.xml`。

## 二、明文工程文件

```
scaffolds/ui/sample/
├── layout.json
└── layout.xml
```

| 字段 | 含义 |
|:---:|:---|
| `format` | `axml` 或 `ui-xml-text` |
| `header` | axml 文件头三个字段 |
| `chunks[]` | 块序列: 类型, 可读类型名, 块头长度, 块总长度, 块原样字节 base64 |
| `string_pool` | 字符串池编码与条数, 只读 |
| `stats` | 元素数, 属性数, 命名空间数, XML 字符数 |
| `xml` | 可读布局文件名与摘要 |
| `codec` `encoding` `bom` `eol` | 文本布局 XML 的编解码参数 |

`layout.xml` 是可读布局: axml 时由字符串池与元素块渲染, 文本 xml 时就是原文。

## 三、编辑与回写

| 操作 | 是否支持 | 说明 |
|:---:|:---|:---|
| axml 不改 `layout.xml` 直接回写 | 是 | 按块序列拼接, 与源逐字节一致 |
| axml 改 `layout.xml` | 否 | 会明确报错: 需要按 XML 重建字符串池与块长度 |
| axml 改块序列 | 是 | 换 `chunks[].payload` 或调整块顺序, 文件头长度自动重算 |
| 文本布局 xml 改内容 | 是 | 按原编码与换行回写, 可反复导出回写 |

## 四、样例工程

`sample/` 由 `res/anim/accelerate_cubic.xml` 导出 (284 字节, 6 个块, 1 个元素)。
回写产物与 `source.sha256` 一致。

```bash
cd single-branch/assets
python -m scaffolds ui build scaffolds/ui/sample -o temp/accelerate_cubic.xml
python -m scaffolds ui verify 资源目录
```

## 五、未覆盖项

1. XML 驱动的 axml 回写: 要重建字符串池, 资源映射与所有块长度, 属于后续工作。
2. 控件定义与图集引用语义: 布局 XML 只保留文件级内容, 不解析控件与纹理的绑定关系。
3. 明文布局的编码互转: 编辑引入原编码无法表示的字符时会抛错, 需要改 `encoding` 或退回原字符集。

## 版权信息

Copyright &copy; 2026 JularDepick。

## 许可证

本目录内容随仓库采用 Apache-2.0 许可证。
