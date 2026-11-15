#!/usr/bin/env python3
"""字体索引: 从 fonts.xml 生成 名字 -> 字号 表."""

import os
import re
import sys

# 脚本位于 src/scripts, 上一层是 src.
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE = os.path.join(ROOT, "..", "..", "assets", "project", "data", "xml", "ui", "mobile",
                        "fonts.xml")
OUT = os.path.join(ROOT, "native", "ui", "generated", "ui_font_index_gen.h")

# 属性里可能含路径斜杠, 不能把 / 排除在字符类外.
FONT_RE = re.compile(r"<Font\s+([^>]*?)/?>")
NAME_RE = re.compile(r"name=\"([^\"]+)\"")
HEIGHT_RE = re.compile(r"height=\"(\d+)\"")


def main():
    try:
        body = open(SOURCE, "r", encoding="utf-8", errors="replace").read()
    except OSError:
        print("字体文件读取失败: %s" % SOURCE)
        return 1
    entries = []
    for hit in FONT_RE.finditer(body):
        attrs = hit.group(1)
        name = NAME_RE.search(attrs)
        height = HEIGHT_RE.search(attrs)
        if name is None or height is None:
            continue
        entries.append((name.group(1), int(height.group(1))))
    entries.sort()
    lines = []
    lines.append("/* 由 gen_font_index.py 生成, 不要手改. */")
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <cstdint>")
    lines.append("")
    lines.append("namespace immirror::ui::gen {")
    lines.append("")
    lines.append("struct UiFontEntry {")
    lines.append("    const char* name;")
    lines.append("    int32_t height;")
    lines.append("};")
    lines.append("")
    lines.append("constexpr int32_t kUiFontCount = %d;" % len(entries))
    lines.append("")
    lines.append("constexpr UiFontEntry kUiFonts[kUiFontCount] = {")
    for name, height in entries:
        lines.append(chr(32) * 4 + chr(34) + name + chr(34) + ", " + str(height) + ",")
    lines.append("};")
    lines.append("")
    lines.append("}  /* namespace immirror::ui::gen */")
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline=chr(10)) as handle:
        handle.write(chr(10).join(lines) + chr(10))
    print("字体索引: %d 条" % len(entries))
    return 0


if __name__ == "__main__":
    sys.exit(main())
