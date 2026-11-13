"""装备战斗数值表生成: tooldef.csv -> 客户端战斗数值表."""

import csv
import io
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
CSV_PATH = os.path.join(SRC_ROOT, "..", "..", "assets", "project", "csvdef", "tooldef.csv")
OUT_PATH = os.path.join(SRC_ROOT, "native", "game", "generated", "tool_combat_gen.h")

ARMOR_COLUMNS = [("armorPhysics", "ArmorPhysics"), ("armorPunch", "ArmorPunch"),
                 ("armorRange", "ArmorRange"), ("armorExplosion", "ArmorExplosion"),
                 ("armorMagic", "ArmorMagic"), ("armorChaos", "ArmorChaos"),
                 ("armorBurn", "ArmorBurn"), ("armorToxin", "ArmorToxin")]

HEADER = """#pragma once

#include <cstdint>

/*
 * 由 scripts/gen_tool_combat.py 生成, 请勿手改.
 * 只含参与战斗结算的列: 类型, 等级, 附魔位, 近战攻击与八类护甲百分比减伤.
 * 类型 8 头盔, 9 胸甲, 10 护腿, 11 靴子; 其余值属于工具与武器.
 */
namespace immirror::game::gen {

struct ToolCombatData {
    uint16_t id;
    uint8_t type;
    uint8_t level;
    uint16_t enchant;
    float attackPunch;
    float armorPhysics;
    float armorPunch;
    float armorRange;
    float armorExplosion;
    float armorMagic;
    float armorChaos;
    float armorBurn;
    float armorToxin;
    float moveSpeed;
    /* 其余六类攻击值: 远程, 爆炸, 混沌, 燃烧, 剧毒, 法术. */
    float attackRange;
    float attackExplosion;
    float attackChaos;
    float attackBurn;
    float attackToxin;
    /* 修理配方: 六组材料与数量, 物品编号 0 表示该组不用. */
    uint16_t repairItem[6];
    uint16_t repairAmount[6];
    /* 音效路径: 攻击, 发射与使用; 空串表示没有. */
    const char* atkSound;
    const char* fireSound;
    const char* useSound;
};
"""


def sound_asset(row, index, column):
    """音效名转资源路径: 点分名扁平化成下划线, 与打包后的命名一致."""
    position = index.get(column)
    if position is None or position >= len(row):
        return ""
    value = (row[position] or "").strip()
    if not value:
        return ""
    return "sounds/" + value.replace(".", "_") + ".ogg"


def to_int(text):
    try:
        return int(float(str(text).strip()))
    except (TypeError, ValueError):
        return 0


def main():
    raw = open(CSV_PATH, "rb").read().decode("utf-8-sig")
    rows = list(csv.reader(io.StringIO(raw)))
    if len(rows) < 3:
        print("装备表为空")
        return 1
    header = rows[1]
    index = {name: position for position, name in enumerate(header)}
    needed = ["ID", "Type", "Level", "AttackPunch", "Enchant", "MoveSpeed", "AttackRange",
              "AttackExplosion", "AttackChaos", "AttackBurn", "AttackToxin",
              "AtkSound", "FireSound", "UseSound"] + \
             ["RepairID%d" % i for i in range(1, 7)] + \
             ["RepairAmount%d" % i for i in range(1, 7)] + [c for _f, c in ARMOR_COLUMNS]
    missing = [name for name in needed if name not in index]
    if missing:
        print("装备表缺少列: %s" % missing)
        return 1

    entries = []
    seen = set()
    for row in rows[2:]:
        source_id = to_int(row[index["ID"]]) if len(row) > index["ID"] else 0
        if source_id <= 0 or source_id in seen:
            continue
        seen.add(source_id)

        def cell(column):
            position = index[column]
            return to_int(row[position]) if len(row) > position else 0

        entry = {
            "id": source_id,
            "type": cell("Type"),
            "level": cell("Level"),
            "attack": cell("AttackPunch"),
            "enchant": cell("Enchant"),
            "moveSpeed": cell("MoveSpeed"),
            "attackRange": cell("AttackRange"),
            "attackExplosion": cell("AttackExplosion"),
            "attackChaos": cell("AttackChaos"),
            "attackBurn": cell("AttackBurn"),
            "attackToxin": cell("AttackToxin"),
            "atkSound": sound_asset(row, index, "AtkSound"),
            "fireSound": sound_asset(row, index, "FireSound"),
            "useSound": sound_asset(row, index, "UseSound"),
        }
        for field, column in ARMOR_COLUMNS:
            entry[field] = cell(column)
        for slot in range(6):
            entry["repairItem%d" % slot] = cell("RepairID%d" % (slot + 1))
            entry["repairAmount%d" % slot] = cell("RepairAmount%d" % (slot + 1))
        entries.append(entry)
    entries.sort(key=lambda item: item["id"])

    out = [HEADER]
    out.append("constexpr int32_t kToolCombatCount = %d;" % len(entries))
    out.append("constexpr ToolCombatData kToolCombat[kToolCombatCount] = {")
    for e in entries:
        out.append(
            "    {%du, %du, %du, %du, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff, "
            "%.1ff, %.1ff, %.1ff, %.1ff, %.1ff, %du, %du, %du, %du, %du, %du, %du, %du, %du, %du, %du, %du, "
            '"%s", "%s", "%s"},'
            % (e["id"], e["type"], e["level"], e["enchant"], e["attack"], e["armorPhysics"],
               e["armorPunch"], e["armorRange"], e["armorExplosion"], e["armorMagic"],
               e["armorChaos"], e["armorBurn"], e["armorToxin"], e["moveSpeed"],
               e["attackRange"], e["attackExplosion"], e["attackChaos"], e["attackBurn"],
               e["attackToxin"],
               e["repairItem0"], e["repairAmount0"], e["repairItem1"], e["repairAmount1"],
               e["repairItem2"], e["repairAmount2"], e["repairItem3"], e["repairAmount3"],
               e["repairItem4"], e["repairAmount4"], e["repairItem5"], e["repairAmount5"],
               e["atkSound"], e["fireSound"], e["useSound"]))
    out.append("};")
    out.append("")
    out.append("}  // namespace immirror::game::gen")
    out.append("")
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    open(OUT_PATH, "w", encoding="utf-8", newline="\n").write("\n".join(out))
    print("战斗数值行 %d" % len(entries))
    return 0


if __name__ == "__main__":
    sys.exit(main())
