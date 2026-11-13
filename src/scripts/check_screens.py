#!/usr/bin/env python3
"""界面可达性核对: 每个界面类都要在清单里声明, 并且有跳转指向它.

界面写好了却没声明或没有入口时, 编译与用例都不会报错, 玩家却永远打不开,
因此这条检查是唯一能在不构建的情况下发现"死界面"的场合.
"""

import os
import re
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_ROOT = os.path.dirname(SCRIPT_DIR)
JAVA_ROOT = os.path.join(SRC_ROOT, "android", "app", "src", "main", "java")
MANIFEST = os.path.join(SRC_ROOT, "android", "app", "src", "main", "AndroidManifest.xml")


def check_strings(sources, manifest):
    """界面文案: 代码与清单里引用的字符串资源必须都有定义."""
    res = os.path.join(SRC_ROOT, "android", "app", "src", "main", "res", "values", "strings.xml")
    if not os.path.isfile(res):
        return 0, 0, []
    defined = set(re.findall(r'<string name="([^"]+)"', open(res, encoding="utf-8").read()))
    used = set()
    for text in sources:
        used |= set(re.findall(r"R\.string\.([A-Za-z0-9_]+)", text))
    used |= set(re.findall(r"@string/([A-Za-z0-9_]+)", manifest))
    missing = sorted(used - defined)
    return len(used), len(defined), missing


def main():
    if not os.path.isdir(JAVA_ROOT) or not os.path.isfile(MANIFEST):
        print("界面源码或清单不存在")
        return 1
    activities = []
    sources = []
    for root, _dirs, files in os.walk(JAVA_ROOT):
        for name in files:
            if not name.endswith(".java"):
                continue
            path = os.path.join(root, name)
            text = open(path, encoding="utf-8", errors="replace").read()
            sources.append(text)
            if name.endswith("Activity.java") and "extends Activity" in text:
                activities.append(name[:-5])
    blob = "\n".join(sources)
    manifest = open(MANIFEST, encoding="utf-8").read()

    undeclared = []
    unreachable = []
    for activity in sorted(activities):
        if activity not in manifest:
            undeclared.append(activity)
        if len(re.findall(r"\b" + activity + r"\.class", blob)) == 0:
            unreachable.append(activity)

    print("界面 %d 个" % len(activities))
    print("未在清单声明 %d: %s" % (len(undeclared), ", ".join(undeclared) or "无"))
    print("没有跳转指向 %d: %s" % (len(unreachable), ", ".join(unreachable) or "无"))

    used, defined, missing = check_strings(sources, manifest)
    print("字符串资源 引用 %d 定义 %d 缺定义 %d: %s"
          % (used, defined, len(missing), ", ".join(missing) or "无"))

    return 0 if not undeclared and not unreachable and not missing else 2


if __name__ == "__main__":
    sys.exit(main())