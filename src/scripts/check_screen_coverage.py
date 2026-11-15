#!/usr/bin/env python3
"""目标覆盖自查: 核对目标点名的每个界面都有布局文件, 主帧与脚本."""

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LAYOUT = os.path.join(ROOT, "..", "..", "assets", "project", "data", "xml", "ui", "mobile")
SCRIPT = os.path.join(ROOT, "..", "..", "assets", "project", "ui", "mobile")

# 目标点名的界面: 显示名, 布局文件, 主帧名, 脚本文件
TARGETS = [
    ("登录", "account.xml", "AccountLoginFrame", "account.lua"),
    ("大厅", "lobby.xml", "LobbyFrame", "lobby.lua"),
    ("单人", "createworld.xml", "CreateWorldFrame", "createworld.lua"),
    ("联机", "multiplayerlobby.xml", "MultiplayerLobbyFrame", "multiplayerlobby.lua"),
    ("背包", "backpack.xml", "BackpackFrame", "backpack.lua"),
    ("设置", "setting.xml", "SetMenuFrame", "setting.lua"),
]

NAME_RE = "name=\"%s\""


def main():
    failures = 0
    print("%-8s %-24s %-22s %s" % ("界面", "布局", "主帧", "脚本"))
    for label, layout, frame, script in TARGETS:
        layout_path = os.path.join(LAYOUT, layout)
        script_path = os.path.join(SCRIPT, script)
        layout_ok = os.path.exists(layout_path)
        script_ok = os.path.exists(script_path)
        frame_ok = False
        if layout_ok:
            try:
                body = open(layout_path, "r", encoding="utf-8", errors="replace").read()
                frame_ok = re.search(NAME_RE % re.escape(frame), body) is not None
            except OSError:
                layout_ok = False
        state = lambda ok: "有" if ok else "缺"
        print("%-8s %-24s %-22s %s" % (label, state(layout_ok), state(frame_ok), state(script_ok)))
        if not (layout_ok and frame_ok and script_ok):
            failures += 1
    if failures > 0:
        print("目标覆盖缺失 %d 项" % failures)
        return 1
    print("目标覆盖完整: %d 个界面全部就位" % len(TARGETS))
    return 0


if __name__ == "__main__":
    sys.exit(main())
