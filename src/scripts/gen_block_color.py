#!/usr/bin/env python3
"""从方块图集生成每格的平均色表.

输入: android/app/src/main/assets/generated/atlas.png 与 atlas.json
输出: native/voxel/generated/block_color_gen.h

用途: 方块破坏的碎屑与后续的粒子按方块 id 取一个代表色, 不必在运行时读图集像素.
平均色取该格全部不透明像素的算术平均, 全透明的格按灰色兜底.
"""

import json
import os
import sys

try:
    from PIL import Image
except ImportError:
    raise SystemExit("缺少 Pillow")

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
ATLAS_DIR = os.path.join(SRC_ROOT, "android", "app", "src", "main", "assets", "generated")
ATLAS_PNG = os.path.join(ATLAS_DIR, "atlas.png")
ATLAS_JSON = os.path.join(ATLAS_DIR, "atlas.json")
OUT_PATH = os.path.join(SRC_ROOT, "native", "voxel", "generated", "block_color_gen.h")

# 兜底色: 图集格全透明时使用, 取中灰, 不与任何方块主色冲突.
FALLBACK = (132, 132, 136)


def load_slots():
    with open(ATLAS_JSON, "r", encoding="utf-8") as handle:
        meta = json.load(handle)
    return meta


def average_color(image, cell, slot):
    col = slot % meta_grid
    row = slot // meta_grid
    box = (col * cell, row * cell, (col + 1) * cell, (row + 1) * cell)
    crop = image.crop(box).convert("RGBA")
    total_r = total_g = total_b = 0
    count = 0
    for r, g, b, a in crop.getdata():
        if a < 32:
            continue
        total_r += r
        total_g += g
        total_b += b
        count += 1
    if count == 0:
        return FALLBACK
    return (total_r // count, total_g // count, total_b // count)


def main():
    global meta_grid
    if not os.path.exists(ATLAS_PNG) or not os.path.exists(ATLAS_JSON):
        raise SystemExit("找不到图集产物: %s" % ATLAS_DIR)
    meta = load_slots()
    meta_grid = int(meta["grid"])
    cell = int(meta["cell"])
    image = Image.open(ATLAS_PNG)

    # 按格序号建表, 同一格可能被多个贴图名共用, 只算一次
    slot_colors = {}
    for name, info in meta["slots"].items():
        slot = int(info["index"])
        if slot not in slot_colors:
            slot_colors[slot] = average_color(image, cell, slot)

    max_slot = max(slot_colors) if slot_colors else 0
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/*",
        " * 由 scripts/gen_block_color.py 从方块图集生成, 请勿手改.",
        " * 图集每格的平均色, 供碎屑与粒子按格位取代表色.",
        " * 索引是图集格序号, 越界时回落到中灰.",
        " */",
        "namespace immirror::voxel::gen {",
        "",
        "struct AtlasCellColor {",
        "    uint8_t red;",
        "    uint8_t green;",
        "    uint8_t blue;",
        "};",
        "",
        "constexpr int32_t kAtlasColorCount = %d;" % (max_slot + 1),
        "",
        "constexpr AtlasCellColor kAtlasColors[kAtlasColorCount] = {",
    ]
    for slot in range(max_slot + 1):
        r, g, b = slot_colors.get(slot, FALLBACK)
        lines.append("    {%du, %du, %du}," % (r, g, b))
    lines.extend([
        "};",
        "",
        "/* 按图集格取代表色, 越界回落到中灰. */",
        "inline AtlasCellColor AtlasColorOf(int32_t slot) {",
        "    if (slot < 0 || slot >= kAtlasColorCount) {",
        "        return AtlasCellColor{%du, %du, %du};" % FALLBACK,
        "    }",
        "    return kAtlasColors[slot];",
        "}",
        "",
        "}  /* namespace immirror::voxel::gen */",
        "",
    ])
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines))
    print("方块平均色表: %d 格" % (max_slot + 1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
