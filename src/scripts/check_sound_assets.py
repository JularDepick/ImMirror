#!/usr/bin/env python3
"""音效断言: sound.csv 的每条 SoundPath 必须能在 sounds/ 里找到对应 ogg."""

import io
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT = os.path.normpath(os.path.join(ROOT, "..", "..", "assets", "project"))
CSV_PATH = os.path.join(PROJECT, "csvdef", "sound.csv")
SOUNDS = os.path.join(PROJECT, "sounds")
OUT = os.path.normpath(os.path.join(ROOT, "..", "..", "..", ".agent", "audio-baseline", "sound-assets.md"))

failures = []


def check(label, ok, detail=""):
    if ok:
        print("ok   %s" % label)
    else:
        print("FAIL %s %s" % (label, detail))
        failures.append("%s %s" % (label, detail))


def main():
    lines = io.open(CSV_PATH, encoding="utf-8-sig", errors="replace").read().splitlines()
    print("      CSV 行数 %d" % len(lines))
    check("CSV 至少四行", len(lines) >= 4)
    # 前两行是表头, 数据从第 3 行起
    data = [line.split(",") for line in lines[2:] if line.strip()]
    print("      数据行 %d" % len(data))

    # 统计分类
    from collections import Counter
    kinds = Counter()
    paths = []
    for row in data:
        if len(row) < 7:
            continue
        kind = row[5].strip()
        path = row[6].strip()
        if kind:
            kinds[kind] += 1
        if path:
            paths.append((row[0].strip(), row[1].strip(), path))

    print("      有路径的条目 %d" % len(paths))
    check("有路径的条目非零", len(paths) > 0)

    # 路径形如 blockd.cloth2, 对应 sounds/blockd/cloth2.ogg
    missing = []
    for ident, name, path in paths:
        if "." not in path:
            missing.append((ident, name, path, "无分隔符"))
            continue
        # 最后一个点才是文件名分隔符: ent.10001.idle -> sounds/ent/10001/idle.ogg
        head, tail = path.rsplit(".", 1)
        candidate = os.path.join(SOUNDS, head.replace(".", os.sep), tail + ".ogg")
        if not os.path.exists(candidate):
            missing.append((ident, name, path, "文件不存在"))

    print("      素材缺失 %d 条" % len(missing))
    check("每条音效都能找到素材", len(missing) == 0,
          "缺 %d 条: %s" % (len(missing), [m[2] for m in missing[:5]]))

    # 分类分布
    lines_out = ["# 音效素材对应断言", "",
                 "| 项 | 值 |", "|:---:|:---:|",
                 "| CSV 数据行 | %d |" % len(data),
                 "| 有路径条目 | %d |" % len(paths),
                 "| 素材缺失 | %d |" % len(missing),
                 "| 失败项 | %d |" % len(failures), "",
                 "## 分类分布", "",
                 "| 分类值 | 条目数 |", "|:---:|:---:|"]
    for kind, count in sorted(kinds.items(), key=lambda x: -x[1]):
        lines_out.append("| %s | %d |" % (kind, count))
    lines_out.append("")

    if missing:
        lines_out.append("## 缺失明细(前 30)")
        lines_out.append("")
        lines_out.append("| ID | 名称 | 路径 | 原因 |")
        lines_out.append("|:---:|:---:|:---:|:---:|")
        for ident, name, path, why in missing[:30]:
            lines_out.append("| %s | %s | %s | %s |" % (ident, name, path, why))
        lines_out.append("")

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    io.open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(lines_out))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
