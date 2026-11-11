#!/usr/bin/env python3
"""从原版方块定义表生成客户端方块数据头文件.

输入: single-branch/assets/project/csvdef/blockdef.csv (双表头, 1193 行)
输出: single-branch/client/src/native/game/generated/block_data_gen.h

只提取客户端当前需要的两列语义: Name (中文显示名) 与掉落物字段
(ToolMineDrop1 优先, 其次 HandMineDrop, 最后 PreciseDrop). 原版掉落物取值是
物品 id, 与本客户端方块 id 不同域; 因此先把掉落 id 折算回原版方块 id, 再按
CLIENT_BLOCKS 的映射折算成客户端方块 id, 折算不到时回落到方块自身,
保证沙盒里破坏任何方块都有可见回收.

生成物列入 .gitignore, 由 scripts/build-native.sh 在构建前调用.
方块顺序与 voxel/block_registry.cpp 的 kBlockTable 一致, 新增方块时在本脚本的
CLIENT_BLOCKS 里补一行, 不改动原生侧的任何手写数据.
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

# 客户端方块表: (客户端 id, 注册表英文键, 原版 blockdef ID, 期望的 Texture1 值)
# 原版 ID 与纹理键都来自 blockdef.csv, 纹理键用于自检映射是否漂移;
# 空气没有贴图, 纹理键留空跳过自检.
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

# 掉落物字段按优先级排列, 概率为万分比, 概率为 0 视为该字段不生效
DROP_FIELDS = (("ToolMineDrop1", "ToolMineProb1"), ("HandMineDrop", "HandMineProb"))

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


def escape(text):
    """转义 C 字符串字面量里的反斜杠与双引号."""
    return text.replace("\\", "\\\\").replace('"', '\\"')


def load_rows():
    """读取方块定义表, 返回英文表头与按 ID 索引的数据行."""
    with open(BLOCKDEF_PATH, "r", encoding="utf-8", newline="") as handle:
        rows = list(csv.reader(handle))
    if len(rows) < 3:
        raise SystemExit("方块定义表行数不足: %s" % BLOCKDEF_PATH)
    header = rows[1]
    required = ("ID", "Name", "Key", "Texture1", "PreciseDrop")
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


def registry_block_count():
    """从 block_registry.h 读方块数量, 读不到返回 -1, 只用于漂移告警."""
    try:
        with open(REGISTRY_HEADER_PATH, "r", encoding="utf-8") as handle:
            text = handle.read()
    except OSError:
        return -1
    match = re.search(r"constexpr\s+BlockId\s+Count\s*=\s*(\d+)\s*;", text)
    return int(match.group(1)) if match else -1


def build_entries(rows):
    """把客户端方块表与原版行合并成待写出的条目."""
    source_to_client = {source_id: client_id for client_id, _, source_id, _ in CLIENT_BLOCKS}
    entries = []
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
        drop_source, drop_field = pick_drop(row)
        # 掉落取值是物品 id: 能折回本方块的按映射取, 折不回则原地回收自身
        drop_block = source_to_client.get(drop_source, client_id) if drop_source > 0 else 0
        entries.append({
            "client_id": client_id,
            "name": name,
            "source_id": source_id,
            "display_name": display_name,
            "drop_source": drop_source,
            "drop_field": drop_field,
            "drop_block": drop_block,
        })
    return entries


def render_header(entries):
    """拼出 C++ 头文件文本."""
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/*",
        " * " + NOTICE + ".",
        " * 数据源: single-branch/assets/project/csvdef/blockdef.csv",
        " * 每行对应一个客户端方块 id, 顺序与 voxel/block_registry.cpp 的 kBlockTable 一致.",
        " * displayName 是原版中文名 (UTF-8), dropBlock 是破坏后进入物品栏的客户端方块 id,",
        " * sourceId 与 dropSourceId 保留原版 ID 便于回溯, 0 表示该方块没有掉落物.",
        " */",
        "namespace immirror::game::gen {",
        "",
        "/* 单个方块的展示与掉落数据. */",
        "struct BlockData {",
        "    uint32_t sourceId;       /* 原版 blockdef 的 ID */",
        "    const char* name;        /* 客户端注册表中的英文键 */",
        "    const char* displayName; /* 原版中文名, UTF-8 编码 */",
        "    uint32_t dropSourceId;   /* 原版掉落物字段的原始取值, 0 表示无掉落 */",
        "    uint32_t dropBlock;      /* 折算后的客户端掉落方块 id, 0 表示无掉落 */",
        "};",
        "",
        "constexpr int32_t kBlockDataCount = %d;" % len(entries),
        "",
        "/* 条目顺序即客户端方块 id, 越界 id 统一回落到首条 (空气). */",
        "constexpr BlockData kBlockData[kBlockDataCount] = {",
    ]
    for entry in entries:
        comment = "掉落字段 %s" % entry["drop_field"] if entry["drop_field"] else "无掉落字段"
        lines.append('    {%du, "%s", "%s", %du, %du},  /* %s */'
                     % (entry["source_id"], escape(entry["name"]), escape(entry["display_name"]),
                        entry["drop_source"], entry["drop_block"], comment))
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
        "}  // namespace immirror::game::gen",
        "",
    ]
    return "\n".join(lines)


def main():
    if not os.path.isfile(BLOCKDEF_PATH):
        raise SystemExit("方块定义表不存在: %s" % BLOCKDEF_PATH)
    header, rows = load_rows()
    del header
    entries = build_entries(rows)

    count = registry_block_count()
    if count >= 0 and count != len(entries):
        emit("警告: 注册表方块数为 %d, 生成表为 %d 条, 请同步 CLIENT_BLOCKS"
             % (count, len(entries)))

    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(render_header(entries))

    emit("方块数据表已生成: %s" % OUT_PATH)
    for entry in entries:
        emit("  %2d %-11s 原版 %4d %-8s 掉落 %5d (%s) -> 客户端 %d"
             % (entry["client_id"], entry["name"], entry["source_id"], entry["display_name"],
                entry["drop_source"], entry["drop_field"] or "无", entry["drop_block"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
