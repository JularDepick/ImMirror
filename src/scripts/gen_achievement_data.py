#!/usr/bin/env python3
"""从原版成就表生成客户端 C++ 成就数据与服务端成就目录.

输入: single-branch/assets/project/csvdef/autogen/achievement.csv (双表头)
输出:
  single-branch/client/src/native/progress/generated/achievement_data_gen.h
  single-branch/client/src/native/progress/generated/daily_task_data_gen.h
  single-branch/server/src/server/api/v1/achievements/catalog.ts

提取字段: 成就 id, 中文名称, 中文描述, 分组, 原版目标类型, 目标对象 id,
目标数量与成就点数. 客户端与服务端读取同一份生成结果, 因此两端的成就定义
不可能漂移; 服务端仓库缺席时只跳过 catalog.ts, 不中断 C++ 头文件的生成.

事件折算: 客户端只观测破坏方块, 放置方块, 行走距离, 采集物品, 在线时长与
签到六类事件, 原版 37 种目标类型中只有下列几种能落到本地事件上,
其余目标 (合成, 熔炼, 击败生物, 驯服, NPC 交易等) 不在本客户端表内:

  1  拾取物品 -> 采集物品, 达成量单位为件
  4  使用物品 -> 放置方块, 客户端没有物品使用系统, 按消耗物品的最接近行为折算
  6  开采物品 -> 破坏方块, 达成量单位为块
  10 行走距离 -> 行走距离, 达成量单位为米
  11 游泳距离 -> 行走距离, 与 10 合并计程
  37 存活天数 -> 在线时长, 达成量按一天 86400 秒折算

镜币奖励: 原版奖励域为道具, 星星与迷你币, 与镜币不同域, 因此镜币数额由
成就点数与目标档位换算, 公式见 coin_reward().

每日任务模板: 成就条件模板按事件类型与目标档位给出, 每日按日期键确定性地
抽取三条, 抽取算法见 daily_task_pick 段注释, 客户端与服务端实现必须一致.
"""

import csv
import json
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
# 成就定义表只在素材仓库维护一份, 与本仓库同级
ACHIEVEMENT_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project",
                                                  "csvdef", "autogen", "achievement.csv"))
CLIENT_OUT_DIR = os.path.join(SRC_ROOT, "native", "progress", "generated")
SERVER_OUT_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "server", "src",
                                                "server", "api", "v1", "achievements",
                                                "catalog.ts"))

# 事件类型取值, 与 native/progress/progress_event.h 的 EventKind 和服务端
# EVENT_KIND 完全一致, 上报时直接传这些整数
EVENT_BREAK = 1
EVENT_PLACE = 2
EVENT_WALK = 3
EVENT_COLLECT = 4
EVENT_ONLINE = 5
EVENT_CHECKIN = 6

# 原版目标类型 -> 本地事件类型; 未列出的目标不进入本客户端成就表
GOAL_TO_EVENT = {
    1: EVENT_COLLECT,
    4: EVENT_PLACE,
    6: EVENT_BREAK,
    10: EVENT_WALK,
    11: EVENT_WALK,
    37: EVENT_ONLINE,
}

# 原版目标类型说明, 只用于生成日志
GOAL_NAMES = {
    1: "拾取物品",
    4: "使用物品",
    6: "开采物品",
    10: "行走距离",
    11: "游泳距离",
    37: "存活天数",
}

# 分组: 1 新手, 2 主线, 3 生产, 4 战斗, 5 其他, 6 极限; 表外取值回落到其他
CATEGORY_OTHER = 5
CATEGORY_RANGE = (1, 6)

# 镜币奖励换算: 基础 30, 每点成就点数 20, 再按达成量档位加成, 上限 500
COIN_BASE = 30
COIN_PER_POINT = 20
COIN_MAX = 500
COIN_TIERS = ((10, 0), (100, 30), (1000, 80))
COIN_TOP_TIER = 150

# 在线时长的换算: 一天 86400 秒
SECONDS_PER_DAY = 86400

# 描述长度上限, 超出截断并补省略号, 保证生成的 C++ 行不会过长
DESC_MAX_CHARS = 80

# 每日任务模板: (id, 事件类型, 目标对象, 达成量, 镜币, 名称)
# 达成量档位取自成就条件的目标档位 (10 / 50 / 200 量级), 按事件单位给出;
# 在线时长以秒为单位, 因此换算为 15 分钟与 30 分钟两档
DAILY_TEMPLATES = [
    (1, EVENT_BREAK, 0, 20, 40, "开山取石"),
    (2, EVENT_BREAK, 0, 60, 80, "挖掘达人"),
    (3, EVENT_PLACE, 0, 10, 40, "开工建造"),
    (4, EVENT_PLACE, 0, 30, 80, "施工队长"),
    (5, EVENT_COLLECT, 0, 10, 40, "采集新手"),
    (6, EVENT_COLLECT, 0, 30, 80, "采集行家"),
    (7, EVENT_WALK, 0, 100, 30, "四处走走"),
    (8, EVENT_WALK, 0, 300, 60, "长途跋涉"),
    (9, EVENT_ONLINE, 0, 900, 40, "上线报到"),
    (10, EVENT_ONLINE, 0, 1800, 80, "冒险常客"),
    (11, EVENT_CHECKIN, 0, 1, 30, "每日签到"),
]

# 每日任务条数, 与 native/progress/daily_task_board.h 的 kTaskCount 一致
DAILY_TASK_COUNT = 3

NOTICE = "由 scripts/gen_achievement_data.py 从 assets 仓库的 autogen/achievement.csv 生成, 请勿手改"


def emit(message):
    """打印进度. 控制台编码受限时降级为 ASCII, 不让日志中断生成."""
    try:
        print(message)
    except UnicodeEncodeError:
        print(message.encode("ascii", "replace").decode("ascii"))


def to_int(text, fallback=0):
    """空串与非数字一律按 fallback 处理, 数据表里存在空列."""
    try:
        return int(str(text).strip())
    except (TypeError, ValueError):
        return fallback


def clamp(value, low, high):
    return low if value < low else (high if value > high else value)


def escape_c(text):
    """转义 C 字符串字面量里的反斜杠与双引号."""
    return text.replace("\\", "\\\\").replace('"', '\\"')


def normalize_text(text):
    """折叠换行与连续空白为单个空格, 数据表里的描述跨行书写."""
    return " ".join(str(text).split())


def shorten(text, limit):
    """截断过长描述, 超出部分用省略号替代."""
    if len(text) <= limit:
        return text
    return text[:limit - 1] + "…"


def coin_reward(point, target, kind):
    """按成就点数与达成量档位换算镜币奖励."""
    bonus = COIN_TOP_TIER
    for threshold, tier_bonus in COIN_TIERS:
        if target <= threshold:
            bonus = tier_bonus
            break
    reward = COIN_BASE + max(point, 0) * COIN_PER_POINT + bonus
    return clamp(reward, COIN_BASE, COIN_MAX)


def load_rows():
    """读取成就表, 返回以英文表头为键的行列表."""
    with open(ACHIEVEMENT_PATH, "r", encoding="utf-8", newline="") as handle:
        rows = list(csv.reader(handle))
    if len(rows) < 3:
        raise SystemExit("成就定义表行数不足: %s" % ACHIEVEMENT_PATH)
    header = rows[1]
    required = ("ID", "Name", "Desc", "Group", "Type", "Goal", "GoalId", "GoalNum", "Point")
    index = {}
    for name in required:
        if name not in header:
            raise SystemExit("成就定义表缺少列 %s: %s" % (name, ACHIEVEMENT_PATH))
        index[name] = header.index(name)

    def cell(row, name):
        column = index[name]
        return row[column] if column < len(row) else ""

    return [{name: cell(row, name) for name in required} for row in rows[2:]]


def build_entries(rows):
    """过滤并折算成就行, 返回按原版 ID 升序排列的条目."""
    entries = []
    seen = set()
    for row in rows:
        source_id = to_int(row.get("ID"))
        name = normalize_text(row.get("Name", ""))
        goal = to_int(row.get("Goal"))
        if source_id <= 0 or not name or goal not in GOAL_TO_EVENT:
            continue
        if source_id in seen:
            continue
        target = to_int(row.get("GoalNum"))
        if target <= 0:
            continue
        kind = GOAL_TO_EVENT[goal]
        if kind == EVENT_ONLINE:
            # 存活天数折算为在线时长, 一天按 86400 秒计
            target *= SECONDS_PER_DAY
        point = max(to_int(row.get("Point")), 0)
        category = to_int(row.get("Group"))
        if category < CATEGORY_RANGE[0] or category > CATEGORY_RANGE[1]:
            category = CATEGORY_OTHER

        seen.add(source_id)
        entries.append({
            "id": source_id,
            "name": name,
            "description": shorten(normalize_text(row.get("Desc", "")), DESC_MAX_CHARS),
            "category": category,
            "kind": kind,
            "target": target,
            "source_goal": goal,
            "source_goal_id": to_int(row.get("GoalId")),
            "point": point,
            "coin_reward": coin_reward(point, target, kind),
        })
    entries.sort(key=lambda item: item["id"])
    return entries


def render_achievement_header(entries):
    """拼出客户端成就数据头文件文本."""
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/*",
        " * " + NOTICE + ".",
        " * 数据源: single-branch/assets/project/csvdef/autogen/achievement.csv",
        " * 只收录能落到本地六类事件的成就: 原版目标 1 拾取物品 -> 采集物品,",
        " * 4 使用物品 -> 放置方块, 6 开采物品 -> 破坏方块, 10 与 11 行走/游泳 -> 行走距离,",
        " * 37 存活天数 -> 在线时长 (按一天 86400 秒折算). 其余目标类型 (合成, 熔炼,",
        " * 击败生物, 驯服, NPC 交易等) 不在本客户端表内.",
        " * category 取值 1 新手, 2 主线, 3 生产, 4 战斗, 5 其他, 6 极限.",
        " * target 是达成所需的事件累计量, 单位随 eventKind 变化: 破坏与放置为块,",
        " * 采集为件, 行走为米, 在线时长为秒, 签名为次数.",
        " * sourceGoalId 是原版目标对象 id, 与原版物品 id 同域, 与客户端方块 id 不同域,",
        " * 因此本地事件的对象筛选暂时只按 0 (不限) 处理, 该字段留给后续的映射表.",
        " * coinReward 是完成时下发的镜币, 由成就点数与达成量档位换算, 公式见生成脚本.",
        " */",
        "namespace immirror::progress::gen {",
        "",
        "/* 单条成就定义, 字段顺序与 AchievementCatalog 的读取顺序一致. */",
        "struct AchievementData {",
        "    uint32_t id;             /* 原版 achievement 表的 ID */",
        "    const char* name;        /* 原版中文名, UTF-8 编码 */",
        "    const char* description; /* 原版中文描述, 已折叠换行并截断 */",
        "    uint8_t category;        /* 1 新手, 2 主线, 3 生产, 4 战斗, 5 其他, 6 极限 */",
        "    uint8_t eventKind;       /* 本地事件类型, 取值见 progress_event.h */",
        "    int32_t target;          /* 达成所需的事件累计量 */",
        "    uint8_t sourceGoal;      /* 原版目标类型 1..37 */",
        "    uint32_t sourceGoalId;   /* 原版目标对象 id */",
        "    uint8_t point;           /* 原版成就点数 */",
        "    int32_t coinReward;      /* 完成时下发的镜币 */",
        "};",
        "",
        "constexpr int32_t kAchievementDataCount = %d;" % len(entries),
        "",
        "/* 条目按原版 ID 升序排列, 越界索引由 AchievementCatalog 回落到首条. */",
        "constexpr AchievementData kAchievementData[kAchievementDataCount] = {",
    ]
    for entry in entries:
        lines.append('    {%du, "%s", "%s", %d, %d, %d, %d, %du, %d, %d},'
                     % (entry["id"], escape_c(entry["name"]), escape_c(entry["description"]),
                        entry["category"], entry["kind"], entry["target"],
                        entry["source_goal"], entry["source_goal_id"], entry["point"],
                        entry["coin_reward"]))
    lines += [
        "};",
        "",
        "}  // namespace immirror::progress::gen",
        "",
    ]
    return "\n".join(lines)


def render_daily_header(templates):
    """拼出每日任务模板头文件文本."""
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/*",
        " * " + NOTICE + ".",
        " * 每日任务模板: 每条模板是一种成就条件 (事件类型 + 达成量 + 镜币档位),",
        " * 每天按日期键确定性地抽取 %d 条, 抽取算法见 daily_task_board.cpp." % DAILY_TASK_COUNT,
        " * 服务端的 catalog.ts 使用同一份模板表与同一套抽取算法, 两端结果必须一致.",
        " */",
        "namespace immirror::progress::gen {",
        "",
        "/* 单条每日任务模板. */",
        "struct DailyTaskTemplate {",
        "    uint32_t id;        /* 模板 id, 服务端同值 */",
        "    uint8_t eventKind;  /* 计数所用的事件类型 */",
        "    int32_t subject;    /* 对象筛选, 0 表示不限 */",
        "    int32_t target;     /* 当日所需的事件累计量 */",
        "    int32_t coinReward; /* 完成时下发的镜币 */",
        "    const char* name;   /* 任务名, UTF-8 编码 */",
        "};",
        "",
        "constexpr int32_t kDailyTaskTemplateCount = %d;" % len(templates),
        "",
        "constexpr DailyTaskTemplate kDailyTaskTemplateData[kDailyTaskTemplateCount] = {",
    ]
    for item in templates:
        lines.append('    {%du, %d, %d, %d, %d, "%s"},'
                     % (item[0], item[1], item[2], item[3], item[4], escape_c(item[5])))
    lines += [
        "};",
        "",
        "}  // namespace immirror::progress::gen",
        "",
    ]
    return "\n".join(lines)


def render_server_catalog(entries, templates):
    """拼出服务端成就目录 TypeScript 文本."""
    kind_names = {
        EVENT_BREAK: "BREAK_BLOCK",
        EVENT_PLACE: "PLACE_BLOCK",
        EVENT_WALK: "WALK_METERS",
        EVENT_COLLECT: "COLLECT_ITEM",
        EVENT_ONLINE: "ONLINE_SECONDS",
        EVENT_CHECKIN: "CHECKIN",
    }
    lines = [
        "/**",
        " * ImMirror 我之镜界 服务端 作者: JularDepick",
        " * 成就与每日任务定义",
        " * " + NOTICE + ".",
        " * 数据源与服务端共用: single-branch/assets/project/csvdef/autogen/achievement.csv",
        " * 事件类型取值与客户端 native/progress/progress_event.h 的 EventKind 一致,",
        " * 客户端上报的 kind 字段直接对应这里的 EVENT_KIND.",
        " * 只收录能落到六类事件的成就, 折算规则见生成脚本的模块说明.",
        " */",
        "",
        "/* 事件类型: 与客户端 EventKind 同值, 上报时作为 kind 字段传输. */",
        "export const EVENT_KIND = {",
        "  BREAK_BLOCK: 1,",
        "  PLACE_BLOCK: 2,",
        "  WALK_METERS: 3,",
        "  COLLECT_ITEM: 4,",
        "  ONLINE_SECONDS: 5,",
        "  CHECKIN: 6,",
        "} as const;",
        "",
        "export type EventKind = (typeof EVENT_KIND)[keyof typeof EVENT_KIND];",
        "",
        "/* 事件类型到中文单位的映射, 用于接口返回里的展示文案. */",
        "export const EVENT_UNITS: Record<EventKind, string> = {",
        "  [EVENT_KIND.BREAK_BLOCK]: \"块\",",
        "  [EVENT_KIND.PLACE_BLOCK]: \"块\",",
        "  [EVENT_KIND.WALK_METERS]: \"米\",",
        "  [EVENT_KIND.COLLECT_ITEM]: \"件\",",
        "  [EVENT_KIND.ONLINE_SECONDS]: \"秒\",",
        "  [EVENT_KIND.CHECKIN]: \"次\",",
        "};",
        "",
        "/* 分组: 1 新手, 2 主线, 3 生产, 4 战斗, 5 其他, 6 极限. */",
        "export const CATEGORY_NAMES: Record<number, string> = {",
        "  1: \"新手\",",
        "  2: \"主线\",",
        "  3: \"生产\",",
        "  4: \"战斗\",",
        "  5: \"其他\",",
        "  6: \"极限\",",
        "};",
        "",
        "export interface AchievementDefinition {",
        "  readonly id: number;",
        "  readonly name: string;",
        "  readonly description: string;",
        "  readonly category: number;",
        "  readonly kind: EventKind;",
        "  readonly target: number;",
        "  readonly coinReward: number;",
        "}",
        "",
        "export interface DailyTaskTemplate {",
        "  readonly id: number;",
        "  readonly kind: EventKind;",
        "  readonly subject: number;",
        "  readonly target: number;",
        "  readonly coinReward: number;",
        "  readonly name: string;",
        "}",
        "",
        "export const ACHIEVEMENT_CATALOG: readonly AchievementDefinition[] = [",
    ]
    for entry in entries:
        lines.append("  { id: %d, name: %s, description: %s, category: %d, kind: %s, target: %d, "
                     "coinReward: %d },"
                     % (entry["id"], json.dumps(entry["name"], ensure_ascii=False),
                        json.dumps(entry["description"], ensure_ascii=False), entry["category"],
                        kind_names[entry["kind"]], entry["target"], entry["coin_reward"]))
    lines += [
        "];",
        "",
        "export const ACHIEVEMENT_BY_ID: ReadonlyMap<number, AchievementDefinition> = new Map(",
        "  ACHIEVEMENT_CATALOG.map((item) => [item.id, item]),",
        ");",
        "",
        "/* 每日任务模板: 与客户端 daily_task_data_gen.h 同表同序. */",
        "export const DAILY_TASK_TEMPLATES: readonly DailyTaskTemplate[] = [",
    ]
    for item in templates:
        lines.append("  { id: %d, kind: %s, subject: %d, target: %d, coinReward: %d, name: %s },"
                     % (item[0], kind_names[item[1]], item[2], item[3], item[4],
                        json.dumps(item[5], ensure_ascii=False)))
    lines += [
        "];",
        "",
        "/* 每日任务条数, 与客户端 DailyTaskBoard::kTaskCount 一致. */",
        "export const DAILY_TASK_COUNT = %d;" % DAILY_TASK_COUNT,
        "",
    ]
    return "\n".join(lines)


def write_file(path, text):
    directory = os.path.dirname(path)
    if directory:
        os.makedirs(directory, exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(text)
    emit("已写入: %s" % path)


def main():
    if not os.path.isfile(ACHIEVEMENT_PATH):
        raise SystemExit("成就定义表不存在: %s" % ACHIEVEMENT_PATH)
    entries = build_entries(load_rows())
    if len(entries) < 50:
        raise SystemExit("成就条目过少 (%d), 请检查过滤规则与数据表" % len(entries))

    write_file(os.path.join(CLIENT_OUT_DIR, "achievement_data_gen.h"),
               render_achievement_header(entries))
    write_file(os.path.join(CLIENT_OUT_DIR, "daily_task_data_gen.h"),
               render_daily_header(DAILY_TEMPLATES))

    if os.path.isdir(os.path.dirname(SERVER_OUT_PATH)):
        write_file(SERVER_OUT_PATH, render_server_catalog(entries, DAILY_TEMPLATES))
    else:
        emit("服务端目录不存在, 跳过 catalog.ts: %s" % SERVER_OUT_PATH)

    by_kind = {}
    for entry in entries:
        by_kind[entry["kind"]] = by_kind.get(entry["kind"], 0) + 1
    emit("成就条目 %d 条" % len(entries))
    for kind in sorted(by_kind):
        emit("  %-8s %3d 条" % (GOAL_NAMES.get(kind, str(kind)), by_kind[kind]))
    emit("每日任务模板 %d 条, 每日抽取 %d 条" % (len(DAILY_TEMPLATES), DAILY_TASK_COUNT))
    total_coin = sum(entry["coin_reward"] for entry in entries)
    emit("全部成就镜币合计 %d" % total_coin)
    return 0


if __name__ == "__main__":
    sys.exit(main())
