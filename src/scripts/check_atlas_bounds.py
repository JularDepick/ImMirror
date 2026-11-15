#!/usr/bin/env python3
"""图集精灵越界检查: 每个精灵的矩形必须落在页面范围内."""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LAYOUT = os.path.join(ROOT, "..", "..", "assets", "project", "data", "xml", "ui", "mobile")
OUT = os.path.join(ROOT, "..", "..", "..", ".agent", "ui-baseline", "atlas-bounds.md")

ATLAS_RE = re.compile(r"<TextureAtlas\s+([^>]*)>")
SPRITE_RE = re.compile(r"<sprite\s+([^/>]*)/?>")
ATTR_RE = re.compile(r"(\w+)=\"([^\"]*)\"")


def parse_attrs(text):
    return {hit.group(1): hit.group(2) for hit in ATTR_RE.finditer(text)}


def main():
    pages = 0
    sprites = 0
    out_of_bounds = []
    for base, _dirs, files in os.walk(LAYOUT):
        for name in sorted(files):
            if not name.lower().endswith(".xml"):
                continue
            path = os.path.join(base, name)
            if "texture" not in path.replace("\\", "/"):
                continue
            try:
                body = open(path, "r", encoding="utf-8", errors="replace").read()
            except OSError:
                continue
            atlas = ATLAS_RE.search(body)
            if atlas is None:
                continue
            page = parse_attrs(atlas.group(1))
            try:
                width = int(page.get("width", "0"))
                height = int(page.get("height", "0"))
            except ValueError:
                continue
            if width <= 0 or height <= 0:
                continue
            pages += 1
            for hit in SPRITE_RE.finditer(body):
                attrs = parse_attrs(hit.group(1))
                try:
                    x = int(attrs.get("x", "0"))
                    y = int(attrs.get("y", "0"))
                    w = int(attrs.get("w", "0"))
                    h = int(attrs.get("h", "0"))
                except ValueError:
                    continue
                sprites += 1
                if x + w > width or y + h > height:
                    out_of_bounds.append((os.path.relpath(path, LAYOUT),
                                          attrs.get("n", "?"), x, y, w, h, width, height))

    lines = [
        "# 图集精灵越界检查",
        "",
        "口径: 每个精灵的矩形(x, y, w, h)必须落在所属页面尺寸内, 越界会导致取样到相邻精灵或空白。",
        "",
        "| 项 | 值 |",
        "|:---:|:---:|",
        "| 页面数 | %d |" % pages,
        "| 精灵数 | %d |" % sprites,
        "| 越界精灵 | %d |" % len(out_of_bounds),
        "",
    ]
    if out_of_bounds:
        lines.append("| 图集 | 精灵 | 矩形 | 页面 |")
        lines.append("|:---:|:---:|:---|:---:|")
        for item in out_of_bounds[:20]:
            lines.append("| %s | %s | %d,%d %dx%d | %dx%d |" % item)
        lines.append("")
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline=chr(10)) as handle:
        handle.write(chr(10).join(lines))
    print("图集越界检查: 页面 %d, 精灵 %d, 越界 %d" % (pages, sprites, len(out_of_bounds)))
    return 1 if out_of_bounds else 0


if __name__ == "__main__":
    sys.exit(main())
