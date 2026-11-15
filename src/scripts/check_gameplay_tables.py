#!/usr/bin/env python3
"""玩法域基线断言: 各生成表的规模必须与基线一致, 且规模非零."""

import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.normpath(os.path.join(ROOT, "..", "..", "..", ".agent", "gameplay-baseline", "tables.md"))

# (生成表相对 native 的路径, 常量名, 基线值, 说明)
TABLES = [
    ("game/generated/item_data_gen.h", "kItemInfoCount", 1631, "物品"),
    ("game/generated/item_data_gen.h", "kBlockItemCount", 1191, "方块物品"),
    ("game/generated/recipe_data_gen.h", "kRecipeInfoCount", 630, "合成配方"),
    ("game/generated/food_data_gen.h", "kFoodEntryCount", 95, "食物"),
    ("game/generated/furnace_data_gen.h", "kFurnaceEntryCount", 34, "熔炉配方"),
    ("game/generated/furnace_data_gen.h", "kFuelEntryCount", 52, "燃料"),
    ("game/generated/tool_data_gen.h", "kToolEntryCount", 25, "工具"),
    ("game/generated/tool_combat_gen.h", "kToolCombatCount", 238, "战斗工具"),
    ("game/generated/enchant_data_gen.h", "kEnchantEntryCount", 120, "附魔"),
    ("game/generated/skin_data_gen.h", "kSkinEntryCount", 110, "皮肤"),
    ("entity/generated/monster_data_gen.h", "kMonsterDataCount", 280, "怪物"),
    ("player/generated/buff_data_gen.h", "kBuffEntryCount", 44, "增益"),
    ("progress/generated/achievement_data_gen.h", "kAchievementDataCount", 142, "成就"),
    ("progress/generated/daily_task_data_gen.h", "kDailyTaskTemplateCount", 14, "日常模板"),
    ("ui/generated/item_icon_atlas_gen.h", "kItemIconCount", 1732, "物品图标"),
    ("ui/generated/item_icon_atlas_gen.h", "kBuffIconCount", 86, "增益图标"),
    ("game/generated/block_data_gen.h", "kBlockEntryCount", 1191, "方块"),
    ("game/generated/block_data_gen.h", "kLegacyBlockCount", 15, "兼容段方块"),
    ("voxel/generated/biome_data_gen.h", "kBiomeBandCount", 6, "群系带"),
    ("ui/generated/string_data_gen.h", "kStringEntryCount", 9595, "字符串"),
    ("ui/generated/ui_font_index_gen.h", "kUiFontCount", 25, "字体"),
]

failures = []


def check(label, ok, detail=""):
    if ok:
        print("ok   %s" % label)
    else:
        print("FAIL %s %s" % (label, detail))
        failures.append("%s %s" % (label, detail))


def main():
    lines_out = ["# 玩法域生成表基线", "",
                 "| 生成表 | 常量 | 说明 | 当前值 | 基线 | 一致 |",
                 "|:---:|:---:|:---:|:---:|:---:|:---:|"]
    for rel, const, baseline, note in TABLES:
        path = os.path.join(ROOT, "native", rel)
        if not os.path.exists(path):
            check("%s 存在" % rel, False)
            lines_out.append("| %s | %s | %s | (缺) | %d | 否 |" % (rel, const, note, baseline))
            continue
        body = io.open(path, encoding="utf-8", errors="replace").read()
        m = re.search(r"constexpr int32_t %s\s*=\s*(\d+)" % const, body)
        value = int(m.group(1)) if m else -1
        check("%s.%s 存在" % (rel, const), value >= 0)
        check("%s.%s 非零" % (rel, const), value > 0, "值=%d" % value)
        same = value == baseline
        lines_out.append("| %s | %s | %s | %d | %d | %s |" %
                         (rel, const, note, value, baseline, "是" if same else "否"))
        check("%s.%s 与基线一致" % (rel, const), same, "当前=%d 基线=%d" % (value, baseline))
    lines_out.append("")

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    io.open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(lines_out))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
