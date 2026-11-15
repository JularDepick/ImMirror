#!/usr/bin/env python3
"""未接入 CSV 的影响面评估: 按规模与被引用情况排序."""

import glob
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT = os.path.normpath(os.path.join(ROOT, "..", "..", "assets", "project"))
CSVDIR = os.path.join(PROJECT, "csvdef")
AUTOGEN = os.path.join(CSVDIR, "autogen")
SCRIPTS = os.path.join(ROOT, "scripts")
NATIVE = os.path.join(ROOT, "native")
UIXML = os.path.join(PROJECT, "data", "xml", "ui", "mobile")
OUT = os.path.normpath(os.path.join(ROOT, "..", "..", "..", ".agent", "gameplay-baseline", "csv-gap.md"))

# 生成器读过的名字
read = set()
uses_autogen = False
for path in glob.glob(os.path.join(SCRIPTS, "gen_*.py")):
    body = io.open(path, encoding="utf-8", errors="replace").read()
    for hit in re.finditer(r"csvdef/([A-Za-z0-9_]+)", body):
        read.add(hit.group(1))
    if "autogen" in body:
        uses_autogen = True

autogen_names = set(os.path.basename(p)[:-4] for p in glob.glob(os.path.join(AUTOGEN, "*.csv")))
top = sorted(os.path.basename(p)[:-4] for p in glob.glob(os.path.join(CSVDIR, "*.csv")))

covered = set(top) & read
if uses_autogen:
    covered |= (set(top) & autogen_names)
missing = [n for n in top if n not in covered]

# 界面里是否出现该名字(粗略: 在 UI 布局文本里搜 csv 名)
ui_text = ""
for path in glob.glob(os.path.join(UIXML, "*.xml")):
    try:
        ui_text += io.open(path, encoding="utf-8", errors="replace").read().lower()
    except OSError:
        pass

# 原生代码里是否出现该名字
native_text = ""
for path in glob.glob(os.path.join(NATIVE, "**", "*.cpp"), recursive=True) + \
            glob.glob(os.path.join(NATIVE, "**", "*.h"), recursive=True):
    try:
        native_text += io.open(path, encoding="utf-8", errors="replace").read().lower()
    except OSError:
        pass

rows = []
for name in missing:
    path = os.path.join(CSVDIR, name + ".csv")
    try:
        lines = io.open(path, encoding="utf-8-sig", errors="replace").read().splitlines()
    except OSError:
        continue
    data_rows = max(0, len([l for l in lines if l.strip()]) - 3)
    in_ui = name.lower() in ui_text
    in_code = name.lower() in native_text
    rows.append((data_rows, name, in_ui, in_code))

rows.sort(reverse=True)

lines_out = ["# 未接入 CSV 影响面评估", "",
             "| CSV | 数据行 | 界面引用 | 代码引用 |",
             "|:---:|:---:|:---:|:---:|"]
for data_rows, name, in_ui, in_code in rows:
    lines_out.append("| %s | %d | %s | %s |" %
                     (name, data_rows, "是" if in_ui else "", "是" if in_code else ""))
lines_out.append("")
lines_out.append("合计 %d 个未接入 CSV。" % len(rows))
lines_out.append("")

os.makedirs(os.path.dirname(OUT), exist_ok=True)
io.open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(lines_out))
print("未接入 CSV %d 个, 已写出 %s" % (len(rows), OUT))
for data_rows, name, in_ui, in_code in rows[:12]:
    print("  %-24s %6d 行  界面=%s 代码=%s" % (name, data_rows, in_ui, in_code))
