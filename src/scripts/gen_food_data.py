#!/usr/bin/env python3
"""从食物表生成客户端的食物属性表.

输入: 素材仓库 csvdef/fooddef.csv
      native/game/generated/item_data_gen.h (已生成的物品表)
输出: native/game/generated/food_data_gen.h

食物表与物品表靠原版物品 ID 关联: 食物表的 ID 就是物品表里的 sourceId.
附加状态按概率触发, 概率为万分比; 同一食物最多三条附加状态.
"""

import os
import re
import csv
import io
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
FOOD_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project",
                                          "csvdef", "fooddef.csv"))
ITEM_HEADER = os.path.join(SRC_ROOT, "native", "game", "generated", "item_data_gen.h")
OUT_PATH = os.path.join(SRC_ROOT, "native", "game", "generated", "food_data_gen.h")

# 物品表里的条目行: {sourceId, "key", "中文名", "icon", 堆叠, 类别, type, block}
ITEM_ROW = re.compile(r'\{(\d+)u, "([^"]*)", "([^"]*)",')


def to_int(text, fallback=0):
    try:
        return int(float((text or "").strip() or fallback))
    except ValueError:
        return fallback


def load_items():
    """从已生成的物品表取 原版物品 ID -> 客户端物品 id."""
    if not os.path.isfile(ITEM_HEADER):
        raise SystemExit("物品表未生成, 请先运行 gen_item_data.py: %s" % ITEM_HEADER)
    mapping = {}
    with open(ITEM_HEADER, "r", encoding="utf-8") as handle:
        inside = False
        index = 0
        for line in handle:
            stripped = line.strip()
            if "kItemInfos[" in stripped and "constexpr" in stripped:
                inside = True
                continue
            if not inside:
                continue
            if stripped.startswith("};"):
                break
            match = ITEM_ROW.search(stripped)
            if match is None:
                continue
            source_id = int(match.group(1))
            if source_id > 0:
                mapping[source_id] = index
            index += 1
    return mapping


def load_foods():
    """读取食物表, 返回按原版 ID 升序的条目列表."""
    if not os.path.isfile(FOOD_PATH):
        raise SystemExit("食物表不存在: %s" % FOOD_PATH)
    rows = list(csv.reader(io.StringIO(open(FOOD_PATH, "rb").read().decode("utf-8-sig"))))
    header = rows[1]
    index = {name: position for position, name in enumerate(header)}
    entries = []
    for row in rows[2:]:
        if len(row) <= index["AddFood"]:
            continue
        source_id = to_int(row[index["ID"]])
        if source_id <= 0:
            continue
        food = to_int(row[index["AddFood"]])
        heal = to_int(row[index["HealAmount"]])
        buffs = []
        for slot in (1, 2, 3):
            buff_id = to_int(row[index["BuffID%d" % slot]])
            odds = to_int(row[index["BuffOdds%d" % slot]])
            if buff_id <= 0 or odds <= 0:
                continue
            buffs.append((buff_id, odds))
        if food <= 0 and heal <= 0 and not buffs:
            continue
        entries.append({
            "source": source_id,
            "name": (row[index["Name"]] or "").strip(),
            "food": max(0, food),
            "heal": max(0, heal),
            "buffs": buffs[:3],
        })
    entries.sort(key=lambda item: item["source"])
    return entries


def render(entries):
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/*",
        " * 由 scripts/gen_food_data.py 生成, 请勿手改.",
        " * 可食用物品的属性: 恢复饥饿度, 治疗量, 以及按概率触发的附加状态.",
        " * buffOdds 是万分比, 0 表示该条不触发.",
        " */",
        "namespace immirror::game::gen {",
        "",
        "struct FoodEntry {",
        "    uint16_t itemId;   /* 客户端物品 id */",
        "    uint16_t food;     /* 恢复的饥饿度 */",
        "    uint16_t heal;     /* 恢复的生命值 */",
        "    uint32_t buffId1;",
        "    uint16_t buffOdds1; /* 万分比 */",
        "    uint32_t buffId2;",
        "    uint16_t buffOdds2;",
        "    uint32_t buffId3;",
        "    uint16_t buffOdds3;",
        "    const char* name;",
        "};",
        "",
        "constexpr int32_t kFoodEntryCount = %d;" % len(entries),
        "",
        "constexpr FoodEntry kFoodEntries[kFoodEntryCount == 0 ? 1 : kFoodEntryCount] = {",
    ]
    for entry in entries:
        buffs = list(entry["buffs"]) + [(0, 0)] * (3 - len(entry["buffs"]))
        lines.append('    {%du, %du, %du, %du, %du, %du, %du, %du, %du, "%s"},'
                     % (entry["item"], entry["food"], entry["heal"],
                        buffs[0][0], buffs[0][1], buffs[1][0], buffs[1][1],
                        buffs[2][0], buffs[2][1], entry["name"]))
    if not entries:
        lines.append('    {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, ""},')
    lines.extend([
        "};",
        "",
        "/* 按客户端物品 id 取食物条目, 不可食用返回空指针. */",
        "inline const FoodEntry* FoodOfItem(uint16_t itemId) {",
        "    for (int32_t index = 0; index < kFoodEntryCount; ++index) {",
        "        if (kFoodEntries[index].itemId == itemId) {",
        "            return &kFoodEntries[index];",
        "        }",
        "    }",
        "    return nullptr;",
        "}",
        "",
        "}  /* namespace immirror::game::gen */",
        "",
    ])
    return "\n".join(lines)


def main():
    mapping = load_items()
    entries = []
    skipped = 0
    for entry in load_foods():
        client_id = mapping.get(entry["source"])
        if client_id is None:
            skipped += 1
            continue
        entry["item"] = client_id
        entries.append(entry)
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(render(entries))
    print("食物表: 原表 %d 条, 挂到物品 %d 条, 跳过 %d 条"
          % (len(entries) + skipped, len(entries), skipped))
    return 0


if __name__ == "__main__":
    sys.exit(main())
