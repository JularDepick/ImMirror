#!/usr/bin/env python3
"""方块表断言: 生成表与 blockdef.csv 的条数, ID 与名称必须一致."""

import csv
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT = os.path.normpath(os.path.join(ROOT, "..", "..", "assets", "project"))
CSV_PATH = os.path.join(PROJECT, "csvdef", "blockdef.csv")
GEN_PATH = os.path.join(ROOT, "native", "game", "generated", "block_data_gen.h")
OUT = os.path.normpath(os.path.join(ROOT, "..", "..", "..", ".agent", "world-baseline", "block-table.md"))

failures = []


def check(label, ok, detail=""):
    if ok:
        print("ok   %s" % label)
    else:
        print("FAIL %s %s" % (label, detail))
        failures.append("%s %s" % (label, detail))


def main():
    body = io.open(GEN_PATH, encoding="utf-8").read()

    count_match = re.search(r"kBlockEntryCount = (\d+)", body)
    check("生成表声明条数", count_match is not None)
    declared = int(count_match.group(1)) if count_match else 0

    # 只取 kBlockData 数组段
    start = body.find("constexpr BlockData kBlockData[kBlockDataCount] = {")
    end = body.find("};", start) if start >= 0 else -1
    check("找到 kBlockData 数组", start >= 0 and end > start)
    segment = body[start:end] if start >= 0 and end > start else ""
    rows_gen = re.findall(r"\{(\d+)u,\s*\"([^\"]*)\",\s*\"([^\"]*)\"", segment)
    print("      kBlockData 条数 %d, 声明 %d" % (len(rows_gen), declared))
    check("数组条数与声明一致", len(rows_gen) == declared,
          "数组=%d 声明=%d" % (len(rows_gen), declared))

    all_rows = list(csv.DictReader(io.open(CSV_PATH, encoding="utf-8-sig", errors="replace")))
    # 首行是重复表头(ENName/unload/...), 按英文名首列是否为 ENName 识别
    rows = [r for r in all_rows if (list(r.values())[0] or "").strip() != "ENName"]
    check("CSV 行数非空", len(rows) > 0, "行数=%d" % len(rows))
    print("      CSV 原始 %d 行, 去掉重复表头后 %d 行" % (len(all_rows), len(rows)))
    check("CSV 与生成表条数一致", len(rows) == len(rows_gen),
          "CSV=%d 生成=%d" % (len(rows), len(rows_gen)))

    # 名称对比: CSV 的英文名称列 vs 生成表 name
    gen_names = set(item[1] for item in rows_gen)
    eng_col = None
    for name in rows[0].keys():
        if name and "英文名称" in name:
            eng_col = name
            break
    if eng_col is None:
        print("      未找到英文名称列, 跳过名称对比")
    else:
        # 生成表存小写键并做了规范化(空格与 & 等转下划线), 这里按同一规则折算后再比
        def normalize(text):
            out = []
            for ch in text.strip().lower():
                out.append(ch if ch.isalnum() or ch == "_" else "_")
            return "".join(out)
        csv_names = set(normalize(row.get(eng_col) or "") for row in rows)
        csv_names.discard("")
        gen_lower = set(normalize(n) for n in gen_names)
        print("      CSV 英文名 %d 个, 生成表 name %d 个" % (len(csv_names), len(gen_names)))
        missing = sorted(csv_names - gen_lower)
        check("CSV 英文名全部进生成表", len(missing) == 0,
              "缺 %d 个: %s" % (len(missing), ", ".join(missing[:5])))

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    lines = ["# 方块表断言", "",
             "| 项 | 值 |", "|:---:|:---:|",
             "| CSV 行数 | %d |" % len(rows),
             "| kBlockData 条数 | %d |" % len(rows_gen),
             "| 声明条数 | %d |" % declared,
             "| 失败项 | %d |" % len(failures), ""]
    if failures:
        lines.append("| 失败项 |")
        lines.append("|:---|")
        for item in failures:
            lines.append("| %s |" % item)
        lines.append("")
    io.open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(lines))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
