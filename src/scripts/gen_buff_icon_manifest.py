"""状态效果图标清单: 由效果表与图标素材生成 效果编号 -> 素材文件 的对照表."""

import csv
import io
import json
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
ASSETS = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project"))
BUFF_CSV = os.path.join(ASSETS, "csvdef", "autogen", "buffdef.csv")
SEARCH_DIRS = ("items", "res", "blocks")
OUT_PATH = os.path.join(SRC_ROOT, "native", "ui", "generated", "buff_icon_manifest.json")


def main():
    if not os.path.isfile(BUFF_CSV):
        print("效果表不存在")
        return 1
    available = {}
    for sub in SEARCH_DIRS:
        path = os.path.join(ASSETS, sub)
        if os.path.isdir(path):
            available[sub] = set(os.listdir(path))
    rows = list(csv.reader(io.StringIO(open(BUFF_CSV, "rb").read().decode("utf-8-sig"))))
    header = rows[1]
    index = {name: position for position, name in enumerate(header)}
    if "IconName" not in index or "ID" not in index:
        print("效果表缺少图标列")
        return 1
    resolved = {}
    for row in rows[2:]:
        if len(row) <= max(index["IconName"], index["ID"]):
            continue
        buff_id = row[index["ID"]].strip()
        icon = row[index["IconName"]].strip()
        if not buff_id.isdigit() or not icon:
            continue
        for sub in SEARCH_DIRS:
            if icon + ".png" in available.get(sub, ()):
                resolved[buff_id] = sub + "/" + icon + ".png"
                break
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        json.dump({"count": len(resolved), "buffs": resolved}, handle, ensure_ascii=False,
                  indent=0, sort_keys=True)
    print("效果图标清单: 可解析 %d 条" % len(resolved))
    return 0


if __name__ == "__main__":
    sys.exit(main())