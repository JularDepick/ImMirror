# 端到端往返验证报告

口径: 解码 -> 明文工程文件 (文本以 UTF-8 落盘) -> 回读 -> 编码回原版容器 -> 与原文件逐字节比对。样本按类别分层, 类别内按编解码器轮转抽取。

## 一. 结论

| 项 | 数值 |
|:---:|:---:|
| 抽样条数 | 200 |
| 索引可用条数 | 23541 |
| 覆盖类别 | 8 |
| 覆盖编解码器 | 14 |
| 逐字节一致 | 200 |
| 失败 | 0 |
| 样本取自磁盘源树 | 198 |
| 样本按索引由工程树回写 | 2 |

## 二. 类别汇总

| 类别 | 抽样 | 通过 | 失败 |
|:---:|:---:|:---:|:---:|
| 数据表 csv | 29 | 29 | 0 |
| 命名区块容器 | 28 | 28 | 0 |
| 纹理 ktx | 29 | 29 | 0 |
| 音频 ogg | 29 | 29 | 0 |
| 图标集与界面 | 28 | 28 | 0 |
| 体素与世界存档 | 28 | 28 | 0 |
| Android 二进制 XML | 5 | 5 | 0 |
| 标准格式与其他 | 24 | 24 | 0 |

## 三. 编解码器汇总

| 编解码器 | 保真级别 | 可读性 | 抽样 | 通过 | 失败 |
|:---:|:---:|:---:|:---:|:---:|:---:|
| container-text-xor16-zlib | lossless-edit | text | 21 | 21 | 0 |
| plain-text | lossless-edit | text | 22 | 22 | 0 |
| named-block | lossless-passthrough | binary-structured | 30 | 30 | 0 |
| container-binary-xor16-head | lossless-passthrough | binary-opaque | 40 | 40 | 0 |
| ktx11 | lossless-passthrough | binary-structured | 14 | 14 | 0 |
| ogg | lossless-passthrough | binary-standard | 14 | 14 | 0 |
| gif | lossless-passthrough | binary-standard | 5 | 5 | 0 |
| jpeg | lossless-passthrough | binary-standard | 4 | 4 | 0 |
| opaque | lossless-passthrough | binary-opaque | 28 | 28 | 0 |
| png | lossless-passthrough | binary-standard | 9 | 9 | 0 |
| ttf | lossless-passthrough | binary-standard | 1 | 1 | 0 |
| webp | lossless-passthrough | binary-standard | 2 | 2 | 0 |
| axml | lossless-passthrough | binary-standard | 5 | 5 | 0 |
| zip | lossless-passthrough | binary-standard | 5 | 5 | 0 |

## 四. 文本编码分布

| 编码 | 条数 |
|:---:|:---:|
| utf-8 | 31 |
| gbk | 12 |

## 五. 失败清单

无失败样本。

## 六. 复现

```bash
python -m forge.cli mirror
python -m forge.cli roundtrip --samples 200 --seed 0
```
