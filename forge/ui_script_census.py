"""界面脚本的绑定动作普查.

只读. 统计界面脚本里出现的控件方法名频次, 用于确定控件绑定层要先支持哪些动作,
以及脚本用到的全局入口集中在哪几类.

用法: python -m forge.ui_script_census [--write]
"""

import collections
import os
import re
import sys

PROJECT_ROOT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "project")
UI_ROOT = os.path.join(PROJECT_ROOT, "ui", "mobile")
OUT_PATH = os.path.join(os.path.dirname(PROJECT_ROOT), "reports", "ui_script_census.md")


def iter_lua():
    for root, _dirs, files in os.walk(UI_ROOT):
        for name in sorted(files):
            if name.endswith(".lua"):
                yield os.path.join(root, name)


def main():
    write = "--write" in sys.argv
    files = 0
    lines = 0
    methods = collections.Counter()
    globals_used = collections.Counter()
    events = collections.Counter()

    for path in iter_lua():
        files += 1
        text = open(path, encoding="utf-8", errors="replace").read()
        lines += text.count("\n") + 1
        for match in re.finditer(r":([A-Za-z_][A-Za-z0-9_]*)\s*\(", text):
            methods[match.group(1)] += 1
        for match in re.finditer(r"\bRegisterEvent\s*\(\s*[\"']([\w.]+)", text):
            events[match.group(1)] += 1
        for match in re.finditer(r"\b([A-Z][A-Za-z0-9_]{2,})\s*\(", text):
            globals_used[match.group(1)] += 1

    report = ["# 界面脚本绑定动作普查", "",
              "| 项 | 值 |", "|:---:|:---:|",
              "| 脚本文件 | %d |" % files,
              "| 源码行数 | %d |" % lines,
              "| 不同控件方法 | %d |" % len(methods),
              "| 不同事件名 | %d |" % len(events),
              "", "## 控件方法 (前 25)", "", "| 方法 | 次数 |", "|:---:|:---:|"]
    for name, count in methods.most_common(25):
        report.append("| %s | %d |" % (name, count))
    total_calls = sum(methods.values())
    report += ["", "## 累计覆盖", "", "| 前 N 个方法 | 调用占比 |", "|:---:|:---:|"]
    running = 0
    for index, (_name, count) in enumerate(methods.most_common(20), start=1):
        running += count
        if index in (5, 9, 13, 20) and total_calls > 0:
            report.append("| %d | %.1f%% |" % (index, 100.0 * running / total_calls))
    report += ["", "## 事件 (前 15)", "", "| 事件 | 次数 |", "|:---:|:---:|"]
    for name, count in events.most_common(15):
        report.append("| %s | %d |" % (name, count))
    report += ["", "## 全局入口 (前 15)", "", "| 名 | 次数 |", "|:---:|:---:|"]
    for name, count in globals_used.most_common(15):
        report.append("| %s | %d |" % (name, count))
    body = "\n".join(report) + "\n"
    print(body)
    if write:
        os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
        open(OUT_PATH, "w", encoding="utf-8", newline="\n").write(body)
        print("已写入 %s" % OUT_PATH)
    return 0


if __name__ == "__main__":
    sys.exit(main())
