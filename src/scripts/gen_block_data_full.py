#!/usr/bin/env python3
"""从方块定义表生成全量方块注册表头文件.

输入: single-branch/assets/project/csvdef/blockdef.csv (双表头, 1193 行)
输出: single-branch/client/src/native/voxel/generated/block_registry_gen.h

全量 1191 条: id 顺序分配客户端 id, 现有 15 种 (air..planks) 的客户端 id
保持不变, 保证世界存档兼容. 硬度/工具/音效名/贴图名按原值保留.

生成物列入 .gitignore, 由 scripts/build-native.sh 在构建前调用.
"""

import csv
import json
import os
import re

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
ROOT = os.path.normpath(os.path.join(SRC_ROOT, "..", ".."))
BLOCKDEF_PATH = os.path.join(ROOT, "single-branch", "assets", "project", "csvdef", "blockdef.csv")
OUT_PATH = os.path.join(SRC_ROOT, "native", "voxel", "generated", "block_registry_gen.h")

# 现有 15 种: 客户端 id -> id (blockdef 第一列 ID), 保持不变保证存档兼容
EXISTING_IDS = {0: 0, 1: 100, 2: 101, 3: 104, 4: 106, 5: 4, 6: 200, 7: 218, 8: 1,
                9: 547, 10: 505, 11: 115, 12: 123, 13: 107, 14: 206}
REVERSE = {v: k for k, v in EXISTING_IDS.items()}

NOTICE = "由 scripts/gen_block_data_full.py 从 assets 仓库的 blockdef.csv 生成, 请勿手改"


def sanitize(text):
    """字段内的引号与换行清理, 保证 C++ 字符串字面量合法."""
    return (text or "").replace('"', "").replace("\\n", " ").strip()


def safe_const_key(en_key, cid):
    key = re.sub(r"[^a-zA-Z0-9]", "_", en_key or ("Block%d" % cid)).upper()
    key = re.sub(r"_+", "_", key).strip("_")
    if not key:
        key = "BLOCK_%d" % cid
    if key[0].isdigit():
        key = "B_" + key
    return key


def safe_float(value, default=1.0):
    try:
        return float(value)
    except (ValueError, TypeError):
        return default


def main():
    with open(BLOCKDEF_PATH, "rb") as handle:
        rows = list(csv.reader(io.StringIO(handle.read().decode("utf-8-sig"))))
    # 双表头: rows[0] 中文列名, rows[1] 英文列名, 数据自 rows[2]
    header_en = rows[1]
    def col(name):
        for index, cell in enumerate(header_en):
            if cell.strip().lower() == name.lower():
                return index
        return -1

    col_id = col("ID")
    col_name = col("Name")
    col_en = col("ENName")
    col_cat = find_cn(rows[0], "类别")
    col_collision = find_cn(rows[0], "移动碰撞")
    col_hard = find_cn(rows[0], "硬度")
    col_tool = find_cn(rows[0], "采集工具")
    col_toollevel = find_cn(rows[0], "工具等级")
    col_dig = find_cn(rows[0], "挖的声音")
    col_place = find_cn(rows[0], "放置的声音")
    col_walk = find_cn(rows[0], "走在上面声音")
    col_tex = find_cn(rows[0], "贴图1")

    # 客户端 id 分配: 现有 15 种保持, 新增按数据行顺序从 15 起
    mapping = {}
    next_id = 15
    for row in rows[2:]:
        if len(row) <= max(col for col in (col_id, col_name, col_hard, col_dig, col_tex) if col >= 0):
            continue
        try:
            source_id = int(row[col_id].strip())
        except (ValueError, IndexError):
            continue
        name = row[col_name].strip() if col_name >= 0 else ""
        if not name:
            continue
        client_id = REVERSE.get(source_id, next_id)
        if source_id not in REVERSE:
            next_id += 1
        mapping[source_id] = {
            "clientId": client_id,
            "name": sanitize(name),
            "key": safe_const_key(row[col_en].strip() if col_en >= 0 else "", client_id).lower(),
            "category": sanitize(row[col_cat].strip() if col_cat >= 0 and len(row) > col_cat else "basic"),
            "collision": int(row[col_collision].strip()) if col_collision >= 0 and row[col_collision].strip().isdigit() else 1,
            "hardness": safe_float(row[col_hard].strip() if col_hard >= 0 and len(row) > col_hard else "1"),
            "tool": int(row[col_tool].strip()) if col_tool >= 0 and row[col_tool].strip().isdigit() else 0,
            "toolLevel": int(row[col_toollevel].strip()) if col_toollevel >= 0 and row[col_toollevel].strip().isdigit() else 0,
            "digSound": sanitize(row[col_dig].strip() if col_dig >= 0 and len(row) > col_dig else ""),
            "placeSound": sanitize(row[col_place].strip() if col_place >= 0 and len(row) > col_place else ""),
            "walkSound": sanitize(row[col_walk].strip() if col_walk >= 0 and len(row) > col_walk else ""),
            "texture": sanitize(row[col_tex].strip() if col_tex >= 0 and len(row) > col_tex else ""),
        }

    entries = sorted(mapping.values(), key=lambda entry: entry["clientId"])
    emit_header(entries, len(mapping))
    return 0


def find_cn(header, name):
    for index, cell in enumerate(header):
        if name in cell:
            return index
    return -1


def emit_header(entries, total):
    lines = [
        "#pragma once",
        "",
        "/*",
        " * " + NOTICE,
        " * 全量 %d 条: id 顺序分配客户端 id, 现有 15 种 (air..planks) 保持不变." % total,
        " * 硬度为原值 (-1 到 200), -1 表示不可破坏.",
        " * 音效名为blockd./blocks. 前缀字符串, 播放映射由音频层负责.",
        " * 贴图名保留名称称, 供图集扩充时映射纹理源.",
        " */",
        "",
        "#include <cstdint>",
        "",
        "namespace immirror::voxel::gen {",
        "",
        "struct BlockEntry {",
        "    uint32_t sourceId;",
        "    uint16_t clientId;",
        "    const char* key;",
        "    const char* name;",
        "    const char* category;",
        "    uint8_t collision;",
        "    float hardness;",
        "    uint8_t tool;",
        "    uint8_t toolLevel;",
        "    const char* digSound;",
        "    const char* placeSound;",
        "    const char* walkSound;",
        "    const char* texture;",
        "};",
        "",
        "constexpr int32_t kBlockEntryCount = %d;" % len(entries),
        "",
        "constexpr BlockEntry kBlockEntries[kBlockEntryCount] = {",
    ]
    for entry in entries:
        lines.append('    {%du, %du, "%s", "%s", "%s", %du, %.1ff, %du, %du, "%s", "%s", "%s", "%s"},' % (
            entry.get("sourceId", 0) if "sourceId" in entry else 0,
            entry["clientId"], entry["key"], entry["name"], entry["category"],
            entry["collision"], entry["hardness"], entry["tool"], entry["toolLevel"],
            entry["digSound"], entry["placeSound"], entry["walkSound"], entry["texture"]))
    lines.append("};")
    lines.append("")
    lines.append("/* 按客户端 id 取条目, 越界回落到首条. */")
    lines.append("constexpr const BlockEntry& BlockEntryOf(uint16_t clientId) {")
    lines.append("    return clientId < kBlockEntryCount ? kBlockEntries[clientId] : kBlockEntries[0];")
    lines.append("}")
    lines.append("")
    lines.append("}  /* namespace immirror::voxel::gen */")
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines) + "\n")
    print("生成: %s (%d 条目)" % (OUT_PATH, len(entries)))


if __name__ == "__main__":
    import sys
    sys.exit(main())