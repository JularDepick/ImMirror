#!/usr/bin/env python3
"""从原版群系定义表生成客户端 C++ 群系带数据头文件.

输入: single-branch/assets/project/csvdef/biomedef.csv (52 行, 含占位与副本行)
输出: single-branch/client/src/native/voxel/generated/biome_data_gen.h

原版表按 52 个独立群系配置, 客户端地形只有温度与湿度两个低频噪声,
因此把原版群系按 (温度, 湿度) 二维空间折算成 6 个群系带. 折算规则:

  海洋:    不占温度湿度带, 由地表高度决定 (低于海平面), 表层沿用沙与水
  雪原:    温度 < 30 (冰原/冰山/冻洋/冻河/针叶林北缘), 表层积雪, 树极疏
  沙漠:    温度 >= 75 且湿度 < 30 (沙漠/沙漠山丘/星球地表), 表层沙, 无树
  沼泽:    湿度 >= 75 且 30 <= 温度 < 75 (沼泽), 表层草, 树疏
  针叶林:  30 <= 温度 < 50 且湿度 >= 40 (针叶林), 表层草, 树中密
  草原:    其余中的低温低湿区 (草原/峭壁), 表层草, 树稀
  森林:    其余 (森林/丛林/雨林/红土), 表层草, 树密

树密度折算: 原版 ChunkTrees 是每区块的植株尝试权重, 相对比例保留,
映射到客户端的 0-255 成树概率 kTreeChance. 顶层块折算:
  100 GrassBlock -> Grass, 106 Sand -> Sand, 123 Ice -> Snow(客户端雪方块),
  红土 233 与星球系 116-118 客户端没有独立方块, 近似折到 Dirt/Stone 并注明.
草颜色与水颜色本期不做顶点色, 只在生成表保留原值供后续界面参考.
"""

import csv
import io
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
BIOME_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project",
                                          "csvdef", "biomedef.csv"))
OUT_PATH = os.path.join(SRC_ROOT, "native", "voxel", "generated", "biome_data_gen.h")

NOTICE = "由 scripts/gen_biome_data.py 从 assets 仓库的 csvdef/biomedef.csv 生成, 请勿手改"

# 群系带定义: (键名, 温度上限, 湿度上限, 顶层原版物品键, 客户端方块键, 成树概率, 中文名)
# 判定顺序即数组顺序, 落到最后一条的森林是兜底带.
# 群系带定义: (键名, 温度下限, 温度上限, 湿度下限, 湿度上限, 顶层原版物品键, 客户端方块键, 成树概率, 中文名)
# 区间为左闭右开, 判定顺序即数组顺序, 落到最后一条的森林是兜底带. 温度湿度取值 0-100.
BANDS = [
    ("Snow",     0,  30,   0, 256, "123", "Snow",   40,  "雪原"),
    ("Desert",  75, 256,   0,  30, "106", "Sand",    0,  "沙漠"),
    ("Swamp",   30,  75,  75, 256, "100", "Grass",  90,  "沼泽"),
    ("Taiga",   30,  50,  40,  75, "100", "Grass", 140,  "针叶林"),
    ("Plains",  50,  65,   0,  55, "100", "Grass",  30,  "草原"),
    ("Forest",   0, 256,   0, 256, "100", "Grass", 180, "森林"),
]

# 原版顶层块物品键到客户端方块的折算, 与 block_registry.h 的兼容段一致.
# 原版顶层块物品键到客户端方块 id 的折算, 数值与 block_registry.h 兼容段一致.
CLIENT_BLOCK_ID = {
    "Grass": 1,
    "Dirt": 2,
    "Stone": 3,
    "Sand": 4,
    "Snow": 11,
}

TOP_BLOCK_MAP = {
    "100": "Grass",
    "106": "Sand",
    "123": "Snow",
    "101": "Dirt",   # 填充块 Soil, 客户端折到泥土
    "104": "Stone",
    "233": "Dirt",   # 红土近似折到泥土, 客户端暂无红土方块
    "116": "Stone",  # 星球系岩石近似折到石头
    "117": "Grass",  # 星球系草近似折到草
    "118": "Stone",  # 星球系云块近似折到石头
}


def emit(message):
    try:
        print(message)
    except UnicodeEncodeError:
        print(message.encode("ascii", "replace").decode("ascii"))


def main():
    rows = list(csv.reader(io.StringIO(
        open(BIOME_PATH, "rb").read().decode("utf-8-sig"))))
    header = rows[1]
    col = {name: index for index, name in enumerate(header)}

    # 统计每个原版群系落入的群系带, 写进生成表注释供追溯
    band_names = [b[0] for b in BANDS]
    mapping = {name: [] for name in band_names}
    mapped = 0
    for row in rows[2:]:
        if len(row) <= col["ChunkTrees"] or not row[col["TypeName"]].strip():
            continue
        try:
            heat = int(row[col["Heat"]] or 0)
            humid = int(row[col["Humid"]] or 0)
        except ValueError:
            continue
        placed = None
        for key, heatMin, heatMax, humidMin, humidMax, _top, _block, _chance, _name in BANDS:
            if heatMin <= heat < heatMax and humidMin <= humid < humidMax:
                placed = key
                break
        if placed is None:
            placed = "Forest"
        mapping[placed].append(row[col["TypeName"]])
        mapped += 1

    lines = []
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <cstdint>")
    lines.append("")
    lines.append("/* %s */" % NOTICE)
    lines.append("/* 数据源: single-branch/assets/project/csvdef/biomedef.csv */")
    lines.append("/* 原版 52 群系按 (温度, 湿度) 二维空间折算成 6 个群系带, */")
    lines.append("/* 判定顺序即数组顺序, 兜底带是森林. 温度湿度取值 0-100 与原版一致. */")
    lines.append("/* 树密度按原版 ChunkTrees 的相对比例折算成 0-255 成树概率. */")
    lines.append("/* 草颜色与水颜色本期不做顶点色, 原值不进生成表. */")
    lines.append("")
    lines.append("namespace immirror::voxel {")
    lines.append("")
    lines.append("/* 一条群系带: 温度湿度四元区间, 顶层与填充客户端方块, 成树概率. */")
    lines.append("struct BiomeBand {")
    lines.append("    uint16_t heatMin;      /* 温度下限 (含), 0-100 */")
    lines.append("    uint16_t heatMax;      /* 温度上限 (不含), 兜底带填 256 表示不设限 */")
    lines.append("    uint16_t humidMin;     /* 湿度下限 (含) */")
    lines.append("    uint16_t humidMax;     /* 湿度上限 (不含), 兜底带填 256 */")
    lines.append("    uint16_t topBlock;     /* 表层方块的客户端 id */")
    lines.append("    uint16_t fillBlock;    /* 表层以下填充方块的客户端 id */")
    lines.append("    uint8_t treeChance;    /* 成树概率, 0-255, 与 kTreeChance 同口径 */")
    lines.append("    const char* key;       /* 群系带英文键 */")
    lines.append("    const char* nameZh;    /* 群系带中文名 */")
    lines.append("};")
    lines.append("")
    lines.append("constexpr int32_t kBiomeBandCount = %d;" % len(BANDS))
    lines.append("")
    lines.append("constexpr BiomeBand kBiomeBands[kBiomeBandCount] = {")
    for key, heat_min, heat_max, humid_min, humid_max, top_src, block_key, chance, name_zh in BANDS:
        fill_src = "101" if top_src == "100" or top_src == "123" else top_src
        fill_key = TOP_BLOCK_MAP.get(fill_src, "Dirt")
        lines.append('    {%du, %du, %du, %du, %uu, %uu, %du, "%s", "%s"},'
                 % (heat_min, heat_max, humid_min, humid_max,
                   CLIENT_BLOCK_ID.get(block_key, 1), CLIENT_BLOCK_ID.get(fill_key, 2),
                   chance, key, name_zh))
    lines.append("};")
    lines.append("")
    lines.append("/* 原版群系到群系带的归属记录, 供折算追溯. */")
    for name in band_names:
        lines.append("/* %s <- %s */" % (name, ", ".join(mapping[name]) if mapping[name] else "-"))
    lines.append("")
    lines.append("}  /* namespace immirror::voxel */")
    lines.append("")

    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    open(OUT_PATH, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
    emit("生成: %s" % OUT_PATH)
    emit("群系带: %d, 原版群系归属: %d" % (len(BANDS), mapped))
    for name in band_names:
        emit("  %s <- %s" % (name, ", ".join(mapping[name]) if mapping[name] else "-"))


if __name__ == "__main__":
    main()
