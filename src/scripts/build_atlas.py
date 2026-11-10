#!/usr/bin/env python3
"""把方块贴图合成为渲染用图集.

输入: src/assets/blocks/*.png (从原版资源复制的方块贴图)
输出: src/android/app/src/main/assets/generated/atlas.rgba 裸 RGBA8 数据与 atlas.json 格位表
图集规格: 1024x1024, 16x16 格, 每格 64 像素, 首行对应 v=0
"""

import json
import os
import sys

try:
    from PIL import Image
except ImportError:
    print("缺少 Pillow, 请先安装: python -m pip install Pillow", file=sys.stderr)
    sys.exit(1)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
BLOCKS_DIR = os.path.join(SRC_ROOT, "assets", "blocks")
OUT_DIR = os.path.join(SRC_ROOT, "android", "app", "src", "main", "assets", "generated")

GRID = 16
CELL = 64

# 格位顺序必须与 voxel/block_registry.cpp 的面索引一致
SLOT_ORDER = [
    ("grass_top", "grass_top.png"),
    ("grass_side", "grass_side.png"),
    ("dirt", "dirt.png"),
    ("stone", "stone.png"),
    ("sand", "sand.png"),
    ("water", "water_still.png"),
    ("log_side", "log_arbor.png"),
    ("log_top", "log_arbor_top.png"),
    ("leaves", "leaves.png"),
    ("bedrock", "bedrock.png"),
    ("brick", "brick.png"),
    ("cobblestone", "cobblestone.png"),
    ("snow", "snow.png"),
    ("ice", "ice.png"),
    ("gravel", "gravel.png"),
    ("planks", "planks_arbor.png"),
]


def main() -> int:
    if not os.path.isdir(BLOCKS_DIR):
        print("方块贴图目录不存在: " + BLOCKS_DIR, file=sys.stderr)
        return 1
    os.makedirs(OUT_DIR, exist_ok=True)

    atlas = Image.new("RGBA", (GRID * CELL, GRID * CELL), (0, 0, 0, 0))
    slots = {}
    missing = []
    for index, (key, filename) in enumerate(SLOT_ORDER):
        path = os.path.join(BLOCKS_DIR, filename)
        if not os.path.exists(path):
            missing.append(filename)
            continue
        image = Image.open(path).convert("RGBA")
        if image.height > image.width * 2:
            image = image.crop((0, 0, image.width, image.width))
        image = image.resize((CELL, CELL), Image.LANCZOS)
        col, row = index % GRID, index // GRID
        atlas.paste(image, (col * CELL, row * CELL))
        slots[key] = {"index": index, "col": col, "row": row, "source": filename}

    atlas.save(os.path.join(OUT_DIR, "atlas.png"))
    with open(os.path.join(OUT_DIR, "atlas.rgba"), "wb") as handle:
        handle.write(atlas.tobytes())
    with open(os.path.join(OUT_DIR, "atlas.json"), "w", encoding="utf-8") as handle:
        json.dump({"grid": GRID, "cell": CELL, "size": GRID * CELL, "slots": slots}, handle,
                  ensure_ascii=False, indent=1)

    print("图集已生成: %s" % os.path.join(OUT_DIR, "atlas.rgba"))
    if missing:
        print("缺图: " + ", ".join(missing), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
