#!/usr/bin/env python3
"""从工具定义表生成客户端的工具属性表.

输入: 素材仓库 csvdef/tooldef.csv
      native/game/generated/item_data_gen.h (已生成的物品表, 含客户端 id 与中文名)
输出: native/game/generated/tool_data_gen.h

关联方式: 工具表与物品表分属两套编号, 两者靠中文名对应.
物品表是生成产物, 这里直接读它的客户端 id 与中文名做匹配,
不重复物品生成器的展开逻辑, 避免两处口径漂移.
名称对不上的工具不生成条目, 挖掘时按徒手处理.
"""

import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
TOOLDEF_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project",
                                             "csvdef", "tooldef.csv"))
ITEM_HEADER = os.path.join(SRC_ROOT, "native", "game", "generated", "item_data_gen.h")
OUT_PATH = os.path.join(SRC_ROOT, "native", "game", "generated", "tool_data_gen.h")

# 工具类型: 与工具表的 Type 列一致, 只收挖掘相关的类型.
# 1 斧 2 镐 3 铲 4 锄 5 剪刀 6 通用工具; 护甲, 弓, 容器不进本表.
DIG_TYPES = {1, 2, 3, 4, 5, 6}

# 类型到采集工具档位的折算, 与 voxel::HarvestTool 的枚举顺序一致.
# 锄与剪刀与通用工具没有对应档位, 按徒手处理, 只保留效率.
TYPE_TO_HARVEST = {1: 1, 2: 2, 3: 3, 4: 0, 5: 0, 6: 0}

# 名字里必须出现下列之一才算挖掘工具. 工具表把连接器, 木棒一类也归进了
# 斧类与通用工具类, 只看类型列会把它们当成工具, 挖掘时给出错误的效率.
TOOL_NAME_HINTS = ("斧", "镐", "铲", "锄", "剪刀", "钻", "锯")

# 物品表里的条目行: {sourceId, "key", "中文名", "icon", 堆叠, 类别, type, block}
ITEM_ROW = re.compile(r'\{(\d+)u, "([^"]*)", "([^"]*)",')


def load_tools():
    """读取工具表, 返回 中文名 -> (类型, 等级, 效率, 耐久)."""
    import csv
    import io

    if not os.path.isfile(TOOLDEF_PATH):
        raise SystemExit("工具表不存在: %s" % TOOLDEF_PATH)
    rows = list(csv.reader(io.StringIO(open(TOOLDEF_PATH, "rb").read().decode("utf-8-sig"))))
    header = rows[1]
    index = {name: position for position, name in enumerate(header)}
    tools = {}
    for row in rows[2:]:
        if len(row) <= index["Duration"]:
            continue
        name = (row[index["Name"]] or "").strip()
        if not name:
            continue
        try:
            tool_type = int((row[index["Type"]] or "0").strip() or 0)
        except ValueError:
            continue
        if tool_type not in DIG_TYPES:
            continue
        if not any(hint in name for hint in TOOL_NAME_HINTS):
            continue
        try:
            level = int((row[index["Level"]] or "1").strip() or 1)
            efficiency = int((row[index["Efficiency"]] or "50").strip() or 50)
            durability = int((row[index["Duration"]] or "1").strip() or 1)
        except ValueError:
            continue
        if level <= 0:
            level = 1
        if efficiency <= 0:
            efficiency = 50
        if durability <= 0:
            durability = 1
        # 同名工具以先出现的为准
        if name in tools:
            continue
        tools[name] = (tool_type, level, efficiency, durability)
    return tools


def load_items():
    """从已生成的物品表里取出 客户端 id 与中文名.

    物品表的条目按客户端 id 升序排列, 数组下标就是客户端 id;
    行首的数字是原版物品 ID, 不能当作客户端 id 用.
    """
    if not os.path.isfile(ITEM_HEADER):
        raise SystemExit("物品表未生成, 请先运行 gen_item_data.py: %s" % ITEM_HEADER)
    items = []
    with open(ITEM_HEADER, "r", encoding="utf-8") as handle:
        inside = False
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
            name = match.group(3).strip()
            if not name:
                continue
            items.append((len(items), name))
    return items


def render(entries):
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/*",
        " * 由 scripts/gen_tool_data.py 生成, 请勿手改.",
        " * 客户端物品 id 到工具属性的对照表, 只收挖掘类工具.",
        " * harvest 与 voxel::HarvestTool 的枚举顺序一致, 0 表示不按工具族加速.",
        " * efficiency 是相对徒手的效率百分比, 50 表示比徒手快一倍.",
        " */",
        "namespace immirror::game::gen {",
        "",
        "struct ToolEntry {",
        "    uint16_t itemId;     /* 客户端物品 id */",
        "    uint8_t harvest;     /* 采集工具档位, 与 HarvestTool 对齐 */",
        "    uint8_t level;       /* 工具等级, 1 起 */",
        "    uint16_t efficiency; /* 效率百分比, 徒手为 100 */",
        "    uint16_t durability; /* 满耐久, 每次破坏扣 1 */",
        "};",
        "",
        "constexpr int32_t kToolEntryCount = %d;" % len(entries),
        "",
        "constexpr ToolEntry kToolEntries[kToolEntryCount == 0 ? 1 : kToolEntryCount] = {",
    ]
    for item_id, harvest, level, efficiency, durability in entries:
        lines.append("    {%du, %du, %du, %du, %du}," % (item_id, harvest, level, efficiency, durability))
    if not entries:
        lines.append("    {0u, 0u, 0u, 0u, 0u},")
    lines.extend([
        "};",
        "",
        "/* 按客户端物品 id 取工具条目, 不是工具时返回空指针. */",
        "inline const ToolEntry* ToolOfItem(uint16_t itemId) {",
        "    for (int32_t index = 0; index < kToolEntryCount; ++index) {",
        "        if (kToolEntries[index].itemId == itemId) {",
        "            return &kToolEntries[index];",
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
    tools = load_tools()
    items = load_items()
    entries = []
    for client_id, name in items:
        if name not in tools:
            continue
        tool_type, level, efficiency, durability = tools[name]
        entries.append((client_id, TYPE_TO_HARVEST.get(tool_type, 0), level, efficiency, durability))
    entries.sort()
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(render(entries))
    print("工具表: 原表 %d 条, 物品 %d 条, 挂到物品 %d 条" % (len(tools), len(items), len(entries)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
