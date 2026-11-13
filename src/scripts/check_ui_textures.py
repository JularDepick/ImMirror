#!/usr/bin/env python3
"""核对界面贴图链路: 布局 -> 模板 -> 子图集 -> 精灵矩形.

链路三段:
  1. 布局元素用 inherits 指向模板;
  2. 模板用 file 指向子图集描述, 用 uvname 指向描述里的精灵名;
  3. 子图集描述给出精灵在整张图里的矩形与整图尺寸.

模板有两种来源: 集中放在模板文件里, 或就地定义在某个界面文件里 (标记为 virtual).
有些模板只带布局与脚本, 不带贴图, 这类不计入精灵核对.
"""

import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "..", "..", "assets", "project"))
UI_ROOT = os.path.join(PROJECT, "ui")
LAYOUT_ROOT = os.path.join(PROJECT, "data", "xml", "ui", "mobile")

SPRITE = re.compile(r'<sprite\s+n="([^"]+)"\s+x="(\d+)"\s+y="(\d+)"\s+w="(\d+)"\s+h="(\d+)"')
ATLAS_HEAD = re.compile(r'<TextureAtlas\s+imagePath="([^"]+)"\s+width="(\d+)"\s+height="(\d+)"')
# 模板可能定义在任何元素上, 因此不限标签名.
ELEMENT = re.compile(r'<([A-Za-z]+)\s+([^>]*)>')


def read(path):
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        return handle.read()


def load_atlases():
    """收集所有子图集描述: 精灵名 -> 描述文件."""
    table = {}
    for root, _dirs, files in os.walk(UI_ROOT):
        for name in files:
            if not name.endswith(".xml"):
                continue
            path = os.path.join(root, name)
            text = read(path)
            if ATLAS_HEAD.search(text) is None:
                continue
            for match in SPRITE.finditer(text):
                table.setdefault(match.group(1), path)
    return table


def layout_files():
    for root, _dirs, files in os.walk(LAYOUT_ROOT):
        for name in files:
            if name.endswith(".xml"):
                yield os.path.join(root, name)


def load_templates():
    """模板名 -> (子图集描述路径, 精灵名); 精灵名可为空."""
    table = {}
    for path in layout_files():
        for match in ELEMENT.finditer(read(path)):
            head = match.group(2)
            # 模板不一定标 virtual: 被别处继承的元素本身就起模板作用.
            label = re.search(r'name="([^"]+)"', head)
            if label is None:
                continue
            fileAttr = re.search(r'file="([^"]+)"', head)
            uv = re.search(r'uvname="([^"]+)"', head)
            table.setdefault(label.group(1),
                             (fileAttr.group(1) if fileAttr else "",
                              uv.group(1) if uv else ""))
    return table


def referenced():
    """布局里引用到的模板名 -> 引用次数."""
    used = {}
    for path in layout_files():
        for match in re.finditer(r'inherits="([^"]+)"', read(path)):
            key = match.group(1)
            used[key] = used.get(key, 0) + 1
    return used


INDEX_HEADER = os.path.join(
    SCRIPT_DIR, "..", "native", "ui", "generated", "ui_sprite_index_gen.h")


def index_names():
    """从生成的索引头文件里取出全部精灵名."""
    path = os.path.normpath(INDEX_HEADER)
    if not os.path.isfile(path):
        return set()
    return set(re.findall(r'^\s*\{"([^"]+)",', read(path), re.M))


def main():
    atlases = load_atlases()
    templates = load_templates()
    used = referenced()
    if not atlases:
        print("没有找到子图集描述文件")
        return 2

    undefined = sorted(name for name in used if name not in templates)
    noSprite = 0
    resolved = 0
    missingAtlas = []
    unresolved = []
    for name in used:
        entry = templates.get(name)
        if entry is None:
            continue
        sprite = entry[1]
        if not sprite:
            noSprite += 1
            continue
        if sprite.endswith(".png"):
            sprite = sprite[:-4]
        if sprite in atlases:
            resolved += 1
            continue
        atlasPath = os.path.join(PROJECT, entry[0]) if entry[0] else ""
        if atlasPath and not os.path.isfile(atlasPath):
            missingAtlas.append((name, entry[0]))
            continue
        unresolved.append((name, entry[1]))

    print("子图集精灵 %d 个, 模板 %d 个, 布局引用的模板 %d 个"
          % (len(atlases), len(templates), len(used)))
    print("引用无定义 %d 个: %s"
          % (len(undefined), ", ".join(undefined[:5]) or "无"))
    # 生成的索引必须覆盖所有带贴图的模板, 否则运行期按名字取不到矩形.
    indexed = index_names()
    if indexed:
        missingIndex = []
        # 子图集本身就缺失的条目已在上一步计入, 这里不重复统计.
        alreadyMissing = {name for name, _ in missingAtlas}
        for name in used:
            entry = templates.get(name)
            if entry is None or not entry[1] or name in alreadyMissing:
                continue
            sprite = entry[1][:-4] if entry[1].endswith(".png") else entry[1]
            if sprite not in indexed:
                missingIndex.append(name)
        print("索引未覆盖 %d 个: %s"
              % (len(missingIndex), ", ".join(missingIndex[:5]) or "无"))
        if missingIndex:
            unresolved.append(("索引未覆盖", missingIndex[0]))
    else:
        print("索引未生成, 跳过覆盖核对")

    if missingAtlas:
        # 把缺失的文件与精灵名一并列出, 免去下次重新追查.
        for name, path in missingAtlas[:3]:
            sprite = templates[name][1]
            print("  子图集缺失: %s -> %s (精灵 %s)" % (name, path, sprite))

    print("不带贴图 %d 个, 精灵能查到 %d 个, 子图集缺失 %d 个, 精灵查不到 %d 个: %s"
          % (noSprite, resolved, len(missingAtlas), len(unresolved),
             ", ".join(name for name, _ in unresolved[:5]) or "无"))
    return 0 if not unresolved else 1


if __name__ == "__main__":
    sys.exit(main())
