#!/usr/bin/env python3
"""从状态效果表生成客户端的 Buff 属性表.

输入: 素材仓库 csvdef/autogen/buffdef.csv
输出: native/player/generated/buff_data_gen.h

只收客户端已能消费的四类效果:
  1001 移动速度, 1002 挖掘速度, 1004 跳跃高度, 1007 生命恢复.
其余效果 (暴击, 击退, 幸运等) 先保留在表外, 等对应系统落地再接.

数值口径:
  移动与挖掘速度是百分比增量, 正值加速, 负值减速, 取值形如 "100101|20" 或 "-25";
  跳跃是高度增量, 形如 "4";
  生命恢复形如 "40|5", 表示每 40 个更新周期恢复 5 点, 这里折算成每秒恢复量.
"""

import csv
import io
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
BUFF_PATH = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project",
                                          "csvdef", "autogen", "buffdef.csv"))
OUT_PATH = os.path.join(SRC_ROOT, "native", "player", "generated", "buff_data_gen.h")

# 效果类别, 与 buff 表的 EffectBankID1 一致.
KIND_SPEED = 1
KIND_DIG = 2
KIND_JUMP = 3
KIND_REGEN = 4

BANK_TO_KIND = {"1001": KIND_SPEED, "1002": KIND_DIG, "1004": KIND_JUMP, "1007": KIND_REGEN}

# 每秒多少个更新周期, 生命恢复的 "间隔|数量" 按这个折算.
TICKS_PER_SECOND = 20.0


def to_float(text, fallback=0.0):
    try:
        return float((text or "").strip() or fallback)
    except ValueError:
        return fallback


def to_int(text, fallback=0):
    try:
        return int(float((text or "").strip() or fallback))
    except ValueError:
        return fallback


def parse_value(kind, raw):
    """把一个效果的 Default1 折算成统一口径的数值."""
    text = (raw or "").strip()
    if not text:
        return 0.0
    parts = [part.strip() for part in text.split("|")]
    if kind in (KIND_SPEED, KIND_DIG):
        # 形如 "100101|20" 取后半段, 形如 "20" 直接取值
        return to_float(parts[-1], 0.0)
    if kind == KIND_JUMP:
        return to_float(parts[0], 0.0)
    if kind == KIND_REGEN:
        # 形如 "40|5": 每 40 个周期恢复 5 点, 折算成每秒
        if len(parts) >= 2:
            interval = to_float(parts[0], 0.0)
            amount = to_float(parts[1], 0.0)
            if interval <= 0.0:
                return 0.0
            return amount * TICKS_PER_SECOND / interval
        return 0.0
    return 0.0


def load_buffs():
    """读取状态表, 返回按编号升序的条目列表."""
    if not os.path.isfile(BUFF_PATH):
        raise SystemExit("状态效果表不存在: %s" % BUFF_PATH)
    rows = list(csv.reader(io.StringIO(open(BUFF_PATH, "rb").read().decode("utf-8-sig"))))
    header = rows[1]
    index = {name: position for position, name in enumerate(header)}
    entries = []
    for row in rows[2:]:
        if len(row) <= index["Default1"]:
            continue
        buff_id = to_int(row[index["ID"]])
        if buff_id <= 0:
            continue
        bank = (row[index["EffectBankID1"]] or "").strip()
        kind = BANK_TO_KIND.get(bank)
        if kind is None:
            continue
        duration = to_float(row[index["EffectTime"]], 0.0)
        if duration <= 0.0:
            continue
        value = parse_value(kind, row[index["Default1"]])
        if value == 0.0:
            continue
        # 音效: 效果表只给音效编号, 具体路径在音效表里, 按编号查一次.
        sound_path = ""
        raw_sound = (row[index["SoundID"]] or "").strip() if "SoundID" in index else ""
        if raw_sound and raw_sound not in ("0", ""):
            sound_path = sound_asset_path(raw_sound)
        entries.append({
            "id": buff_id,
            "sound": sound_path,
            "name": (row[index["Name"]] or "").strip(),
            "kind": kind,
            "level": max(1, to_int(row[index["Level"]], 1)),
            "duration": duration,
            "value": value,
        })
    entries.sort(key=lambda item: item["id"])
    return entries


SOUND_CSV = os.path.normpath(os.path.join(os.path.dirname(BUFF_PATH), "..", "sound.csv"))
_sound_cache = {}


def sound_asset_path(sound_id):
    """音效编号转资源路径: 音效表的 SoundPath 形如 buff.buff_speed_1, 扁平化成 sounds/buff_buff_speed_1.ogg."""
    global _sound_cache
    if not _sound_cache:
        try:
            raw = open(SOUND_CSV, "rb").read().decode("utf-8-sig")
        except OSError:
            _sound_cache[""] = ""
            return ""
        rows = list(csv.reader(io.StringIO(raw)))
        header = rows[1]
        id_index = header.index("ID")
        path_index = header.index("SoundPath")
        for row in rows[2:]:
            if len(row) <= path_index:
                continue
            key = row[id_index].strip()
            value = row[path_index].strip()
            if key and value:
                _sound_cache[key] = "sounds/" + value.replace(".", "_") + ".ogg"
    return _sound_cache.get(str(sound_id), "")


def render(entries):
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/*",
        " * 由 scripts/gen_buff_data.py 生成, 请勿手改.",
        " * 客户端可消费的状态效果: 移动速度, 挖掘速度, 跳跃高度, 生命恢复.",
        " * kind 取值 1 移速 2 挖掘 3 跳跃 4 生命恢复;",
        " * value 的口径随 kind 变化: 速度是百分比增量, 跳跃是高度增量, 生命恢复是每秒点数.",
        " */",
        "namespace immirror::player::gen {",
        "",
        "struct BuffEntry {",
        "    uint32_t id;",
        "    uint8_t kind;",
        "    uint8_t level;",
        "    float duration; /* 持续秒数 */",
        "    float value;    /* 效果数值, 口径见文件头 */",
        "    const char* name;",
        "    const char* sound; /* 施加时播放的音效路径, 空串表示没有 */",
        "};",
        "",
        "constexpr int32_t kBuffEntryCount = %d;" % len(entries),
        "",
        "constexpr BuffEntry kBuffEntries[kBuffEntryCount == 0 ? 1 : kBuffEntryCount] = {",
    ]
    for entry in entries:
        lines.append('    {%du, %du, %du, %.1ff, %.2ff, "%s", "%s"},'
                     % (entry["id"], entry["kind"], entry["level"], entry["duration"],
                        entry["value"], entry["name"], entry["sound"]))
    if not entries:
        lines.append('    {0u, 0u, 0u, 0.0f, 0.0f, "", ""},')
    lines.extend([
        "};",
        "",
        "/* 按编号取状态条目, 找不到返回空指针. */",
        "inline const BuffEntry* BuffOf(uint32_t id) {",
        "    for (int32_t index = 0; index < kBuffEntryCount; ++index) {",
        "        if (kBuffEntries[index].id == id) {",
        "            return &kBuffEntries[index];",
        "        }",
        "    }",
        "    return nullptr;",
        "}",
        "",
        "}  /* namespace immirror::player::gen */",
        "",
    ])
    return "\n".join(lines)


def main():
    entries = load_buffs()
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(render(entries))
    kinds = {}
    for entry in entries:
        kinds[entry["kind"]] = kinds.get(entry["kind"], 0) + 1
    print("状态效果表: %d 条, 分类 %s" % (len(entries), kinds))
    return 0


if __name__ == "__main__":
    sys.exit(main())
