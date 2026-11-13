#!/usr/bin/env python3
"""资源路径核对: 把代码里写死的资源路径与打包目录逐一比对.

资源找不到时运行期只写一行日志并静默回落, 因此这条检查是唯一能在不构建的
情况下发现路径写错的场合.
"""

import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
ASSETS = os.path.join(SRC_ROOT, "android", "app", "src", "main", "assets")
CODE_DIRS = [os.path.join(SRC_ROOT, "android", "app", "src", "main", "java"),
             os.path.join(SRC_ROOT, "native")]

PATTERN = re.compile(r'"((?:ui|generated|sounds|xml|data|fonts)/[^"]+\.[a-z0-9]{2,4})"')


def main():
    if not os.path.isdir(ASSETS):
        print("打包资源目录不存在")
        return 1
    available = set()
    for root, _dirs, files in os.walk(ASSETS):
        for name in files:
            full = os.path.join(root, name)
            available.add(os.path.relpath(full, ASSETS).replace(os.sep, "/"))

    total = 0
    missing = []
    for base in CODE_DIRS:
        for root, _dirs, files in os.walk(base):
            for name in files:
                if not name.endswith((".java", ".cpp", ".h")):
                    continue
                path = os.path.join(root, name)
                text = open(path, encoding="utf-8", errors="replace").read()
                for match in PATTERN.finditer(text):
                    target = match.group(1)
                    if "%" in target or "+" in target:
                        continue
                    total += 1
                    if target not in available:
                        missing.append((name, target))
    print("资源字面量 %d, 找不到 %d" % (total, len(missing)))
    for name, target in sorted(set(missing)):
        print("  %-28s %s" % (name, target))
    return 0 if not missing else 2


if __name__ == "__main__":
    sys.exit(main())