#!/usr/bin/env python3
"""从原版数据表生成编辑器侧 C++ 参数表.

输入:
  single-branch/assets/project/csvdef/blockdef.csv     方块定义表 (1193 行)
  single-branch/assets/project/csvdef/triggeritem.csv  触发器控件表 (事件/条件/动作)
  single-branch/assets/project/csvdef/triggerparam.csv 触发器参数表
输出:
  single-branch/client/src/native/editor/generated/editor_data_gen.h

编辑器需要三份只读数据:
  1. 方块调色板的完整属性: 中文名, 硬度, 采集工具, 地形编辑器开放位与触发器开放位.
     可放置的方块以客户端注册表为准, 原版行只用来补充属性, 两者由 CLIENT_BLOCKS 关联.
  2. 触发器控件目录: 编辑器支持的三个事件与三个动作, 每个都带原版 triggeritem ID,
     右侧参数取原版 triggeritem 的 Param1 与 Param2, 参数类型再回查 triggerparam.
  3. 触发器参数描述: 参数 ID 到中文名与 ASCII 键的映射, 属性面板与校验脚本共用.

输出文件列入 .gitignore, 由 scripts/build-native.sh 在构建前调用本脚本生成.
改动支持的事件或动作时只改本脚本的 SUPPORTED_ITEMS, 原生侧不硬编码任何 ID.
"""

import csv
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
sys.path.insert(0, SCRIPT_DIR)

# 方块定义表的路径与客户端方块映射只在 gen_block_data.py 维护一份, 这里直接复用
from gen_block_data import CLIENT_BLOCKS, load_rows, to_int  # noqa: E402

CSVDEF_DIR = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project", "csvdef"))
TRIGGERITEM_PATH = os.path.join(CSVDEF_DIR, "triggeritem.csv")
TRIGGERPARAM_PATH = os.path.join(CSVDEF_DIR, "triggerparam.csv")
OUT_PATH = os.path.join(SRC_ROOT, "native", "editor", "generated", "editor_data_gen.h")

NOTICE = "由 scripts/gen_editor_data.py 从 assets 仓库的 csvdef 生成, 请勿手改"

# 编辑器支持的事件与动作: (原版 triggeritem ID, ASCII 键, ASCII 显示名, 备注)
# 事件取原版玩家进入区域, 方块被破坏与计时器变化; 动作取创建方块, 改变位置与输出日志.
# 输出日志在原版里没有对应控件, 按 triggeritem 的 ID 规则另起动作子类 90 作为编辑器扩展.
SUPPORTED_ITEMS = [
    (1030021, "EVENT_ENTER_REGION", "ENTER REGION", "事件: 玩家进入区域"),
    (1050002, "EVENT_BLOCK_BROKEN", "BLOCK BROKEN", "事件: 任意方块被破坏"),
    (1020009, "EVENT_TIMER", "TIMER", "事件: 计时器变化"),
    (3040001, "ACTION_SET_BLOCK", "SET BLOCK", "动作: 在指定位置创建方块"),
    (3050011, "ACTION_TELEPORT", "TELEPORT", "动作: 改变玩家当前位置"),
    (3900001, "ACTION_LOG", "LOG", "动作: 输出日志 (编辑器扩展)"),
]

# 编辑器扩展控件的名称, 原版表里查不到
EXTENSION_NAMES = {3900001: "输出日志"}

# 属性面板与工程校验使用的参数 ASCII 键, 其余参数按 PARAM_<id> 兜底
PARAM_KEYS = {
    1001: "REGION",
    1002: "POSITION",
    1005: "NUMBER",
    1006: "COORD",
    1018: "PLAYER",
    1025: "BLOCK_TYPE",
}


def emit(message):
    """打印进度. 控制台编码受限时降级为 ASCII, 不让日志中断生成."""
    try:
        print(message)
    except UnicodeEncodeError:
        print(message.encode("ascii", "replace").decode("ascii"))


def escape(text):
    """转义 C 字符串字面量里的反斜杠与双引号."""
    return str(text).replace("\\", "\\\\").replace('"', '\\"')


def load_double_header(path, required):
    """读取双表头 CSV, 返回按 ID 索引的行字典; 第二行是英文表头."""
    with open(path, "r", encoding="utf-8", newline="") as handle:
        rows = list(csv.reader(handle))
    if len(rows) < 3:
        raise SystemExit("数据表行数不足: %s" % path)
    header = rows[1]
    for column in required:
        if column not in header:
            raise SystemExit("数据表 %s 缺少列 %s" % (path, column))
    table = {}
    for row in rows[2:]:
        if len(row) <= header.index("ID"):
            continue
        key = row[header.index("ID")].strip()
        if not key or key in table:
            continue
        table[key] = {name: (row[index] if index < len(row) else "")
                      for index, name in enumerate(header) if name}
    return table


def parse_param_id(text):
    """取 triggeritem 参数列的参数 ID, 形如 "1001,1" 取 1001, 空列返回 0."""
    head = str(text).split(",")[0].strip()
    return to_int(head)


def build_blocks():
    """把客户端方块映射与原版方块行合并, 补充属性面板需要的只读属性."""
    _, rows = load_rows()
    entries = []
    for client_id, name, source_id, _texture in CLIENT_BLOCKS:
        row = rows.get(str(source_id))
        if row is None:
            raise SystemExit("方块定义表中找不到 ID %d (%s)" % (source_id, name))
        name_zh = row.get("Name", "").strip()
        if not name_zh:
            raise SystemExit("ID %d (%s) 缺少中文名" % (source_id, name))
        entries.append({
            "client_id": client_id,
            "legacy_id": source_id,
            "key": name,
            "name_zh": name_zh,
            "hardness": to_int(row.get("Hardness", "0")),
            "mine_tool": to_int(row.get("MineTool", "0")),
            "terrain_editor": to_int(row.get("TerrainEditor", "0")),
            "trigger_type": to_int(row.get("TriggerType", "0")),
        })
    return entries


def count_legacy_flags(rows, column):
    """统计原版方块表里某一列的开放方块数, 作为数据接入的规模指标."""
    total = 0
    for row in rows.values():
        if to_int(row.get(column, "0")) > 0:
            total += 1
    return total


def build_trigger_items():
    """从触发器控件表取支持的事件与动作, 并补上编辑器扩展项."""
    items = load_double_header(TRIGGERITEM_PATH, ("ID", "Name", "Type", "ChildType", "Param1"))
    entries = []
    for item_id, key, name_en, note in SUPPORTED_ITEMS:
        row = items.get(str(item_id))
        if row is None:
            if item_id in EXTENSION_NAMES:
                entries.append({
                    "item_id": item_id,
                    "key": key,
                    "name_en": name_en,
                    "name_zh": EXTENSION_NAMES[item_id],
                    "category": 3,
                    "child_type": 90,
                    "param_id": 0,
                    "param_two_id": 0,
                })
                continue
            raise SystemExit("触发器控件表中找不到 ID %d (%s)" % (item_id, note))
        entries.append({
            "item_id": item_id,
            "key": key,
            "name_en": name_en,
            "name_zh": row.get("Name", "").strip(),
            "category": to_int(row.get("Type", "0")),
            "child_type": to_int(row.get("ChildType", "0")),
            "param_id": parse_param_id(row.get("Param1", "")),
            "param_two_id": parse_param_id(row.get("Param2", "")),
        })
    return entries


def build_params():
    """参数表全量导出, 属性面板按 ID 反查中文名."""
    params = load_double_header(TRIGGERPARAM_PATH, ("ID", "Desc"))
    entries = []
    for key in sorted(params, key=lambda value: to_int(value)):
        param_id = to_int(key)
        entries.append({
            "id": param_id,
            "key": PARAM_KEYS.get(param_id, "PARAM_%d" % param_id),
            "name_zh": params[key].get("Desc", "").strip(),
        })
    return entries


def render_header(blocks, items, params, stats):
    """拼出 C++ 头文件文本."""
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/*",
        " * " + NOTICE + ".",
        " * 数据源: assets/project/csvdef 下的 blockdef.csv, triggeritem.csv 与 triggerparam.csv.",
        " * 本表只承载只读属性, 工程数据与运行时状态不写入这里.",
        " */",
        "namespace immirror::editor::gen {",
        "",
        "/* 方块调色板条目: 客户端可放置方块与原版方块表属性的关联. */",
        "struct BlockEntry {",
        "    uint16_t clientId;      /* 客户端方块 id, 与 voxel::block 命名空间一致 */",
        "    int32_t legacyId;       /* 原版 blockdef 的 ID, 便于回溯 */",
        "    const char* key;        /* 客户端注册表英文键, ASCII */",
        "    const char* nameZh;     /* 原版中文名, UTF-8 */",
        "    float hardness;         /* 原版硬度, -1 表示不可破坏 */",
        "    int32_t mineTool;       /* 原版采集工具: 1 斧 2 镐 3 铲 5 剪刀, 0 徒手 */",
        "    int32_t terrainEditor;  /* 地形编辑器开放位, 0 表示原版不开放 */",
        "    int32_t triggerType;    /* 触发器开放位, 0 表示原版不开放 */",
        "};",
        "",
        "/* 触发器控件: 编辑器支持的事件与动作. */",
        "struct TriggerItemEntry {",
        "    int32_t itemId;         /* 原版 triggeritem ID */",
        "    const char* key;        /* ASCII 键, 工程 JSON 与界面共用 */",
        "    const char* nameEn;     /* ASCII 显示名, 点阵字只收录 ASCII */",
        "    const char* nameZh;     /* 原版中文名, UTF-8 */",
        "    int32_t category;       /* 1 事件, 3 动作 */",
        "    int32_t childType;      /* 原版子类序号 */",
        "    int32_t paramId;        /* 第一参数的 triggerparam ID, 0 表示无 */",
        "    int32_t paramTwoId;     /* 第二参数的 triggerparam ID, 0 表示无 */",
        "};",
        "",
        "/* 触发器参数描述. */",
        "struct TriggerParamEntry {",
        "    int32_t id;             /* 原版 triggerparam ID */",
        "    const char* key;        /* ASCII 键 */",
        "    const char* nameZh;     /* 中文描述, UTF-8 */",
        "};",
        "",
    ]
    for name, count in stats:
        lines.append("/* %s: %d */" % (name, count))
    lines.extend([
        "",
        "extern const BlockEntry kBlockEntries[];",
        "extern const int32_t kBlockEntryCount;",
        "",
        "extern const TriggerItemEntry kTriggerItems[];",
        "extern const int32_t kTriggerItemCount;",
        "",
        "extern const TriggerParamEntry kTriggerParams[];",
        "extern const int32_t kTriggerParamCount;",
        "",
        "/* 原版方块表行数与地形编辑器/触发器开放方块数, 供自检与界面展示. */",
        "constexpr int32_t kLegacyBlockRows = %d;" % stats[0][1],
        "constexpr int32_t kLegacyTerrainEditorBlocks = %d;" % stats[1][1],
        "constexpr int32_t kLegacyTriggerOpenBlocks = %d;" % stats[2][1],
        "",
        "/* 按客户端方块 id 取条目, 越界返回空气条目. */",
        "const BlockEntry& BlockEntryOf(uint16_t clientId);",
        "",
        "/* 按 ASCII 键取触发器控件, 未收录返回空指针. */",
        "const TriggerItemEntry* TriggerItemOfKey(const char* key);",
        "",
        "/* 按原版 ID 取触发器控件, 未收录返回空指针. */",
        "const TriggerItemEntry* TriggerItemOfId(int32_t itemId);",
        "",
        "}  /* namespace immirror::editor::gen */",
        "",
    ])
    return "\n".join(lines)


def render_source(blocks, items, params):
    """拼出 C++ 实现文本, 表体与查表函数一起给出."""
    lines = [
        "/* " + NOTICE + ". */",
        "",
        "#include \"editor/generated/editor_data_gen.h\"",
        "",
        "#include <cstring>",
        "",
        "namespace immirror::editor::gen {",
        "",
        "const BlockEntry kBlockEntries[] = {",
    ]
    for entry in blocks:
        lines.append('    {%d, %d, "%s", "%s", %.1ff, %d, %d, %d},'
                     % (entry["client_id"], entry["legacy_id"], escape(entry["key"]),
                        escape(entry["name_zh"]), float(entry["hardness"]),
                        entry["mine_tool"], entry["terrain_editor"], entry["trigger_type"]))
    lines.extend([
        "};",
        "",
        "const int32_t kBlockEntryCount = static_cast<int32_t>(sizeof(kBlockEntries) / sizeof(kBlockEntries[0]));",
        "",
        "const TriggerItemEntry kTriggerItems[] = {",
    ])
    for entry in items:
        lines.append('    {%d, "%s", "%s", "%s", %d, %d, %d, %d},'
                     % (entry["item_id"], escape(entry["key"]), escape(entry["name_en"]),
                        escape(entry["name_zh"]), entry["category"], entry["child_type"],
                        entry["param_id"], entry["param_two_id"]))
    lines.extend([
        "};",
        "",
        "const int32_t kTriggerItemCount = static_cast<int32_t>(sizeof(kTriggerItems) / sizeof(kTriggerItems[0]));",
        "",
        "const TriggerParamEntry kTriggerParams[] = {",
    ])
    for entry in params:
        lines.append('    {%d, "%s", "%s"},'
                     % (entry["id"], escape(entry["key"]), escape(entry["name_zh"])))
    lines.extend([
        "};",
        "",
        "const int32_t kTriggerParamCount = static_cast<int32_t>(sizeof(kTriggerParams) / sizeof(kTriggerParams[0]));",
        "",
        "const BlockEntry& BlockEntryOf(uint16_t clientId) {",
        "    for (int32_t index = 0; index < kBlockEntryCount; ++index) {",
        "        if (kBlockEntries[index].clientId == clientId) {",
        "            return kBlockEntries[index];",
        "        }",
        "    }",
        "    return kBlockEntries[0];",
        "}",
        "",
        "const TriggerItemEntry* TriggerItemOfKey(const char* key) {",
        "    if (key == nullptr) {",
        "        return nullptr;",
        "    }",
        "    for (int32_t index = 0; index < kTriggerItemCount; ++index) {",
        "        if (std::strcmp(kTriggerItems[index].key, key) == 0) {",
        "            return &kTriggerItems[index];",
        "        }",
        "    }",
        "    return nullptr;",
        "}",
        "",
        "const TriggerItemEntry* TriggerItemOfId(int32_t itemId) {",
        "    for (int32_t index = 0; index < kTriggerItemCount; ++index) {",
        "        if (kTriggerItems[index].itemId == itemId) {",
        "            return &kTriggerItems[index];",
        "        }",
        "    }",
        "    return nullptr;",
        "}",
        "",
        "}  /* namespace immirror::editor::gen */",
        "",
    ])
    return "\n".join(lines)

def main():
    blocks = build_blocks()
    items = build_trigger_items()
    params = build_params()

    _header, legacy_rows = load_rows()
    stats = [
        ("原版方块表行数", len(legacy_rows)),
        ("原版地形编辑器开放方块数", count_legacy_flags(legacy_rows, "TerrainEditor")),
        ("原版触发器开放方块数", count_legacy_flags(legacy_rows, "TriggerType")),
    ]

    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(render_header(blocks, items, params, stats))
    source_path = os.path.splitext(OUT_PATH)[0] + ".cpp"
    with open(source_path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(render_source(blocks, items, params))

    emit("编辑器参数表已生成: %s" % OUT_PATH)
    emit("编辑器参数表实现: %s" % source_path)
    emit("  方块调色板 %d 条, 触发器控件 %d 条, 参数 %d 条"
         % (len(blocks), len(items), len(params)))
    for name, count in stats:
        emit("  %s: %d" % (name, count))
    return 0


if __name__ == "__main__":
    sys.exit(main())
