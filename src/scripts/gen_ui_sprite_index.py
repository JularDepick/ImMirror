#!/usr/bin/env python3
"""从子图集描述文件生成界面精灵索引.

输入: single-branch/assets/project/ui 下所有带图集头的描述文件.
输出: single-branch/client/src/native/ui/generated/ui_sprite_index_gen.h

索引按精灵名排序, 运行期用二分查找. 每条记下所属描述文件, 整图尺寸与归一化矩形,
绘制时按矩形取图集格位.
"""

import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(SCRIPT_DIR, ".."))
PROJECT = os.path.normpath(os.path.join(ROOT, "..", "..", "assets", "project"))
UI_ROOT = os.path.join(PROJECT, "ui")
OUT = os.path.join(ROOT, "native", "ui", "generated", "ui_sprite_index_gen.h")

ATLAS_HEAD = re.compile(r'<TextureAtlas\s+imagePath="([^"]+)"\s+width="(\d+)"\s+height="(\d+)"')
SPRITE = re.compile(r'<sprite\s+n="([^"]+)"\s+x="(\d+)"\s+y="(\d+)"\s+w="(\d+)"\s+h="(\d+)"')


def collect():
    """精灵名 -> (描述文件相对路径, 归一化矩形)."""
    table = {}
    for base, _dirs, files in os.walk(UI_ROOT):
        for name in sorted(files):
            if not name.endswith(".xml"):
                continue
            path = os.path.join(base, name)
            with open(path, "r", encoding="utf-8", errors="replace") as handle:
                text = handle.read()
            head = ATLAS_HEAD.search(text)
            if head is None:
                continue
            width = float(head.group(2))
            height = float(head.group(3))
            if width <= 0.0 or height <= 0.0:
                continue
            relative = os.path.relpath(path, PROJECT).replace(os.sep, "/")
            for match in SPRITE.finditer(text):
                key = match.group(1)
                x = float(match.group(2))
                y = float(match.group(3))
                w = float(match.group(4))
                h = float(match.group(5))
                table.setdefault(key, (relative, x / width, y / height,
                                          (x + w) / width, (y + h) / height))
    return table


def escape(text):
    return text.replace("\\", "\\\\").replace('"', '\\"')


def main():
    table = collect()
    if not table:
        print("没有找到子图集描述文件")
        return 1
    names = sorted(table)
    lines = [
        "/* 由 scripts/gen_ui_sprite_index.py 生成, 不要手工修改. */",
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "namespace immirror::ui::gen {",
        "",
        "/* 一条精灵索引: 名字, 所属描述文件, 归一化矩形. */",
        "struct UiSpriteRect {",
        "    const char* name;",
        "    const char* atlas;",
        "    float u0;",
        "    float v0;",
        "    float u1;",
        "    float v1;",
        "};",
        "",
        "constexpr int32_t kUiSpriteRectCount = %d;" % len(names),
        "",
        "constexpr UiSpriteRect kUiSpriteRects[kUiSpriteRectCount] = {",
    ]
    for key in names:
        atlas, u0, v0, u1, v1 = table[key]
        lines.append('    {"%s", "%s", %.6ff, %.6ff, %.6ff, %.6ff},'
                     % (escape(key), escape(atlas), u0, v0, u1, v1))
    lines += [
        "};",
        "",
        "}  /* namespace immirror::ui::gen */",
        "",
    ]
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines))
    print("界面精灵索引: %d 条" % len(names))
    return 0


if __name__ == "__main__":
    sys.exit(main())
