#!/usr/bin/env python3
"""从音效工程树的生物分类目录生成生物音效表.

输入: single-branch/assets/project/sounds/ent/<生物ID>/<动作>.ogg
      native/entity/generated/monster_data_gen.h 的生物 ID 集合
输出: native/entity/generated/entity_sound_gen.h

只收四类动作: 待机, 受击, 死亡, 攻击. 这四类覆盖了战斗反馈的全部听觉线索,
其余动作 (脚步, 变身, 飞行) 留待逐类接线.
音效文件名在打包时被扁平化成 <分类>_<子目录>_<名称>, 这里直接给出打包后的路径,
运行时不拼字符串, 也不读目录.
"""

import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
SOUND_ENT = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project", "sounds", "ent"))
MONSTER_HEADER = os.path.join(SRC_ROOT, "native", "entity", "generated", "monster_data_gen.h")
OUT_PATH = os.path.join(SRC_ROOT, "native", "entity", "generated", "entity_sound_gen.h")

# 动作名到字段名的映射; 表里没有的动作不生成.
ACTIONS = ("idle", "hit", "die", "attack")

# 同一动作可能有编号变体 (hit1, hit2), 取编号最小的一个.
VARIANT_SUFFIX = re.compile(r"^(\d+)$")


def monster_ids():
    """从生物数据表里取出全部生物 ID, 只收有音效目录的那些."""
    try:
        with open(MONSTER_HEADER, "r", encoding="utf-8") as handle:
            text = handle.read()
    except OSError:
        raise SystemExit("找不到生物数据表: %s" % MONSTER_HEADER)
    return {match.group(1) for match in re.finditer(r"\{(\d+)u, \"", text)}


def collect():
    """扫描生物音效目录, 返回按生物 ID 升序的条目列表."""
    if not os.path.isdir(SOUND_ENT):
        raise SystemExit("找不到生物音效目录: %s" % SOUND_ENT)
    known = monster_ids()
    entries = []
    for name in sorted(os.listdir(SOUND_ENT), key=lambda value: int(value) if value.isdigit() else 0):
        directory = os.path.join(SOUND_ENT, name)
        if not os.path.isdir(directory) or not name.isdigit():
            continue
        if name not in known:
            continue
        picked = {}
        for action in ACTIONS:
            candidates = []
            for file_name in os.listdir(directory):
                if not file_name.endswith(".ogg"):
                    continue
                stem = file_name[:-4]
                if stem == action:
                    candidates.append((0, file_name))
                    continue
                if stem.startswith(action):
                    suffix = VARIANT_SUFFIX.match(stem[len(action):])
                    if suffix is not None:
                        candidates.append((int(suffix.group(1)), file_name))
            if candidates:
                candidates.sort()
                picked[action] = candidates[0][1]
        if not picked:
            continue
        entries.append((int(name), picked))
    entries.sort()
    return entries


def render(entries):
    """拼出 C++ 头文件文本."""
    lines = [
        "#pragma once",
        "",
        "#include <cstdint>",
        "",
        "/*",
        " * 由 scripts/gen_entity_sound.py 生成, 请勿手改.",
        " * 生物 ID 到四类动作音效的对照表, 路径是安装包内的相对路径.",
        " * 空串表示该生物没有对应动作的音效, 播放侧跳过.",
        " */",
        "namespace immirror::entity::gen {",
        "",
        "struct EntitySoundSet {",
        "    uint32_t sourceId;",
        "    const char* idle;",
        "    const char* hit;",
        "    const char* die;",
        "    const char* attack;",
        "};",
        "",
        "constexpr int32_t kEntitySoundCount = %d;" % len(entries),
        "",
        "constexpr EntitySoundSet kEntitySounds[kEntitySoundCount == 0 ? 1 : kEntitySoundCount] = {",
    ]

    def path_of(source_id, file_name):
        if file_name is None:
            return ""
        # 打包后的扁平化命名: ent_<生物ID>_<动作>
        return "sounds/ent_%s_%s.ogg" % (source_id, file_name[:-4])

    for source_id, picked in entries:
        lines.append('    {%du, "%s", "%s", "%s", "%s"},'
                     % (source_id,
                        path_of(source_id, picked.get("idle")),
                        path_of(source_id, picked.get("hit")),
                        path_of(source_id, picked.get("die")),
                        path_of(source_id, picked.get("attack"))))
    if not entries:
        lines.append('    {0u, "", "", "", ""},')
    lines.extend([
        "};",
        "",
        "/*",
        " * 按生物 ID 与动作取音效路径: 0 受击, 1 死亡, 2 待机, 3 攻击.",
        " * 找不到对应条目或该动作没有音效时返回空串, 调用方据此跳过播放.",
        " */",
        "inline const char* EntitySoundPath(uint32_t sourceId, uint8_t action) {",
        "    for (int32_t index = 0; index < kEntitySoundCount; ++index) {",
        "        const EntitySoundSet& entry = kEntitySounds[index];",
        "        if (entry.sourceId != sourceId) {",
        "            continue;",
        "        }",
        "        switch (action) {",
        "            case 0:",
        "                return entry.hit;",
        "            case 1:",
        "                return entry.die;",
        "            case 2:",
        "                return entry.idle;",
        "            case 3:",
        "                return entry.attack;",
        "            default:",
        "                return \"\";",
        "        }",
        "    }",
        "    return \"\";",
        "}",
        "",
        "}  /* namespace immirror::entity::gen */",
        "",
    ])
    return "\n".join(lines)


def main():
    entries = collect()
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(render(entries))
    total = sum(len(picked) for _, picked in entries)
    print("生物音效表: %d 个生物, %d 条音效" % (len(entries), total))
    return 0


if __name__ == "__main__":
    sys.exit(main())
