#!/usr/bin/env python3
"""从附魔定义表生成客户端 C++ 附魔数据头文件.

输入: single-branch/assets/project/csvdef/autogen/enchant.csv (双表头)
输出: single-branch/client/src/native/game/generated/enchant_data_gen.h

只保留客户端能消费的列: 编号, 名称, 效果类型, 附魔等级, 两个效果数值,
冲突编号与工具类型掩码 (工具类型 1 到 12 折成位掩码, 0 表示不限工具).
属性描述按 UTF-8 原样带入, 供界面展示.
"""

import csv
import io
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
ENCHANT_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project",
                                             "csvdef", "autogen", "enchant.csv"))
OUT_PATH = os.path.join(SRC_ROOT, "native", "game", "generated", "enchant_data_gen.h")

NOTICE = "由 scripts/gen_enchant_data.py 从 assets 仓库的 autogen/enchant.csv 生成, 请勿手改"
TOOL_TYPE_COUNT = 12


def emit(message):
    try:
        print(message)
    except UnicodeEncodeError:
        print(message.encode("ascii", "replace").decode("ascii"))


def to_int(text, fallback=0):
    try:
        return int(str(text).strip())
    except (TypeError, ValueError):
        return fallback


def c_string(text):
    """转成 C 字符串字面量, 转义引号与反斜杠."""
    return '"' + str(text).replace("\\", "\\\\").replace('"', '\\"') + '"'


def main():
    rows = list(csv.reader(io.StringIO(
        open(ENCHANT_PATH, "rb").read().decode("utf-8-sig"))))
    if len(rows) < 3:
        raise SystemExit("附魔表为空")
    header = rows[1]
    col = {name: index for index, name in enumerate(header)}
    required = ("ID", "Name", "EnchantType", "EnchantLevel", "EnchantValue1", "EnchantValue2",
                "ConflictID")
    for name in required:
        if name not in col:
            raise SystemExit("附魔表缺列: %s" % name)

    entries = []
    seen = set()
    for row in rows[2:]:
        if len(row) <= col["ID"]:
            continue
        source_id = to_int(row[col["ID"]])
        level = to_int(row[col["EnchantLevel"]])
        # 同一附魔按等级分行: 保留 (编号, 等级) 全部组合, 供按等级查数值.
        key = (source_id, level)
        if source_id <= 0 or key in seen:
            continue
        seen.add(key)
        # 工具类型列存的是"适用工具类型编号", 不是标志位:
        # 把编号折成位掩码 (编号 1 对应第 0 位), 编号超过 32 的忽略.
        mask = 0
        for index in range(1, TOOL_TYPE_COUNT + 1):
            column = "ToolType%d" % index
            if column not in col or col[column] >= len(row):
                continue
            tool_type = to_int(row[col[column]])
            if 1 <= tool_type <= 32:
                mask |= 1 << (tool_type - 1)
        entries.append({
            "sourceId": source_id,
            "name": row[col["Name"]].strip(),
            # 效果类型是枚举字符串 (SHARP, DIG_SPEED, DURABLE 一类), 不是数字.
            "type": row[col["EnchantType"]].strip(),
            "level": level,
            "value1": to_int(row[col["EnchantValue1"]]),
            "value2": to_int(row[col["EnchantValue2"]]),
            "conflict": to_int(row[col["ConflictID"]]),
            "toolMask": mask,
            "desc": row[col["AttrDesc"]].strip() if "AttrDesc" in col and col["AttrDesc"] < len(row) else "",
        })

    if not entries:
        raise SystemExit("附魔条目为空")

    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/* %s */" % NOTICE,
        "/* 数据源: single-branch/assets/project/csvdef/autogen/enchant.csv */",
        "/* toolMask 的第 n 位对应工具类型 n+1, 0 表示不限工具. */",
        "",
        "namespace immirror::game::gen {",
        "",
        "/* 单条附魔定义. */",
        "struct EnchantEntry {",
        "    uint32_t sourceId;   /* 附魔表 ID */",
        "    const char* name;    /* 中文名, UTF-8 编码 */",
        "    const char* type;    /* 效果类型枚举名, 如 SHARP / DIG_SPEED / DURABLE */",
        "    uint16_t level;      /* 附魔等级 */",
        "    int32_t value1;      /* 效果数值 1 */",
        "    int32_t value2;      /* 效果数值 2 */",
        "    uint32_t conflict;   /* 冲突编号, 0 表示不冲突 */",
        "    uint32_t toolMask;   /* 适用工具类型位掩码, 第 n 位对应类型 n+1 */",
        "    const char* desc;    /* 属性描述, 供界面展示 */",
        "};",
        "",
        "constexpr int32_t kEnchantEntryCount = %d;" % len(entries),
        "",
        "constexpr EnchantEntry kEnchantEntries[kEnchantEntryCount] = {",
    ]
    for entry in entries:
        lines.append(
            '    {%du, %s, %s, %du, %d, %d, %du, %du, %s},'
            % (entry["sourceId"], c_string(entry["name"]), c_string(entry["type"]), entry["level"],
               entry["value1"], entry["value2"], entry["conflict"], entry["toolMask"],
               c_string(entry["desc"])))
    lines += [
        "};",
        "",
        "}  /* namespace immirror::game::gen */",
        "",
    ]
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    open(OUT_PATH, "w", encoding="utf-8", newline="\n").write("\n".join(lines))
    emit("生成: %s" % OUT_PATH)
    emit("附魔条目: %d 条" % len(entries))
    return 0


if __name__ == "__main__":
    sys.exit(main())
