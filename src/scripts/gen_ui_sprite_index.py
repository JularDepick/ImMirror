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

ATLAS_HEAD = re.compile(r'<TextureAtlas\s+imagePath\s*=\s*"([^"]+)"\s+width\s*=\s*"(\d+)"'
                        r'\s+height\s*=\s*"(\d+)"')
SPRITE = re.compile(r'<sprite\s+n\s*=\s*"([^"]+)"\s+x\s*=\s*"(\d+)"\s+y\s*=\s*"(\d+)"'
                    r'\s+w\s*=\s*"(\d+)"\s+h\s*=\s*"(\d+)"')
COMMENT = re.compile(r'<!--.*?-->', re.S)


def collect():
    """精灵名 -> (描述文件相对路径, 归一化矩形)."""
    table = {}
    for base, _dirs, files in os.walk(UI_ROOT):
        for name in sorted(files):
            if not name.endswith(".xml"):
                continue
            path = os.path.join(base, name)
            with open(path, "r", encoding="utf-8", errors="replace") as handle:
                text = COMMENT.sub("", handle.read())
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
                # 像素尺寸一并带出: 布局里不给 Size 的元素按精灵原始尺寸补齐.
                table.setdefault(key, (relative, x / width, y / height,
                                          (x + w) / width, (y + h) / height, w, h))
    return table


KTEX_IDENTIFIER = bytes([0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A])


def collect_pages():
    """贴图页尺寸: 直接引用的整图(非子图集精灵)按页面像素尺寸绘制."""
    pages = {}
    for base, _dirs, files in os.walk(UI_ROOT):
        for name in sorted(files):
            if not name.lower().endswith(".png"):
                continue
            path = os.path.join(base, name)
            try:
                with open(path, "rb") as handle:
                    head = handle.read(24)
            except OSError:
                continue
            if len(head) < 24 or head[:8] != b"\x89PNG\r\n\x1a\n":
                continue
            width = int.from_bytes(head[16:20], "big")
            height = int.from_bytes(head[20:24], "big")
            if width <= 0 or height <= 0:
                continue
            relative = os.path.relpath(path, PROJECT).replace(os.sep, "/")
            pages[relative] = (width, height)
    # 界面图集多数是 ktx(ETC1 容器), 头部同样带像素宽高.
    for base, _dirs, files in os.walk(UI_ROOT):
        for name in sorted(files):
            if not name.lower().endswith(".ktx"):
                continue
            path = os.path.join(base, name)
            try:
                with open(path, "rb") as handle:
                    head = handle.read(64)
            except OSError:
                continue
            if len(head) < 64 or head[:12] != KTEX_IDENTIFIER:
                continue
            fields = head[12:]
            if int.from_bytes(fields[0:4], "little") != 0x04030201:
                continue
            width = int.from_bytes(fields[24:28], "little")
            height = int.from_bytes(fields[28:32], "little")
            if width <= 0 or height <= 0:
                continue
            relative = os.path.relpath(path, PROJECT).replace(os.sep, "/")
            pages[relative] = (width, height)
    return pages


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
        "    /* 精灵在整页里的像素宽高. */",
        "    float w;",
        "    float h;",
        "};",
        "",
        "constexpr int32_t kUiSpriteRectCount = %d;" % len(names),
        "",
        "constexpr UiSpriteRect kUiSpriteRects[kUiSpriteRectCount] = {",
    ]
    for key in names:
        atlas, u0, v0, u1, v1, w, h = table[key]
        lines.append('    {"%s", "%s", %.6ff, %.6ff, %.6ff, %.6ff, %.1ff, %.1ff},'
                     % (escape(key), escape(atlas), u0, v0, u1, v1, w, h))
    lines += [
        "};",
        "",
        "}  /* namespace immirror::ui::gen */",
        "",
    ]
    pages = collect_pages()
    if pages:
        # 页面尺寸表必须写在命名空间内部, 否则消费方拿不到 gen:: 前缀.
        block = [
            "",
            "/* 整图页尺寸: 直接引用的贴图按这个尺寸绘制. */",
            "struct UiPageSize {",
            "    const char* path;",
            "    float width;",
            "    float height;",
            "};",
            "",
            "constexpr int32_t kUiPageSizeCount = %d;" % len(pages),
            "",
            "constexpr UiPageSize kUiPageSizes[kUiPageSizeCount] = {",
        ]
        for key in sorted(pages):
            width, height = pages[key]
            block.append('    {"%s", %.1ff, %.1ff},' % (escape(key), float(width), float(height)))
        block.append("};")
        block.append("")
        closeIndex = len(lines)
        for index, line in enumerate(lines):
            if line.startswith("}  /* namespace"):
                closeIndex = index
                break
        lines[closeIndex:closeIndex] = block

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines))
    print("界面精灵索引: %d 条" % len(names))
    return 0


if __name__ == "__main__":
    sys.exit(main())
