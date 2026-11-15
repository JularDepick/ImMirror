#!/usr/bin/env python3
"""布局规模核对: 对比布局文件声明的元素数与引擎解析出的帧数."""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LAYOUT = os.path.join(ROOT, "..", "..", "assets", "project", "data", "xml", "ui", "mobile")
LOG_DIR = os.path.join(ROOT, "..", "..", "temp")
OUT = os.path.join(ROOT, "..", "..", "..", ".agent", "ui-baseline", "layout-scale.md")

SCREENS = [
    ("\u5927\u5385", "lobby", "LobbyFrame"),
    ("\u767b\u5f55", "account", "AccountLoginFrame"),
    ("\u5355\u4eba", "createworld", "CreateWorldFrame"),
    ("\u8bbe\u7f6e", "setting", "SetMenuFrame"),
    ("\u80cc\u5305", "backpack", "BackpackFrame"),
    ("\u8054\u673a\u5927\u5385", "multiplayerlobby", "MultiplayerLobbyFrame"),
    ("\u8054\u673a\u623f\u95f4", "room", "RoomFrame"),
    ("\u9009\u89d2", "selectrole", "SelectRoleFrame"),
]

ELEMENT_RE = re.compile(r"<(Frame|Button|Texture|FontString|EditBox|RichText|SlidingFrame|LayoutBox)\b")
HIDDEN_RE = re.compile(r"hidden=\"true\"")


def main():
    lines = [
        "# 布局规模核对",
        "",
        "\u53e3\u5f84: \u5e03\u5c40\u6587\u4ef6\u91cc\u58f0\u660e\u7684\u5143\u7d20\u6570\u4e0e\u5f15\u64ce\u89e3\u6790\u51fa\u7684\u5e27\u6570\u5bf9\u6bd4\u3002",
        "\u89e3\u6790\u5e27\u6570\u5305\u542b\u6a21\u677f\u514b\u9686, \u56e0\u6b64\u53ef\u80fd\u591a\u4e8e\u58f0\u660e\u6570; \u5c11\u4e8e\u58f0\u660e\u6570\u7684 60% \u9700\u8981\u67e5\u539f\u56e0\u3002",
        "",
        "| \u5c4f | \u58f0\u660e\u5143\u7d20 | \u5176\u4e2d\u9690\u85cf | \u89e3\u6790\u5e27\u6570 | \u5360\u6bd4 |",
        "|:---:|:---:|:---:|:---:|:---:|",
    ]
    for label, stem, frame in SCREENS:
        path = os.path.join(LAYOUT, stem + ".xml")
        declared = 0
        hidden = 0
        try:
            body = open(path, "r", encoding="utf-8", errors="replace").read()
            declared = len(ELEMENT_RE.findall(body))
            hidden = len(HIDDEN_RE.findall(body))
        except OSError:
            pass
        parsed = 0
        log = os.path.join(LOG_DIR, "screen-%s.log" % stem)
        try:
            for text in open(log, "r", encoding="utf-8", errors="replace"):
                # 一屏可能有多个主帧, 必须累加, 否则占比会被严重低估.
                if "\u5b50\u6811" in text and "\u5e27" in text:
                    found = re.search(r"\u5e27 (\d+)", text)
                    if found is not None:
                        parsed += int(found.group(1))
        except OSError:
            pass
        ratio = (parsed * 100 // declared) if declared > 0 else 0
        lines.append("| %s | %d | %d | %d | %d%% |" % (label, declared, hidden, parsed, ratio))
    lines.append("")
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8", newline=chr(10)) as handle:
        handle.write(chr(10).join(lines))
    print("\u5e03\u5c40\u89c4\u6a21\u6838\u5bf9\u5df2\u5199\u51fa")
    return 0


if __name__ == "__main__":
    sys.exit(main())
