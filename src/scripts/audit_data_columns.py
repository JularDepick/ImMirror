#!/usr/bin/env python3
"""数据列审计: 找出数据表里"有数据但代码里没读到"的列.

只报告同时满足两个条件的列: 整列为空的不报 (没有可实现的语义),
代码里已出现列名的不报. 输出按有数据的行数降序, 便于判断价值.
"""

import csv
import io
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
REPO_ROOT = os.path.normpath(os.path.join(SRC_ROOT, "..", ".."))
CSV_DIR = os.path.join(REPO_ROOT, "assets", "project", "csvdef")
CODE_DIRS = [os.path.join(SRC_ROOT, "native"), SCRIPT_DIR]

TABLES = [
    "blockdef.csv",
    "autogen/itemdef.csv",
    "tooldef.csv",
    "autogen/buffdef.csv",
    "autogen/monsterdef.csv",
    "fooddef.csv",
    "enchant.csv",
    "furnacedef.csv",
    "crafting.csv",
    "biomedef.csv",
    "oredef.csv",
    "dropitems.csv",
]


def collect_code():
    chunks = []
    for base in CODE_DIRS:
        for root, _dirs, files in os.walk(base):
            for name in files:
                if name.endswith((".cpp", ".h", ".py")):
                    path = os.path.join(root, name)
                    chunks.append(open(path, encoding="utf-8", errors="replace").read())
    return "\n".join(chunks)


# 本地化列: 客户端只有英文字形, 这些列不读是既定取舍, 不计入待评估.
LOCALIZED_PREFIXES = ("TW", "EN", "RUS", "VIE", "THA", "JPN", "KOR", "FRA", "ESN", "PTB", "DEU")
LOCALIZED_SUFFIXES = ("Name", "Desc")


def is_localized(name):
    if not name.endswith(LOCALIZED_SUFFIXES):
        return False
    return any(name.startswith(prefix) for prefix in LOCALIZED_PREFIXES)


def is_read(name, blob):
    """列名或去掉末尾序号后的词干在代码里出现过, 即视为已读."""
    if name in blob:
        return True
    stem = name.rstrip("0123456789")
    return stem != name and stem in blob


def main():
    blob = collect_code()
    total = 0
    for relative in TABLES:
        path = os.path.join(CSV_DIR, relative)
        if not os.path.isfile(path):
            continue
        rows = list(csv.reader(io.StringIO(open(path, "rb").read().decode("utf-8-sig"))))
        if len(rows) < 3:
            continue
        header = rows[1]
        data = [row for row in rows[2:] if len(row) > 3 and row[0].strip()]
        actionable = []
        for position, column in enumerate(header):
            name = column.strip()
            if not name or is_localized(name) or is_read(name, blob):
                continue
            filled = 0
            for row in data:
                if position < len(row) and row[position].strip() not in ("", "0"):
                    filled += 1
            if filled > 0:
                actionable.append((name, filled))
        if not actionable:
            continue
        actionable.sort(key=lambda pair: -pair[1])
        total += len(actionable)
        print("%-24s 有数据但未读 %d 列" % (relative, len(actionable)))
        for name, filled in actionable[:8]:
            print("    %-22s %d 行" % (name, filled))
    print("合计 %d 列待评估" % total)
    return 0


if __name__ == "__main__":
    sys.exit(main())