#!/usr/bin/env python3
"""从方块贴图与 blockdef 生成全量图集.

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


# 贴图名与源文件名语义不同但确实是同一件东西的显式对照.
# 只收能一眼确认的: 盒子取第一个状态, 茎取连接段, 蜂巢取满巢, 蘑菇取红菇,
# 红外感应器取侧面. 其余缺源名宁可留占位格, 也不做猜测性匹配.
SOURCE_ALIASES = {
    "chrismas_box": "chrismas_box0",
    "melon_stem": "melon_stem_connected",
    "pumpkin_stem": "pumpkin_stem_connected",
    "hive": "hive_full",
    "absorb_claws": "absorb_claws_off",
    "thicket": "thicket1",
    "mushroom": "mushroom_red",
    "Infrared_induction": "infrared_side",
}


# 贴图文件名到实际源文件的回落后缀.
# 多方块贴图在源目录里按状态或部件拆分存放: 铁砧是 anvil_s0/s1/s2,
# 门是 door_birch_lower/upper, 花是 agave_middle, 台阶类取 _s0 即可.
SUFFIX_CANDIDATES = ("", "_s0", "_lower", "_middle", "_bottom", "_top")


def resolve_source(name, pngs, lower):
    """把贴图名解析成源文件名; 找不到返回 None.

    先按原样匹配, 再按部件后缀匹配, 最后统一转小写重试一次,
    源目录里大小写并不统一 (Air_wall 与 air_wall 都存在).
    """
    alias = SOURCE_ALIASES.get(name)
    if alias is not None:
        if alias in pngs:
            return alias
        if alias.lower() in lower:
            return lower[alias.lower()]
    for suffix in SUFFIX_CANDIDATES:
        candidate = name + suffix
        if candidate in pngs:
            return candidate
    for suffix in SUFFIX_CANDIDATES:
        candidate = (name + suffix).lower()
        if candidate in lower:
            return lower[candidate]
    return None


def paint_missing_tile(atlas, slot):
    """缺图占位格: 64 像素一格的品红与深灰棋盘, 与任何贴图都不混淆."""
    col, row = slot % GRID, slot // GRID
    tile = Image.new("RGBA", (CELL, CELL), (255, 0, 255, 255))
    step = CELL // 4
    pixels = tile.load()
    for y in range(CELL):
        for x in range(CELL):
            if ((x // step) + (y // step)) % 2 == 1:
                pixels[x, y] = (32, 32, 32, 255)
    atlas.paste(tile, (col * CELL, row * CELL))


def main():
    names = collect_names()
    pngs = {name[:-4] for name in os.listdir(TEX_SOURCE) if name.endswith(".png")}
    lower = {stem.lower(): stem for stem in pngs}
    resolved = {name: resolve_source(name, pngs, lower) for name in names}

    """
 格位分配分三类: 自绘名, 别名, 缺图名.
 自绘名独占一个格位并写入自己的贴图; 别名只借用源贴图的格位, 自身不绘制,
 否则别名贴图会覆盖掉源贴图所在的格位 (此前 lavastill 覆盖 stone 即是此因);
 缺图名统一落到一个专用占位格, 实机上一眼可辨.
    """
    slot_of = {}
    drawn_slot_of = {}
    for name, slot in LEGACY_SLOTS.items():
        slot_of[name] = slot
        drawn_slot_of[slot] = name
    next_slot = 16
    missing_source = []
    for name in names:
        if name in slot_of:
            continue
        if name in FALLBACK:
            source_name = FALLBACK[name]
            if source_name == name or source_name not in slot_of:
                missing_source.append(name)
                continue
            slot_of[name] = slot_of[source_name]
            continue
        source_stem = resolved.get(name)
        if source_stem is not None:
            if next_slot >= GRID * GRID - 1:
                print("图集格位不足", file=sys.stderr)
                break
            slot_of[name] = next_slot
            drawn_slot_of[next_slot] = source_stem
            next_slot += 1
        else:
            missing_source.append(name)

    missing_slot = next_slot
    for name in missing_source:
        slot_of[name] = missing_slot

    atlas = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    drawn = 0
    for slot, name in sorted(drawn_slot_of.items()):
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

    paint_missing_tile(atlas, missing_slot)

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
    gen_lines.append("namespace immirror::voxel::atlas {")
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
    gen_lines.append("}  /* namespace immirror::voxel::atlas */")

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