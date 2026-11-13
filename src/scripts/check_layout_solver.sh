#!/usr/bin/env bash
# 界面布局的离线断言: 帧树与锚点求解, 布局解析.
# 三者都不依赖 Android 与图形接口, 因此可以用系统编译器直接验证.
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$(cd "$ROOT/../.." && pwd)/temp"
mkdir -p "$OUT_DIR"
STATUS=0

echo "== 锚点求解"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_frame_test.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    -o "$OUT_DIR/layout_frame_test.exe" || exit 1
"$OUT_DIR/layout_frame_test.exe" || STATUS=1

echo "== 布局解析"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_xml_test.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    -o "$OUT_DIR/layout_xml_test.exe" || exit 1
"$OUT_DIR/layout_xml_test.exe" "$ROOT/../../assets/project/data/xml/ui/mobile/account.xml" || STATUS=1

echo "== 控件绑定"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/widget_tree_test.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    -o "$OUT_DIR/widget_tree_test.exe" || exit 1
"$OUT_DIR/widget_tree_test.exe" "$ROOT/../../assets/project/data/xml/ui/mobile/account.xml" || STATUS=1

echo "== 全量控件加载"
"$OUT_DIR/widget_tree_test.exe" --batch "$ROOT/../../assets/project/data/xml/ui" || STATUS=1

echo "== 飞行物"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/projectile_test.cpp" \
    "$ROOT/native/entity/projectile.cpp" \
    -o "$OUT_DIR/projectile_test.exe" || exit 1
"$OUT_DIR/projectile_test.exe" || STATUS=1

echo "== 界面脚本解析"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/lua_parse_test.cpp" \
    "$ROOT/native/ui/script/lua_lexer.cpp" \
    "$ROOT/native/ui/script/lua_parser.cpp" \
    -o "$OUT_DIR/lua_parse.exe" || exit 1
"$OUT_DIR/lua_parse.exe" "$ROOT/../../assets/project/ui/mobile" || STATUS=1

echo "== 全量布局解析"
"$OUT_DIR/layout_xml_test.exe" --batch "$ROOT/../../assets/project/data/xml/ui" || STATUS=1

exit $STATUS
