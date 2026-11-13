#!/usr/bin/env bash
# 锚点求解器的宿主断言: 用系统 g++ 编译并运行, 不依赖 Android 工具链.
# 求解器只做几何, 因此可以离线用数值断言验证, 不需要模拟器与截图.
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$(cd "$ROOT/../.." && pwd)/temp"
mkdir -p "$OUT_DIR"

g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_frame_test.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    -o "$OUT_DIR/layout_frame_test.exe" || exit 1
"$OUT_DIR/layout_frame_test.exe"
