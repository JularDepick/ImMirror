#!/usr/bin/env python3
"""从原版生物定义表生成客户端 C++ 生物数据头文件.

输入: single-branch/assets/project/csvdef/autogen/monster.csv (双表头, 440 行)
输出: single-branch/client/src/native/entity/generated/monster_data_gen.h

只提取客户端当前需要的字段: 标识, 中文名, 生命, 攻击, 移动速度, 尺寸,
感知距离, 攻击距离, 攻击间隔与生物类别. 单位折算与原版表的量纲见下方常量:
原版的高度, 宽度与厚度以厘米记录 (玩家体型 195/95/100 对应 1.95 米),
速度同样以厘米每秒记录, 因此统一按 0.01 折算为米与米每秒.

过滤规则: 只保留有生命值与攻击力, 且类别属于怪物, 动物, 稀有动物, 水生物,
boss 与飞行生物的行; 村民, npc, 坐骑, 球体与占位行不进入客户端生物表.

掉落物在生物掉落系统就位前按类别折算为一种客户端方块, 仅作占位,
掉落物真实取值在原版表里是物品 id, 与本客户端的方块 id 不同域.
"""

import csv
import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
# 生物定义表只在素材仓库维护一份, 与本仓库同级
MONSTER_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project",
                                             "csvdef", "autogen", "monster.csv"))
OUT_PATH = os.path.join(SRC_ROOT, "native", "entity", "generated", "monster_data_gen.h")

# 客户端接收的生物类别: 0 怪物, 1 动物, 2 稀有动物, 3 水生物, 6 boss, 8 飞行生物
KEEP_TYPES = (0, 1, 2, 3, 6, 8)

# 占位与模板行的中文名标记, 命中即跳过: 这些行是原版用来占位或给编辑器取模型的
PLACEHOLDER_NAMES = re.compile(r"备用|模型")

# 类别到生成权重的折算: 怪物常见, 动物略多, 水生物与稀有种少见, boss 极稀有.
# 权重只影响生成时的抽取比例, 0 表示该类别不参与生成.
SPAWN_WEIGHT_BY_TYPE = {
    0: 1.0,
    1: 1.2,
    2: 0.5,
    3: 0.4,
    6: 0.02,
    8: 0.6,
}

# 类别到占位掉落的折算: (客户端方块 id, 数量), 方块 id 与 voxel/block_registry.h 一致
LOOT_BY_TYPE = {
    0: (13, 1),  # 怪物 -> 砂砾
    1: (2, 1),   # 动物 -> 土
    2: (4, 1),   # 稀有动物 -> 沙
    3: (13, 1),  # 水生物 -> 砂砾
    6: (9, 3),   # boss -> 砖
    8: (7, 1),   # 飞行生物 -> 树叶
}

# 单位与取值范围: 原版长度与速度为厘米, 折算后裁剪到客户端可用的区间
CM_TO_M = 0.01
SPEED_MIN = 0.6
SPEED_MAX = 5.0
HEIGHT_MIN = 0.4
HEIGHT_MAX = 4.0
WIDTH_MIN = 0.3
WIDTH_MAX = 4.0
VIEW_MIN = 4.0
VIEW_MAX = 40.0
ATTACK_DISTANCE_MIN = 1.0
ATTACK_DISTANCE_MAX = 16.0
ATTACK_INTERVAL_MIN = 1.0
ATTACK_INTERVAL_MAX = 4.0
# 原版攻击动作时间以二十分之一秒为单位
TICKS_PER_SECOND = 20.0

NOTICE = "由 scripts/gen_monster_data.py 从 assets 仓库的 autogen/monster.csv 生成, 请勿手改"


def emit(message):
    """打印进度. 控制台编码受限时降级为 ASCII, 不让日志中断生成."""
    try:
        print(message)
    except UnicodeEncodeError:
        print(message.encode("ascii", "replace").decode("ascii"))


def to_int(text):
    """空串与非数字一律按 0 处理, 数据表里存在空列."""
    try:
        return int(str(text).strip())
    except (TypeError, ValueError):
        return 0


def clamp(value, low, high):
    return low if value < low else (high if value > high else value)


def escape(text):
    """转义 C 字符串字面量里的反斜杠与双引号."""
    return text.replace("\\", "\\\\").replace('"', '\\"')


def load_rows():
    """读取生物定义表, 返回以英文表头为键的行列表."""
    with open(MONSTER_PATH, "r", encoding="utf-8", newline="") as handle:
        rows = list(csv.reader(handle))
    if len(rows) < 3:
        raise SystemExit("生物定义表行数不足: %s" % MONSTER_PATH)
    header = rows[1]
    required = ("ID", "Name", "Type", "Life", "Attack", "Speed", "Height", "Width",
                "Thickness", "ViewDistance", "AttackDistance", "AttackAnimTicks",
                "AttackType")
    index = {}
    for name in required:
        if name not in header:
            raise SystemExit("生物定义表缺少列 %s: %s" % (name, MONSTER_PATH))
        index[name] = header.index(name)
    table = []
    for row in rows[2:]:
        if len(row) <= index["Name"]:
            continue
        table.append({name: (row[column] if column < len(row) else "")
                      for name, column in index.items()})
    return table


def build_entries(rows):
    """过滤并折算生物行, 返回按原版 ID 升序排列的条目."""
    entries = []
    seen = set()
    for row in rows:
        source_id = to_int(row.get("ID"))
        name = row.get("Name", "").strip()
        kind = to_int(row.get("Type"))
        life = to_int(row.get("Life"))
        attack = to_int(row.get("Attack"))
        # 占位行与模板行不进入客户端表: 原版用 "备用ID" 与 "生物模型" 标记这类记录
        if source_id <= 0 or not name or source_id in seen:
            continue
        if PLACEHOLDER_NAMES.search(name):
            continue
        if life <= 0 or attack <= 0 or kind not in KEEP_TYPES:
            continue

        speed = clamp(to_int(row.get("Speed")) * CM_TO_M, SPEED_MIN, SPEED_MAX)
        height = clamp(to_int(row.get("Height")) * CM_TO_M, HEIGHT_MIN, HEIGHT_MAX)
        width = clamp(to_int(row.get("Width")) * CM_TO_M, WIDTH_MIN, WIDTH_MAX)
        thickness = clamp(to_int(row.get("Thickness")) * CM_TO_M, WIDTH_MIN, WIDTH_MAX)
        view = clamp(float(to_int(row.get("ViewDistance")) or 12), VIEW_MIN, VIEW_MAX)
        attack_distance = clamp(float(to_int(row.get("AttackDistance")) or 2),
                                ATTACK_DISTANCE_MIN, ATTACK_DISTANCE_MAX)
        attack_ticks = to_int(row.get("AttackAnimTicks"))
        attack_interval = clamp(attack_ticks / TICKS_PER_SECOND if attack_ticks > 0 else 1.0,
                                ATTACK_INTERVAL_MIN, ATTACK_INTERVAL_MAX)
        loot_block, loot_count = LOOT_BY_TYPE.get(kind, (2, 1))

        seen.add(source_id)
        entries.append({
            "source_id": source_id,
            "name": name,
            "life": float(life),
            "attack": float(attack),
            "speed": speed,
            "height": height,
            "width": width,
            "thickness": thickness,
            "view": view,
            "attack_distance": attack_distance,
            "attack_interval": attack_interval,
            "attack_type": clamp(to_int(row.get("AttackType")), 0, 2),
            "type": kind,
            "hostile": 1 if kind in (0, 6) else 0,
            "weight": SPAWN_WEIGHT_BY_TYPE.get(kind, 0.0),
            "loot_block": loot_block,
            "loot_count": loot_count,
        })
    entries.sort(key=lambda item: item["source_id"])
    return entries


def render_header(entries):
    """拼出 C++ 头文件文本."""
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/*",
        " * " + NOTICE + ".",
        " * 数据源: single-branch/assets/project/csvdef/autogen/monster.csv",
        " * 长度与速度按原版量纲从厘米折算为米与米每秒, 尺寸分高度, 宽度与厚度三项,",
        " * 攻击间隔由原版攻击动作时间 (二十分之一秒为单位) 折算为秒.",
        " * 原版攻击类型 0 近程, 1 远程, 2 爆炸; 生物类别 0 怪物, 1 动物, 2 稀有动物,",
        " * 3 水生物, 6 boss, 8 飞行生物. hostile 表示会主动攻击玩家.",
        " * lootBlock 是死亡后进入物品栏的客户端方块 id, 在生物掉落系统就位前按类别占位.",
        " */",
        "namespace immirror::entity::gen {",
        "",
        "/* 单个生物的定义, 字段顺序与 MonsterTable 的读取顺序一致. */",
        "struct MonsterData {",
        "    uint32_t sourceId;       /* 原版 monster 表的 ID */",
        "    const char* displayName; /* 原版中文名, UTF-8 编码 */",
        "    float life;              /* 生命值上限 */",
        "    float attack;            /* 单次攻击伤害 */",
        "    float speed;             /* 移动速度, 米每秒 */",
        "    float height;            /* 身高, 米 */",
        "    float width;             /* 身宽, 米 */",
        "    float thickness;         /* 身厚, 米 */",
        "    float viewDistance;      /* 感知半径, 米 */",
        "    float attackDistance;    /* 攻击距离, 米 */",
        "    float attackInterval;    /* 攻击间隔, 秒 */",
        "    uint8_t attackType;      /* 0 近程, 1 远程, 2 爆炸 */",
        "    uint8_t type;            /* 生物类别, 取值见文件头注释 */",
        "    uint8_t hostile;         /* 1 主动攻击玩家, 0 遇人躲避 */",
        "    float spawnWeight;       /* 生成权重, 0 表示不参与生成 */",
        "    uint16_t lootBlock;      /* 死亡掉落的客户端方块 id */",
        "    uint16_t lootCount;      /* 死亡掉落数量 */",
        "};",
        "",
        "constexpr int32_t kMonsterDataCount = %d;" % len(entries),
        "",
        "/* 条目按原版 ID 升序排列, 越界索引由 MonsterTable 回落到首条. */",
        "constexpr MonsterData kMonsterData[kMonsterDataCount] = {",
    ]
    for entry in entries:
        lines.append('    {%du, "%s", %.1ff, %.1ff, %.2ff, %.2ff, %.2ff, %.2ff, %.1ff, %.1ff, '
                     '%.2ff, %d, %d, %d, %.2ff, %du, %du},'
                     % (entry["source_id"], escape(entry["name"]), entry["life"], entry["attack"],
                        entry["speed"], entry["height"], entry["width"], entry["thickness"],
                        entry["view"], entry["attack_distance"], entry["attack_interval"],
                        entry["attack_type"], entry["type"], entry["hostile"], entry["weight"],
                        entry["loot_block"], entry["loot_count"]))
    lines += [
        "};",
        "",
        "}  // namespace immirror::entity::gen",
        "",
    ]
    return "\n".join(lines)


def main():
    if not os.path.isfile(MONSTER_PATH):
        raise SystemExit("生物定义表不存在: %s" % MONSTER_PATH)
    entries = build_entries(load_rows())
    if len(entries) < 20:
        raise SystemExit("生物条目过少 (%d), 请检查过滤规则与数据表" % len(entries))

    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(render_header(entries))

    hostile = sum(1 for entry in entries if entry["hostile"])
    emit("生物数据表已生成: %s" % OUT_PATH)
    emit("  条目 %d 条, 其中主动攻击 %d 条" % (len(entries), hostile))
    for entry in entries:
        emit("  %5d %-10s type=%d life=%.0f attack=%.0f speed=%.2f size=%.2fx%.2fx%.2f"
             % (entry["source_id"], entry["name"], entry["type"], entry["life"],
                entry["attack"], entry["speed"], entry["height"], entry["width"],
                entry["thickness"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
