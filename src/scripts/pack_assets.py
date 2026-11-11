#!/usr/bin/env python3
"""把运行时需要的资源子集复制到 Android assets 目录.

完整资源保存在素材管线仓库 (single-branch/assets) 的工程树里, 客户端仓库只保留
运行时直接消费的二进制文件, APK 再从中挑选实际用到的部分, 避免安装包过大.
新增资源时在 MANIFEST 里登记一行即可.
"""

import os
import shutil
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
ASSETS_ROOT = os.path.join(SRC_ROOT, "assets")
TARGET_ROOT = os.path.join(SRC_ROOT, "android", "app", "src", "main", "assets")

# 音效文件按材质分族, 破坏音与放置音各一套
MATERIALS = ("grass", "stone", "sand", "wood", "snow", "gravel")
MANIFEST = [(f"sounds/dig_{name}.ogg", f"sounds/dig_{name}.ogg") for name in MATERIALS]
MANIFEST += [(f"sounds/place_{name}.ogg", f"sounds/place_{name}.ogg") for name in MATERIALS]

# 界面素材按用途分目录, 全部随包下发
UI_DIRS = ("button", "panel", "dialog", "tab", "slot", "icon", "misc")

# 物品图标源目录: 素材管线的明文工程树 (PNG), 未打包时物品图标回落占位色块
ITEM_ICON_SOURCE = "../../../assets/project/items"


def collect_item_entries():
    """把物品图标全部纳入清单, 源与目标都带 items/ 前缀; 源目录是素材管线仓库."""
    entries = []
    source_dir = os.path.normpath(os.path.join(SCRIPT_DIR, ITEM_ICON_SOURCE))
    if not os.path.isdir(source_dir):
        return entries
    for entry in sorted(os.listdir(source_dir)):
        if entry.lower().endswith(".png"):
            source = os.path.join(source_dir, entry)
            entries.append((source, "items/" + entry))
    return entries


def collect_ui_entries():
    """把界面素材目录下的 PNG 全部纳入清单."""
    entries = []
    for name in UI_DIRS:
        directory = os.path.join(ASSETS_ROOT, "ui", name)
        if not os.path.isdir(directory):
            continue
        for entry in sorted(os.listdir(directory)):
            if entry.lower().endswith(".png"):
                entries.append((f"ui/{name}/{entry}", f"ui/{name}/{entry}"))
    return entries


def main() -> int:
    copied = 0
    missing = []
    for source_rel, target_rel in MANIFEST + collect_ui_entries() + collect_item_entries():
        source = os.path.join(ASSETS_ROOT, source_rel)
        target = os.path.join(TARGET_ROOT, target_rel)
        if not os.path.exists(source):
            missing.append(source_rel)
            continue
        os.makedirs(os.path.dirname(target), exist_ok=True)
        shutil.copy2(source, target)
        copied += 1
    print("资源打包完成: %d 个文件" % copied)
    if missing:
        print("缺少源文件: " + ", ".join(missing), file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())