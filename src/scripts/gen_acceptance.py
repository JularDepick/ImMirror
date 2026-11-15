#!/usr/bin/env python3
"""逐屏验收表: 从六屏转储日志汇总可见命令与精灵命中, 输出 markdown 表."""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LOG_DIR = os.path.join(ROOT, "..", "temp")
OUT = os.path.join(ROOT, "..", "..", ".agent", "ui-baseline", "acceptance.md")

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

FIELDS = ["可见命令", "带贴图", "带精灵", "精灵命中", "带文字", "点击脚本", "九宫", "四元组", "顶点"]
# 子树规模单独从统计行取.
SCALE_FIELDS = ["帧", "可绘制", "可见", "应输出"]
# 质量指标: 这些值必须为 0, 单独成表便于逐轮对比.
QUALITY_FIELDS = ["裁字", "九宫退化", "坏色", "零面积", "空壳继承", "贴图断", "克隆残缺", "零尺有物", "尺寸不符", "父子断", "死按钮"]


def pick(line, field):
    match = re.search(field + r" (\d+)", line)
    return match.group(1) if match is not None else "0"


def pick_scale(path):
    """规模数字按所有子树累加: 一屏可能有多个主帧, 只取第一个会与命令数对不上."""
    totals = [0] * len(SCALE_FIELDS)
    found = False
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            for text in handle:
                if "子树" not in text:
                    continue
                found = True
                for index, field in enumerate(SCALE_FIELDS):
                    totals[index] += int(pick(text, field))
    except OSError:
        pass
    if not found:
        return ["0"] * len(SCALE_FIELDS)
    return [str(value) for value in totals]


def main():
    rows = []
    for label, stem in SCREENS:
        path = os.path.join(LOG_DIR, "screen-%s.log" % stem)
        line = ""
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as handle:
                for text in handle:
                    if text.startswith("布局"):
                        line = text
                        break
        except OSError:
            pass
        values = pick_scale(path) + [pick(line, field) for field in FIELDS]
        errors = "0"
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as handle:
                for text in handle:
                    if "脚本错误" in text:
                        found = re.search(r"脚本错误 (\d+)", text)
                        if found is not None:
                            errors = found.group(1)
        except OSError:
            pass
        rows.append([label] + values + [errors])

    lines = [
        "# 逐屏验收表",
        "",
        "口径: 实际命令为本次转储的可见绘制命令数; 精灵命中指带精灵名的命令里能查到图集矩形的条数。",
        "",
        "| 屏 | 帧 | 可绘制 | 可见 | 应输出 | 实际命令 | 带贴图 | 带精灵 | 精灵命中 | 文字 | 点击脚本 | 九宫 | 四元组 | 顶点 | 脚本错误 |",
        "|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|",
    ]
    for row in rows:
        lines.append("| " + " | ".join(row) + " |")
    lines.append("")
    lines.append("## 质量指标")
    lines.append("")
    lines.append("口径: 这些值都应为 0; 非 0 说明绘制质量有问题。")
    lines.append("")
    lines.append("| 屏 | " + " | ".join(QUALITY_FIELDS) + " |")
    lines.append("|:---:|" + ":---:|" * len(QUALITY_FIELDS))
    for label, stem in SCREENS:
        path = os.path.join(LOG_DIR, "screen-%s.log" % stem)
        line = ""
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as handle:
                for text in handle:
                    if text.startswith("布局"):
                        line = text
                        break
        except OSError:
            pass
        lines.append("| %s | %s |" % (label, " | ".join(pick(line, field) for field in QUALITY_FIELDS)))
    lines.append("")
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines))
    print("验收表已写出 %s" % OUT)
    return 0


if __name__ == "__main__":
    sys.exit(main())
