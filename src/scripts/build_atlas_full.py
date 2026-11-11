#!/usr/bin/env python3
"""从原版方块贴图与 blockdef 生成全量图集.

输入: assets/project/blocks/*.png 与 blockdef.csv 的贴图 1 列
输出: generated/ 下 atlas.rgba + atlas.json + atlas_index_gen.h

图集规格: 4096x4096, 64x64 格, 每格 64 像素.
格 0 到 15 保持与旧版兼容段一致 (存档兼容).
"""

import csv
import os
import sys

try:
    from PIL import Image
except ImportError:
    print("缺少 Pillow", file=sys.stderr)
    sys.exit(1)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
ROOT = os.path.normpath(os.path.join(SRC_ROOT, "..", ".."))
BLOCKDEF = os.path.join(ROOT, "assets", "project", "csvdef", "blockdef.csv")
TEX_SOURCE = os.path.join(ROOT, "assets", "project", "blocks")
OUT_DIR = os.path.join(SRC_ROOT, "android", "app", "src", "main", "assets", "generated")
GEN_HEADER = os.path.join(SRC_ROOT, "native", "voxel", "generated", "atlas_index_gen.h")

GRID = 64
CELL = 64
SIZE = GRID * CELL

LEGACY_SLOTS = {
    "grass_top": 0, "grass_side": 1, "dirt": 2, "stone": 3, "sand": 4,
    "water_flow": 5, "log_fruittree": 6, "log_fruittree_top": 7,
    "leaves_fruittree": 8, "bedrock": 9, "brick": 10, "cobblestone": 11,
    "snow": 12, "ice": 13, "gravel": 14, "planks_fruittree": 15,
}

FALLBACK = {
    "grass": "grass_top", "grass_side": "grass_side",
    "farmland": "dirt", "buryland": "dirt",
    "cosmicgrass": "grass_top", "cosmicburyland": "dirt",
    "reeds": "planks_fruittree", "wheat": "planks_fruittree",
    "portalframe": "bedrock", "portalframe_air": "bedrock",
    "lava_still": "stone", "lava_flow": "stone",
    "honey_still": "sand", "honey_flow": "sand",
}


def collect_names():
    with open(BLOCKDEF, "rb") as handle:
        rows = list(csv.reader(handle.read().decode("utf-8-sig").splitlines()))
    tex_index = -1
    for index, cell in enumerate(rows[0]):
        if "贴图1" in cell:
            tex_index = index
            break
    if tex_index < 0:
        print("blockdef 缺贴图 1 列", file=sys.stderr)
        sys.exit(1)
    names = set()
    for row in rows[2:]:
        if len(row) > tex_index and row[tex_index].strip():
            names.add(row[tex_index].strip())
    return sorted(names)


def main():
    names = collect_names()
    pngs = {name[:-4] for name in os.listdir(TEX_SOURCE) if name.endswith(".png")}

    slot_of = {}
    for name, slot in LEGACY_SLOTS.items():
        slot_of[name] = slot
    next_slot = 16
    missing_source = []
    for name in names:
        if name in slot_of:
            continue
        if name in FALLBACK:
            slot_of[name] = slot_of[FALLBACK[name]]
            continue
        if name in pngs:
            if next_slot >= GRID * GRID:
                print("图集格位不足", file=sys.stderr)
                break
            slot_of[name] = next_slot
            next_slot += 1
        else:
            missing_source.append(name)

    for name in missing_source:
        if name in FALLBACK:
            slot_of[name] = slot_of.get(FALLBACK[name], 0)
        else:
            slot_of[name] = 3

    atlas = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    drawn = 0
    for name, slot in sorted(slot_of.items(), key=lambda kv: kv[1]):
        source = os.path.join(TEX_SOURCE, name + ".png")
        if not os.path.exists(source):
            continue
        col, row = slot % GRID, slot // GRID
        image = Image.open(source).convert("RGBA")
        if image.height > image.width * 2:
            image = image.crop((0, 0, image.width, image.width))
        image = image.resize((CELL, CELL), Image.LANCZOS)
        atlas.paste(image, (col * CELL, row * CELL))
        drawn += 1

    os.makedirs(OUT_DIR, exist_ok=True)
    atlas.save(os.path.join(OUT_DIR, "atlas.png"))
    with open(os.path.join(OUT_DIR, "atlas.rgba"), "wb") as handle:
        handle.write(atlas.tobytes())

    import json
    slots_json = {}
    for name, slot in sorted(slot_of.items()):
        slots_json[name] = {"index": slot, "col": slot % GRID, "row": slot // GRID}
    meta = {"grid": GRID, "cell": CELL, "size": SIZE,
            "slots": slots_json, "missingSource": sorted(missing_source)}
    with open(os.path.join(OUT_DIR, "atlas.json"), "w", encoding="utf-8") as handle:
        json.dump(meta, handle, ensure_ascii=False, indent=1)

    gen_lines = []
    gen_lines.append("#pragma once")
    gen_lines.append("")
    gen_lines.append("/* 由 scripts/build_atlas_full.py 生成, 请勿手改. */")
    gen_lines.append("/* 贴图名 -> 图集格序号. 名单与 atlas.json 一致. */")
    gen_lines.append("")
    gen_lines.append("#include <cstdint>")
    gen_lines.append("")
    gen_lines.append("namespace immirror::voxel::gen {")
    gen_lines.append("")
    gen_lines.append("constexpr int32_t kAtlasGrid = %d;" % GRID)
    gen_lines.append("constexpr int32_t kAtlasCell = %d;" % CELL)
    gen_lines.append("constexpr int32_t kAtlasSize = %d;" % SIZE)
    gen_lines.append("")
    gen_lines.append("struct AtlasSlot {")
    gen_lines.append("    const char* name;")
    gen_lines.append("    int32_t slot;")
    gen_lines.append("};")
    gen_lines.append("")
    gen_lines.append("constexpr AtlasSlot kAtlasSlots[] = {")
    for name, slot in sorted(slot_of.items(), key=lambda kv: kv[1]):
        gen_lines.append('    {"%s", %d},' % (name, slot))
    gen_lines.append("};")
    gen_lines.append("")
    gen_lines.append("constexpr int32_t kAtlasSlotCount = %d;" % len(slot_of))
    gen_lines.append("")
    gen_lines.append("}  /* namespace immirror::voxel::gen */")

    with open(GEN_HEADER, "w", encoding="utf-8") as handle:
        for line in gen_lines:
            handle.write(line)
            handle.write(chr(10))

    print("图集生成: %d 贴图名, %d 格位, %d 张源图绘制" % (len(names), len(slot_of), drawn))
    print("缺源回落: %d 个" % len(missing_source))
    print("图集: %dx%d, 格 %dx%d" % (SIZE, SIZE, CELL, CELL))
    return 0


if __name__ == "__main__":
    sys.exit(main())