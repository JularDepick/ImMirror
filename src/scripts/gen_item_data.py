#!/usr/bin/env python3
"""从物品定义表与合成表生成客户端的物品数据与配方数据头文件.

输入:
  single-branch/assets/project/csvdef/autogen/itemdef.csv 物品定义
  single-branch/assets/project/csvdef/crafting.csv 合成配方
  single-branch/assets/project/csvdef/blockdef.csv 方块定义 (仅用于自检)
  single-branch/client/src/native/game/generated/block_data_gen.h 全量方块表
输出:
  single-branch/client/src/native/game/generated/item_data_gen.h
  single-branch/client/src/native/game/generated/recipe_data_gen.h

物品 id 与方块 id 一一对应:
  0 号是空气, 方块 1 起的物品 id 与客户端方块 id 同值, 每个客户端方块
 都有一个可放置物品, 放置与掉落通过同值关系直接换算;
 其余物品以全部方块为原料逐层合成闭包 (BFS, 无序合成口径) 排在方块之后,
 闭包只认物品表里有名有型的条目.
 客户端方块 id 到方块 ID 的对照读自生成的方块表 (kSourceIds),
 方块表缺失时回落到本脚本的 CLIENT_BLOCKS 兼容段.

配方用横占格/竖占格描述摆放区域, 每格记一种材料与数量; 同一材料出现在多格时
数量按格累加, 也就是"材料数量决定配方"的无序语义 (即如此). 因此生成时把配方
规整成按材料聚合的材料清单, kind 记 kShapeless; 有序配方匹配器同样实现,
由 SHAPED_RECIPES 显式列出需要按格位精确摆放的配方, 目前表内没有这类数据.

生成物列入 .gitignore, 由 scripts/build-native.sh 在构建前调用.
"""

import csv
import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
ASSETS_CSV = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project", "csvdef"))
ITEMDEF_PATH = os.path.join(ASSETS_CSV, "autogen", "itemdef.csv")
CRAFTING_PATH = os.path.join(ASSETS_CSV, "crafting.csv")
BLOCKDEF_PATH = os.path.join(ASSETS_CSV, "blockdef.csv")
BLOCK_DATA_PATH = os.path.join(SRC_ROOT, "native", "game", "generated", "block_data_gen.h")
OUT_ITEM_PATH = os.path.join(SRC_ROOT, "native", "game", "generated", "item_data_gen.h")
OUT_RECIPE_PATH = os.path.join(SRC_ROOT, "native", "game", "generated", "recipe_data_gen.h")

# 兼容段方块与blockdef ID 的对应关系, 与 scripts/gen_block_data.py 的
# CLIENT_BLOCKS 一致; 正常路径以 block_data_gen.h 的 kSourceIds 为准, 本表只兜底
CLIENT_BLOCKS = [
    (0, 0), (1, 100), (2, 101), (3, 104), (4, 106), (5, 4), (6, 200), (7, 218),
    (8, 1), (9, 547), (10, 505), (11, 115), (12, 123), (13, 107), (14, 206),
]

# 需要按格位精确摆放的配方 id; 配方是数量制, 这里留作后续手写有序配方的入口
SHAPED_RECIPES = set()

# 占位条目, 名称与内容都不可用
PLACEHOLDER_NAMES = ("游戏版本过低",)

NOTICE_ITEM = "由 scripts/gen_item_data.py 从 assets 仓库的 itemdef.csv 生成, 请勿手改"
NOTICE_RECIPE = "由 scripts/gen_item_data.py 从 assets 仓库的 crafting.csv 生成, 请勿手改"

# 背包筛选类型到物品类别: 1 装备, 2 工具, 3 方块, 4 材料
FILTER_TO_CATEGORY = {"1": "kEquipment", "2": "kTool", "3": "kBlock", "4": "kMaterial"}


def emit(message):
    """打印进度; 控制台编码受限时降级为 ASCII, 不让日志中断生成."""
    try:
        print(message)
    except UnicodeEncodeError:
        print(message.encode("ascii", "replace").decode("ascii"))


def to_int(text, fallback=0):
    """空串与非数字按 fallback 处理, 数据表里存在空列与非数字占位."""
    try:
        return int(str(text).strip())
    except (TypeError, ValueError):
        return fallback


def escape(text):
    """转义 C 字符串字面量里的反斜杠与双引号."""
    return text.replace("\\", "\\\\").replace('"', '\\"')


def load_table(path):
    """读取双表头 CSV, 返回以英文字段名为键的行字典列表."""
    with open(path, "r", encoding="utf-8-sig", newline="") as handle:
        rows = list(csv.reader(handle))
    if len(rows) < 3:
        raise SystemExit("数据表行数不足: %s" % path)
    header = rows[1]
    table = []
    for row in rows[2:]:
        if len(row) < len(header):
            row = row + [""] * (len(header) - len(row))
        table.append({name: row[index] for index, name in enumerate(header) if name})
    return table


def read_block_sources():
    """取客户端方块 id 到方块 ID 的对照, 优先读生成的方块数据表.

 方块表里 kSourceIds 的下标即客户端 id, 取值即方块 ID, 每行 12 个;
 读取失败或格式不符时回落到 CLIENT_BLOCKS 兼容段, 保证单独重跑物品
 生成也能工作, 两处不一致时以方块表为准并给出告警.
    """
    pairs = [(client_id, source) for client_id, source in CLIENT_BLOCKS]
    if os.path.isfile(BLOCK_DATA_PATH):
        with open(BLOCK_DATA_PATH, "r", encoding="utf-8") as handle:
            text = handle.read()
        match = re.search(r"kSourceIds\[kBlockEntryCount\] = \{(.*?)\};", text, re.DOTALL)
        if match:
            numbers = [int(value) for value in re.findall(r"(\d+)u", match.group(1))]
            parsed = list(enumerate(numbers))
            legacy_pairs = sorted(pairs)
            found_legacy = sorted(parsed[:len(pairs)])
            if legacy_pairs != found_legacy:
                emit("提示: 方块表与 CLIENT_BLOCKS 的兼容段映射不一致, 以方块表为准")
            return parsed
        emit("警告: block_data_gen.h 的 kSourceIds 无法解析, 回落到 CLIENT_BLOCKS")
    else:
        emit("警告: 未找到方块数据表 %s, 回落到 CLIENT_BLOCKS" % BLOCK_DATA_PATH)
    return pairs


def material_counts(recipe):
    """把配方的九个材料格聚合成 {物品 ID: 总数量}, 保持首次出现顺序."""
    counts = {}
    order = []
    for index in range(1, 10):
        item = to_int(recipe.get("MaterialID%d" % index))
        count = to_int(recipe.get("MaterialCount%d" % index))
        if item <= 0:
            continue
        if count <= 0:
            count = 1
        if item not in counts:
            counts[item] = 0
            order.append(item)
        counts[item] += count
    return [(item, counts[item]) for item in order]


def cell_materials(recipe):
    """按格位取九个材料格, 未填的格以 (0, 0) 占位, 供有序配方使用."""
    cells = []
    for index in range(1, 10):
        item = to_int(recipe.get("MaterialID%d" % index))
        count = to_int(recipe.get("MaterialCount%d" % index))
        cells.append((item, count if item > 0 and count > 0 else 0))
    return cells


def item_usable(row, source_id):
    """判断物品条目是否可以进入客户端物品表."""
    if source_id <= 0:
        return False
    name = (row.get("Name") or "").strip()
    if not name or name in PLACEHOLDER_NAMES:
        return False
    if (row.get("Type") or "").strip() == "":
        return False
    return True


def compute_closure(recipes, seed_sources):
    """以方块为原料逐层展开可合成闭包, 无序口径: 材料齐备即可合成."""
    available = set(seed_sources)
    rounds = []
    while True:
        added = []
        for recipe in recipes:
            result = to_int(recipe.get("ResultID"))
            if result <= 0 or result in available:
                continue
            materials = material_counts(recipe)
            if not materials:
                continue
            if all(item in available for item, _ in materials):
                available.add(result)
                added.append(result)
        if not added:
            break
        rounds.append(added)
    return available, rounds


def block_item_map(source_ids):
    """客户端方块 id 到客户端物品 id 的映射: 同值关系, 空气除外."""
    return [(client_id, 0 if client_id == 0 else client_id)
            for client_id, _ in source_ids]


def main():
    for path in (ITEMDEF_PATH, CRAFTING_PATH):
        if not os.path.isfile(path):
            raise SystemExit("数据表不存在: %s" % path)

    items = load_table(ITEMDEF_PATH)
    recipes = load_table(CRAFTING_PATH)
    items_by_id = {}
    for row in items:
        key = to_int(row.get("ID"))
        if key > 0 and key not in items_by_id:
            items_by_id[key] = row
    source_pairs = read_block_sources()
    seed_sources = [source for _, source in source_pairs]
    emit("物品 %d 条, 配方 %d 条, 客户端方块 %d 种"
         % (len(items_by_id), len(recipes), len(source_pairs)))

    closure, rounds = compute_closure(recipes, seed_sources)
    selected = [source for _, source in source_pairs]
    selected[0] = 0  # 方块 0 是空气, 物品 0 也是空气
    for source in sorted(closure):
        if source in seed_sources or source <= 0:
            continue
        if source in selected:
            continue
        if not item_usable(items_by_id.get(source, {}), source):
            continue
        selected.append(source)
    emit("可合成闭包 %d 种, 展开 %d 层; 物品表共 %d 种"
         % (len(closure), len(rounds), len(selected)))

    key_seen = {}
    item_rows = []
    source_to_block = {}
    for client_id, source in source_pairs:
        if source not in source_to_block or client_id == 0:
            source_to_block[source] = client_id
    for item_id, source in enumerate(selected):
        if source == 0:
            item_rows.append({
                "id": 0, "source": 0, "key": "Air", "name": "空气", "icon": "",
                "max_stack": 64, "category": "kBlock", "type": 1, "block": 0,
            })
            continue
        row = items_by_id.get(source, {})
        if not item_usable(row, source):
            # 方块对应的物品行缺失时用方块数据兜底, 保证物品与方块同值
            block_client_id = source_to_block.get(source)
            if block_client_id is None:
                raise SystemExit("物品 %d 不在物品表且不与方块关联" % source)
            key = "block_%d" % source
            name = "未命名"
            icon = ""
            max_stack = 64
            category = "kBlock"
            item_type = 1
            emit("提示: 方块 %d 的物品行缺失, 物品 %d 用兜底数据"
                 % (source, item_id))
        else:
            key = (row.get("Key") or "").strip()
            if not key:
                key = (row.get("ENName") or "").strip()
            if not key:
                key = "Item%d" % source
            key = re.sub(r"[^0-9A-Za-z_]", "_", key)
            if key in key_seen:
                key = "%s_%d" % (key, source)
            key_seen[key] = True
            name = (row.get("Name") or "").strip()
            icon = (row.get("Icon") or "").strip()
            max_stack = to_int(row.get("StackMax"), 64)
            if max_stack <= 0:
                max_stack = 64
            filter_type = (row.get("FilterType") or "").strip()
            category = FILTER_TO_CATEGORY.get(filter_type)
            if category is None:
                category = "kBlock" if to_int(row.get("Type")) == 1 else "kMaterial"
            item_type = to_int(row.get("Type"))
        block = source_to_block.get(source, -1)
        if block == 0:
            block = -1
        item_rows.append({
            "id": item_id, "source": source, "key": key, "name": name, "icon": icon,
            "max_stack": max_stack, "category": category, "type": item_type, "block": block,
        })

    # 物品 id 与方块 id 对齐检查: 方块 1 起的物品 id 必须与方块 id 同值
    for item_id, row in enumerate(item_rows):
        if row["block"] > 0 and row["block"] != item_id:
            raise SystemExit("方块 %d 的物品 id 未对齐: 期望 %d, 实际 %d"
                             % (row["block"], row["block"], item_id))
    block_items = [0] * len(source_pairs)
    for client_id, item_id in block_item_map(source_pairs):
        if client_id < len(block_items):
            block_items[client_id] = item_id

    # 配方: 产物与全部材料都在物品表内的才纳入, 其余留在表中待后续扩展
    source_to_item = {row["source"]: row["id"] for row in item_rows}
    kept = []
    skipped_missing = 0
    skipped_empty = 0
    for recipe in recipes:
        result_source = to_int(recipe.get("ResultID"))
        if result_source <= 0:
            skipped_empty += 1
            continue
        cells = cell_materials(recipe)
        involved = sorted(set(item for item, count in cells if item > 0))
        if result_source not in source_to_item:
            skipped_missing += 1
            continue
        if any(item not in source_to_item for item in involved):
            skipped_missing += 1
            continue
        materials = [(item, count) for item, count in material_counts(recipe)]
        if not materials:
            skipped_empty += 1
            continue
        result_count = to_int(recipe.get("ResultCount"), 1)
        if result_count <= 0:
            result_count = 1
        recipe_id = to_int(recipe.get("ID"))
        shaped = recipe_id in SHAPED_RECIPES
        grid_width = max(1, min(3, to_int(recipe.get("GridX"), 1)))
        grid_height = max(1, min(3, to_int(recipe.get("GridY"), 1)))
        if shaped:
            # 有序配方保留格位, 汇总成九格材料清单, 空位 item 记 0
            cells_out = [(source_to_item.get(item, 0), count) for item, count in cells]
            entries = cells_out
            material_count = sum(1 for item, _ in cells_out if item > 0)
        else:
            entries = [(source_to_item[item], count) for item, count in materials]
            material_count = len(entries)
            while len(entries) < 9:
                entries.append((0, 0))
        kept.append({
            "id": recipe_id, "result": source_to_item[result_source], "result_count": result_count,
            "kind": "kShaped" if shaped else "kShapeless", "grid_width": grid_width,
            "grid_height": grid_height, "material_count": material_count, "materials": entries,
        })

    emit("配方纳入 %d 条, 跳过 %d 条 (材料或产物不在物品表内), 空配方 %d 条"
         % (len(kept), skipped_missing, skipped_empty))

    os.makedirs(os.path.dirname(OUT_ITEM_PATH), exist_ok=True)
    with open(OUT_ITEM_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(render_items(item_rows, block_items))
    with open(OUT_RECIPE_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(render_recipes(kept))

    emit("物品数据表已生成: %s (%d 条)" % (OUT_ITEM_PATH, len(item_rows)))
    emit("配方数据表已生成: %s (%d 条)" % (OUT_RECIPE_PATH, len(kept)))

    blocks_linked = [row for row in item_rows if row["block"] > 0]
    tools = [row for row in item_rows if row["category"] == "kTool"]
    emit(" 方块物品 %d 种, 工具 %d 种, 材料 %d 种"
         % (len(blocks_linked), len(tools),
            sum(1 for row in item_rows if row["category"] == "kMaterial")))
    return 0


def render_items(rows, block_items):
    """拼出物品数据头文件文本."""
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "#include \"game/item/item_def.h\"",
        "",
        "/*",
        " * " + NOTICE_ITEM + ".",
        " * 数据源: single-branch/assets/project/csvdef/autogen/itemdef.csv",
        " * 条目顺序即客户端物品 id: 0 为空气, 方块 1 起的物品 id 与客户端方块 id 同值,",
        " * 每个客户端方块都有可放置物品, 之后是可合成闭包; sourceId 保留物品 ID.",
        " * block 为物品放置出的客户端方块 id, -1 表示该物品不可放置.",
        " */",
        "namespace immirror::game::gen {",
        "",
        "constexpr int32_t kItemInfoCount = %d;" % len(rows),
        "",
        "/* 条目顺序即客户端物品 id, 越界一律回落到首条 (空气). */",
        "constexpr item::ItemInfo kItemInfos[kItemInfoCount] = {",
    ]
    for row in rows:
        comment = "方块 %d" % row["block"] if row["block"] >= 0 else "无关联方块"
        lines.append(
            '    {%du, "%s", "%s", "%s", %du, item::ItemCategory::%s, %du, %d},  /* %s */'
            % (row["source"], escape(row["key"]), escape(row["name"]), escape(row["icon"]),
               row["max_stack"], row["category"], row["type"], row["block"], comment))
    lines += [
        "};",
        "",
        "/* 客户端方块 id 到物品 id 的映射, 与 voxel 方块表逐项对应. */",
        "constexpr int32_t kBlockItemCount = %d;" % len(block_items),
        "constexpr item::ItemId kBlockItems[kBlockItemCount] = {",
        "    " + ", ".join("%du" % value for value in block_items),
        "};",
        "",
        "}  /* namespace immirror::game::gen */",
        "",
    ]
    return "\n".join(lines)


def render_recipes(recipes):
    """拼出配方数据头文件文本."""
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "#include \"game/crafting/recipe.h\"",
        "",
        "/*",
        " * " + NOTICE_RECIPE + ".",
        " * 数据源: single-branch/assets/project/csvdef/crafting.csv",
        " * 只收录产物与全部材料都在客户端物品表内的配方, 网格尺寸沿用横占格与竖占格.",
        " * 无序配方的 materials 是聚合后的材料清单 (材料种类数 = materialCount),",
        " * 有序配方的 materials 按格位行优先排列, 空位 item 为 0.",
        " */",
        "namespace immirror::game::gen {",
        "",
        "constexpr int32_t kRecipeInfoCount = %d;" % len(recipes),
        "",
        "constexpr crafting::RecipeInfo kRecipeInfos[kRecipeInfoCount] = {",
    ]
    for recipe in recipes:
        cells = list(recipe["materials"])
        while len(cells) < 9:
            cells.append((0, 0))
        material_text = ", ".join("{%du, %d}" % (item, count) for item, count in cells[:9])
        lines.append(
            "    {%du, %du, %d, crafting::RecipeKind::%s, %d, %d, %d,\n"
            "     {%s}},"
            % (recipe["id"], recipe["result"], recipe["result_count"], recipe["kind"],
               recipe["grid_width"], recipe["grid_height"], recipe["material_count"],
               material_text))
    lines += [
        "};",
        "",
        "}  /* namespace immirror::game::gen */",
        "",
    ]
    return "\n".join(lines)


if __name__ == "__main__":
    sys.exit(main())
