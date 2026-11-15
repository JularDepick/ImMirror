#!/usr/bin/env python3
"""布局几何属性扫描: 找出负数或异常大的坐标与尺寸."""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LAYOUT = os.path.join(ROOT, "..", "..", "assets", "project", "data", "xml", "ui", "mobile")
OUT = os.path.join(ROOT, "..", "..", "..", ".agent", "ui-baseline", "geometry-scan.md")

SCREENS = ["lobby", "account", "createworld", "setting", "backpack",
           "multiplayerlobby", "room", "selectrole"]

# 只扫元素自身的几何属性: AbsDimension 在 Offset 里是偏移量, 负数正常, 不纳入.
GEOMETRY = ["abs_x", "abs_y", "w", "h", "spacex", "spacey"]
ATTR_RE = re.compile(r"\b(" + "|".join(GEOMETRY) + r")=\"(-?[0-9]+(?:\.[0-9]+)?)\"")
# 阈值: 单值超过这个数视为异常(画布 1280x720, 留足余量).
LIMIT = 4096.0


def main():
    findings = []
    for stem in SCREENS:
        path = os.path.join(LAYOUT, stem + ".xml")
        try:
            lines = open(path, "r", encoding="utf-8", errors="replace").readlines()
        except OSError:
            continue
        for number, text in enumerate(lines, 1):
            for hit in ATTR_RE.finditer(text):
                value = float(hit.group(2))
                # 尺寸与间距不能为负; 绝对坐标可以为负(往左偏移), 只查超大值.
                is_size = hit.group(1) in ("w", "h", "spacex", "spacey")
                if (is_size and value < 0.0) or value > LIMIT:
                    findings.append((stem, number, hit.group(1), hit.group(2), text.strip()[:60]))

    lines = [
        "# 布局几何属性扫描",
        "",
        "\u53e3\u5f84: \u626b\u63cf\u5e03\u5c40\u91cc\u7684\u5750\u6807\u4e0e\u5c3a\u5bf8\u5c5e\u6027, \u8d1f\u6570\u6216\u8d85\u8fc7 4096 \u7684\u89c6\u4e3a\u5f02\u5e38\u3002",
        "\u8d1f\u6570\u5750\u6807\u5728\u5e03\u5c40\u91cc\u5f88\u5e38\u89c1(\u5f80\u5de6\u4e0a\u504f\u79fb), \u56e0\u6b64\u8fd9\u91cc\u53ea\u505a\u6e05\u5355\u4e0d\u505a\u5224\u5b9a\u3002",
        "",
        "| \u5c4f | \u884c | \u5c5e\u6027 | \u503c | \u7247\u6bb5 |",
        "|:---:|:---:|:---:|:---:|:---|",
    ]
    for item in findings[:40]:
        lines.append("| %s | %d | %s | %s | %s |" % item)
    lines.append("")
    lines.append("\u5408\u8ba1 %d \u5904\u3002" % len(findings))
    lines.append("")
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline=chr(10)) as handle:
        handle.write(chr(10).join(lines))
    print("\u51e0\u4f55\u5c5e\u6027\u626b\u63cf: %d \u5904\u5f02\u5e38\u503c" % len(findings))
    return 0


if __name__ == "__main__":
    sys.exit(main())
