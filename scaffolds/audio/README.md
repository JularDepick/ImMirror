# 音频脚手架

> 受理 `.ogg` (可套二进制族外壳)。工程文件是单个 `audio.json`,
> 音频正文以 base64 原样保存, 头部与页表出可读摘要。

## 一、二进制格式

Ogg 页头 27 字节:

| 偏移 | 类型 | 含义 |
|:---:|:---:|:---|
| +0x00 | u8[4] | 捕获模式 `OggS` |
| +0x04 | u8 | 流结构版本, 固定 0 |
| +0x05 | u8 | 页标志 (continued, BOS, EOS) |
| +0x06 | i64le | 页粒度位置 |
| +0x0E | u32le | 流序列号 |
| +0x12 | u32le | 页序号 |
| +0x16 | u32le | 校验和 |
| +0x1A | u8 | 段数, 其后为该数量的段长度表 |

页正文长度等于段长度表之和; 第一个 BOS 页的正文里是编码识别头, Vorbis 为
`01 vorbis` 加版本与声道数、采样率, Opus 为 `OpusHead`。

二进制族外壳 `FF D9 FF D8` 包裹 1290 个加密 ogg, 工程文档的 `envelope` 记录外壳参数。

## 二、明文工程文件

```
scaffolds/audio/sample/
└── audio.json
```

| 字段 | 含义 |
|:---:|:---|
| `format` | 固定 `ogg` |
| `envelope` | 外壳参数, 无外壳时为 `null` |
| `stream` | 编码名, 声道数, 采样率, 只读 |
| `pages` | 页数, 流序列号, 首页与末页粒度位置, 是否铺满到末尾 |
| `payload` | Ogg 正文原样字节, base64 |

## 三、编辑与回写

| 操作 | 是否支持 | 说明 |
|:---:|:---|:---|
| 不改工程直接回写 | 是 | 与源逐字节一致 |
| 换音频 | 是 | 用新 Ogg 的 base64 替换 `payload.base64`, 同步改 `bytes` 与 `sha256` |
| 改头部或页表摘要 | 否 | 只读; 回写以 `payload` 为准 |
| 重编码音频 | 否 | 编码器不在管线职责内, 换音频就是整文件替换 |

## 四、样例工程

`sample/audio.json` 由 `sounds/buff/buff_pop.ogg` 导出 (3888 字节, Vorbis)。
回写产物与 `source.sha256` 一致。

```bash
cd single-branch/assets
python -m scaffolds audio build scaffolds/audio/sample -o temp/buff_pop.ogg
python -m scaffolds audio verify 音频目录
```

## 五、未覆盖项

1. 音频重编码与转码: 不嵌入 Vorbis 或 Opus 编码器。
2. 流级编辑: 不能改采样率或裁剪时长, 只能整文件替换。
3. 多流 Ogg: 页表摘要列出全部序列号, 但不拆流。

## 版权信息

Copyright &copy; 2026 JularDepick。

## 许可证

本目录内容随仓库采用 Apache-2.0 许可证。
