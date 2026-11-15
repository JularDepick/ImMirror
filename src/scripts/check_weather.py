#!/usr/bin/env python3
"""天气断言: 群系带的天气参数必须落在 biomedef.csv 的取值范围内."""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT = os.path.normpath(os.path.join(ROOT, "..", "..", "assets", "project"))
CSV_PATH = os.path.join(PROJECT, "csvdef", "biomedef.csv")
GEN_PATH = os.path.join(ROOT, "native", "voxel", "generated", "biome_data_gen.h")
OUT = os.path.normpath(os.path.join(ROOT, "..", "..", "..", ".agent", "world-baseline", "weather.md"))

failures = []


def check(label, ok, detail=""):
    if ok:
        print("ok   %s" % label)
    else:
        print("FAIL %s %s" % (label, detail))
        failures.append("%s %s" % (label, detail))


def main():
    body = io.open(GEN_PATH, encoding="utf-8").read()
    start = body.find("constexpr BiomeBand kBiomeBands[kBiomeBandCount] = {")
    end = body.find("};", start) if start >= 0 else -1
    check("找到 kBiomeBands 数组", start >= 0 and end > start)
    segment = body[start:end] if start >= 0 and end > start else ""

    pattern = (r"\{(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,"
               r"\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,"
               r"\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,\s*(\d+)u,"
               r'\s*"([^"]*)",\s*"([^"]*)"\}')
    rows = re.findall(pattern, segment)
    print("      解析到 %d 个群系带" % len(rows))
    check("群系带数量为 6", len(rows) == 6, "实际 %d" % len(rows))

    lines = io.open(CSV_PATH, encoding="utf-8-sig", errors="replace").read().splitlines()
    check("CSV 至少四行", len(lines) >= 4)
    header_cn = lines[0].split(",")
    data_rows = [line.split(",") for line in lines[3:] if line.strip()]
    print("      CSV 群系数 %d" % len(data_rows))

    def col_index(names, needle):
        for i, name in enumerate(names):
            if needle in name:
                return i
        return -1

    idx = {
        "rain": col_index(header_cn, "是否可以下雨"),
        "snow": col_index(header_cn, "是否可以下雪"),
        "clear": col_index(header_cn, "晴天系数"),
        "rainW": col_index(header_cn, "雨天系数"),
        "thunderW": col_index(header_cn, "雷暴系数"),
    }
    print("      列下标 %s" % idx)
    check("天气五列都能定位", min(idx.values()) >= 0)

    # 是否可以下雨/下雪是布尔值, 不缩放; 三个系数放大 100 倍存整数
    SCALED = {"clear": 100, "rainW": 100, "thunderW": 100}
    allowed = {}
    for name, i in idx.items():
        scale = SCALED.get(name, 1)
        vals = set()
        for row in data_rows:
            raw = row[i].strip() if i < len(row) else ""
            try:
                vals.add(int(round(float(raw) * scale)))
            except ValueError:
                continue
        allowed[name] = vals
        print("      CSV %-9s 取值 %s (缩放 %d)" % (name, sorted(vals), scale))

    order = ["rain", "snow", "clear", "rainW", "thunderW"]
    lines_out = ["# 天气参数断言", "",
                 "| 项 | 值 |", "|:---:|:---:|",
                 "| 群系带数 | %d |" % len(rows),
                 "| CSV 群系数 | %d |" % len(data_rows),
                 "| 失败项 | %d |" % len(failures), "",
                 "| 带 | key | 降雨 | 降雪 | 晴 | 雨 | 雷 |",
                 "|:---:|:---:|:---:|:---:|:---:|:---:|:---:|"]
    for item in rows:
        key = item[20]
        band = [int(item[7]), int(item[8]), int(item[9]), int(item[10]), int(item[11])]
        marks = []
        for i, name in enumerate(order):
            ok = band[i] in allowed[name]
            marks.append("%d%s" % (band[i], "" if ok else "?"))
            if not ok:
                failures.append("带 %s 的 %s=%d 不在 CSV 取值集合" % (key, name, band[i]))
        lines_out.append("| %s | %s | %s |" % (item[19], key, " | ".join(marks)))
    lines_out.append("")

    for item in rows:
        key = item[20]
        band = [int(item[7]), int(item[8]), int(item[9]), int(item[10]), int(item[11])]
        ok = all(band[i] in allowed[order[i]] for i in range(5))
        check("带 %s 的天气参数在 CSV 取值范围内" % key, ok, str(band))

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    io.open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(lines_out))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
