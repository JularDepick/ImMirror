"""物品图标清单: 由物品表与图标素材生成 图标名 -> 素材文件 的对照表."""

import csv
import io
import json
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
ASSETS = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project"))
ITEM_CSV = os.path.join(ASSETS, "csvdef", "autogen", "itemdef.csv")
ICON_DIR = os.path.join(ASSETS, "items")
OUT_PATH = os.path.join(SRC_ROOT, "native", "ui", "generated", "item_icon_manifest.json")


def main():
    if not os.path.isfile(ITEM_CSV) or not os.path.isdir(ICON_DIR):
        print("物品表或图标目录不存在")
        return 1
    rows = list(csv.reader(io.StringIO(open(ITEM_CSV, "rb").read().decode("utf-8-sig"))))
    header = rows[1]
    index = {name: position for position, name in enumerate(header)}
    if "Icon" not in index or "ID" not in index:
        print("物品表缺少图标列")
        return 1
    files = set(os.listdir(ICON_DIR))
    resolved = {}
    missing = 0
    for row in rows[2:]:
        if len(row) <= max(index["Icon"], index["ID"]):
            continue
        item_id = row[index["ID"]].strip()
        icon = row[index["Icon"]].strip()
        if not item_id.isdigit() or not icon:
            continue
        # 图标名可能带前导方括号等装饰, 取最后一段可读名字.
        candidate = icon.lstrip("[")
        for name in (icon, candidate):
            if name + ".png" in files:
                resolved[item_id] = "items/" + name + ".png"
                break
        else:
            missing += 1
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        json.dump({"count": len(resolved), "items": resolved}, handle, ensure_ascii=False,
                  indent=0, sort_keys=True)
    print("物品图标清单: 可解析 %d 条, 无素材 %d 条" % (len(resolved), missing))
    return 0


if __name__ == "__main__":
    sys.exit(main())