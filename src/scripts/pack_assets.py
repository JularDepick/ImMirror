#!/usr/bin/env python3
"""把运行时需要的资源复制到 Android assets 目录.

两类来源:
  1. 工程树 single-branch/assets/project 下的音效与物品图标, 由本脚本直接扫描并扁平化;
  2. 客户端仓库 src/assets/ui 下的界面素材, 由素材导入脚本登记后随包下发.

工程树是唯一权威来源: 客户端 src/assets 只保留界面素材的裁剪结果,
音效不再经中转目录, 避免中转层与工程树脱节.
新增界面素材时在 ui_manifest.json 里登记, 新增音效只需放进工程树的分类子目录.
"""

import os
import shutil
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
ASSETS_ROOT = os.path.join(SRC_ROOT, "assets")
TARGET_ROOT = os.path.join(SRC_ROOT, "android", "app", "src", "main", "assets")

# 界面素材按用途分目录, 全部随包下发
UI_DIRS = ("button", "panel", "dialog", "tab", "slot", "icon", "misc", "common")

# 物品图标源目录: 素材管线的明文工程树 (PNG), 未打包时物品图标回落占位色块
ITEM_ICON_SOURCE = "../../../assets/project/items"

# 大厅素材目录: 大厅模板图与直接引用图, 含 bigtex/ 与 extra/ 两个子目录
LOBBY_DIR = os.path.join(ASSETS_ROOT, "ui", "lobby")

# HUD 状态条素材目录: 从操作图集导出的 RGBA 精灵
HUD_DIR = os.path.join(ASSETS_ROOT, "ui", "hud")


# 音效源目录: 素材管线的明文工程树, 按子目录分类保存
SOUND_SOURCE = "../../../assets/project/sounds"

# 子目录到扁平文件名的前缀: 挖掘 / 放置 / 脚步 / 环境音各占一个前缀,
# 其余分类直接用子目录名做前缀, 保证扁平化后不重名.
SOUND_PREFIX = {
    "blockd": "dig",
    "blocks": "place",
    "env": "env",
    # phy 是机械与载具音效 (刹车, 碰撞, 引擎), 不是脚步声;
    # 脚步声来自客户端自制目录, 前缀已经在文件名里, 这里不能再改.
}


def collect_sound_entries():
    """音效清单: 工程树的分类子目录扁平化, 再叠加客户端自制音效.

    播放侧按扁平文件名拼路径, 因此这里统一改名; 带二级子目录的分类
    (例如界面音的 button 分组) 把子目录名并入文件名, 不丢分类信息.

    客户端仓库的 assets/sounds 里另有一批自制音效 (脚步声与部分挖掘放置音),
    工程树里没有对应来源, 必须一并收进来并覆盖同名项, 否则这些族会整体失声.
    """
    entries = []
    source_root = os.path.normpath(os.path.join(SCRIPT_DIR, SOUND_SOURCE))
    if not os.path.isdir(source_root):
        return entries
    for base, dirs, files in os.walk(source_root):
        dirs.sort()
        relative = os.path.relpath(base, source_root).replace("\\", "/")
        category = relative.split("/")[0]
        sub_dirs = relative.split("/")[1:] if "/" in relative else []
        prefix = SOUND_PREFIX.get(category, category)
        for name in sorted(files):
            if not name.lower().endswith(".ogg"):
                continue
            stem = name[:-4]
            parts = [prefix] + sub_dirs + [stem]
            target = "sounds/" + "_".join(parts) + ".ogg"
            entries.append((os.path.join(base, name), target))

    # 客户端自制音效放在前面, 同名的后写覆盖先写, 保证自制版本优先.
    local_root = os.path.join(ASSETS_ROOT, "sounds")
    local = []
    if os.path.isdir(local_root):
        for name in sorted(os.listdir(local_root)):
            if name.lower().endswith(".ogg"):
                local.append((os.path.join(local_root, name), "sounds/" + name))
    if local:
        local_targets = {target for _, target in local}
        entries = [pair for pair in entries if pair[1] not in local_targets]
        entries = local + entries
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
                entries.append((os.path.join(directory, entry), "ui/" + name + "/" + entry))
    return entries


def collect_ui_atlas_entries():
    """界面图集: 生成器产出的裸 RGBA 与格位表, 复制到 assets/ui/generated."""
    entries = []
    generated = os.path.join(ASSETS_ROOT, "ui", "generated")
    if not os.path.isdir(generated):
        return entries
    for name in ("atlas_ui.rgba", "atlas_ui.json"):
        source = os.path.join(generated, name)
        if os.path.exists(source):
            entries.append((source, "ui/generated/" + name))
    return entries


def collect_lobby_entries():
    """大厅素材: 模板图与补充图, 按 assets/ui/lobby 下的目录结构整体复制."""
    entries = []
    if not os.path.isdir(LOBBY_DIR):
        return entries
    for directory, subdirs, files in os.walk(LOBBY_DIR):
        subdirs.sort()
        for name in sorted(files):
            if not name.lower().endswith(".png"):
                continue
            source = os.path.join(directory, name)
            relative = os.path.relpath(source, LOBBY_DIR).replace(os.sep, "/")
            entries.append((source, "ui/lobby/" + relative))
    return entries


def collect_hud_entries():
    """HUD 素材: 状态条精灵, 按 assets/ui/hud 下的目录结构整体复制."""
    entries = []
    if not os.path.isdir(HUD_DIR):
        return entries
    for directory, subdirs, files in os.walk(HUD_DIR):
        subdirs.sort()
        for name in sorted(files):
            if not name.lower().endswith(".png"):
                continue
            source = os.path.join(directory, name)
            relative = os.path.relpath(source, HUD_DIR).replace(os.sep, "/")
            entries.append((source, "ui/hud/" + relative))
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
    entries = (collect_sound_entries() + collect_ui_entries()
               + collect_ui_atlas_entries()
               + collect_lobby_entries()
               + collect_hud_entries() + collect_item_entries())
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
