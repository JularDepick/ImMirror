#!/usr/bin/env bash
# 把一份布局转储成绘制命令表, 用于与原版 XML 逐项比对.
# 用法: dump_layout.sh <布局.xml> [输出.tsv] [--all]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$(cd "$ROOT/../.." && pwd)/temp"
mkdir -p "$OUT_DIR"
cd "$ROOT" || exit 1

LAYOUT="$1"
TARGET="${2:-}"
MODE="${3:-}"
BIN="$OUT_DIR/layout_dump.exe"

g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_dump.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    "$ROOT/scripts/tests/log_stub.cpp" \
    -o "$BIN"

TEMPLATES="$ROOT/../../assets/project/ui/mobile/uiconfig/uitemplate.xml"
if [ ! -f "$TEMPLATES" ]; then
    TEMPLATES=""
fi

if [ -n "$TARGET" ]; then
    "$BIN" "$LAYOUT" "$TEMPLATES" 1280 720 $MODE > "$TARGET"
    echo "已写出 $TARGET"
else
    "$BIN" "$LAYOUT" "$TEMPLATES" 1280 720 $MODE
fi
