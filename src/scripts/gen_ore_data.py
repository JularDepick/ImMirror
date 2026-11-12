#!/usr/bin/env python3
"""从矿脉定义生成主世界矿脉分布表.

输入: 素材仓库 csvdef/oredef.csv 与 blockdef.csv
输出: native/voxel/generated/ore_data_gen.h

字段语义取自 oredef 表:
  MapID       0 地表世界, 1 地下世界; 客户端只生成地表世界, 因此只取 0 的行
  MinHeight 矿脉允许出现的最低高度 (含)
  MaxHeight 矿脉允许出现的最高高度 (含)
  Odds 单次矿脉尝试的命中概率, 万分比
  MaxNum 一条矿脉最多替换的方块数
  Lode 每个区块的矿脉尝试次数
  ReplaceBlock 被替换的方块的客户端 id (取石头 104, 石油取其它)

客户端的方块 id 由 gen_block_data.py 分配, 本脚本直接复用它的分配结果,
不另行维护一份映射, 避免两处漂移.
"""

import csv
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)

import gen_block_data  # noqa: E402  (需要先改好 sys.path 再导入)

SRC_ROOT = gen_block_data.SRC_ROOT
ORE_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project",
                                         "csvdef", "oredef.csv"))
OUT_PATH = os.path.join(SRC_ROOT, "native", "voxel", "generated", "ore_data_gen.h")

# 地表世界的 MapID; 地下世界与活动地图的矿脉不进主世界生成
SURFACE_MAP_ID = 0
# 矿石 id 的兜底替换方块: 石头 (blockdef 104) 的客户端 id
DEFAULT_REPLACE_SOURCE = 104

REQUIRED_COLUMNS = ("ID", "MapID", "Name", "MinHeight", "MaxHeight", "Odds", "MaxNum",
                    "Lode", "ReplaceBlock")


def load_ores():
    """读取矿脉表, 返回按ID 排序的条目列表."""
    with open(ORE_PATH, "r", encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.reader(handle))
    if len(rows) < 3:
        raise SystemExit("矿脉表行数不足: %s" % ORE_PATH)
    header = rows[1]
    for column in REQUIRED_COLUMNS:
        if column not in header:
            raise SystemExit("矿脉表缺少列 %s: %s" % (column, ORE_PATH))
    index = {name: header.index(name) for name in REQUIRED_COLUMNS}
    entries = []
    for row in rows[2:]:
        if len(row) <= index["Lode"]:
            continue
        if row[index["MapID"]].strip() != str(SURFACE_MAP_ID):
            continue
        entries.append({
            "source_id": int(row[index["ID"]].strip()),
            "name": row[index["Name"]].strip(),
            "min_y": int(row[index["MinHeight"]].strip() or 0),
            "max_y": int(row[index["MaxHeight"]].strip() or 0),
            "odds": int(row[index["Odds"]].strip() or 0),
            "max_num": int(row[index["MaxNum"]].strip() or 0),
            "lode": int(row[index["Lode"]].strip() or 0),
            "replace_source": int(row[index["ReplaceBlock"]].strip() or DEFAULT_REPLACE_SOURCE),
        })
    entries.sort(key=lambda item: item["source_id"])
    return entries


def build_client_id_map():
    """复用 gen_block_data 的分配结果, 得到 方块 id -> 客户端方块 id."""
    _, rows = gen_block_data.load_rows()
    legacy_rows, extension_rows = gen_block_data.collect_source_rows(rows)
    allocated, _ = gen_block_data.allocate_ids(extension_rows)
    mapping = {source_id: client_id for client_id, _, source_id, _ in legacy_rows}
    for client_id, source_id, _ in allocated:
        mapping[source_id] = client_id
    return mapping


def render_header(entries):
    """拼出 C++ 头文件文本."""
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/*",
        " * 由 scripts/gen_ore_data.py 从素材仓库 csvdef/oredef.csv 生成, 请勿手改.",
        " * 只收录 MapID = 0 (地表世界) 的矿脉, 地下世界与活动地图的矿脉不进主世界生成.",
        " * 高度是客户端世界坐标的方块高度, 概率为万分比, 每区块按 lode 次尝试逐条推进.",
        " */",
        "namespace immirror::voxel::gen {",
        "",
        "struct OreEntry {",
        "    uint16_t block;        /* 矿脉方块客户端 id */",
        "    uint16_t replace;      /* 被替换方块客户端 id, 只替换这种方块 */",
        "    int16_t minY;          /* 允许出现的最低高度 (含) */",
        "    int16_t maxY;          /* 允许出现的最高高度 (含) */",
        "    uint16_t odds;         /* 单次尝试命中概率, 万分比 */",
        "    uint8_t maxBlocks;     /* 一条矿脉最多替换的方块数 */",
        "    uint8_t lodes;         /* 每个区块的矿脉尝试次数 */",
        "    const char* name;      /* 中文名, 调试用 */",
        "};",
        "",
        "constexpr int32_t kOreEntryCount = %d;" % len(entries),
        "",
        "constexpr OreEntry kOreEntries[kOreEntryCount == 0 ? 1 : kOreEntryCount] = {",
    ]
    for entry in entries:
        lines.append(
            '    {%du, %du, %d, %d, %du, %du, %du, "%s"},'
            % (entry["block"], entry["replace"], entry["min_y"], entry["max_y"],
               entry["odds"], entry["max_num"], entry["lode"], entry["name"]))
    if not entries:
        lines.append("    {0u, 0u, 0, 0, 0u, 0u, 0u, \"\"},")
    lines.extend([
        "};",
        "",
        "}  /* namespace immirror::voxel::gen */",
        "",
    ])
    return "\n".join(lines)


def main():
    mapping = build_client_id_map()
    entries = []
    skipped = []
    for entry in load_ores():
        block = mapping.get(entry["source_id"])
        replace = mapping.get(entry["replace_source"])
        if block is None or replace is None:
            skipped.append(entry["name"])
            continue
        entry["block"] = block
        entry["replace"] = replace
        entries.append(entry)

    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(render_header(entries))
    print("矿脉表生成: %d 条, 跳过 %d 条 (%s)" % (len(entries), len(skipped), ", ".join(skipped)))
    for entry in entries:
        print("  %-10s y=%d..%d odds=%d/%d blocks=%d lodes=%d block=%d replace=%d"
              % (entry["name"], entry["min_y"], entry["max_y"], entry["odds"], 10000,
                 entry["max_num"], entry["lode"], entry["block"], entry["replace"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
