#!/usr/bin/env python3
"""资源核对: 把代码与生成表里的资源路径与打包目录逐一比对.

资源找不到时运行期只写一行日志并静默回落, 因此这条检查是唯一能在不构建的
情况下发现路径写错的场合. 覆盖三类: 代码里的字面量路径, 生成表里的音效路径,
图标清单里的素材路径.
"""

import json
import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
PROJECT_ROOT = os.path.normpath(os.path.join(SRC_ROOT, "..", "..", "assets", "project"))
ASSETS = os.path.join(SRC_ROOT, "android", "app", "src", "main", "assets")
CODE_DIRS = [os.path.join(SRC_ROOT, "android", "app", "src", "main", "java"),
             os.path.join(SRC_ROOT, "native")]

LITERAL = re.compile(r'"((?:ui|generated|sounds|xml|data|fonts)/[^"]+\.[a-z0-9]{2,4})"')
SOUND_LITERAL = re.compile(r'"(sounds/[A-Za-z0-9_]+\.ogg)"')


def packaged_files():
    available = set()
    for root, _dirs, files in os.walk(ASSETS):
        for name in files:
            full = os.path.join(root, name)
            available.add(os.path.relpath(full, ASSETS).replace(os.sep, "/"))
    return available


def iter_code():
    for base in CODE_DIRS:
        for root, _dirs, files in os.walk(base):
            for name in files:
                if name.endswith((".java", ".cpp", ".h")):
                    yield name, os.path.join(root, name)


def check_literals(available):
    total = 0
    missing = []
    for name, path in iter_code():
        text = open(path, encoding="utf-8", errors="replace").read()
        for match in LITERAL.finditer(text):
            target = match.group(1)
            if "%" in target or "+" in target:
                continue
            total += 1
            if target not in available:
                missing.append((name, target))
    return total, missing


def check_generated_sounds(available):
    total = 0
    missing = []
    for name, path in iter_code():
        text = open(path, encoding="utf-8", errors="replace").read()
        for match in SOUND_LITERAL.finditer(text):
            total += 1
            if match.group(1) not in available:
                missing.append((name, match.group(1)))
    return total, missing


def check_icon_manifest():
    manifest = os.path.join(SRC_ROOT, "native", "ui", "generated", "item_icon_manifest.json")
    if not os.path.isfile(manifest):
        return 0, []
    data = json.load(open(manifest, encoding="utf-8")).get("items", {})
    missing = []
    for item_id, relative in data.items():
        if not os.path.isfile(os.path.join(PROJECT_ROOT, relative)):
            missing.append((item_id, relative))
    return len(data), missing


def check_ui_packaging():
    """界面布局与脚本: 打包目录的数量应当与素材源一致."""
    pairs = (
        (os.path.join(PROJECT_ROOT, "data", "xml", "ui", "mobile"),
         os.path.join(ASSETS, "ui", "layout"), ".xml"),
        (os.path.join(PROJECT_ROOT, "ui", "mobile"),
         os.path.join(ASSETS, "ui", "script"), ".lua"),
    )
    total = 0
    missing = []
    for source_root, target_root, extension in pairs:
        if not os.path.isdir(source_root):
            continue
        for root, _dirs, files in os.walk(source_root):
            for name in files:
                if not name.endswith(extension):
                    continue
                total += 1
                relative = os.path.relpath(os.path.join(root, name), source_root)
                if not os.path.isfile(os.path.join(target_root, relative)):
                    missing.append(("未打包", os.path.join(os.path.basename(target_root), relative)))
    return total, missing


def report(label, total, missing, limit=8):
    print("%s %d, 找不到 %d" % (label, total, len(missing)))
    for name, target in sorted(set(missing))[:limit]:
        print("  %-28s %s" % (name, target))
    return not missing


def main():
    if not os.path.isdir(ASSETS):
        print("打包资源目录不存在")
        return 1
    available = packaged_files()
    ok = True
    ok = report("资源字面量", *check_literals(available)) and ok
    ok = report("生成表音效", *check_generated_sounds(available)) and ok
    ok = report("图标清单", *check_icon_manifest()) and ok
    ok = report("界面资源", *check_ui_packaging()) and ok
    return 0 if ok else 2


if __name__ == "__main__":
    sys.exit(main())