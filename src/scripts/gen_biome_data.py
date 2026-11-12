#!/usr/bin/env python3
"""从群系定义表生成客户端 C++ 群系带数据头文件.

输入: single-branch/assets/project/csvdef/biomedef.csv (52 行, 含占位与副本行)
输出: single-branch/client/src/native/voxel/generated/biome_data_gen.h

表按 52 个独立群系配置, 客户端地形只有温度与湿度两个低频噪声,
因此把群系按 (温度, 湿度) 二维空间折算成 6 个群系带. 折算规则:

 海洋: 不占温度湿度带, 由地表高度决定 (低于海平面), 表层沿用沙与水
 雪原: 温度 < 30 (冰原/冰山/冻洋/冻河/针叶林北缘), 表层积雪, 树极疏
 沙漠: 温度 >= 75 且湿度 < 30 (沙漠/沙漠山丘/星球地表), 表层沙, 无树
 沼泽: 湿度 >= 75 且 30 <= 温度 < 75 (沼泽), 表层草, 树疏
 针叶林:  30 <= 温度 < 50 且湿度 >= 40 (针叶林), 表层草, 树中密
 草原: 其余中的低温低湿区 (草原/峭壁), 表层草, 树稀
 森林: 其余 (森林/丛林/雨林/红土), 表层草, 树密

树密度折算: ChunkTrees 是每区块的植株尝试权重, 相对比例保留,
映射到客户端的 0-255 成树概率 kTreeChance. 顶层块折算:
  100 GrassBlock -> Grass, 106 Sand -> Sand, 123 Ice -> Snow(客户端雪方块),
 红土 233 与星球系 116-118 客户端没有独立方块, 近似折到 Dirt/Stone 并注明.
草颜色与水颜色本期不做顶点色, 只在生成表保留原值供后续界面参考.
"""

import csv
import io
import os
import re

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
BIOME_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project",
                                          "csvdef", "biomedef.csv"))
OUT_PATH = os.path.join(SRC_ROOT, "native", "voxel", "generated", "biome_data_gen.h")

NOTICE = "由 scripts/gen_biome_data.py 从 assets 仓库的 csvdef/biomedef.csv 生成, 请勿手改"

# 群系带定义: (键名, 温度上限, 湿度上限, 顶层数据表物品键, 客户端方块键, 成树概率, 中文名)
# 判定顺序即数组顺序, 落到最后一条的森林是兜底带.
# 群系带定义: (键名, 温度下限, 温度上限, 湿度下限, 湿度上限, 顶层数据表物品键, 客户端方块键, 成树概率, 中文名)
# 区间为左闭右开, 判定顺序即数组顺序, 落到最后一条的森林是兜底带. 温度湿度取值 0-100.
BANDS = [
    ("Snow",     0,  30,   0, 256, "123", "Snow",   40,  "雪原"),
    ("Desert",  75, 256,   0,  30, "106", "Sand",    0,  "沙漠"),
    ("Swamp",   30,  75,  75, 256, "100", "Grass",  90,  "沼泽"),
    ("Taiga",   30,  50,  40,  75, "100", "Grass", 140,  "针叶林"),
    ("Plains",  50,  65,   0,  55, "100", "Grass",  30,  "草原"),
    ("Forest",   0, 256,   0, 256, "100", "Grass", 180, "森林"),
]

# 数据表顶层块物品键到客户端方块的折算, 与 block_registry.h 的兼容段一致.
# 数据表顶层块物品键到客户端方块 id 的折算, 数值与 block_registry.h 兼容段一致.
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


# 群系装饰: 每区块尝试次数上限, 防止表里按百分比口径的列被当成次数放大
MAX_DECORATION_ATTEMPTS = 8
# 装饰列名尾部数字即方块 id, 例如 ChunkFlowers300 -> 300
DECORATION_SUFFIX = re.compile(r"(\d+)$")


def collect_decorations(rows, col, mapping):
    """把 biomedef 的 Chunk* 列折算成每个群系带的装饰表.

 列名尾部数字就是方块 id (如 ChunkFlowers300 -> 300); 取值是该列在
 本群系带内的最大尝试次数. 只保留移动碰撞为 0 (不挡路) 的方块,
 避免把树, 石头一类实心方块当装饰撒在地表.
 草列 (ChunkGrass224) 在表里是百分比口径, 这里与其他列一样按次数上限裁剪.
    """
    import gen_block_data  # 局部导入: 只有本函数需要方块表

    header, block_rows = gen_block_data.load_rows()
    # 复用方块生成器的 id 分配结果, 不做第二份映射
    legacy_rows, extension_rows = gen_block_data.collect_source_rows(block_rows)
    allocated, _ = gen_block_data.allocate_ids(extension_rows)
    client_of_source = {source_id: client_id for client_id, _, source_id, _ in legacy_rows}
    for client_id, source_id, _ in allocated:
        client_of_source[source_id] = client_id
    band_of_row = {name: band for band, names in mapping.items() for name in names}

    attempts = {}
    for row in rows[2:]:
        type_name = row[col["TypeName"]].strip() if len(row) > col["TypeName"] else ""
        if not type_name:
            continue
        band = band_of_row.get(type_name)
        if band is None:
            continue
        for column, index in col.items():
            if not column.startswith("Chunk") or column == "ChunkTrees":
                continue
            # 列名形如 ChunkFlowers300, 尾部数字就是方块 id
            suffix = DECORATION_SUFFIX.search(column)
            if suffix is None:
                continue
            suffix = suffix.group(1)
            try:
                value = int(row[index] or 0)
            except ValueError:
                continue
            if value <= 0:
                continue
            key = (band, suffix)
            if value > attempts.get(key, 0):
                attempts[key] = value

    decorations = []
    skipped = []
    for (band, source_text), value in sorted(attempts.items(),
                                             key=lambda item: (band_names_index(item[0][0]), int(item[0][1]))):
        row = block_rows.get(source_text)
        if row is None:
            skipped.append(source_text)
            continue
        if (row.get("MoveCollide") or "0").strip() not in ("0", ""):
            skipped.append(source_text)
            continue
        client_id = client_of_source.get(int(source_text))
        if client_id is None:
            skipped.append(source_text)
            continue
        name = row.get("Name", "").strip() or source_text
        decorations.append((band, min(value, MAX_DECORATION_ATTEMPTS), client_id, name))
    return decorations, skipped


def band_names_index(band):
    return [b[0] for b in BANDS].index(band)


def weather_of(rows_, col):
    """把一个群系带内所有行的天气列折算成四元组.

 能否下雨/下雪取逻辑或 (带内任一群系允许即可), 三档权重取最大值,
 避免带内行数多寡影响天气频次.
    """
    can_rain = 0
    can_snow = 0
    clear_w = 0
    rain_w = 0
    thunder_w = 0
    for row_ in rows_:
        def value(name):
            """三档权重是小数比值 (晴天 1.0 到 1.2, 雨 0.3 到 0.6, 雷暴 0 到 0.5)."""
            if name not in col or col[name] >= len(row_):
                return 0.0
            try:
                return float((row_[col[name]] or "0").strip() or 0.0)
            except ValueError:
                return 0.0

        can_rain = max(can_rain, 1 if value("EnableRain") > 0 else 0)
        can_snow = max(can_snow, 1 if value("EnableSnow") > 0 else 0)
        # 乘 100 转成整数相对权重, 保留三档之间的比例
        clear_w = max(clear_w, min(255, int(round(value("ClearModulus") * 100))))
        rain_w = max(rain_w, min(255, int(round(value("RainModulus") * 100))))
        thunder_w = max(thunder_w, min(255, int(round(value("ThunderModulus") * 100))))
    return can_rain, can_snow, clear_w, rain_w, thunder_w


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

    # 统计每个群系落入的群系带, 写进生成表注释供追溯
    band_names = [b[0] for b in BANDS]
    mapping = {name: [] for name in band_names}
    band_rows = {name: [] for name in band_names}
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
        band_rows[placed].append(row)
        mapped += 1

    lines = []
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <cstdint>")
    lines.append("")
    lines.append("/* %s */" % NOTICE)
    lines.append("/* 数据源: single-branch/assets/project/csvdef/biomedef.csv */")
    lines.append("/* 52 群系按 (温度, 湿度) 二维空间折算成 6 个群系带, */")
    lines.append("/* 判定顺序即数组顺序, 兜底带是森林. 温度湿度取值 0-100 与数据表一致. */")
    lines.append("/* 树密度按ChunkTrees 的相对比例折算成 0-255 成树概率. */")
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
    lines.append("    uint8_t canRain;       /* EnableRain, 1 允许下雨 */")
    lines.append("    uint8_t canSnow;       /* EnableSnow, 1 允许下雪 */")
    lines.append("    uint8_t clearWeight;   /* ClearModulus, 相对权重 */")
    lines.append("    uint8_t rainWeight;    /* RainModulus, 相对权重 */")
    lines.append("    uint8_t thunderWeight; /* ThunderModulus, 相对权重 */")
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
        can_rain, can_snow, clear_w, rain_w, thunder_w = weather_of(band_rows[key], col)
        lines.append('    {%du, %du, %du, %du, %uu, %uu, %du, %du, %du, %du, %du, %du, "%s", "%s"},'
                 % (heat_min, heat_max, humid_min, humid_max,
                   CLIENT_BLOCK_ID.get(block_key, 1), CLIENT_BLOCK_ID.get(fill_key, 2),
                   chance, can_rain, can_snow, clear_w, rain_w, thunder_w, key, name_zh))
    lines.append("};")
    lines.append("")
    decorations, skipped = collect_decorations(rows, col, mapping)
    lines.append("/*")
    lines.append(" * 群系装饰: biomedef 的 Chunk* 列按群系带取最大值, 表示每区块尝试次数.")
    lines.append(" * 只收移动碰撞为 0 的方块; 草列的百分比口径与次数口径混用, 统一按 8 次封顶.")
    lines.append(" */")
    lines.append("struct BiomeDecoration {")
    lines.append("    uint8_t band;          /* 群系带下标, 与 kBiomeBands 同序 */")
    lines.append("    uint8_t attempts;      /* 每区块尝试次数 */")
    lines.append("    uint16_t block;        /* 客户端方块 id */")
    lines.append("    const char* name;      /* 中文名, 调试用 */")
    lines.append("};")
    lines.append("")
    lines.append("constexpr int32_t kBiomeDecorationCount = %d;" % len(decorations))
    lines.append("")
    lines.append("constexpr BiomeDecoration kBiomeDecorations[kBiomeDecorationCount == 0 ? 1 : kBiomeDecorationCount] = {")
    for band, attempts, client_id, name in decorations:
        lines.append('    {%du, %du, %du, "%s"},'
                     % (band_names_index(band), attempts, client_id, name))
    if not decorations:
        lines.append('    {0u, 0u, 0u, ""},')
    lines.append("};")
    lines.append("")
    lines.append("/* 群系到群系带的归属记录, 供折算追溯. */")
    for name in band_names:
        lines.append("/* %s <- %s */" % (name, ", ".join(mapping[name]) if mapping[name] else "-"))
    lines.append("")
    lines.append("}  /* namespace immirror::voxel */")
    lines.append("")

    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    open(OUT_PATH, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")
    emit("生成: %s" % OUT_PATH)
    emit("群系带: %d, 群系归属: %d, 装饰条目: %d (跳过 %d)"
         % (len(BANDS), mapped, len(decorations), len(skipped)))
    for name in band_names:
        emit("  %s <- %s" % (name, ", ".join(mapping[name]) if mapping[name] else "-"))


if __name__ == "__main__":
    main()
