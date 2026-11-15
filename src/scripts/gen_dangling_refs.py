#!/usr/bin/env python3
"""悬空引用清单: 汇总各屏锚点指向但从未定义的帧名."""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG_DIR = os.path.join(ROOT, "..", "..", "temp")
OUT = os.path.join(ROOT, "..", "..", "..", ".agent", "ui-baseline", "dangling-refs.md")

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


def main():
    per_screen = {}
    all_names = {}
    for label, stem in SCREENS:
        path = os.path.join(LOG_DIR, "screen-%s.log" % stem)
        names = []
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as handle:
                for text in handle:
                    if "锚点参照物缺失" not in text:
                        continue
                    tail = text.split(":", 1)
                    if len(tail) < 2:
                        continue
                    names.extend(tail[1].split())
        except OSError:
            pass
        per_screen[label] = names
        for name in names:
            all_names[name] = all_names.get(name, 0) + 1

    lines = [
        "# 悬空引用清单",
        "",
        "口径: 锚点的 relativeTo 指向某个帧名, 而该名字在所有布局文件里都没有定义。",
        "这类引用会让位置退化为父帧, 属布局文件自身的问题, 引擎只能容错。",
        "",
        "| 帧名 | 出现屏数 | 出现的屏 |",
        "|:---:|:---:|:---|",
    ]
    for name in sorted(all_names, key=lambda key: (-all_names[key], key)):
        screens = [label for label, _ in SCREENS if name in per_screen[label]]
        lines.append("| %s | %d | %s |" % (name, len(screens), " ".join(screens)))
    lines.append("")
    lines.append("合计 %d 个不同的悬空引用名。" % len(all_names))
    lines.append("")
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline=chr(10)) as handle:
        handle.write(chr(10).join(lines))
    print("悬空引用: %d 个不同名字" % len(all_names))
    return 0


if __name__ == "__main__":
    sys.exit(main())
