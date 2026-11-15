#!/usr/bin/env python3
"""熔炉与工具断言: sourceId 可溯源, 客户端 id 落在物品总数内."""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT = os.path.normpath(os.path.join(ROOT, "..", "..", "assets", "project"))
OUT = os.path.normpath(os.path.join(ROOT, "..", "..", "..", ".agent", "gameplay-baseline", "furnace-tool.md"))

failures = []


def check(label, ok, detail=""):
    if ok:
        print("ok   %s" % label)
    else:
        print("FAIL %s %s" % (label, detail))
        failures.append("%s %s" % (label, detail))


def item_total():
    path = os.path.join(ROOT, "native", "game", "generated", "item_data_gen.h")
    if not os.path.exists(path):
        return 0
    m = re.search(r"constexpr int32_t kItemInfoCount\s*=\s*(\d+)",
                  io.open(path, encoding="utf-8", errors="replace").read())
    return int(m.group(1)) if m else 0


def main():
    total = item_total()
    print("      客户端物品总数 %d" % total)
    check("客户端物品总数非零", total > 0)

    # 熔炉
    fpath = os.path.join(ROOT, "native", "game", "generated", "furnace_data_gen.h")
    fbody = io.open(fpath, encoding="utf-8", errors="replace").read()
    frows = re.findall(r'\{(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*"([^"]*)",\s*"([^"]*)"', fbody)
    print("      熔炉条目 %d" % len(frows))
    check("熔炉条目非零", len(frows) > 0)

    flines = io.open(os.path.join(PROJECT, "csvdef", "furnace.csv"), encoding="utf-8-sig",
                     errors="replace").read().splitlines()
    fdata = [line.split(",") for line in flines[2:] if line.strip()]
    fids = set()
    for row in fdata:
        if row and row[0].strip().isdigit():
            fids.add(int(row[0].strip()))
    print("      furnace.csv 数据行 %d, 熔炼 id %d 个" % (len(fdata), len(fids)))
    miss = [int(r[0]) for r in frows if int(r[0]) not in fids]
    check("熔炉 sourceId 全部可溯源", len(miss) == 0, "缺 %d: %s" % (len(miss), miss[:5]))

    over = [r for r in frows if int(r[1]) >= total or int(r[2]) >= total]
    check("熔炉材料与产物 id 落在物品范围内", len(over) == 0,
          "越界 %d 条" % len(over))

    # 工具
    tpath = os.path.join(ROOT, "native", "game", "generated", "tool_data_gen.h")
    tbody = io.open(tpath, encoding="utf-8", errors="replace").read()
    trows = re.findall(r"\{(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u\}", tbody)
    print("      工具条目 %d" % len(trows))
    check("工具条目非零", len(trows) > 0)
    tover = [r for r in trows if int(r[0]) >= total]
    check("工具 itemId 落在物品范围内", len(tover) == 0, "越界 %d 条" % len(tover))

    # 战斗工具
    cpath = os.path.join(ROOT, "native", "game", "generated", "tool_combat_gen.h")
    cbody = io.open(cpath, encoding="utf-8", errors="replace").read()
    cm = re.search(r"constexpr int32_t kToolCombatCount\s*=\s*(\d+)", cbody)
    ccount = int(cm.group(1)) if cm else 0
    print("      战斗工具条目 %d" % ccount)
    check("战斗工具条目非零", ccount > 0)

    lines_out = ["# 熔炉与工具断言", "",
                 "| 项 | 值 |", "|:---:|:---:|",
                 "| 客户端物品总数 | %d |" % total,
                 "| 熔炉条目 | %d |" % len(frows),
                 "| furnace.csv 数据行 | %d |" % len(fdata),
                 "| furnace.csv 熔炼 id | %d |" % len(fids),
                 "| 熔炉 sourceId 不可溯源 | %d |" % len(miss),
                 "| 熔炉 id 越界 | %d |" % len(over),
                 "| 工具条目 | %d |" % len(trows),
                 "| 工具 id 越界 | %d |" % len(tover),
                 "| 战斗工具条目 | %d |" % ccount,
                 "| 失败项 | %d |" % len(failures), ""]

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    io.open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(lines_out))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
