"""从物品图标图集生成每格平均色表."""

import json
import os
import sys

try:
    from PIL import Image
except ImportError:
    raise SystemExit("缺少 Pillow")

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
ASSETS = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets"))
ATLAS_PNG = os.path.join(SRC_ROOT, "android", "app", "src", "main", "assets", "generated", "atlas_item_icon.png")
GEN_HEADER = os.path.join(SRC_ROOT, "native", "ui", "generated", "item_icon_atlas_gen.h")
OUT_PATH = os.path.join(SRC_ROOT, "native", "ui", "generated", "item_icon_color_gen.h")

# 兜底色: 与方块色表的兜底一致, 取中灰.
FALLBACK = (132, 132, 136)


def read_grid_and_cell():
    text = open(GEN_HEADER, encoding="utf-8").read()
    grid = 64
    cell = 32
    for line in text.splitlines():
        if "kItemIconGrid" in line and "=" in line:
            grid = int(line.split("=")[1].strip().rstrip(";"))
        if "kItemIconCell" in line and "=" in line:
            cell = int(line.split("=")[1].strip().rstrip(";"))
    return grid, cell


def main():
    if not os.path.isfile(ATLAS_PNG):
        print("图标图集不存在, 先运行 build_item_icon_atlas.py")
        return 1
    grid, cell = read_grid_and_cell()
    image = Image.open(ATLAS_PNG).convert("RGBA")
    colors = []
    for slot in range(grid * grid):
        col, row = slot % grid, slot // grid
        box = (col * cell, row * cell, (col + 1) * cell, (row + 1) * cell)
        tile = image.crop(box)
        pixels = list(tile.getdata())
        total = [0, 0, 0]
        count = 0
        for r, g, b, a in pixels:
            if a < 8:
                continue
            total[0] += r
            total[1] += g
            total[2] += b
            count += 1
        if count == 0:
            colors.append(FALLBACK)
        else:
            colors.append((total[0] // count, total[1] // count, total[2] // count))
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/* 由 scripts/gen_item_icon_color.py 生成, 请勿手改. */",
        "/* 图标图集每格的平均色, 供掉落物一类只需要代表色的场合使用. */",
        "namespace immirror::ui::gen {",
        "",
        "struct ItemIconColor {",
        "    uint8_t red;",
        "    uint8_t green;",
        "    uint8_t blue;",
        "};",
        "",
        "constexpr int32_t kItemIconColorCount = %d;" % len(colors),
        "constexpr ItemIconColor kItemIconColors[kItemIconColorCount] = {",
    ]
    for index in range(0, len(colors), 6):
        chunk = colors[index:index + 6]
        parts = ", ".join("{%d, %d, %d}" % c for c in chunk)
        lines.append("    " + parts + ",")
    lines += [
        "};",
        "",
        "/* 按格序号取平均色, 越界返回中灰. */",
        "inline ItemIconColor ItemIconColorOf(int32_t slot) {",
        "    if (slot < 0 || slot >= kItemIconColorCount) {",
        "        return ItemIconColor{132, 132, 136};",
        "    }",
        "    return kItemIconColors[slot];",
        "}",
        "",
        "}  // namespace immirror::ui::gen",
        "",
    ]
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines))
    print("图标色表: %d 格" % len(colors))
    return 0


if __name__ == "__main__":
    sys.exit(main())