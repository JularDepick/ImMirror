#!/usr/bin/env python3
"""属性覆盖核对: 布局里出现的属性名必须被解析器读取, 或明确不需要."""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LAYOUT = os.path.join(ROOT, "..", "..", "assets", "project", "data", "xml", "ui", "mobile")
SRC = os.path.join(ROOT, "native", "ui", "layout_xml.cpp")
OUT = os.path.join(ROOT, "..", "..", "..", ".agent", "ui-baseline", "attr-coverage.md")

# 不需要读取的属性: XML 声明 / 图集内偏移 / 需要子系统的参数.
NOT_NEEDED = {
    "version", "encoding", "standalone",
    "offsetx", "offsety",
    "framelevel", "parent", "name", "file", "uvname", "id",
}
# 需要子系统支持, 暂不实现.
NEEDS_SUBSYSTEM = {
    "slideplane", "slidingy", "slidingx", "click_threshold", "input_transparent",
    "letters", "editmethod", "autoextend", "autowrap", "texrows", "texcols",
    "orientation", "minvalue", "maxvalue", "valuestep", "defaultvalue", "default",
    "barname", "iconnum", "direction", "icontexture", "bgicon",
    "quantityicon1", "quantityicon2", "quantityicon3", "quantityicon4",
    "hollow_x", "hollow_y", "hollow_width", "hollow_height", "shadename",
    "hotspotx", "hotspoty", "row", "col", "time", "dotline", "clipradius",
    "notclipped", "clipped", "letterspacing", "stretchedge",
    # 热键与输入法
    "keydown", "key", "alt", "ctrl", "shift", "password", "historylines", "returnkey",
    "ignoreleftdown", "ignoreleftdownslider", "force_input_transparent", "autotextlangscale",
    "startposition", "editimetype", "prefuncname",
    # 文字细节: 对齐变体与装饰
    "normaltextcolor", "checkedtextcolor", "justifyw", "justifym", "justifv", "spacing",
    "underline", "fontstyle1", "tiptext", "tipstex", "hangtipframe", "lineinterval",
    "ineinterval", "vxsyd_ditext",
    # 控件参数
    "type", "slidebar", "slidername", "background", "holddown_threshold", "style", "gray",
    "autoupdate", "wheel", "progressvalue", "cachesize", "samplermode", "gridwidth",
    "disappear_icon", "draggable", "overflow", "checked", "toplevel", "template", "index",
    "autorect", "atuorect", "hide", "hidde", "hidden1", "val", "url", "abs_t",
    "enableoffsetx", "enableoffsety", "enablew", "enableh", "rejectoffsety",
    # 拼写变体与残留写法
    "relativetolativepoint", "rey_x", "ffile", "topright", "layer", "angle", "height",
    # 语言版本
    "lang_ver",
    # 光标颜色与拼写错误的 hidden
    "cursorcolor", "hiden",
}


def main():
    src = io.open(SRC, encoding="utf-8").read()
    handled = {item.lower() for item in re.findall(r'key == "([A-Za-z][A-Za-z0-9_]*)"', src)}

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
            for hit in re.finditer(r'\b([A-Za-z][A-Za-z0-9_]*)="', body):
                key = hit.group(1).lower()
                counts[key] = counts.get(key, 0) + 1

    unexplained = []
    for key, count in sorted(counts.items(), key=lambda item: -item[1]):
        if key in handled or key in NOT_NEEDED or key in NEEDS_SUBSYSTEM:
            continue
        unexplained.append((key, count))

    lines = [
        "# 属性覆盖核对",
        "",
        "\u53e3\u5f84: \u5e03\u5c40\u91cc\u51fa\u73b0\u7684\u5c5e\u6027\u540d, \u5fc5\u987b\u5c5e\u4e8e\u4e09\u7c7b\u4e4b\u4e00: \u89e3\u6790\u5668\u5df2\u8bfb\u3001\u4e0d\u9700\u8981\u8bfb\u3001\u9700\u8981\u5b50\u7cfb\u7edf\u3002",
        "",
        "| \u9879 | \u503c |",
        "|:---:|:---:|",
        "| \u5e03\u5c40\u5c5e\u6027\u79cd\u7c7b | %d |" % len(counts),
        "| \u89e3\u6790\u5668\u5df2\u8bfb | %d |" % len([k for k in counts if k in handled]),
        "| \u672a\u5f52\u7c7b | %d |" % len(unexplained),
        "",
    ]
    if unexplained:
        lines.append("| \u5c5e\u6027 | \u6b21\u6570 |")
        lines.append("|:---:|:---:|")
        for key, count in unexplained:
            lines.append("| %s | %d |" % (key, count))
        lines.append("")
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with io.open(OUT, "w", encoding="utf-8", newline=chr(10)) as handle:
        handle.write(chr(10).join(lines))
    print("\u5c5e\u6027\u8986\u76d6: \u5e03\u5c40 %d \u79cd, \u672a\u5f52\u7c7b %d \u79cd" % (len(counts), len(unexplained)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
