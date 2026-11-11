#!/usr/bin/env python3
"""校验 ImMirror 地图工程目录 (UGC 编辑器工程格式).

工程是明文目录, 可以直接用手工编辑, 因此需要一个独立于客户端的校验器:
它在提交前给出字段级错误与警告, 避免把写坏的工程带到真机上才暴露问题.

用法:
    python single-branch/client/src/scripts/validate_map_project.py <工程目录>
    python single-branch/client/src/scripts/validate_map_project.py <工程目录> --strict
    python single-branch/client/src/scripts/validate_map_project.py <工程目录> --json

目录结构 (与 native/editor/project_format.cpp 的实现一致):

    工程目录/
    ├── project.json             工程描述
    ├── chunks/c.<x>.<z>.vox     区块游程文本, 只保存与地形不同的区块
    ├── triggers/t.<id>.json     触发器, 一条一个文件
    └── export/<名字>.world      导出产物, IMWF 二进制, 可缺省

只依赖标准库; 方块 id 取值直接从 native/voxel/block_registry.h 读取,
新增方块时不需要同步修改本脚本.
"""

import argparse
import json
import os
import re
import struct
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
REGISTRY_HEADER_PATH = os.path.join(SRC_ROOT, "native", "voxel", "block_registry.h")

# 与 voxel/voxel_types.h 保持一致的世界尺寸
CHUNK_SIZE_X = 16
CHUNK_SIZE_Z = 16
WORLD_HEIGHT = 256
CHUNK_VOLUME = CHUNK_SIZE_X * WORLD_HEIGHT * CHUNK_SIZE_Z

# 与 native/editor/project_format.cpp 保持一致的固定值
PROJECT_FORMAT = "immirror.project"
PROJECT_FORMAT_VERSION = 1
TRIGGER_FORMAT = "immirror.trigger"
TRIGGER_FORMAT_VERSION = 1
CHUNK_HEADER = "# immirror.chunk 1"
CHUNK_FILE_PATTERN = re.compile(r"^c\.(-?\d+)\.(-?\d+)\.vox$")
TRIGGER_FILE_PATTERN = re.compile(r"^t\.(\d+)\.json$")
RUN_PATTERN = re.compile(r"^(\d+):(\d+),?$")

# 与 native/editor/editor_types.h 的 limits 命名空间保持一致的上限
MAX_CHUNKS = 4096
MAX_TRIGGERS = 32
MAX_ACTIONS = 8
MAX_VIEW_DISTANCE = 16
MAX_TIMER_SECONDS = 3600.0
MIN_TIMER_SECONDS = 0.1
MAX_WORLD_XZ = 32768

EVENT_TYPES = ("enter_region", "block_broken", "timer")
ACTION_TYPES = ("log", "set_block", "teleport")

# IMWF 存档头: 魔数 4 字节, 版本/种子/区块数各 4 字节, 玩家坐标与朝向共 20 字节
IMWF_MAGIC = b"IMWF"
IMWF_VERSION = 1
IMWF_HEADER_BYTES = 36


class Report:
    """收集错误与警告. 错误让退出码为 1, 警告只在严格模式下生效."""

    def __init__(self):
        self.errors = []
        self.warnings = []

    def error(self, location, message):
        self.errors.append((location, message))

    def warning(self, location, message):
        self.warnings.append((location, message))

    def ok(self):
        return not self.errors


def load_block_table(header_path):
    """从方块注册表头文件读取 id 与名称, 返回 (名称表, 条目数) 或 (None, 0)."""
    try:
        with open(header_path, "r", encoding="utf-8") as handle:
            text = handle.read()
    except OSError:
        return None, 0
    names = {}
    count = 0
    for match in re.finditer(r"constexpr\s+BlockId\s+(\w+)\s*=\s*(\d+)\s*;", text):
        name = match.group(1)
        value = int(match.group(2))
        if name == "Count":
            count = value
            continue
        names[value] = name
    return names, count


def read_json(path, report):
    """读取 JSON 文件; 失败时记录错误并返回 None."""
    try:
        with open(path, "r", encoding="utf-8") as handle:
            text = handle.read()
    except OSError as error:
        report.error(rel(path), "无法读取: %s" % error)
        return None
    try:
        return json.loads(text)
    except json.JSONDecodeError as error:
        report.error(rel(path), "JSON 解析失败: 第 %d 行第 %d 列 %s" %
                     (error.lineno, error.colno, error.msg))
        return None


def rel(path):
    """错误信息里优先显示相对路径, 绝对路径只在工作目录之外时保留."""
    try:
        return os.path.relpath(path)
    except ValueError:
        return path


def require_type(report, path, holder, key, expected, label, required=True):
    """字段存在性与类型检查. 返回字段值, 缺失或类型不符返回 None."""
    if key not in holder:
        if required:
            report.error(rel(path), "%s 缺少字段 %s" % (label, key))
        return None
    value = holder[key]
    if not isinstance(value, expected) or isinstance(value, bool) != (expected is bool):
        report.error(rel(path), "%s 字段 %s 类型应为 %s" %
                     (label, key, _type_name(expected)))
        return None
    return value


def _type_name(expected):
    if expected is bool:
        return "布尔"
    if expected is int:
        return "整数"
    if expected is float:
        return "数值"
    if expected is str:
        return "字符串"
    if expected is list:
        return "数组"
    if expected is dict:
        return "对象"
    return "合法类型"


def is_number(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def as_int(report, path, holder, key, label, minimum=None, maximum=None, required=True):
    """读取整数字段并检查范围."""
    if key not in holder:
        if required:
            report.error(rel(path), "%s 缺少字段 %s" % (label, key))
        return None
    value = holder[key]
    if isinstance(value, bool) or not isinstance(value, int):
        report.error(rel(path), "%s 字段 %s 应为整数" % (label, key))
        return None
    if minimum is not None and value < minimum:
        report.error(rel(path), "%s 字段 %s = %d 小于下限 %d" % (label, key, value, minimum))
        return None
    if maximum is not None and value > maximum:
        report.error(rel(path), "%s 字段 %s = %d 超出上限 %d" % (label, key, value, maximum))
        return None
    return value


def as_number(report, path, holder, key, label, minimum=None, maximum=None):
    """读取数值字段, 整数与浮点都接受."""
    if key not in holder:
        report.error(rel(path), "%s 缺少字段 %s" % (label, key))
        return None
    value = holder[key]
    if not is_number(value):
        report.error(rel(path), "%s 字段 %s 应为数值" % (label, key))
        return None
    if minimum is not None and value < minimum:
        report.error(rel(path), "%s 字段 %s = %s 小于下限 %s" %
                     (label, key, value, minimum))
        return None
    if maximum is not None and value > maximum:
        report.error(rel(path), "%s 字段 %s = %s 超出上限 %s" %
                     (label, key, value, maximum))
        return None
    return float(value)


def check_block(report, path, block_id, block_count, label):
    """方块 id 必须在注册表范围内."""
    if block_count <= 0:
        return
    if block_id < 0 or block_id >= block_count:
        report.error(rel(path), "%s 的方块 id %d 超出范围 0..%d" %
                     (label, block_id, block_count - 1))


def validate_project_meta(root, data, block_count, report):
    """校验 project.json 的顶层字段."""
    path = os.path.join(root, "project.json")
    if not isinstance(data, dict):
        report.error(rel(path), "顶层应为 JSON 对象")
        return None, None, None

    fmt = require_type(report, path, data, "format", str, "project.json")
    if fmt is not None and fmt != PROJECT_FORMAT:
        report.error(rel(path), "format 字段应为 %s, 实际为 %s" % (PROJECT_FORMAT, fmt))
    version = as_int(report, path, data, "formatVersion", "project.json")
    if version is not None and version != PROJECT_FORMAT_VERSION:
        report.error(rel(path), "formatVersion 应为 %d, 实际为 %d" %
                     (PROJECT_FORMAT_VERSION, version))

    name = require_type(report, path, data, "name", str, "project.json")
    if name is not None and not name.strip():
        report.error(rel(path), "name 不能为空")
    if isinstance(name, str) and len(name.encode("utf-8")) > 64:
        report.warning(rel(path), "name 超过 64 字节, 界面显示可能被截断")
    require_type(report, path, data, "author", str, "project.json", required=False)

    seed = data.get("seed")
    if seed is None:
        report.error(rel(path), "缺少字段 seed")
    elif isinstance(seed, bool) or not isinstance(seed, int) or seed < 0 or seed > 0xFFFFFFFF:
        report.error(rel(path), "seed 应为 0 到 4294967295 之间的整数")

    spawn = data.get("spawn")
    if not isinstance(spawn, dict):
        report.error(rel(path), "缺少 spawn 对象")
    else:
        spawn_x = as_number(report, path, spawn, "x", "spawn")
        spawn_y = as_number(report, path, spawn, "y", "spawn")
        spawn_z = as_number(report, path, spawn, "z", "spawn")
        as_number(report, path, spawn, "yaw", "spawn", -360.0, 360.0)
        as_number(report, path, spawn, "pitch", "spawn", -90.0, 90.0)
        if spawn_y is not None and (spawn_y < 0.0 or spawn_y > WORLD_HEIGHT):
            report.error(rel(path), "spawn.y = %s 超出世界高度 0..%d" %
                         (spawn_y, WORLD_HEIGHT))
        for axis, value in (("x", spawn_x), ("z", spawn_z)):
            if value is not None and abs(value) > MAX_WORLD_XZ:
                report.error(rel(path), "spawn.%s = %s 超出 ±%d" %
                             (axis, value, MAX_WORLD_XZ))

    editor = data.get("editor")
    if not isinstance(editor, dict):
        report.error(rel(path), "缺少 editor 对象")
    else:
        selected = as_int(report, path, editor, "selectedBlock", "editor")
        if selected is not None:
            check_block(report, path, selected, block_count, "editor.selectedBlock")
        view = as_int(report, path, editor, "viewDistance", "editor", 1, MAX_VIEW_DISTANCE)
        if view is None and "viewDistance" in editor:
            report.warning(rel(path), "editor.viewDistance 超出 1..%d, 运行时会裁剪" %
                           MAX_VIEW_DISTANCE)

    for key in ("createdAt", "updatedAt"):
        value = data.get(key)
        if value is not None and not isinstance(value, str):
            report.error(rel(path), "%s 应为字符串" % key)
    return data.get("chunks"), data.get("triggers"), spawn


def validate_chunk_file(path, expect_x, expect_z, block_count, report):
    """校验单个区块游程文件."""
    try:
        with open(path, "r", encoding="utf-8") as handle:
            lines = handle.readlines()
    except OSError as error:
        report.error(rel(path), "无法读取区块文件: %s" % error)
        return
    except UnicodeDecodeError:
        report.error(rel(path), "区块文件不是 UTF-8 文本")
        return

    if not lines:
        report.error(rel(path), "区块文件为空")
        return
    header = lines[0].strip()
    if header != CHUNK_HEADER:
        report.error(rel(path), "首行应为 %s, 实际为 %s" % (CHUNK_HEADER, header))

    total = 0
    declared_changed = None
    for lineno, raw in enumerate(lines[1:], start=2):
        line = raw.strip()
        if not line:
            continue
        if line.startswith("#"):
            match = re.search(r"\bx=(-?\d+)\s+z=(-?\d+)", line)
            if match:
                file_x = int(match.group(1))
                file_z = int(match.group(2))
                if expect_x is not None and file_x != expect_x:
                    report.error(rel(path), "注释声明的 x=%d 与文件名的 %d 不一致" %
                                 (file_x, expect_x))
                if expect_z is not None and file_z != expect_z:
                    report.error(rel(path), "注释声明的 z=%d 与文件名的 %d 不一致" %
                                 (file_z, expect_z))
            changed = re.search(r"\bchanged=(-?\d+)", line)
            if changed:
                declared_changed = int(changed.group(1))
            continue
        for token in line.split():
            match = RUN_PATTERN.match(token)
            if not match:
                report.error(rel(path), "第 %d 行游程格式不正确: %s" % (lineno, token))
                continue
            block_id = int(match.group(1))
            count = int(match.group(2))
            if count <= 0:
                report.error(rel(path), "第 %d 行游程数量必须为正: %s" % (lineno, token))
            if block_count > 0 and block_id >= block_count:
                report.error(rel(path), "第 %d 行方块 id %d 超出范围 0..%d" %
                             (lineno, block_id, block_count - 1))
            total += count
    if total != CHUNK_VOLUME:
        report.error(rel(path), "游程总格数 %d 不等于一个区块的 %d" % (total, CHUNK_VOLUME))
    if declared_changed is not None and declared_changed > CHUNK_VOLUME:
        report.error(rel(path), "注释声明的 changed=%d 超过区块格数" % declared_changed)


def validate_chunks(root, entries, block_count, report):
    """校验 chunks 数组与磁盘上的区块文件."""
    chunks_dir = os.path.join(root, "chunks")
    if entries is None:
        report.warning(rel(os.path.join(root, "project.json")), "缺少 chunks 字段, 视为空工程")
        entries = []
    if not isinstance(entries, list):
        report.error(rel(os.path.join(root, "project.json")), "chunks 字段应为数组")
        return
    if len(entries) > MAX_CHUNKS:
        report.error(rel(os.path.join(root, "project.json")), "区块数量 %d 超过上限 %d" %
                     (len(entries), MAX_CHUNKS))

    referenced = set()
    validated = set()
    coordinates = set()
    for index, item in enumerate(entries):
        label = "chunks[%d]" % index
        if not isinstance(item, dict):
            report.error(rel(os.path.join(root, "project.json")), "%s 应为对象" % label)
            continue
        cx = as_int(report, os.path.join(root, "project.json"), item, "x", label)
        cz = as_int(report, os.path.join(root, "project.json"), item, "z", label)
        file_name = require_type(report, os.path.join(root, "project.json"), item, "file",
                                 str, label)
        if cx is None or cz is None:
            continue
        key = (cx, cz)
        if key in coordinates:
            report.error(rel(os.path.join(root, "project.json")),
                         "区块坐标重复: x=%d z=%d" % key)
        coordinates.add(key)
        expected_name = "c.%d.%d.vox" % key
        if file_name is None:
            file_name = "chunks/" + expected_name
        normalized = file_name.replace("\\", "/")
        if normalized.startswith("chunks/"):
            base_name = normalized[len("chunks/"):]
        else:
            base_name = os.path.basename(normalized)
        if base_name != expected_name:
            report.warning(rel(os.path.join(root, "project.json")),
                           "%s 的文件名 %s 与坐标 %d,%d 不匹配" %
                           (label, base_name, cx, cz))
        if ".." in normalized:
            report.error(rel(os.path.join(root, "project.json")),
                         "%s 的文件路径不允许包含 .." % label)
            continue
        chunk_path = os.path.join(chunks_dir, base_name)
        key_path = os.path.normcase(os.path.abspath(chunk_path))
        referenced.add(key_path)
        if not os.path.isfile(chunk_path):
            report.error(rel(chunk_path), "区块文件不存在")
            continue
        if key_path in validated:
            report.error(rel(os.path.join(root, "project.json")),
                         "%s 重复引用了同一个区块文件" % label)
            continue
        validated.add(key_path)
        validate_chunk_file(chunk_path, cx, cz, block_count, report)

    if os.path.isdir(chunks_dir):
        for name in sorted(os.listdir(chunks_dir)):
            full = os.path.abspath(os.path.join(chunks_dir, name))
            if not os.path.isfile(full):
                continue
            if not CHUNK_FILE_PATTERN.match(name):
                report.warning(rel(full), "chunks/ 下的文件不符合 c.<x>.<z>.vox 命名")
                continue
            if os.path.normcase(full) not in referenced:
                report.warning(rel(full), "区块文件未被 project.json 引用")


def validate_trigger_file(path, expected_id, block_count, report):
    """校验单个触发器 JSON."""
    data = read_json(path, report)
    if data is None:
        return
    label = "触发器"
    if not isinstance(data, dict):
        report.error(rel(path), "顶层应为 JSON 对象")
        return
    fmt = require_type(report, path, data, "format", str, label, required=False)
    if fmt is not None and fmt != TRIGGER_FORMAT:
        report.error(rel(path), "format 字段应为 %s, 实际为 %s" % (TRIGGER_FORMAT, fmt))
    version = data.get("formatVersion")
    if version is not None and version != TRIGGER_FORMAT_VERSION:
        report.error(rel(path), "formatVersion 应为 %d" % TRIGGER_FORMAT_VERSION)

    trigger_id = as_int(report, path, data, "id", label, 0)
    if trigger_id is not None and expected_id is not None and trigger_id != expected_id:
        report.error(rel(path), "id 字段 %d 与文件名的 %d 不一致" %
                     (trigger_id, expected_id))
    name = data.get("name")
    if name is not None and not isinstance(name, str):
        report.error(rel(path), "name 应为字符串")
    enabled = data.get("enabled")
    if enabled is not None and not isinstance(enabled, bool):
        report.error(rel(path), "enabled 应为布尔")

    event = data.get("event")
    if not isinstance(event, dict):
        report.error(rel(path), "缺少 event 对象")
    else:
        event_type = event.get("type")
        if event_type not in EVENT_TYPES:
            report.error(rel(path), "event.type 应为 %s 之一, 实际为 %s" %
                         (" / ".join(EVENT_TYPES), event_type))
        elif event_type == "enter_region":
            region = event.get("region")
            if not isinstance(region, dict):
                report.error(rel(path), "enter_region 事件缺少 region 对象")
            else:
                for key in ("cx", "cy", "cz", "sx", "sy", "sz"):
                    as_int(report, path, region, key, "region")
                for key in ("sx", "sy", "sz"):
                    value = region.get(key)
                    if isinstance(value, int) and not isinstance(value, bool) and value < 1:
                        report.warning(rel(path), "region.%s = %d 小于 1, 载入时会被裁剪到 1" %
                                       (key, value))
        elif event_type == "block_broken":
            block = event.get("block", 0)
            if isinstance(block, bool) or not isinstance(block, int):
                report.error(rel(path), "block_broken 事件的 block 应为整数")
            else:
                check_block(report, path, block, block_count, "block_broken.block")
        elif event_type == "timer":
            interval = as_number(report, path, event, "intervalSeconds", "timer",
                                 MIN_TIMER_SECONDS, MAX_TIMER_SECONDS)
            if interval is None and "intervalSeconds" in event:
                report.warning(rel(path), "intervalSeconds 超出 %.1f..%.0f, 载入时会被裁剪" %
                               (MIN_TIMER_SECONDS, MAX_TIMER_SECONDS))
            fire = event.get("fireImmediately")
            if fire is not None and not isinstance(fire, bool):
                report.error(rel(path), "fireImmediately 应为布尔")

    actions = data.get("actions")
    if actions is None:
        report.warning(rel(path), "缺少 actions 数组, 该触发器不会产生任何效果")
        actions = []
    if not isinstance(actions, list):
        report.error(rel(path), "actions 应为数组")
        actions = []
    if len(actions) > MAX_ACTIONS:
        report.warning(rel(path), "动作数量 %d 超过上限 %d, 载入时多余动作会被丢弃" %
                       (len(actions), MAX_ACTIONS))
    for index, action in enumerate(actions):
        action_label = "actions[%d]" % index
        if not isinstance(action, dict):
            report.error(rel(path), "%s 应为对象" % action_label)
            continue
        action_type = action.get("type")
        if action_type not in ACTION_TYPES:
            report.error(rel(path), "%s.type 应为 %s 之一, 实际为 %s" %
                         (action_label, " / ".join(ACTION_TYPES), action_type))
            continue
        if action_type == "log":
            text = action.get("text")
            if not isinstance(text, str):
                report.error(rel(path), "%s 的 text 应为字符串" % action_label)
        elif action_type == "set_block":
            for key in ("x", "y", "z"):
                as_int(report, path, action, key, action_label)
            y = action.get("y")
            if isinstance(y, int) and not isinstance(y, bool) and (y < 0 or y >= WORLD_HEIGHT):
                report.error(rel(path), "%s 的 y = %d 超出世界高度 0..%d" %
                             (action_label, y, WORLD_HEIGHT - 1))
            block = action.get("block")
            if isinstance(block, bool) or not isinstance(block, int):
                report.error(rel(path), "%s 的 block 应为整数" % action_label)
            else:
                check_block(report, path, block, block_count, action_label + ".block")
        elif action_type == "teleport":
            for key in ("x", "y", "z"):
                as_number(report, path, action, key, action_label)
            y = action.get("y")
            if is_number(y) and (y < 0.0 or y > WORLD_HEIGHT):
                report.error(rel(path), "%s 的 y = %s 超出世界高度 0..%d" %
                             (action_label, y, WORLD_HEIGHT))

    script = data.get("script")
    if script is not None and not isinstance(script, str):
        report.error(rel(path), "script 应为字符串")
    if isinstance(script, str) and script.strip() and not actions:
        report.warning(rel(path), "只填了 script 没有 actions, 编辑器以 actions 为准")


def validate_triggers(root, entries, block_count, report):
    """校验 triggers 数组与触发器文件."""
    triggers_dir = os.path.join(root, "triggers")
    project_path = os.path.join(root, "project.json")
    if entries is None:
        report.warning(rel(project_path), "缺少 triggers 字段, 视为没有触发器")
        entries = []
    if not isinstance(entries, list):
        report.error(rel(project_path), "triggers 字段应为数组")
        return
    if len(entries) > MAX_TRIGGERS:
        report.error(rel(project_path), "触发器数量 %d 超过上限 %d" %
                     (len(entries), MAX_TRIGGERS))

    referenced = set()
    validated = set()
    seen_ids = set()
    for index, item in enumerate(entries):
        label = "triggers[%d]" % index
        if not isinstance(item, dict):
            report.error(rel(project_path), "%s 应为对象" % label)
            continue
        trigger_id = as_int(report, project_path, item, "id", label, 0)
        file_name = require_type(report, project_path, item, "file", str, label)
        if trigger_id is None:
            continue
        if trigger_id in seen_ids:
            report.error(rel(project_path), "触发器 id 重复: %d" % trigger_id)
        seen_ids.add(trigger_id)
        expected_name = "t.%d.json" % trigger_id
        if file_name is None:
            file_name = "triggers/" + expected_name
        normalized = file_name.replace("\\", "/")
        base_name = normalized[len("triggers/"):] if normalized.startswith("triggers/") \
            else os.path.basename(normalized)
        if base_name != expected_name:
            report.warning(rel(project_path), "%s 的文件名 %s 与 id %d 不匹配" %
                           (label, base_name, trigger_id))
        if ".." in normalized:
            report.error(rel(project_path), "%s 的文件路径不允许包含 .." % label)
            continue
        trigger_path = os.path.join(triggers_dir, base_name)
        key_path = os.path.normcase(os.path.abspath(trigger_path))
        referenced.add(key_path)
        if not os.path.isfile(trigger_path):
            report.error(rel(trigger_path), "触发器文件不存在")
            continue
        if key_path in validated:
            report.error(rel(project_path), "%s 重复引用了同一个触发器文件" % label)
            continue
        validated.add(key_path)
        validate_trigger_file(trigger_path, trigger_id, block_count, report)

    if os.path.isdir(triggers_dir):
        for name in sorted(os.listdir(triggers_dir)):
            full = os.path.abspath(os.path.join(triggers_dir, name))
            if not os.path.isfile(full):
                continue
            if not TRIGGER_FILE_PATTERN.match(name):
                report.warning(rel(full), "triggers/ 下的文件不符合 t.<id>.json 命名")
                continue
            if os.path.normcase(full) not in referenced:
                report.warning(rel(full), "触发器文件未被 project.json 引用")


def validate_exports(root, report):
    """校验导出目录里的 IMWF 存档头."""
    export_dir = os.path.join(root, "export")
    if not os.path.isdir(export_dir):
        return
    for name in sorted(os.listdir(export_dir)):
        if not name.lower().endswith(".world"):
            continue
        path = os.path.join(export_dir, name)
        if not os.path.isfile(path):
            continue
        try:
            with open(path, "rb") as handle:
                head = handle.read(IMWF_HEADER_BYTES)
                size = os.fstat(handle.fileno()).st_size
        except OSError as error:
            report.error(rel(path), "无法读取导出产物: %s" % error)
            continue
        if len(head) < IMWF_HEADER_BYTES:
            report.error(rel(path), "导出产物不足 %d 字节" % IMWF_HEADER_BYTES)
            continue
        magic = head[0:4]
        if magic != IMWF_MAGIC:
            report.error(rel(path), "导出产物魔数应为 IMWF, 实际为 %r" % magic)
            continue
        version, seed, chunk_count = struct.unpack("<III", head[4:16])
        if version != IMWF_VERSION:
            report.error(rel(path), "导出产物版本 %d 不受支持" % version)
        if chunk_count == 0:
            report.warning(rel(path), "导出产物不含任何区块")
        if chunk_count > MAX_CHUNKS:
            report.error(rel(path), "导出产物区块数 %d 超过上限 %d" % (chunk_count, MAX_CHUNKS))
        if size < IMWF_HEADER_BYTES:
            report.error(rel(path), "导出产物长度异常")


def validate_project(root, block_count, report):
    """校验一个工程目录."""
    project_path = os.path.join(root, "project.json")
    if not os.path.isdir(root):
        report.error(root, "工程目录不存在")
        return
    if not os.path.isfile(project_path):
        report.error(rel(project_path), "缺少 project.json, 不是地图工程目录")
        return
    data = read_json(project_path, report)
    if data is None:
        return
    chunks, triggers, _ = validate_project_meta(root, data, block_count, report)
    validate_chunks(root, chunks, block_count, report)
    validate_triggers(root, triggers, block_count, report)
    validate_exports(root, report)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="校验 ImMirror 地图工程目录的格式与取值")
    parser.add_argument("project", help="工程目录, 内含 project.json")
    parser.add_argument("--strict", action="store_true", help="把警告也视为失败")
    parser.add_argument("--json", action="store_true", help="以 JSON 输出报告")
    parser.add_argument("--quiet", action="store_true", help="只输出错误与总结")
    args = parser.parse_args(argv)

    root = os.path.abspath(args.project)
    report = Report()
    names, block_count = load_block_table(REGISTRY_HEADER_PATH)
    if names is None:
        report.warning(rel(REGISTRY_HEADER_PATH), "未找到方块注册表, 跳过方块 id 范围校验")
    validate_project(root, block_count, report)

    failed = not report.ok() or (args.strict and report.warnings)

    if args.json:
        payload = {
            "project": root,
            "ok": not failed,
            "errors": [{"path": path, "message": message} for path, message in report.errors],
            "warnings": [{"path": path, "message": message} for path, message in report.warnings],
            "blockCount": block_count,
        }
        print(json.dumps(payload, ensure_ascii=False, indent=2))
        return 1 if failed else 0

    if not args.quiet:
        print("校验工程: %s" % root)
        if block_count > 0:
            print("方块表: %d 条 (id 0..%d)" % (block_count, block_count - 1))
    for path, message in report.errors:
        print("error: %s: %s" % (path, message))
    for path, message in report.warnings:
        if args.strict or not args.quiet:
            print("warning: %s: %s" % (path, message))
    print("总结: %d 个错误, %d 个警告%s" %
          (len(report.errors), len(report.warnings), " (严格模式)" if args.strict else ""))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
