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

# 源相对路径 -> 目标相对路径
MANIFEST = [
    ("sounds/dig_grass.ogg", "sounds/dig_grass.ogg"),
    ("sounds/dig_stone.ogg", "sounds/dig_stone.ogg"),
    ("sounds/dig_sand.ogg", "sounds/dig_sand.ogg"),
    ("sounds/place_grass.ogg", "sounds/place_grass.ogg"),
    ("sounds/place_stone.ogg", "sounds/place_stone.ogg"),
    ("sounds/place_sand.ogg", "sounds/place_sand.ogg"),
]


def main() -> int:
    copied = 0
    missing = []
    for source_rel, target_rel in MANIFEST:
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
