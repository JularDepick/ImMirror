#!/usr/bin/env python3
"""标签覆盖核对: 布局里出现的标签必须被解析器处理, 或明确属于帧类型与脚本事件."""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LAYOUT = os.path.join(ROOT, "..", "..", "assets", "project", "data", "xml", "ui", "mobile")
SRC = os.path.join(ROOT, "native", "ui", "layout_xml.cpp")
OUT = os.path.join(ROOT, "..", "..", "..", ".agent", "ui-baseline", "tag-coverage.md")

# 帧类型: 走通用的"创建帧"分支, 不需要专门的 tag 判断.
FRAME_TYPES = {"frame", "button", "texture", "fontstring", "richtext", "slider", "editbox",
               "slidingframe", "layoutbox", "listviewframe", "modelview", "safeeditbox",
               "progressbar", "safemultieditbox", "multieditbox", "webframe", "doubleslider",
               "lineframe", "treeframe", "terrainprewview", "terrainmonsterpreview", "ui",
               "lineshape", "hollowshade", "thumbtexture", "thumbtexture1", "thumbtexture2",
               "checkedpushedtexture", "iconbar"}
# 脚本事件: 除 On 开头外, 还有这些以动作名结尾的写法.
SCRIPT_EVENTS = {"mousedownupdate", "mouseupupdate", "mouseoverupdate"}


def main():
    src = io.open(SRC, encoding="utf-8").read()
    handled = set(re.findall(r'tag(?:Name)? == "([A-Za-z][A-Za-z0-9]*)"', src))
    start = src.find("bool IsStructural")
    end = src.find("}", start)
    handled.update(re.findall(r'"([A-Za-z][A-Za-z0-9]*)"', src[start:end]))

    counts = {}
    for base, _dirs, files in os.walk(LAYOUT):
        for name in files:
            if not name.lower().endswith(".xml"):
                continue
            path = os.path.join(base, name)
            if "texture" in path.replace(os.sep, "/"):
                continue
            try:
                body = io.open(path, encoding="utf-8", errors="replace").read()
            except OSError:
                continue
            for hit in re.finditer(r"<([A-Za-z][A-Za-z0-9]*)[\s/>]", body):
                tag = hit.group(1)
                counts[tag] = counts.get(tag, 0) + 1

    handled_lower = {item.lower() for item in handled}
    unexplained = []
    for tag, count in sorted(counts.items(), key=lambda item: -item[1]):
        key = tag.lower()
        if key in handled_lower or key in FRAME_TYPES or key in SCRIPT_EVENTS or key.startswith("on"):
            continue
        unexplained.append((tag, count))

    lines = [
        "# 标签覆盖核对",
        "",
        "\u53e3\u5f84: \u5e03\u5c40\u91cc\u51fa\u73b0\u7684\u6807\u7b7e, \u5fc5\u987b\u5c5e\u4e8e\u4e09\u7c7b\u4e4b\u4e00: \u89e3\u6790\u5668\u6709\u4e13\u95e8\u5206\u652f\u3001\u5e27\u7c7b\u578b(\u8d70\u901a\u7528\u521b\u5efa)\u3001\u811a\u672c\u4e8b\u4ef6(On \u5f00\u5934)\u3002",
        "",
        "| \u9879 | \u503c |",
        "|:---:|:---:|",
        "| \u5e03\u5c40\u6807\u7b7e\u79cd\u7c7b | %d |" % len(counts),
        "| \u89e3\u6790\u5668\u5df2\u63d0\u53ca | %d |" % len([t for t in counts if t in handled]),
        "| \u672a\u5f52\u7c7b | %d |" % len(unexplained),
        "",
    ]
    if unexplained:
        lines.append("| \u6807\u7b7e | \u6b21\u6570 |")
        lines.append("|:---:|:---:|")
        for tag, count in unexplained:
            lines.append("| %s | %d |" % (tag, count))
        lines.append("")
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with io.open(OUT, "w", encoding="utf-8", newline=chr(10)) as handle:
        handle.write(chr(10).join(lines))
    print("\u6807\u7b7e\u8986\u76d6: \u5e03\u5c40 %d \u79cd, \u672a\u5f52\u7c7b %d \u79cd" % (len(counts), len(unexplained)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
