#!/usr/bin/env python3
"""布局健康度检查: 从绘制清单检测负坐标, 超界与完全重叠, 支持已知例外清单."""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG_DIR = os.path.join(ROOT, "..", "..", "temp")
BASE = os.path.join(ROOT, "..", "..", "..", ".agent", "ui-baseline")
OUT = os.path.join(BASE, "layout-health.md")
ALLOWLIST = os.path.join(BASE, "health-allowlist.txt")
CANVAS_W = 1280.0
CANVAS_H = 720.0

SCREENS = [
    ("大厅", "lobby"),
    ("登录", "account"),
    ("单人", "createworld"),
    ("设置", "setting"),
    ("背包", "backpack"),
    ("联机大厅", "multiplayerlobby"),
    ("联机房间", "room"),
    ("选角", "selectrole"),


]

ROW_RE = re.compile(
    r"^\|\s*(\d+)\s*\|\s*([^|]+?)\s*\|\s*(-?[\d.]+),\s*(-?[\d.]+),\s*([\d.]+)x([\d.]+)\s*"
    r"\|\s*([^|]*?)\s*\|\s*([^|]*?)\s*\|"
)


def load_allowlist():
    """已知例外前缀: 设计上允许超出画布或与同类重叠的元素."""
    prefixes = []
    try:
        with open(ALLOWLIST, "r", encoding="utf-8") as handle:
            for text in handle:
                item = text.strip()
                if not item or item.startswith("#"):
                    continue
                prefixes.append(item)
    except OSError:
        pass
    return prefixes


def missing_refs(stem):
    """该屏锚点参照物缺失的处数: 这类缺失会让位置退化为父帧, 与真实错位要分开看."""
    path = os.path.join(LOG_DIR, "screen-%s.log" % stem)
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            for text in handle:
                if "锚点参照物缺失" in text:
                    found = re.search(r"锚点参照物缺失 (\d+)", text)
                    if found is not None:
                        return int(found.group(1))
    except OSError:
        pass
    return 0


def allowed(name, prefixes):
    for prefix in prefixes:
        if name.startswith(prefix):
            return True
    return False


def parse(path):
    rows = []
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            for text in handle:
                hit = ROW_RE.match(text)
                if hit is None:
                    continue
                atlas = hit.group(7).strip()
                sprite = hit.group(8).strip()
                rows.append(
                    (
                        hit.group(2).strip(),
                        float(hit.group(3)),
                        float(hit.group(4)),
                        float(hit.group(5)),
                        float(hit.group(6)),
                        atlas not in ("", "-") or sprite not in ("", "-"),
                    )
                )
    except OSError:
        pass
    return rows


def main():
    prefixes = load_allowlist()
    lines = [
        "# 布局健康度",
        "",
        "口径: 从各屏绘制清单读矩形, 检测负坐标, 超界(超出 1280x720)与完全重叠。",
        "已登记的已知例外(装饰性超出、互斥页签)不计入统计, 清单见 health-allowlist.txt。",
        "超界只统计带贴图的元素: 纯文字帧允许比画布宽, 文字已按帧宽裁剪。",
        "",
        "| 屏 | 元素 | 负坐标 | 超界 | 完全重叠组 | 重叠元素 | 参照物缺失 |",
        "|:---:|:---:|:---:|:---:|:---:|:---:|:---:|",
    ]
    details = []
    for label, stem in SCREENS:
        rows = parse(os.path.join(BASE, "drawlist-%s.md" % stem))
        negative = [r for r in rows if (r[1] < 0 or r[2] < 0) and not allowed(r[0], prefixes)]
        # 超界只统计带贴图的元素: 纯文字帧可以比画布宽, 文字本身已按帧宽裁剪.
        outside = [
            r
            for r in rows
            if (r[1] < 0 or r[2] < 0 or r[1] + r[3] > CANVAS_W + 1 or r[2] + r[4] > CANVAS_H + 1)
            and r[5]
            and not allowed(r[0], prefixes)
        ]
        groups = {}
        for r in rows:
            if allowed(r[0], prefixes):
                continue
            key = (round(r[1], 1), round(r[2], 1), round(r[3], 1), round(r[4], 1))
            groups.setdefault(key, []).append(r[0])
        stacked = {k: v for k, v in groups.items() if len(v) > 1}
        lines.append(
            "| %s | %d | %d | %d | %d | %d | %d |"
            % (label, len(rows), len(negative), len(outside), len(stacked),
               sum(len(v) for v in stacked.values()), missing_refs(stem))
        )
        if negative:
            details.append("")
            details.append("## %s 负坐标元素" % label)
            details.append("")
            details.append("| 元素 | 矩形 |")
            details.append("|:---|:---|")
            for r in negative[:12]:
                details.append("| %s | %.1f, %.1f, %.0fx%.0f |" % (r[0], r[1], r[2], r[3], r[4]))
        if outside:
            details.append("")
            details.append("## %s 超界元素" % label)
            details.append("")
            details.append("| 元素 | 矩形 |")
            details.append("|:---|:---|")
            for r in outside[:12]:
                details.append("| %s | %.1f, %.1f, %.0fx%.0f |" % (r[0], r[1], r[2], r[3], r[4]))
        if stacked:
            details.append("")
            details.append("## %s 完全重叠" % label)
            details.append("")
            details.append("| 矩形 | 元素 |")
            details.append("|:---|:---|")
            for key in list(stacked)[:8]:
                details.append(
                    "| %.1f, %.1f, %.0fx%.0f | %s |"
                    % (key[0], key[1], key[2], key[3], ", ".join(stacked[key][:6]))
                )

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines + details) + "\n")
    print("布局健康度已写出 %s (已知例外 %d 条)" % (OUT, len(prefixes)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
