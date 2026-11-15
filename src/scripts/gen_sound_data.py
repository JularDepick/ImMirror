#!/usr/bin/env python3
"""从 sound.csv 生成音效定义表."""

import io
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROJECT = os.path.normpath(os.path.join(ROOT, "..", "..", "assets", "project"))
CSV_PATH = os.path.join(PROJECT, "csvdef", "sound.csv")
OUT = os.path.join(ROOT, "native", "audio", "generated", "sound_data_gen.h")


def main():
    lines = io.open(CSV_PATH, encoding="utf-8-sig", errors="replace").read().splitlines()
    data = [line.split(",") for line in lines[2:] if line.strip()]
    print("音效条目: %d" % len(data))

    rows = []
    for row in data:
        if len(row) < 8:
            continue
        ident = row[0].strip()
        name = row[1].strip()
        kind = row[5].strip()
        path = row[6].strip()
        icon = row[7].strip()
        if not ident.isdigit() or not path:
            continue
        rows.append((int(ident), name, int(kind) if kind.isdigit() else 0, path, icon))
    rows.sort()
    print("有效条目: %d" % len(rows))

    lines_out = ["#pragma once", "", "#include <cstdint>", "",
                 "/*",
                 " * 音效定义表, 由 scripts/gen_sound_data.py 从 csvdef/sound.csv 生成, 请勿手改.",
                 " * path 是安装包内相对路径, 已把源表的点分隔换成斜杠并补 .ogg 后缀.",
                 " */",
                 "namespace immirror::audio::gen {", "",
                 "struct SoundEntry {",
                 "    uint32_t sourceId;  /* sound.csv 的 ID */",
                 "    const char* name;   /* 音效名 */",
                 "    uint8_t kind;       /* 分类: 1 脚步 2 行为 3 受击 4 待机 5 技能 6 环境 7 背景音乐 8 提示 9 其它 */",
                 "    const char* path;   /* 包内相对路径 */",
                 "    const char* icon;   /* 图标名 */",
                 "};", "",
                 "constexpr int32_t kSoundEntryCount = %d;" % len(rows), "",
                 "constexpr SoundEntry kSoundEntries[kSoundEntryCount] = {"]
    for ident, name, kind, path, icon in rows:
        pkg = "sounds/" + path.replace(".", "/") + ".ogg"
        safe_name = name.replace('"', "'")
        lines_out.append('    {%du, "%s", %du, "%s", "%s"},' % (ident, safe_name, kind, pkg, icon))
    lines_out.append("};")
    lines_out.append("")
    lines_out.append("}  /* namespace immirror::audio::gen */")
    lines_out.append("")
    payload = "\n".join(lines_out)

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    old = io.open(OUT, encoding="utf-8").read() if os.path.exists(OUT) else ""
    if old != payload:
        io.open(OUT, "w", encoding="utf-8", newline="\n").write(payload)
        print("写出 %s (%d 条)" % (OUT, len(rows)))
    else:
        print("内容未变")
    return 0


if __name__ == "__main__":
    sys.exit(main())
