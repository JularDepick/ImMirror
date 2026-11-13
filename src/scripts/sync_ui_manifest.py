#!/usr/bin/env python3
"""把界面素材清单与安装包内实际文件对齐.

清单记录每个素材的路径, 尺寸, 分类与用途. 新增素材经导入脚本写入后,
清单可能落后于实际文件; 本脚本按目录实际内容补齐缺登记项, 尺寸直接从
PNG 文件头读取, 不改动已登记项, 也不删除任何条目.

用法: python scripts/sync_ui_manifest.py [--write]
"""

import json
import os
import struct
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
MANIFEST = os.path.join(SRC_ROOT, "assets", "ui", "ui_manifest.json")
ASSET_ROOT = os.path.join(SRC_ROOT, "android", "app", "src", "main", "assets", "ui")


def png_size(path):
    """从 PNG 的 IHDR 读宽高, 读不出返回 (0, 0)."""
    try:
        with open(path, "rb") as handle:
            head = handle.read(24)
    except OSError:
        return (0, 0)
    if len(head) < 24 or head[:8] != b"\x89PNG\r\n\x1a\n":
        return (0, 0)
    width, height = struct.unpack(">II", head[16:24])
    return (width, height)


def walk_assets():
    found = []
    for root, _dirs, files in os.walk(ASSET_ROOT):
        for name in files:
            if not name.endswith(".png"):
                continue
            full = os.path.join(root, name)
            rel = os.path.relpath(full, ASSET_ROOT).replace(os.sep, "/")
            found.append((rel, full))
    found.sort()
    return found


def main():
    write = "--write" in sys.argv
    with open(MANIFEST, encoding="utf-8") as handle:
        manifest = json.load(handle)
    entries = manifest.setdefault("assets", [])
    listed = {entry.get("path", "") for entry in entries}

    added = []
    for rel, full in walk_assets():
        if rel in listed:
            continue
        width, height = png_size(full)
        category = rel.split("/", 1)[0] if "/" in rel else ""
        entries.append({
            "path": rel,
            "width": width,
            "height": height,
            "category": category,
            "purpose": "",
        })
        added.append(rel)

    entries.sort(key=lambda entry: entry.get("path", ""))
    print("清单原有 %d 条, 本次补登记 %d 条, 现共 %d 条"
          % (len(listed), len(added), len(entries)))
    for rel in added[:10]:
        print("  +", rel)

    if write and added:
        with open(MANIFEST, "w", encoding="utf-8", newline="\n") as handle:
            json.dump(manifest, handle, ensure_ascii=False, indent=2)
            handle.write("\n")
        print("已写入", MANIFEST)
    elif not added:
        print("无需修改")
    return 0


if __name__ == "__main__":
    sys.exit(main())
