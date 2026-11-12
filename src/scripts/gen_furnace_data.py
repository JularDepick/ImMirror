#!/usr/bin/env python3
"""从原版熔炼定义表生成客户端 C++ 熔炉数据头文件.

输入: single-branch/assets/project/csvdef/furnace.csv (双表头, 97 条)
输出: single-branch/client/src/native/game/generated/furnace_data_gen.h

原版表的行分两类, 与列结构对齐:
  配方行: Result 列有值, 记一次熔炼的材料与产出;
  燃料行: Result 列为空且 Combustion 大于 0, 记一种可燃烧物及其燃烧值与热量;
  其余行 (既无产出也无燃烧值) 是编辑器占位, 不收录.

物品 id 折算: 两表的物品引用都是原版 itemdef 的物品 ID, 与客户端物品 id 不同域.
折算读 gen_item_data.py 生成的 item_data_gen.h, 其数组顺序即客户端物品 id,
条目首字段是原版 sourceId, 按 sourceId 对齐换算; 材料或产出不在客户端物品表内的
条目跳过并计数, 覆盖数与跳过数见生成日志.
"""

import csv
import io
import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
FURNACE_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project",
                                            "csvdef", "furnace.csv"))
ITEM_GEN_PATH = os.path.join(SRC_ROOT, "native", "game", "generated", "item_data_gen.h")
OUT_PATH = os.path.join(SRC_ROOT, "native", "game", "generated", "furnace_data_gen.h")

NOTICE = "由 scripts/gen_furnace_data.py 从 assets 仓库的 csvdef/furnace.csv 生成, 请勿手改"


def emit(message):
    """打印进度. 控制台编码受限时降级为 ASCII, 不让日志中断生成."""
    try:
        print(message)
    except UnicodeEncodeError:
        print(message.encode("ascii", "replace").decode("ascii"))


def load_item_mapping(path):
    """从 item_data_gen.h 折算 sourceId -> clientId 映射.

    生成表的 kItemInfos 数组元素顺序即客户端物品 id, 条目首字段是原版 sourceId,
    用宽松的正则逐条捕获, 不依赖字段内部的具体排版.
    """
    text = open(path, encoding="utf-8").read()
    start = text.index("kItemInfos[")
    body_start = text.index("{", text.index("= {", start))
    body_end = text.index("};", body_start)
    mapping = {}
    for match in re.finditer(r"\{(\d+)u,", text[body_start:body_end]):
        source_id = int(match.group(1))
        if source_id not in mapping:
            # 同一 sourceId 出现多次时取首个, 与 ItemTable 的首条优先一致
            mapping[source_id] = len(mapping)
    return mapping


def main():
    mapping = load_item_mapping(ITEM_GEN_PATH)
    rows = list(csv.reader(io.StringIO(
        open(FURNACE_PATH, "rb").read().decode("utf-8-sig"))))
    header = rows[1]
    col = {name: index for index, name in enumerate(header)}

    recipes = []
    fuels = []
    skipped_recipe_result = 0
    skipped_recipe_material = 0
    skipped_fuel_material = 0
    skipped_other = 0
    seen_ids = set()
    for row in rows[2:]:
        if len(row) <= col["Result"]:
            continue
        source_id_text = row[col["ID"]].strip()
        if not source_id_text:
            continue
        source_id = int(source_id_text)
        if source_id in seen_ids:
            continue
        seen_ids.add(source_id)

        material_source = int(row[col["MaterialID"]].strip() or 0)
        result_text = row[col["Result"]].strip()
        combustion_text = row[col["Combustion"]].strip()

        if result_text and result_text != "0":
            # 配方行: 材料与产出都要在客户端物品表内
            material_item = mapping.get(material_source)
            result_item = mapping.get(int(result_text))
            if material_item is None:
                skipped_recipe_material += 1
                continue
            if result_item is None:
                skipped_recipe_result += 1
                continue
            recipes.append({
                "sourceId": source_id,
                "materialItem": material_item,
                "resultItem": result_item,
                "key": row[col["Key"]].strip(),
                "nameZh": row[col["Name"]].strip(),
                "furnaceType": int(row[col["Type"]].strip() or 0),
            })
        elif combustion_text and combustion_text != "0":
            # 燃料行: 燃烧值以原版 0-1000 的整数记录, 折算成 0-1 的比例保留两位
            material_item = mapping.get(material_source)
            if material_item is None:
                skipped_fuel_material += 1
                continue
            combustion = int(combustion_text)
            fuels.append({
                "item": material_item,
                "combustion": combustion,
            })

        else:
            skipped_other += 1

    lines = []
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <cstdint>")
    lines.append("")
    lines.append("/* %s */" % NOTICE)
    lines.append("/* 数据源: single-branch/assets/project/csvdef/furnace.csv */")
    lines.append("/* 原版表行分两类: Result 有值的是配方行, Result 为空且燃烧值大于零的是燃料行. */")
    lines.append("/* 物品引用已按 item_data_gen.h 的 sourceId 映射折算成客户端物品 id, */")
    lines.append("/* 材料或产出不在客户端物品表内的条目跳过, 数量见生成日志. */")
    lines.append("")
    lines.append("namespace immirror::game::gen {")
    lines.append("")
    lines.append("/* 熔炉类型: 0 熔炉, 1 氧气提炼装置, 与原版 Type 列一致. */")
    lines.append("struct FurnaceEntry {")
    lines.append("    uint32_t sourceId;     /* 原版 furnace.csv 的熔炼 ID */")
    lines.append("    uint16_t materialItem; /* 投入材料的客户端物品 id */")
    lines.append("    uint16_t resultItem;   /* 熔炼产出的客户端物品 id */")
    lines.append("    const char* key;       /* 原版英文键, 供日志与后续界面引用 */")
    lines.append("    const char* nameZh;    /* 原版中文名, UTF-8 编码 */")
    lines.append("    uint8_t furnaceType;   /* 熔炉类型 */")
    lines.append("};")
    lines.append("")
    lines.append("/* 一条燃料定义: 燃烧物与燃烧值, 燃烧值是原版 0-1000 的整数. */")
    lines.append("struct FuelEntry {")
    lines.append("    uint16_t item;      /* 可燃烧物的客户端物品 id */")
    lines.append("    uint16_t combustion; /* 原版燃烧值, 越大烧得越久 */")
    lines.append("};")
    lines.append("")
    lines.append("constexpr int32_t kFurnaceEntryCount = %d;" % len(recipes))
    lines.append("constexpr int32_t kFuelEntryCount = %d;" % len(fuels))
    lines.append("")
    lines.append("constexpr FurnaceEntry kFurnaceEntries[kFurnaceEntryCount] = {")
    for entry in recipes:
        lines.append("    {%du, %uu, %uu, \"%s\", \"%s\", %uu},"
                     % (entry["sourceId"], entry["materialItem"], entry["resultItem"],
                        entry["key"], entry["nameZh"], entry["furnaceType"]))
    lines.append("};")
    lines.append("")
    lines.append("constexpr FuelEntry kFuelEntries[kFuelEntryCount] = {")
    for fuel in fuels:
        lines.append("    {%uu, %uu}," % (fuel["item"], fuel["combustion"]))
    lines.append("};")
    lines.append("")
    lines.append("}  /* namespace immirror::game::gen */")
    lines.append("")

    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    open(OUT_PATH, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
    emit("生成: %s" % OUT_PATH)
    emit("配方收录: %d 条 (材料缺映射 %d, 产出缺映射 %d)" % (len(recipes), skipped_recipe_material, skipped_recipe_result))
    emit("燃料收录: %d 条 (材料缺映射 %d)" % (len(fuels), skipped_fuel_material))
    emit("占位行跳过: %d 条" % skipped_other)
    emit("总覆盖: %d 条" % (len(recipes) + len(fuels)))


if __name__ == "__main__":
    sys.exit(main())
