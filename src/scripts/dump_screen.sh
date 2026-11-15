#!/usr/bin/env bash
# 屏幕级转储: 布局 + 同名脚本 + OnLoad, 输出可见绘制命令.
# 用法: dump_screen.sh <布局.xml> [输出.tsv]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$(cd "$ROOT/../.." && pwd)/temp"
mkdir -p "$OUT_DIR"
cd "$ROOT" || exit 1

LAYOUT="$1"
TARGET="${2:-}"
BIN="$OUT_DIR/ui_screen_dump.exe"

g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/ui_screen_dump.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    "$ROOT/native/ui/script/lua_lexer.cpp" \
    "$ROOT/native/ui/script/lua_parser.cpp" \
    "$ROOT/native/ui/script/lua_value.cpp" \
    "$ROOT/native/ui/script/lua_vm.cpp" \
    "$ROOT/native/ui/script/widget_host.cpp" \
    "$ROOT/scripts/tests/log_stub.cpp" \
    -o "$BIN"

SCRIPT_DIR="$ROOT/../../assets/project/ui/mobile"
TEMPLATE_DIR="$ROOT/../../assets/project/data/xml/ui/mobile"

shift || true
TARGET="${1:-}"
if [ -n "$TARGET" ]; then
    shift || true
fi

if [ -n "$TARGET" ]; then
    "$BIN" "$LAYOUT" "$SCRIPT_DIR" "$TEMPLATE_DIR" "$@" > "$TARGET"
    echo "已写出 $TARGET"
else
    "$BIN" "$LAYOUT" "$SCRIPT_DIR" "$TEMPLATE_DIR" "$@"
fi
