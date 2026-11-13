"""界面布局索引: 把原版界面 XML 汇总成可查阅的结构统计.

只读. 解析 data/xml/ui 下的全部布局, 统计节点标签, 属性名, 脚本引用与
贴图引用, 为界面运行时的实现顺序提供依据.

用法: python -m forge.ui_layout_index [--write]
"""

import collections
import os
import sys
import xml.etree.ElementTree as ET

PROJECT_ROOT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "project")
UI_ROOT = os.path.join(PROJECT_ROOT, "data", "xml", "ui")
OUT_PATH = os.path.join(os.path.dirname(PROJECT_ROOT), "reports", "ui_layout_index.md")


def iter_xml():
    for root, _dirs, files in os.walk(UI_ROOT):
        for name in sorted(files):
            if name.endswith(".xml"):
                yield os.path.join(root, name)


def main():
    write = "--write" in sys.argv
    files = 0
    failed = []
    tags = collections.Counter()
    attrs = collections.Counter()
    scripts = 0
    sprites = set()
    per_dir = collections.Counter()
    max_depth = 0

    def walk(node, depth):
        nonlocal max_depth
        max_depth = max(max_depth, depth)
        tags[node.tag] += 1
        for key in node.attrib:
            attrs[key] += 1
        for child in node:
            walk(child, depth + 1)

    for path in iter_xml():
        files += 1
        rel = os.path.relpath(path, UI_ROOT).replace(os.sep, "/")
        per_dir[rel.split("/")[0] if "/" in rel else "."] += 1
        try:
            tree = ET.parse(path)
        except ET.ParseError:
            failed.append(rel)
            continue
        root = tree.getroot()
        walk(root, 0)
        scripts += len(list(root.iter("Script")))
        for node in root.iter():
            for key in ("file", "name", "res", "texture", "image", "img"):
                value = node.attrib.get(key)
                if value and value.endswith(".png"):
                    sprites.add(value)

    lines = [
        "# 界面布局索引",
        "",
        "由 forge.ui_layout_index 生成, 只读统计.",
        "",
        "| 项 | 值 |",
        "|:---:|:---:|",
        "| 布局文件 | %d |" % files,
        "| 解析失败 | %d |" % len(failed),
        "| 脚本引用 | %d |" % scripts,
        "| 不同贴图引用 | %d |" % len(sprites),
        "| 最大嵌套深度 | %d |" % max_depth,
        "| 不同节点标签 | %d |" % len(tags),
        "",
        "## 节点标签 (前 15)",
        "",
        "| 标签 | 次数 |",
        "|:---:|:---:|",
    ]
    for tag, count in tags.most_common(15):
        lines.append("| %s | %d |" % (tag, count))
    lines += ["", "## 属性名 (前 20)", "", "| 属性 | 次数 |", "|:---:|:---:|"]
    for key, count in attrs.most_common(20):
        lines.append("| %s | %d |" % (key, count))
    lines += ["", "## 目录分布", "", "| 目录 | 文件数 |", "|:---:|:---:|"]
    for name, count in sorted(per_dir.items()):
        lines.append("| %s | %d |" % (name, count))
    if failed:
        lines += ["", "## 解析失败", ""]
        for rel in failed[:20]:
            lines.append("- " + rel)
    report = "\n".join(lines) + "\n"
    print(report)
    if write:
        os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
        open(OUT_PATH, "w", encoding="utf-8", newline="\n").write(report)
        print("已写入 %s" % OUT_PATH)
    return 0


if __name__ == "__main__":
    sys.exit(main())
