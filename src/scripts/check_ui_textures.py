#!/usr/bin/env python3
"""核对界面贴图链路: 布局 -> 模板 -> 子图集 -> 精灵矩形.

链路三段:
  1. 布局元素用 inherits 指向模板;
  2. 模板用 file 指向子图集描述, 用 uvname 指向描述里的精灵名;
  3. 子图集描述给出精灵在整张图里的矩形与整图尺寸.
三段任一段断开, 该元素就取不到贴图, 只能退化为纯色块.
"""

import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "..", "..", "assets", "project"))
UI_ROOT = os.path.join(PROJECT, "ui")
LAYOUT_ROOT = os.path.join(PROJECT, "data", "xml", "ui", "mobile")
TEMPLATE_PATH = os.path.join(LAYOUT_ROOT, "uiconfig", "uitemplate.xml")

SPRITE = re.compile(r'<sprite\s+n="([^"]+)"\s+x="(\d+)"\s+y="(\d+)"\s+w="(\d+)"\s+h="(\d+)"')
ATLAS_HEAD = re.compile(r'<TextureAtlas\s+imagePath="([^"]+)"\s+width="(\d+)"\s+height="(\d+)"')


def read(path):
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        return handle.read()


def load_atlases():
    """收集所有子图集描述: 精灵名 -> (描述文件, 整图宽高, 矩形)."""
    table = {}
    for root, _dirs, files in os.walk(UI_ROOT):
        for name in files:
            if not name.endswith(".xml"):
                continue
            path = os.path.join(root, name)
            text = read(path)
            head = ATLAS_HEAD.search(text)
            if head is None:
                continue
            width = int(head.group(2))
            height = int(head.group(3))
            for match in SPRITE.finditer(text):
                table.setdefault(match.group(1), (path, width, height,
                                                   int(match.group(2)), int(match.group(3)),
                                                   int(match.group(4)), int(match.group(5))))
    return table


def load_templates():
    """模板名 -> (子图集描述路径, 精灵名). 只收带 uvname 的条目."""
    if not os.path.isfile(TEMPLATE_PATH):
        return {}
    text = read(TEMPLATE_PATH)
    table = {}
    for match in re.finditer(r'<(?:Texture|Frame|Button|FontString)\s+([^>]*)>', text):
        body = match.group(1)
        name = re.search(r'name="([^"]+)"', body)
        if name is None:
            continue
        file_attr = re.search(r'file="([^"]+)"', body)
        uv = re.search(r'uvname="([^"]+)"', body)
        if uv is None:
            continue
        table[name.group(1)] = (file_attr.group(1) if file_attr else "", uv.group(1))
    return table


def main():
    atlases = load_atlases()
    templates = load_templates()
    if not atlases:
        print("没有找到子图集描述文件")
        return 2

    used = {}
    for root, _dirs, files in os.walk(LAYOUT_ROOT):
        for name in files:
            if not name.endswith(".xml") or "uitemplate" in name:
                continue
            text = read(os.path.join(root, name))
            for match in re.finditer(r'inherits="([^"]+)"', text):
                key = match.group(1)
                used[key] = used.get(key, 0) + 1

    missingTemplate = sorted(name for name in used if name not in templates)
    resolved = 0
    unresolved = []
    missingAtlas = []
    for name in used:
        entry = templates.get(name)
        if entry is None:
            continue
        # 模板里的精灵名带扩展名, 图集里不带, 比对时统一去掉.
        sprite = entry[1][:-4] if entry[1].endswith(".png") else entry[1]
        if sprite in atlases:
            resolved += 1
            continue
        # 模板指向的子图集描述本身不存在时, 属于素材缺口, 不是本仓库能补的.
        atlasPath = os.path.join(PROJECT, entry[0]) if entry[0] else ""
        if atlasPath and not os.path.isfile(atlasPath):
            missingAtlas.append((name, entry[0]))
            continue
        unresolved.append((name, entry[1]))

    print("子图集精灵 %d 个, 模板 %d 个, 布局引用的模板 %d 个"
          % (len(atlases), len(templates), len(used)))
    print("模板有定义 %d 个, 无定义 %d 个 (素材侧缺定义, 非本仓库可补): %s"
          % (len(used) - len(missingTemplate), len(missingTemplate),
             ", ".join(missingTemplate[:5]) or "无"))
    print("已定义模板里精灵能查到 %d 个, 子图集缺失 %d 个, 精灵查不到 %d 个: %s"
          % (resolved, len(missingAtlas), len(unresolved),
             ", ".join(name for name, _ in unresolved[:5]) or "无"))
    # 只有"模板有定义但精灵查不到"属于可修问题; 模板本身没定义的属于素材缺口,
    # 只报告不判定失败.
    return 0 if not unresolved else 1


if __name__ == "__main__":
    sys.exit(main())
