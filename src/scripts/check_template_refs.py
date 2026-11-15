#!/usr/bin/env python3
"""模板引用核对: inherits 引用的模板必须存在定义, 否则实例没有外观."""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LAYOUT = os.path.join(ROOT, "..", "..", "assets", "project", "data", "xml", "ui", "mobile")
OUT = os.path.join(ROOT, "..", "..", "..", ".agent", "ui-baseline", "template-refs.md")

HEAD = "# \u6a21\u677f\u5f15\u7528\u6838\u5bf9"
NOTE = "\u53e3\u5f84: \u5e03\u5c40\u91cc inherits \u5f15\u7528\u7684\u6a21\u677f\u5fc5\u987b\u6709 virtual=true \u7684\u5b9a\u4e49, \u5426\u5219\u5b9e\u4f8b\u6ca1\u6709\u5916\u89c2\u3002"


def main():
    defined = set()
    used = {}
    where = {}
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
            for hit in re.finditer(r'name="([A-Za-z0-9_]+)"[^>]*virtual="true"', body):
                defined.add(hit.group(1))
            for hit in re.finditer(r'inherits="([A-Za-z0-9_]+)"', body):
                key = hit.group(1)
                used[key] = used.get(key, 0) + 1
                where.setdefault(key, set()).add(name)

    missing = sorted(((c, t) for t, c in used.items() if t not in defined), reverse=True)
    lines = [HEAD, "", NOTE, ""]
    lines.append("| \u9879 | \u503c |")
    lines.append("|:---:|:---:|")
    lines.append("| \u6a21\u677f\u5b9a\u4e49 | %d |" % len(defined))
    lines.append("| \u88ab\u5f15\u7528 | %d |" % len(used))
    lines.append("| \u5f15\u7528\u4f46\u672a\u5b9a\u4e49 | %d |" % len(missing))
    lines.append("")
    if missing:
        lines.append("| \u6a21\u677f | \u5f15\u7528\u6b21\u6570 | \u51fa\u73b0\u5728 |")
        lines.append("|:---:|:---:|:---|")
        for count, tag in missing:
            files = ", ".join(sorted(where.get(tag, set()))[:3])
            lines.append("| %s | %d | %s |" % (tag, count, files))
        lines.append("")
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with io.open(OUT, "w", encoding="utf-8", newline=chr(10)) as handle:
        handle.write(chr(10).join(lines))
    print("\u6a21\u677f\u5f15\u7528: \u5b9a\u4e49 %d, \u5f15\u7528 %d, \u7f3a\u5c11 %d" % (len(defined), len(used), len(missing)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
