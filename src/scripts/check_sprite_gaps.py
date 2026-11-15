#!/usr/bin/env python3
"""精灵缺口普查: 布局里引用的精灵名与图集定义比对, 输出完整缺口清单."""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LAYOUT = os.path.join(ROOT, "..", "assets", "project", "data", "xml", "ui", "mobile")
OUT = os.path.join(ROOT, "..", "..", ".agent", "ui-baseline", "sprite-gaps.md")

SPRITE_RE = re.compile(r"<sprite\s+n=\"([^\"]+)\"")
UVNAME_RE = re.compile(r"uvname=\"([^\"]+)\"")
FILE_RE = re.compile(r"file=\"([^\"]+)\"")
TICK = chr(96)


def normalize(name):
    base = name.replace("\\", "/").rsplit("/", 1)[-1]
    for ext in (".png", ".ktx"):
        if base.lower().endswith(ext):
            base = base[: -len(ext)]
    return base.lower()


def collect_defined():
    defined = set()
    for base, _dirs, files in os.walk(LAYOUT):
        for name in files:
            if not name.lower().endswith(".xml"):
                continue
            path = os.path.join(base, name)
            if "texture" not in path.replace("\\", "/"):
                continue
            try:
                with open(path, "r", encoding="utf-8", errors="replace") as handle:
                    body = handle.read()
            except OSError:
                continue
            for hit in SPRITE_RE.finditer(body):
                defined.add(normalize(hit.group(1)))
    return defined


def collect_used():
    used = []
    for name in sorted(os.listdir(LAYOUT)):
        if not name.lower().endswith(".xml"):
            continue
        path = os.path.join(LAYOUT, name)
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as handle:
                lines = handle.readlines()
        except OSError:
            continue
        for number, text in enumerate(lines, 1):
            match = UVNAME_RE.search(text)
            if match is None:
                continue
            atlas = FILE_RE.search(text)
            used.append((match.group(1), name, number, atlas.group(1) if atlas else ""))
    return used


def collect_texture_gaps():
    """布局里 file 直接指向的整图, 磁盘上不存在的记为贴图缺口."""
    missing = {}
    for name in sorted(os.listdir(LAYOUT)):
        if not name.lower().endswith(".xml"):
            continue
        path = os.path.join(LAYOUT, name)
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as handle:
                lines = handle.readlines()
        except OSError:
            continue
        for number, text in enumerate(lines, 1):
            for hit in FILE_RE.finditer(text):
                target = hit.group(1)
                if not target.lower().endswith((".png", ".ktx")):
                    continue
                disk = os.path.join(LAYOUT, target.replace("ui/mobile/", "").replace("/", os.sep))
                if os.path.exists(disk):
                    continue
                key = target
                missing.setdefault(key, []).append("%s:%d" % (name, number))
    return missing


def main():
    defined = collect_defined()
    used = collect_used()
    gaps = [item for item in used if normalize(item[0]) not in defined]
    unique = {}
    for sprite, layout, line, atlas in gaps:
        key = normalize(sprite)
        unique.setdefault(key, {"sprite": sprite, "atlas": atlas, "hits": []})
        unique[key]["hits"].append("%s:%d" % (layout, line))

    lines = []
    lines.append("# 精灵缺口清单")
    lines.append("")
    lines.append("口径: 布局文件里 uvname 引用的精灵名, 在所有图集的 sprite 定义里都找不到的, 记为缺口。")
    lines.append("")
    lines.append("| 项 | 值 |")
    lines.append("|:---:|:---:|")
    lines.append("| 图集已定义精灵 | %d |" % len(defined))
    lines.append("| 布局引用次数 | %d |" % len(used))
    lines.append("| 缺口引用次数 | %d |" % len(gaps))
    lines.append("| 缺口精灵种类 | %d |" % len(unique))
    lines.append("")
    if unique:
        lines.append("| 精灵名 | 声明的图集 | 引用处 |")
        lines.append("|:---:|:---:|:---|")
        for key in sorted(unique):
            entry = unique[key]
            lines.append("| %s%s%s | %s | %s |" % (TICK, entry["sprite"], TICK,
                                                  entry["atlas"] or "-",
                                                  "<br>".join(entry["hits"][:6])))
        lines.append("")
    textures = collect_texture_gaps()
    lines.append("")
    lines.append("## 整图贴图缺口")
    lines.append("")
    lines.append("口径: 布局里 file 直接指向 .png/.ktx 的整图, 磁盘上不存在的记为缺口。")
    lines.append("")
    lines.append("| 项 | 值 |")
    lines.append("|:---:|:---:|")
    lines.append("| 缺口贴图种类 | %d |" % len(textures))
    lines.append("")
    if textures:
        lines.append("| 贴图路径 | 引用处 |")
        lines.append("|:---:|:---|")
        for key in sorted(textures):
            lines.append("| %s%s%s | %s |" % (TICK, key, TICK, "<br>".join(textures[key][:6])))
        lines.append("")

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline="\n") as handle:
        handle.write("\n".join(lines))
    print("图集精灵 %d, 引用 %d, 精灵缺口 %d 次 / %d 种, 贴图缺口 %d 种"
          % (len(defined), len(used), len(gaps), len(unique), len(textures)))
    print("清单已写出 %s" % OUT)
    return 0


if __name__ == "__main__":
    sys.exit(main())
