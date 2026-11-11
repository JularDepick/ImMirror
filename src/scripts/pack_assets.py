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

# 界面素材按用途分目录, 全部随包下发
UI_DIRS = ("button", "panel", "dialog", "tab", "slot", "icon", "misc")

# 物品图标源目录: 素材管线的明文工程树 (PNG), 未打包时物品图标回落占位色块
ITEM_ICON_SOURCE = "../../../assets/project/items"


def collect_sound_entries():
    """音效清单: 全部已打包的 ogg, 覆盖原版 blockdef 里的挖掘/放置/行走音取值."""
    entries = []
    directory = os.path.join(ASSETS_ROOT, "sounds")
    if not os.path.isdir(directory):
        return entries
    for name in sorted(os.listdir(directory)):
        if name.lower().endswith(".ogg"):
            entries.append(("sounds/" + name, "sounds/" + name))
    return entries


def collect_ui_entries():
    """界面素材: 按用途分目录的 PNG 全部纳入."""
    entries = []
    for name in UI_DIRS:
        directory = os.path.join(ASSETS_ROOT, "ui", name)
        if not os.path.isdir(directory):
            continue
        for entry in sorted(os.listdir(directory)):
            if entry.lower().endswith(".png"):
                entries.append(("ui/" + name + "/" + entry, "ui/" + name + "/" + entry))
    return entries


def collect_item_entries():
    """物品图标: 源在素材管线仓库, 保持文件名与物品表 icon 字段一致."""
    entries = []
    source_dir = os.path.normpath(os.path.join(SCRIPT_DIR, ITEM_ICON_SOURCE))
    if not os.path.isdir(source_dir):
        return entries
    for entry in sorted(os.listdir(source_dir)):
        if entry.lower().endswith(".png"):
            source = os.path.join(source_dir, entry)
            entries.append((source, "items/" + entry))
    return entries


def main() -> int:
    copied = 0
    missing = []
    entries = collect_sound_entries() + collect_ui_entries() + collect_item_entries()
    for source, target_rel in entries:
        target = os.path.join(TARGET_ROOT, target_rel)
        if not os.path.exists(source):
            missing.append(target_rel)
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
