#!/usr/bin/env python3
"""从原版方块定义表生成客户端方块数据头文件.

输入: single-branch/assets/project/csvdef/blockdef.csv (双表头, 1191 行数据)
输出: single-branch/client/src/native/game/generated/block_data_gen.h

生成内容分三块, 全部来自原版方块定义表的实测行:
  1. BlockEntry 全量表: 原版 id, 客户端 id, 英文键, 中文名, 类别, 移动碰撞,
     硬度, 采集工具, 工具等级, 三种音效名, 贴图名与掉落物原值.
  2. BlockData 兼容视图: 沿用旧表字段 (sourceId, name, displayName, 掉落折算),
     供物品表与界面沿用; 扩展段的掉落折算留 0, 原值保留在 dropSourceId 里.
  3. RegistryEntry 注册表扩展段: 与 voxel::BlockInfo 对齐的折算结果
     (solid, opaque, 硬度, 采集工具, 材质族, 音效名与贴图名), 客户端 id 15 起生效.

客户端方块 id 分两段:
  兼容段 0 到 14: 存档与界面引用的 15 种基础方块, 客户端 id 固定不变,
  由 CLIENT_BLOCKS 按 (客户端 id, 英文键, 原版 ID, 期望贴图) 维护,
  原版 ID 与贴图键都来自 blockdef.csv, 贴图键用于自检映射是否漂移.
  扩展段 15 起: 剩余方块按类别分段, 分段顺序由 KEY_SEGMENTS 固定,
  大类别在前, 段容量取原版表该类别的实测条数, 段内按原版 ID 升序;
  KEY_SEGMENTS 未列出的类别全部落入收尾的 other 段, 同样按原版 ID 升序.
  原版表 1191 行数据全部接入: 原版编辑器内部占位行 (无键, 无英文名,
  无贴图, 无音效, 无掉落) 同样进表, 英文键用 block_<原版 ID> 兜底,
  硬度按空缺取 -1 保守处理, 供后续原版启用时直接替换数据.

掉落物字段按 ToolMineDrop1, HandMineDrop, PreciseDrop 的优先级取原值,
概率为万分比, 概率为 0 视为该字段不生效; 原版掉落取值是物品 id,
与客户端方块 id 不同域, 折算映射留待物品侧接入, 当前记 0 表示无掉落.

移动碰撞折算进注册表扩展段: 1 固体与 3 不挡投射物按实心处理,
0 空气, 2 液体, 4 空气挡子弹, 5 不阻挡物理与空缺按非实心处理.
遮挡折算只认原版"遮挡旁边块"为 1 的行, 其余 (含厚度 2 与同材质 3)
一律按非遮挡处理, 交由后续按厚度建模时再放开.

生成物列入 .gitignore, 由 scripts/build-native.sh 在构建前调用.
本脚本可重复运行, 产物字节级确定: 所有遍历都按 id 或固定表序进行,
不依赖文件行序与字典序.
"""

import csv
import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
# 素材仓库与本仓库同级, 方块定义表只在素材仓库维护一份
BLOCKDEF_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project",
                                              "csvdef", "blockdef.csv"))
OUT_PATH = os.path.join(SRC_ROOT, "native", "game", "generated", "block_data_gen.h")
REGISTRY_HEADER_PATH = os.path.join(SRC_ROOT, "native", "voxel", "block_registry.h")

# 兼容段方块表: (客户端 id, 注册表英文键, 原版 blockdef ID, 期望的 Texture1 值)
# 空气没有贴图, 纹理键留空跳过自检; 这 15 行的客户端 id 与语义不得变动
CLIENT_BLOCKS = [
    (0, "air", 0, ""),
    (1, "grass", 100, "grass"),
    (2, "dirt", 101, "dirt"),
    (3, "stone", 104, "stone"),
    (4, "sand", 106, "sand"),
    (5, "water", 4, "water_flow"),
    (6, "wood", 200, "log_fruittree"),
    (7, "leaves", 218, "leaves_fruittree"),
    (8, "bedrock", 1, "bedrock"),
    (9, "brick", 547, "brick"),
    (10, "cobblestone", 505, "cobblestone"),
    (11, "snow", 115, "snow"),
    (12, "ice", 123, "ice"),
    (13, "gravel", 107, "gravel"),
    (14, "planks", 206, "planks_fruittree"),
]

# 扩展段类别分段表: (原版 Type 值, 段容量). 顺序即客户端 id 的分配顺序,
# 容量取原版表该类别的实测条数, 原版新增同类条数超出容量时会直接报错,
# 此时按追加顺序扩一段即可, 已分配的客户端 id 不受影响.
KEY_SEGMENTS = (
    ("basic", 255),
    ("log", 35),
    ("glass", 35),
    ("lamp", 32),
    ("furniture", 30),
    ("slab", 26),
    ("glasspane", 20),
    ("stair", 18),
    ("fence", 18),
    ("chair", 18),
    ("colorherbs", 16),
    ("starmushroom", 16),
    ("carpet", 16),
    ("mobspawner", 16),
    ("teamspawn", 15),
    ("screen", 15),
    ("flowergrow", 14),
    ("horseegg", 14),
    ("homelandplant", 14),
    ("chest", 12),
    ("sapling", 11),
    ("jar", 11),
    ("door", 11),
    ("bed", 11),
    ("grayleaf", 10),
)

# 收尾段类别名: KEY_SEGMENTS 未列出的类别全部落入这一段
OTHER_CATEGORY = "other"

# 掉落物字段按优先级排列, 概率为万分比, 概率为 0 视为该字段不生效
DROP_FIELDS = (("ToolMineDrop1", "ToolMineProb1"), ("HandMineDrop", "HandMineProb"))

# 原版挖掘音族到注册表材质族的折算: 键是去掉 blockd./blocks. 前缀的族名,
# 值与 voxel::HarvestMaterial 的取值一致, 0 表示不发声
SOUND_FAMILY_TO_MATERIAL = {
    "grass": 1,
    "stone": 2,
    "sand": 3,
    "wood": 4,
    "snow": 5,
    "gravel": 6,
    "cloth": 7,
    "glass": 8,
    "jar": 9,
    "ladder": 10,
}

# 原版采集工具值域: 0 徒手, 1 斧, 2 镐, 3 铲, 5 剪刀
VALID_TOOLS = (0, 1, 2, 3, 5)

# 硬度合法范围, 与 voxel 侧的编译期校验一致
HARDNESS_MIN = -1.0
HARDNESS_MAX = 200.0

NOTICE = "由 scripts/gen_block_data.py 从 assets 仓库的 blockdef.csv 生成, 请勿手改"


def emit(message):
    """打印进度. 控制台编码受限时降级为 ASCII, 不让日志中断生成."""
    try:
        print(message)
    except UnicodeEncodeError:
        print(message.encode("ascii", "replace").decode("ascii"))


def to_int(text):
    """空串与非数字一律按 0 处理, 数据表里存在空列."""
    try:
        return int(str(text).strip())
    except (TypeError, ValueError):
        return 0


def to_float(text, fallback=0.0):
    """空串与非数字按 fallback 处理, 硬度列存在空缺."""
    try:
        return float(str(text).strip())
    except (TypeError, ValueError):
        return fallback


def escape(text):
    """转义 C 字符串字面量里的反斜杠与双引号."""
    return text.replace("\\", "\\\\").replace('"', '\\"')


def fold_key(en_name, key, source_id):
    """把原版英文名折算成客户端英文键: 小写下划线, 非字母数字折成下划线.

    英文名称列缺失时回退到原版插件键列 (同为英文标识), 两者都缺时
    用 block_<原版 ID> 兜底, 保证每个方块都有稳定键名.
    """
    base = (en_name or key or "").strip().lower()
    base = re.sub(r"[^0-9a-z]+", "_", base).strip("_")
    if not base:
        base = "block_%d" % source_id
    return base


def load_rows():
    """读取方块定义表, 返回英文表头与按 ID 索引的数据行."""
    with open(BLOCKDEF_PATH, "r", encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.reader(handle))
    if len(rows) < 3:
        raise SystemExit("方块定义表行数不足: %s" % BLOCKDEF_PATH)
    header = rows[1]
    required = ("ID", "Name", "Key", "ENName", "Type", "MoveCollide", "Hardness",
                "MineTool", "ToolLevel", "Texture1", "WalkSound", "DigSound",
                "PlaceSound", "CoverNeighbor", "PreciseDrop")
    for column in required + tuple(name for pair in DROP_FIELDS for name in pair):
        if column not in header:
            raise SystemExit("方块定义表缺少列 %s: %s" % (column, BLOCKDEF_PATH))
    table = {}
    for row in rows[2:]:
        if len(row) <= header.index("Name"):
            continue
        key = row[header.index("ID")].strip()
        if not key or key in table:
            continue
        table[key] = {name: (row[index] if index < len(row) else "")
                      for index, name in enumerate(header) if name}
    return header, table


def pick_drop(row):
    """按优先级取掉落物原始取值, 返回 (掉落 id, 命中的字段名)."""
    for drop_column, prob_column in DROP_FIELDS:
        drop = to_int(row.get(drop_column))
        if drop > 0 and to_int(row.get(prob_column)) > 0:
            return drop, drop_column
    precise = to_int(row.get("PreciseDrop"))
    if precise > 0:
        return precise, "PreciseDrop"
    return 0, ""


def sound_family(sound_name):
    """拆出音效名的族段与原值, 返回 (族名, 原始名). 原始名保留前缀原样透传."""
    name = sound_name.strip()
    if not name:
        return "", ""
    parts = name.split(".")
    return parts[-1] if len(parts) > 1 else name, name


def registry_block_count():
    """从 block_registry.h 读方块数量, 读不到返回 -1, 只用于漂移告警."""
    try:
        with open(REGISTRY_HEADER_PATH, "r", encoding="utf-8") as handle:
            text = handle.read()
    except OSError:
        return -1
    match = re.search(r"constexpr\s+BlockId\s+Count\s*=\s*(\d+)\s*;", text)
    return int(match.group(1)) if match else -1


def collect_source_rows(rows):
    """把原版行拆成兼容段映射与扩展段候选, 返回 (兼容行表, 扩展行列表, 排除行表)."""
    legacy_rows = []
    for client_id, name, source_id, texture in CLIENT_BLOCKS:
        row = rows.get(str(source_id))
        if row is None:
            raise SystemExit("方块定义表中找不到 ID %d (%s)" % (source_id, name))
        actual_texture = row.get("Texture1", "").strip()
        if texture and actual_texture != texture:
            raise SystemExit("ID %d (%s) 的纹理键变化: 期望 %s, 实际 %s"
                             % (source_id, name, texture, actual_texture))
        display_name = row.get("Name", "").strip()
        if not display_name:
            raise SystemExit("ID %d (%s) 没有中文名" % (source_id, name))
        legacy_rows.append((client_id, name, source_id, row))

    legacy_sources = {source_id for _, _, source_id, _ in CLIENT_BLOCKS}
    extension_rows = []
    for source_text in sorted(rows, key=lambda value: int(value)):
        source_id = int(source_text)
        if source_id in legacy_sources:
            continue
        extension_rows.append((source_id, rows[source_text]))
    return legacy_rows, extension_rows


def allocate_ids(extension_rows):
    """把扩展段候选按类别分段分配客户端 id, 返回 (分配表, 分段区间表).

    分配表元素是 (客户端 id, 原版 id, 原版行); 分段区间表元素是
    (类别名, 段起点, 段条数), 供生成头文件的说明注释与界面分组使用.
    """
    by_category = {}
    for source_id, row in extension_rows:
        category = row.get("Type", "").strip() or OTHER_CATEGORY
        by_category.setdefault(category, []).append((source_id, row))
    for entries in by_category.values():
        entries.sort(key=lambda pair: pair[0])

    allocated = []
    segments = []
    next_id = len(CLIENT_BLOCKS)
    for category, capacity in KEY_SEGMENTS:
        entries = by_category.pop(category, [])
        if len(entries) > capacity:
            raise SystemExit("类别 %s 实测 %d 条超出段容量 %d, 请在 KEY_SEGMENTS 追加段"
                             % (category, len(entries), capacity))
        segments.append((category, next_id, len(entries)))
        for source_id, row in entries:
            allocated.append((next_id, source_id, row))
            next_id += 1
    rest = []
    for category in sorted(by_category):
        rest.extend(by_category[category])
    rest.sort(key=lambda pair: pair[0])
    segments.append((OTHER_CATEGORY, next_id, len(rest)))
    for source_id, row in rest:
        allocated.append((next_id, source_id, row))
        next_id += 1
    return allocated, segments


def build_extension_entry(client_id, source_id, row, used_keys):
    """把一条扩展段原版行折算成生成表条目."""
    category = row.get("Type", "").strip()
    display_name = row.get("Name", "").strip() or "未命名"
    key = fold_key(row.get("ENName"), row.get("Key"), source_id)
    if key in used_keys:
        key = "%s_%d" % (key, source_id)
    used_keys[key] = True

    collision_text = row.get("MoveCollide", "").strip()
    collision = to_int(collision_text) if collision_text else 1
    if collision not in (0, 1, 2, 3, 4, 5):
        collision = 1

    hardness = to_float(row.get("Hardness"), -1.0)
    if not HARDNESS_MIN <= hardness <= HARDNESS_MAX:
        hardness = -1.0

    tool = to_int(row.get("MineTool"))
    if tool not in VALID_TOOLS:
        tool = 0
    tool_level = to_int(row.get("ToolLevel"))
    if not 0 <= tool_level <= 15:
        tool_level = 0

    dig_family, dig_name = sound_family(row.get("DigSound"))
    _, place_name = sound_family(row.get("PlaceSound"))
    _, walk_name = sound_family(row.get("WalkSound"))
    material = SOUND_FAMILY_TO_MATERIAL.get(dig_family, 0)

    texture = row.get("Texture1", "").strip()
    drop_source, drop_field = pick_drop(row)

    # 移动碰撞到实心属性的折算: 1 固体与 3 不挡投射物按实心处理
    solid = collision in (1, 3)
    # 遮挡只认原版"遮挡旁边块"为 1 的行, 厚度与同材质的行保守按非遮挡处理
    opaque = row.get("CoverNeighbor", "").strip() == "1"

    return {
        "client_id": client_id,
        "source_id": source_id,
        "key": key,
        "display_name": display_name,
        "category": category,
        "collision": collision,
        "hardness": hardness,
        "tool": tool,
        "tool_level": tool_level,
        "dig_sound": dig_name,
        "place_sound": place_name,
        "walk_sound": walk_name,
        "material": material,
        "solid": solid,
        "opaque": opaque,
        "texture": texture,
        "drop_source": drop_source,
        "drop_field": drop_field,
    }


def format_float(value):
    """把硬度浮点数写成稳定的 C 字面量: 一位小数覆盖 0.5 步长与 0.4 树叶."""
    return "%.1ff" % value


def render_legacy_row(client_id, name, source_id, data, drop_source, drop_field,
                      drop_block):
    """拼出兼容段的一行全表条目. 字段与扩展段一致, 数值取原版原值."""
    comment = "兼容段, 掉落字段 %s" % drop_field if drop_field else "兼容段, 无掉落字段"
    collision_text = data.get("MoveCollide", "").strip()
    collision = to_int(collision_text) if collision_text else 1
    tool_text = data.get("MineTool", "").strip()
    tool = to_int(tool_text) if tool_text in ("0", "1", "2", "3", "5") else 0
    tool_level = to_int(data.get("ToolLevel"))
    if not 0 <= tool_level <= 15:
        tool_level = 0
    hardness = to_float(data.get("Hardness"), -1.0)
    if not HARDNESS_MIN <= hardness <= HARDNESS_MAX:
        hardness = -1.0
    return ('    {%du, %du, "%s", "%s", "%s", %du, %s, %du, %du, "%s", "%s", "%s", "%s", %du},'
            "  /* %s */"
            % (source_id, client_id, escape(name),
               escape(data.get("Name", "").strip()),
               escape(data.get("Type", "").strip()), collision,
               format_float(hardness), tool, tool_level,
               escape(data.get("DigSound", "").strip()),
               escape(data.get("PlaceSound", "").strip()),
               escape(data.get("WalkSound", "").strip()),
               escape(data.get("Texture1", "").strip()),
               drop_source, comment))


def render_header(entries, legacy_rows, segments):
    """拼出 C++ 头文件文本."""
    total = len(CLIENT_BLOCKS) + len(entries)
    segment_lines = []
    for category, start, count in segments:
        segment_lines.append(" *   %-13s %4d 起, %d 条" % (category + ":", start, count))
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/*",
        " * " + NOTICE + ".",
        " * 数据源: single-branch/assets/project/csvdef/blockdef.csv",
        " *",
        " * 客户端方块 id 分配规则:",
        " *   兼容段 0 到 %d: 存档与界面引用的 %d 种基础方块, id 固定不变,"
        % (len(CLIENT_BLOCKS) - 1, len(CLIENT_BLOCKS)),
        " *   由生成脚本的 CLIENT_BLOCKS 按 (客户端 id, 原版 ID) 固定映射;",
        " *   扩展段 %d 起: 按类别分段, 大类别在前, 段内按原版 ID 升序," % len(CLIENT_BLOCKS),
        " *   分段区间如下 (起点即该类别的首个客户端 id):",
    ]
    lines.extend(segment_lines)
    lines += [
        " *   原版未启用的内部占位行不接入客户端, 后续启用时在段尾追加.",
        " *",
        " * 字段语义:",
        " *   sourceId 是原版 blockdef 的 ID, key 是客户端注册表英文键",
        " *   (原版英文名折算小写下划线, 重名追加原版 ID 后缀),",
        " *   displayName 是原版中文名 (UTF-8), category 是原版 Type 值,",
        " *   collision 是原版移动碰撞 (0 空气 1 固体 2 液体 3 不挡投射物",
        " *   4 空气挡子弹 5 不阻挡物理), hardness 是原版硬度原值",
        " *   (-1 不可破坏, 上限 200), tool 是采集工具 (0 徒手 1 斧 2 镐",
        " *   3 铲 5 剪刀), toolLevel 是工具等级, digSound/placeSound/walkSound",
        " *   保留原版 blockd./blocks. 前缀音效名 (空串表示没有), texture 是",
        " *   原版贴图1 字段名 (空串表示没有), dropSourceId 是原版掉落物",
        " *   物品 id (0 表示无掉落), 掉落到客户端 id 的折算映射后续接入.",
        " */",
        "namespace immirror::game::gen {",
        "",
        "/* 兼容段方块数, 与 voxel::block 的固定常量一致. */",
        "constexpr int32_t kLegacyBlockCount = %d;" % len(CLIENT_BLOCKS),
        "",
        "/* 全量表条目数, 也是合法客户端方块 id 的上界. */",
        "constexpr int32_t kBlockEntryCount = %d;" % total,
        "",
        "/* 单个方块的完整定义. */",
        "struct BlockEntry {",
        "    uint32_t sourceId;       /* 原版 blockdef 的 ID */",
        "    uint16_t clientId;       /* 客户端方块 id, 与数组下标一致 */",
        "    const char* key;         /* 客户端注册表英文键 */",
        "    const char* displayName; /* 原版中文名, UTF-8 编码 */",
        "    const char* category;    /* 原版 Type 类别值 */",
        "    uint8_t collision;       /* 原版移动碰撞类型 0 到 5 */",
        "    float hardness;          /* 原版硬度, -1 表示不可破坏 */",
        "    uint8_t tool;            /* 采集工具: 0 徒手 1 斧 2 镐 3 铲 5 剪刀 */",
        "    uint8_t toolLevel;       /* 采集工具等级 */",
        "    const char* digSound;    /* 原版挖掘音名, 空串表示没有 */",
        "    const char* placeSound;  /* 原版放置音名, 空串表示没有 */",
        "    const char* walkSound;   /* 原版行走音名, 空串表示没有 */",
        "    const char* texture;     /* 原版贴图1 名, 空串表示没有 */
    uint8_t lightSource;     /* 原版光源强度 0-15, 0 不发光 */
    uint8_t lightAtten;      /* 原版光衰减 0-15 */
    uint8_t coverMode;       /* 原版遮挡旁边块 0-4 */",
        "    uint32_t dropSourceId;   /* 原版掉落物物品 id, 0 表示无掉落 */",
        "};",
        "",
        "/* 条目按客户端 id 升序排列, 下标即客户端 id, 越界回落到首条 (空气). */",
        "constexpr BlockEntry kBlockEntries[kBlockEntryCount] = {",
    ]
    source_to_client = {source_id: client_id
                        for client_id, _, source_id, _ in CLIENT_BLOCKS}
    for client_id, name, source_id, data in legacy_rows:
        drop_source, drop_field = pick_drop(data)
        drop_block = source_to_client.get(drop_source, client_id) if drop_source > 0 else 0
        del drop_block
        lines.append(render_legacy_row(client_id, name, source_id, data,
                                       drop_source, drop_field, 0))
    for entry in entries:
        comment = "%s, 掉落字段 %s" % (entry["category"] or "无类别",
                                     entry["drop_field"] or "无")
        lines.append('    {%du, %du, "%s", "%s", "%s", %du, %s, %du, %du, "%s", "%s", "%s", "%s", %du},'
                     "  /* %s */"
                     % (entry["source_id"], entry["client_id"], escape(entry["key"]),
                        escape(entry["display_name"]), escape(entry["category"]),
                        entry["collision"], format_float(entry["hardness"]),
                        entry["tool"], entry["tool_level"],
                        escape(entry["dig_sound"]), escape(entry["place_sound"]),
                        escape(entry["walk_sound"]), escape(entry["texture"]),
                        entry["drop_source"], comment))
    lines += [
        "};",
        "",
        "/* 按客户端方块 id 取数据, 越界回落到空气条目, 与 BlockRegistry 的回落约定一致. */",
        "inline const BlockEntry& BlockEntryOf(uint32_t id) {",
        "    return kBlockEntries[id < static_cast<uint32_t>(kBlockEntryCount) ? id : 0u];",
        "}",
        "",
        "/* 按原版 ID 反查条目, 未收录返回空指针; 表内按 id 升序, 线性扫描足够. */",
        "inline const BlockEntry* BlockEntryOfSource(uint32_t sourceId) {",
        "    for (int32_t index = 0; index < kBlockEntryCount; ++index) {",
        "        if (kBlockEntries[index].sourceId == sourceId) {",
        "            return &kBlockEntries[index];",
        "        }",
        "    }",
        "    return nullptr;",
        "}",
        "",
        "/* 客户端方块 id 到原版 ID 的对照表, 供物品生成与编辑器关联使用. */",
        "constexpr uint32_t kSourceIds[kBlockEntryCount] = {",
    ]
    source_ids = [source_id for _, _, source_id, _ in legacy_rows]
    source_ids += [entry["source_id"] for entry in entries]
    for start in range(0, len(source_ids), 12):
        lines.append("    " + " ".join("%du," % value for value in source_ids[start:start + 12]))
    lines += [
        "};",
        "",
        "/*",
        " * 兼容段掉落视图: 沿用旧 BlockData 的字段与语义, 供物品表与界面沿用.",
        " * dropBlock 是破坏后进入物品栏的客户端方块 id, 仅兼容段有折算值,",
        " * 扩展段掉落折算后续接入时在此补充; dropSourceId 保留原版物品 id.",
        " */",
        "struct BlockData {",
        "    uint32_t sourceId;       /* 原版 blockdef 的 ID */",
        "    const char* name;        /* 客户端注册表中的英文键 */",
        "    const char* displayName; /* 原版中文名, UTF-8 编码 */",
        "    uint32_t dropSourceId;   /* 原版掉落物字段的原始取值, 0 表示无掉落 */",
        "    uint32_t dropBlock;      /* 折算后的客户端掉落方块 id, 0 表示无掉落 */",
        "};",
        "",
        "constexpr int32_t kBlockDataCount = kBlockEntryCount;",
        "",
        "/* 条目顺序即客户端方块 id, 越界 id 统一回落到首条 (空气). */",
        "constexpr BlockData kBlockData[kBlockDataCount] = {",
    ]
    for client_id, name, source_id, data in legacy_rows:
        drop_source, drop_field = pick_drop(data)
        # 掉落取值是物品 id: 能折回兼容段的按映射取, 折不回则原地回收自身
        drop_block = source_to_client.get(drop_source, client_id) if drop_source > 0 else 0
        comment = "掉落字段 %s" % drop_field if drop_field else "无掉落字段"
        lines.append('    {%du, "%s", "%s", %du, %du},  /* %s */'
                     % (source_id, escape(name),
                        escape(data.get("Name", "").strip()),
                        drop_source, drop_block, comment))
    for entry in entries:
        lines.append('    {%du, "%s", "%s", %du, 0u},  /* 扩展段, 掉落待折算 */'
                     % (entry["source_id"], escape(entry["key"]),
                        escape(entry["display_name"]), entry["drop_source"]))
    lines += [
        "};",
        "",
        "/* 按客户端方块 id 取数据, 越界回落到空气条目, 与 BlockRegistry 的回落约定一致. */",
        "inline const BlockData& BlockDataOf(uint32_t id) {",
        "    return kBlockData[id < static_cast<uint32_t>(kBlockDataCount) ? id : 0u];",
        "}",
        "",
        "/*",
        " * 破坏方块后进入物品栏的方块 id, 0 表示该方块没有掉落物.",
        " * 超出生成表的方块原地回收自身, 与物品栏的容错约定一致.",
        " */",
        "inline uint32_t DropBlockOf(uint32_t id) {",
        "    if (id >= static_cast<uint32_t>(kBlockDataCount)) {",
        "        return id;",
        "    }",
        "    return BlockDataOf(id).dropBlock;",
        "}",
        "",
        "/*",
        " * 注册表扩展段: 与 voxel::BlockInfo 对齐的折算结果, 下标 i 对应客户端",
        " * 方块 id (kLegacyBlockCount + i). solid 由移动碰撞折算 (1 固体与",
        " * 3 不挡投射物按实心), opaque 由原版遮挡旁边块折算 (仅 1 视为遮挡),",
        " * material 由挖掘音族折算, 取值与 voxel::HarvestMaterial 一致,",
        " * 音效名与贴图名保留原版原值, 供音频与图集层直接引用.",
        " */",
        "struct RegistryEntry {",
        "    uint16_t clientId;       /* 客户端方块 id, 编译期校验连续 */",
        "    const char* name;        /* 注册表英文键, 与 BlockEntry.key 一致 */",
        "    bool solid;              /* 参与移动碰撞 */",
        "    bool opaque;             /* 遮挡邻面 */",
        "    float hardness;          /* 原版硬度, -1 表示不可破坏 */",
        "    uint8_t tool;            /* 采集工具, 与 voxel::HarvestTool 对齐 */",
        "    uint8_t material;        /* 采集材质族, 与 voxel::HarvestMaterial 对齐 */",
        "    const char* digSound;    /* 原版挖掘音名, 空串表示没有 */",
        "    const char* placeSound;  /* 原版放置音名, 空串表示没有 */",
        "    const char* walkSound;   /* 原版行走音名, 空串表示没有 */",
        "    const char* texture;     /* 原版贴图1 名, 空串表示没有 */
    uint8_t lightSource;     /* 原版光源强度 0-15, 0 不发光 */
    uint8_t lightAtten;      /* 原版光衰减 0-15 */
    uint8_t coverMode;       /* 原版遮挡旁边块 0-4 */",
        "};",
        "",
        "constexpr int32_t kRegistryEntryCount = kBlockEntryCount - kLegacyBlockCount;",
        "",
        "constexpr RegistryEntry kRegistryEntries[kRegistryEntryCount] = {",
    ]
    for entry in entries:
        lines.append('    {%du, "%s", %s, %s, %s, %du, %du, "%s", "%s", "%s", "%s"},'
                     % (entry["client_id"], escape(entry["key"]),
                        "true" if entry["solid"] else "false",
                        "true" if entry["opaque"] else "false",
                        format_float(entry["hardness"]), entry["tool"],
                        entry["material"], escape(entry["dig_sound"]),
                        escape(entry["place_sound"]), escape(entry["walk_sound"]),
                        escape(entry["texture"])))
    lines += [
        "};",
        "",
        "/* 扩展段条目的客户端 id 必须从兼容段末尾起连续编号. */",
        "constexpr bool RegistryIdsValid() {",
        "    for (int32_t index = 0; index < kRegistryEntryCount; ++index) {",
        "        if (kRegistryEntries[index].clientId !=",
        "                static_cast<uint16_t>(kLegacyBlockCount + index)) {",
        "            return false;",
        "        }",
        "    }",
        "    return true;",
        "}",
        "static_assert(RegistryIdsValid(),",
        "              \"扩展段客户端 id 不连续, 请重新运行 scripts/gen_block_data.py\");",
        "",
        "}  // namespace immirror::game::gen",
        "",
    ]
    return "\n".join(lines)


def main():
    if not os.path.isfile(BLOCKDEF_PATH):
        raise SystemExit("方块定义表不存在: %s" % BLOCKDEF_PATH)
    _, rows = load_rows()
    legacy_rows, extension_rows = collect_source_rows(rows)
    allocated, segments = allocate_ids(extension_rows)

    used_keys = {}
    for _, name, _, _ in legacy_rows:
        used_keys[name] = True
    entries = []
    for client_id, source_id, row in allocated:
        entries.append(build_extension_entry(client_id, source_id, row, used_keys))

    total = len(CLIENT_BLOCKS) + len(entries)
    count = registry_block_count()
    if count >= 0 and count != total:
        emit("警告: 注册表方块数为 %d, 生成表为 %d 条, 请同步 block_registry"
             % (count, total))

    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(render_header(entries, legacy_rows, segments))

    emit("方块数据表已生成: %s" % OUT_PATH)
    emit("  原版行 %d 条, 全量接入 %d 条 (兼容段 %d + 扩展段 %d)"
         % (len(rows), total, len(CLIENT_BLOCKS), len(entries)))
    for category, start, count_in_segment in segments:
        emit("  段 %-16s %4d 起, %3d 条" % (category, start, count_in_segment))
    emit("  block_<原版ID> 兜底键: %d 个, 重名追加后缀键: %d 个"
         % (sum(1 for entry in entries if entry["key"].startswith("block_")),
            sum(1 for entry in entries if entry["key"].endswith("_%d" % entry["source_id"])
                and not entry["key"].startswith("block_"))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
