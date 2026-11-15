#!/usr/bin/env python3
"""把屏幕转储的 TSV 转成可读的绘制清单, 供与原版截图逐项对照."""

import os
import sys

HEADER = (
    "| 序 | 元素 | 矩形 (x, y, 宽x高) | 图集页 | 精灵 | 文案 | 对齐 | 颜色 |\n"
    "|:---:|:---|:---|:---|:---|:---|:---:|:---|\n"
)


def align_name(horizontal, vertical):
    """对齐代码转可读文字."""
    names_h = {0: "左", 1: "中", 2: "右"}
    names_v = {0: "上", 1: "中", 2: "下"}
    return "%s/%s" % (names_h.get(int(horizontal), "?"), names_v.get(int(vertical), "?"))


def color_text(r, g, b, a):
    """颜色转十六进制与透明度."""
    red = int(round(float(r) * 255))
    green = int(round(float(g) * 255))
    blue = int(round(float(b) * 255))
    return "#%02X%02X%02X a=%.2f" % (red, green, blue, float(a))


def convert(source, title, target):
    rows = []
    with open(source, "r", encoding="utf-8", errors="replace") as handle:
        lines = handle.read().splitlines()
    for line in lines:
        cells = line.split("\t")
        if len(cells) < 18:
            continue
        # 表头可能出现多次(多次转储追加), 一律跳过.
        if cells[0] == "idx":
            continue
        (index, layer, name, x, y, w, h, texture, sprite, resolved, text, jh, jv,
         line_interval, red, green, blue, alpha) = cells[:18]
        texture_short = os.path.basename(texture) if texture else "-"
        sprite_text = sprite if sprite else "-"
        if sprite and resolved != "1":
            sprite_text += " (未命中)"
        text_text = text if text else "-"
        rows.append(
            "| %s | %s | %s, %s, %sx%s | %s | %s | %s | %s | %s |"
            % (index, name, x, y, w, h, texture_short, sprite_text, text_text,
               align_name(jh, jv), color_text(red, green, blue, alpha))
        )
    with open(target, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("# %s 绘制清单\n\n" % title)
        handle.write("共 %d 条可见绘制命令。矩形为设计画布(1280x720)像素坐标。\n\n" % len(rows))
        handle.write(HEADER)
        handle.write("\n".join(rows))
        handle.write("\n")
    return len(rows)


def main(argv):
    if len(argv) < 3:
        print("用法: gen_drawlist.py <输入.tsv> <标题> [输出.md]")
        return 2
    source = argv[1]
    title = argv[2]
    target = argv[3] if len(argv) > 3 else os.path.splitext(source)[0] + ".md"
    count = convert(source, title, target)
    print("%s: %d 条 -> %s" % (title, count, target))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
