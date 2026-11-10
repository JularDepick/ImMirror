# 素材脚手架: 分类矩阵与验证

> 本文件定义 `scaffolds/` 的分类矩阵, 各分类的回写能力, 端到端验证结果,
> 以及新增一个分类的执行步骤。格式细节与字段说明在各分类的 README 里。

## 一、定位

素材管线有两条入口, 职责不同:

| 入口 | 单位 | 产物 | 回写能力 |
|:---:|:---|:---|
| `python -m forge.cli` | 整棵资源树 | 载荷原样加旁挂视图 | 载荷原样回写 |
| `python -m scaffolds` | 单份素材 | 分类定制的明文工程目录 | 按工程结构重组 |

脚手架不改动 `forge/` 下任何文件, 只调用 `forge.codecs` 与 `forge.util` 的公开函数;
新增代码全部落在 `scaffolds/` 下。

## 二、分类矩阵

| 分类 | 二进制扩展名 | 明文工程文件 | 是否已可逆 | 依赖的 codec |
|:---:|:---:|:---:|:---:|:---|
| models | omod emo ent oanim r119085 r119179 | model.json | 是, 未改载荷时逐字节一致 | `named-block` 加 `container-binary-xor16-head` |
| textures | ktx otex | manifest.json 加 mip0.png | 是, 未改 PNG 时逐字节一致; 改 PNG 后按像素重编码 | `ktx11` 加 `etc`; otex 用 `named-block` |
| ui | xml | layout.json 加 layout.xml | axml 未改 XML 时逐字节一致; 文本布局改内容后仍可回写 | `axml` 加 `container-text-xor16-zlib` 或 `plain-text` |
| data | csv xml txt toc ini json cfg dat | data.json 加同名文本文件 | 是, 未改文本时逐字节一致 | `plain-text` 与 `container-text-xor16-zlib` |
| audio | ogg | audio.json | 是, 正文 base64 原样回写 | `ogg` 加 `container-binary-xor16-head` |
| world | vmo vox r cm fcm bp | world.json | 是, 未改数组或载荷时逐字节一致 | 无外部 codec, 自解析加 `util` 的 FlatBuffers 只读解析 |

说明:

1. "是否已可逆" 指 `export` 出的工程目录在未编辑时 `build` 回与源完全相同的字节。
2. 纹理改 PNG 后不保证与源逐字节一致, 因为 ETC 块编码有多解; `build` 返回的
   `pixel_diff` 给出重编码前后的像素误差, 只影响被改动的块。
3. axml 的 XML 文本是只读视图, 改它会在 `build` 时明确报错, 见 `ui/README.md`。

## 三、逐类可编辑项

| 分类 | 可直接编辑的内容 | 只读视图 |
|:---:|:---|:---|
| models | 块载荷 base64, 块顺序, 块名与类型标识 | 块字段视图, 闭式形态 |
| textures | mip0.png, 压缩块 base64, KTX 头字段 | 像素格式名, mip 尺寸 |
| ui | 文本布局内容; axml 的块序列 | axml 的 layout.xml |
| data | 表格文本与数据定义文本 | 列名提示, 行数 |
| audio | 正文 base64 (整文件替换) | 编码, 声道, 采样率, 页表摘要 |
| world | 体素数组, 尺寸, 调色板, 世界坐标, CUST 与 FlatBuffers 正文 | FlatBuffers 槽位, 方块记录 |

## 四、端到端验证

口径: 对每个真实样本执行 `export` -> 工程目录 -> `build` -> 与源逐字节比对。
命令形如 `python -m scaffolds <分类> verify <样本...>`, 样本位于原版逆向树的资源目录。

| 分类 | 样本 | 字节 | 格式 | 结果 | 备注 |
|:---:|:---:|:---:|:---:|:---:|:---|
| models | entity/100035/hand.omod | 231 | named-block | 一致 | 压缩容器套外壳 |
| models | particles/1001.emo | 4899 | named-block | 一致 | 9 块粒子 |
| models | particles/1002.ent | 59 | named-block | 一致 | 单块实体 |
| models | entity/player/body04.oanim | 181360 | named-block | 一致 | 62 块动画 |
| textures | ui/dotted.ktx | 212 | ktx11 | 一致 | ETC1, 17x6, 5 级 mip |
| textures | entity/100003/male.ktx | 43812 | ktx11 | 一致 | ETC1, 256x256, 9 级 mip, 套外壳 |
| textures | itemmods/coord/$1.otex | 3176 | otex | 一致 | RGB8, 32x32 |
| textures | particles/movedir.otex | 65640 | otex | 一致 | RGBA8, 128x128 |
| ui | res/anim/accelerate_cubic.xml | 284 | axml | 一致 | 6 块, 1 个元素 |
| ui | res/drawable-v21/abc_action_bar_item_background_material.xml | 264 | axml | 一致 | 6 块, 1 个元素 |
| ui | assets/blockgeom.xml | 7549 | ui-xml-text | 一致 | 文本族容器, UTF-8 |
| data | csvdef/ARstylesetting.csv | 180 | text-table | 一致 | GBK, 文本族容器 |
| data | csvdef/ARgradepercent.csv | 349 | text-table | 一致 | GBK, 文本族容器 |
| data | checkTask.json | 25241 | text-table | 一致 | 明文 UTF-8 |
| audio | sounds/buff/buff_pop.ogg | 3888 | ogg | 一致 | Vorbis, 套外壳 |
| audio | sounds/misc/pop.ogg | 3888 | ogg | 一致 | Vorbis, 套外壳 |
| audio | sounds/ent/3409/foot2.ogg | 3908 | ogg | 一致 | Vorbis, 套外壳 |
| world | voxel/baozi.vmo | 11220 | vmof | 一致 | 版本 100, 10x7x80 |
| world | voxel/god_temple_room_small1.vmo | 252 | vmof | 一致 | 版本 200, 带尾块 |
| world | voxel/longdan.vox | 516 | vox | 一致 | MAIN 加 SIZE 与 XYZI |
| world | worlds/homegarden_customize_map/10/1/x-1z-4.r | 8207 | world-chunk | 一致 | 坐标 (-1, -4), 密钥 42 |
| world | custommodel_equip/7345778771590029785.cm | 552 | cust | 一致 | 25 条方块记录 |
| world | custommodel_equip/fully/7345778771590031430.fcm | 1912 | flatbuffers | 一致 | 11 槽位根表 |

汇总: 23 个真实样本全部逐字节一致。各分类的 `sample/` 工程也逐一回写并与
`source.sha256` 比对通过 (6/6), 因此样例目录本身可以作为不依赖源二进制的回归输入。

编辑路径另做了行为验证:

| 场景 | 期望 | 实测 |
|:---:|:---|:---|
| 改 models 块载荷 | 容器重排, 总长度随载荷变化 | 通过 |
| 改 KTX 的 mip0.png | 重新压块并重算 mip 链, 返回像素误差 | 通过, 平均误差小于 1 |
| 改 otex 的 mip0.png | 替换 SurfaceData 像素区, 容器长度不变 | 通过 |
| 改 axml 的 layout.xml | 明确报错而不是静默忽略 | 通过 |
| 改文本布局内容 | 按原编码回写并可再次导出 | 通过 |

## 五、新增一个分类的步骤

1. 在 `scaffolds/` 下新建子目录 `<分类>/`, 放三个文件: `README.md`, `__init__.py`, `sample/`。
2. 模块里声明 `NAME` `TITLE` `EXTENSIONS` `FORMATS` `PROJECT_FILE` 五个常量,
   `FORMATS` 的键就是工程文档的 `format` 取值。
3. 实现三个函数:
   - `detect(路径, 字节)` 按魔数判断, 不受理时抛 `util.ScaffoldError`;
   - `export(源路径, 工程目录)` 用 `util.make_doc` 起文档, 用 `util.make_blob_field`
     存二进制兜底, 用 `util.unwrap_envelope` 剥外壳, 写主 JSON 与附属文件;
   - `build(工程目录, 输出路径)` 用 `util.find_project_json` 找主 JSON,
     `util.check_doc` 校验公共字段, 用 `util.wrap_envelope` 重新套壳, 返回摘要字典。
4. 二进制族外壳, base64 兜底, JSON 稳定输出等公共设施都在 `scaffolds/util.py`, 不要复制。
5. 在 `scaffolds/__init__.py` 的 `CATEGORY_NAMES` 里登记分类名, 命令行会自动出现该分类。
6. 生成样例: `python -m scaffolds <分类> export <一份真实素材> -o scaffolds/<分类>/sample`;
   样例目录只保留明文文件, 源摘要留在主 JSON 里。
7. 在 `tests/test_scaffolds.py` 里补两类用例: 样例工程回写与真实样本往返
   (真实样本缺失时跳过)。
8. 在本文件的分类矩阵与验证表补一行, 在 `scaffolds/README.md` 的分类表补一行。
9. 门禁:

```bash
cd single-branch/assets
python -m unittest tests.test_scaffolds -v
python -m scaffolds <分类> verify <至少 3 个真实样本>
```

## 六、未覆盖项与原因

| 项 | 影响 | 原因 |
|:---:|:---|:---|
| axml 的 XML 驱动回写 | 不能按 XML 改布局 | 需重建字符串池, 资源映射与全部块长度, 属独立子任务 |
| 命名区块字段级写回 | 改字段视图不生效 | 21 个块各需一个序列化器, 且 29 个残差块的边界未定 |
| 世界区块数据区 | 不能做方块级编辑 | 数据区编码未定, 单字节密钥与常见压缩参数都解不开 |
| CUST 与 FlatBuffers 结构化写回 | 只能整份替换正文 | 字段 schema 只有槽位候选, 没有可判定的类型真值 |
| 多面与数组 KTX | 只做整份保留 | 引擎资源里没有该类样本, 切分规则未验证 |
| ETC 以外的像素格式 | 只做整份保留 | 资源里没有 PVRTC, ATC, DXT 样本, 无编码器 |
| 音频重编码 | 只能整文件替换 | 不嵌入音频编码器 |
| 图集语义 | 不解析控件与图集的对应 | 属客户端渲染层职责, 脚手架只保证文件级往返 |

## 七、与 forge 管线的关系

1. 脚手架与管线共用编解码器与文本工具, 同一份二进制在两边得到同一份载荷。
2. 脚手架的工程目录是**单份素材**的可编辑形态; 管线索引仍是全树回写的唯一权威。
3. 需要批量处理时用 `forge`; 需要按类修改与重建时用 `scaffolds`。
4. 客户端只消费二进制产物, 工程目录不进客户端资源树。
