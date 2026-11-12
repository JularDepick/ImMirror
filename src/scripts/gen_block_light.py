#!/usr/bin/env python3
"""从原版方块定义表生成方块光照头文件.

输入: single-branch/assets/project/csvdef/blockdef.csv 的光源强度, 光衰减与遮挡三列.
输出: single-branch/client/src/native/voxel/generated/block_light_gen.h

客户端 id 映射与 gen_block_data.py 完全同源: 兼容段按 CLIENT_BLOCKS 固定,
扩展段按 KEY_SEGMENTS 分段与原版 ID 升序分配, 因此两表条目天然逐位对齐,
不依赖任何中间产物 (此前 temp/gen_light.py 依赖过期 block_mapping.json
导致扩展段 970 条整体错位的教训, 见生成头注释).

生成物列入 .gitignore, 由 scripts/build-native.sh 在 gen_block_data.py 之后调用.
本脚本可重复运行, 产物字节级确定.
"""

import csv
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
BLOCKDEF_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project",
                                              "csvdef", "blockdef.csv"))
OUT_PATH = os.path.join(SRC_ROOT, "native", "voxel", "generated", "block_light_gen.h")

sys.path.insert(0, SCRIPT_DIR)
import gen_block_data as base

NOTICE = "由 scripts/gen_block_light.py 从 assets 仓库的 blockdef.csv 生成, 请勿手改"


def emit(message):
    """打印进度, 控制台编码受限时降级, 不让日志中断生成."""
    try:
        print(message)
    except UnicodeEncodeError:
        print(message.encode("ascii", "replace").decode("ascii"))


def to_int(text):
    """空串与非数字按 0 处理, 光照三列存在空缺."""
    try:
        return int(str(text).strip())
    except (TypeError, ValueError):
        return 0


def base_row_light(source_id, rows):
    """取某原版方块的光照三列, 缺行按全零处理 (与空气一致)."""
    row = rows.get(str(source_id))
    if row is None:
        return {"light": 0, "atten": 0, "cover": 0}
    return {
        "light": to_int(row.get("LightSrc")),
        "atten": to_int(row.get("LightAtten")),
        "cover": to_int(row.get("CoverNeighbor")),
    }


def main():
    _header, rows = base.load_rows()
    legacy_rows, extension_rows = base.collect_source_rows(rows)
    allocated, _segments = base.allocate_ids(extension_rows)

    entries = []
    for client_id, _name, source_id, _row in legacy_rows:
        entries.append((client_id, base_row_light(source_id, rows)))
    for client_id, source_id, _row in allocated:
        entries.append((client_id, base_row_light(source_id, rows)))

    entries.sort(key=lambda pair: pair[0])
    for index, (client_id, _data) in enumerate(entries):
        if client_id != index:
            raise SystemExit("客户端 id 不连续: 下标 %d 处出现 id %d" % (index, client_id))

    emit_header(entries)
    return 0


def emit_header(entries):
    """写出 C++ 头文件, 条目顺序与注册表 clientId 严格一致."""
    lines = [
        "#pragma once",
        "",
        "/*",
        " * " + NOTICE,
        " * 与 block_data_gen.h 同源映射: 兼容段按 CLIENT_BLOCKS, 扩展段按 KEY_SEGMENTS",
        " * 分段加原版 ID 升序, 条目顺序与注册表 clientId 逐位一致.",
        " * lightSource: 0 不发光, 15 最亮; lightAtten: 0-15; coverMode: 0-4",
        " */",
        "",
        "#include <cstdint>",
        "",
        "namespace immirror::voxel::light {",
        "",
        "struct LightEntry {",
        "    uint8_t lightSource;",
        "    uint8_t lightAtten;",
        "    uint8_t coverMode;",
        "};",
        "",
        "constexpr int32_t kLightEntryCount = %d;" % len(entries),
        "",
        "constexpr LightEntry kLightEntries[kLightEntryCount] = {",
    ]
    for _client_id, data in entries:
        lines.append("    {%du, %du, %du}," % (data["light"], data["atten"], data["cover"]))
    lines.append("};")
    lines.append("")
    lines.append("/* 按客户端 id 取条目, 越界回落到首条. */")
    lines.append("constexpr const LightEntry& LightEntryOf(uint16_t clientId) {")
    lines.append("    return clientId < kLightEntryCount ? kLightEntries[clientId] : kLightEntries[0];")
    lines.append("}")
    lines.append("")
    lines.append("}  /* namespace immirror::voxel::light */")
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines) + "\n")
    nonzero = sum(1 for _i, d in entries if d["light"] > 0)
    print("生成: %s (%d 条目, 光源 %d 个)" % (OUT_PATH, len(entries), nonzero))


if __name__ == "__main__":
    sys.exit(main())
