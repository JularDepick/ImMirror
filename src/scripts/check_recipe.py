#!/usr/bin/env python3
"""配方断言: 生成表的配方 id 与产物 id 必须能在 crafting.csv 里溯源."""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT = os.path.normpath(os.path.join(ROOT, "..", "..", "assets", "project"))
CSV_PATH = os.path.join(PROJECT, "csvdef", "crafting.csv")
GEN_PATH = os.path.join(ROOT, "native", "game", "generated", "recipe_data_gen.h")
OUT = os.path.normpath(os.path.join(ROOT, "..", "..", "..", ".agent", "gameplay-baseline", "recipe.md"))

failures = []


def check(label, ok, detail=""):
    if ok:
        print("ok   %s" % label)
    else:
        print("FAIL %s %s" % (label, detail))
        failures.append("%s %s" % (label, detail))


def main():
    body = io.open(GEN_PATH, encoding="utf-8").read()
    rows = re.findall(r"\{(\d+)u,\s*(\d+)u,", body)
    print("      生成表解析到 %d 条配方" % len(rows))
    check("生成表条数非零", len(rows) > 0)

    lines = io.open(CSV_PATH, encoding="utf-8-sig", errors="replace").read().splitlines()
    print("      CSV 行数 %d" % len(lines))
    check("CSV 至少三行", len(lines) >= 3)
    # 前两行是表头, 数据从第 3 行起
    data = [line.split(",") for line in lines[2:] if line.strip()]
    print("      CSV 数据行 %d" % len(data))

    csv_ids = set()
    csv_results = set()
    for row in data:
        if len(row) > 2:
            if row[0].strip().isdigit():
                csv_ids.add(int(row[0].strip()))
            if row[2].strip().isdigit():
                csv_results.add(int(row[2].strip()))
    print("      CSV 配方 id %d 个, 产物 id %d 个" % (len(csv_ids), len(csv_results)))

    gen_ids = [int(r[0]) for r in rows]
    gen_results = [int(r[1]) for r in rows]
    miss_id = [i for i in gen_ids if i not in csv_ids]
    miss_res = [i for i in gen_results if i not in csv_results]
    print("      生成表配方 id %d 个, 缺 %d" % (len(gen_ids), len(miss_id)))
    print("      生成表产物 id %d 个, 缺 %d" % (len(gen_results), len(miss_res)))
    check("配方 id 全部可溯源", len(miss_id) == 0, "缺 %d: %s" % (len(miss_id), miss_id[:5]))

    # 产物 id 是客户端 id, 与 CSV 的素材源 id 不是同一套; 只校验落在客户端物品总数范围内
    item_gen = os.path.join(ROOT, "native", "game", "generated", "item_data_gen.h")
    item_count = 0
    if os.path.exists(item_gen):
        m = re.search(r"constexpr int32_t kItemInfoCount\s*=\s*(\d+)",
                      io.open(item_gen, encoding="utf-8", errors="replace").read())
        item_count = int(m.group(1)) if m else 0
    print("      客户端物品总数 %d" % item_count)
    out_of_range = [i for i in gen_results if item_count > 0 and i >= item_count]
    print("      产物 id 越界 %d 个" % len(out_of_range))
    check("产物 id 落在客户端物品范围内", len(out_of_range) == 0,
          "越界 %d: %s" % (len(out_of_range), out_of_range[:5]))

    lines_out = ["# 配方断言", "",
                 "| 项 | 值 |", "|:---:|:---:|",
                 "| 生成表条数 | %d |" % len(rows),
                 "| CSV 数据行 | %d |" % len(data),
                 "| CSV 配方 id | %d |" % len(csv_ids),
                 "| CSV 产物 id | %d |" % len(csv_results),
                 "| 配方 id 不可溯源 | %d |" % len(miss_id),
                 "| 产物 id 越界 | %d |" % len(out_of_range),
                 "| 失败项 | %d |" % len(failures), ""]
    if miss_id:
        lines_out.append("## 不可溯源的配方 id")
        lines_out.append("")
        lines_out.append(", ".join(str(i) for i in miss_id[:20]))
        lines_out.append("")
    if miss_res:
        lines_out.append("## 不可溯源的产物 id")
        lines_out.append("")
        lines_out.append(", ".join(str(i) for i in miss_res[:20]))
        lines_out.append("")

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    io.open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(lines_out))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
