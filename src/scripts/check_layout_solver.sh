#!/usr/bin/env bash
# 界面布局的离线断言: 帧树与锚点求解, 布局解析.
# 三者都不依赖 Android 与图形接口, 因此可以用系统编译器直接验证.
set -u

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$(cd "$ROOT/../.." && pwd)/temp"
mkdir -p "$OUT_DIR"
# 用例用相对路径写临时存档, 因此统一切到源码目录再跑, 与调用方所在目录无关.
cd "$ROOT" || exit 1
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
    "$ROOT/native/ui/hud_font.cpp" \
    -o "$OUT_DIR/layout_xml_test.exe" || exit 1
"$OUT_DIR/layout_xml_test.exe" "$ROOT/../../assets/project/data/xml/ui/mobile/account.xml" || STATUS=1

echo "== 绘制命令转顶点"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_render_test.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    -o "$OUT_DIR/layout_render_test.exe" || exit 1
"$OUT_DIR/layout_render_test.exe" || STATUS=1

echo "== 控件绑定"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/widget_tree_test.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    "$ROOT/scripts/tests/log_stub.cpp" \
    -o "$OUT_DIR/widget_tree_test.exe" || exit 1
"$OUT_DIR/widget_tree_test.exe" "$ROOT/../../assets/project/data/xml/ui/mobile/account.xml" || STATUS=1

echo "== 全量控件加载"
"$OUT_DIR/widget_tree_test.exe" --batch "$ROOT/../../assets/project/data/xml/ui" || STATUS=1

echo "== 背包容量"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/inventory_room_test.cpp" \
    "$ROOT/native/game/inventory.cpp" \
    "$ROOT/native/game/item/item_table.cpp" \
    "$ROOT/native/game/item/tool_table.cpp" \
    "$ROOT/native/game/item/enchant_table.cpp" \
    "$ROOT/native/voxel/block_registry.cpp" \
    "$ROOT/scripts/tests/log_stub.cpp" \
    -o "$OUT_DIR/inventory_room_test.exe" || exit 1
"$OUT_DIR/inventory_room_test.exe" || STATUS=1

echo "== 存档容错"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/save_load_test.cpp" \
    "$ROOT/native/world/world.cpp" \
    "$ROOT/native/world/world_time.cpp" \
    "$ROOT/native/world/weather.cpp" \
    "$ROOT/native/voxel/block_registry.cpp" \
    "$ROOT/native/voxel/chunk_column.cpp" \
    "$ROOT/native/voxel/chunk_mesher.cpp" \
    "$ROOT/native/voxel/light_field.cpp" \
    "$ROOT/native/voxel/terrain_generator.cpp" \
    "$ROOT/native/game/containers.cpp" \
    "$ROOT/native/game/inventory.cpp" \
    "$ROOT/native/game/armor.cpp" \
    "$ROOT/native/game/item/item_table.cpp" \
    "$ROOT/native/game/item/tool_table.cpp" \
    "$ROOT/native/game/item/enchant_table.cpp" \
    "$ROOT/scripts/tests/log_stub.cpp" \
    -o "$OUT_DIR/save_load_test.exe" || exit 1
"$OUT_DIR/save_load_test.exe" || STATUS=1

echo "== 角色皮肤"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/skin_data_test.cpp" \
    -o "$OUT_DIR/skin_data_test.exe" || exit 1
"$OUT_DIR/skin_data_test.exe" || STATUS=1

echo "== 爆炸抗性"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/block_blast_test.cpp" \
    -o "$OUT_DIR/block_blast_test.exe" || exit 1
"$OUT_DIR/block_blast_test.exe" || STATUS=1

echo "== 食物效果"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/food_buff_test.cpp" \
    -o "$OUT_DIR/food_buff_test.exe" || exit 1
"$OUT_DIR/food_buff_test.exe" || STATUS=1

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

echo "== 脚本求值"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/lua_vm_test.cpp" \
    "$ROOT/native/ui/script/lua_lexer.cpp" \
    "$ROOT/native/ui/script/lua_parser.cpp" \
    "$ROOT/native/ui/script/lua_value.cpp" \
    "$ROOT/native/ui/script/lua_vm.cpp" \
    -o "$OUT_DIR/lua_vm_test.exe" || exit 1
"$OUT_DIR/lua_vm_test.exe" || STATUS=1

echo "== 控件宿主"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/widget_host_test.cpp" \
    "$ROOT/native/ui/script/lua_lexer.cpp" \
    "$ROOT/native/ui/script/lua_parser.cpp" \
    "$ROOT/native/ui/script/lua_value.cpp" \
    "$ROOT/native/ui/script/lua_vm.cpp" \
    "$ROOT/native/ui/script/widget_host.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    "$ROOT/scripts/tests/log_stub.cpp" \
    -o "$OUT_DIR/widget_host_test.exe" || exit 1
"$OUT_DIR/widget_host_test.exe" "$ROOT/../../assets/project/data/xml/ui/mobile/account.xml" || STATUS=1

echo "== 事件系统"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/event_test.cpp" \
    "$ROOT/native/ui/script/lua_lexer.cpp" \
    "$ROOT/native/ui/script/lua_parser.cpp" \
    "$ROOT/native/ui/script/lua_value.cpp" \
    "$ROOT/native/ui/script/lua_vm.cpp" \
    "$ROOT/native/ui/script/widget_host.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    "$ROOT/scripts/tests/log_stub.cpp" \
    -o "$OUT_DIR/event_test.exe" || exit 1
"$OUT_DIR/event_test.exe" "$ROOT/../../assets/project/data/xml/ui/mobile/account.xml" || STATUS=1

echo "== 界面全局"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/ui_globals_test.cpp" \
    "$ROOT/native/ui/script/lua_lexer.cpp" \
    "$ROOT/native/ui/script/lua_parser.cpp" \
    "$ROOT/native/ui/script/lua_value.cpp" \
    "$ROOT/native/ui/script/lua_vm.cpp" \
    "$ROOT/native/ui/script/widget_host.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    "$ROOT/scripts/tests/log_stub.cpp" \
    -o "$OUT_DIR/ui_globals_test.exe" || exit 1
"$OUT_DIR/ui_globals_test.exe" "$ROOT/../../assets/project/data/xml/ui/mobile/account.xml" || STATUS=1

echo "== 数据绑定"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/data_binding_test.cpp" \
    "$ROOT/native/ui/script/lua_lexer.cpp" \
    "$ROOT/native/ui/script/lua_parser.cpp" \
    "$ROOT/native/ui/script/lua_value.cpp" \
    "$ROOT/native/ui/script/lua_vm.cpp" \
    "$ROOT/native/ui/script/widget_host.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    "$ROOT/scripts/tests/log_stub.cpp" \
    -o "$OUT_DIR/data_binding_test.exe" || exit 1
"$OUT_DIR/data_binding_test.exe" "$ROOT/../../assets/project/data/xml/ui/mobile/account.xml" || STATUS=1

echo "== 真实界面批量"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/ui_batch_test.cpp" \
    "$ROOT/native/ui/script/lua_lexer.cpp" \
    "$ROOT/native/ui/script/lua_parser.cpp" \
    "$ROOT/native/ui/script/lua_value.cpp" \
    "$ROOT/native/ui/script/lua_vm.cpp" \
    "$ROOT/native/ui/script/widget_host.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    "$ROOT/scripts/tests/log_stub.cpp" \
    -o "$OUT_DIR/ui_batch.exe" || exit 1
"$OUT_DIR/ui_batch.exe" "$ROOT/../../assets/project/data/xml/ui/mobile" \
    "$ROOT/../../assets/project/ui/mobile" || STATUS=1

echo "== 全量布局解析"
"$OUT_DIR/layout_xml_test.exe" --batch "$ROOT/../../assets/project/data/xml/ui" || STATUS=1

echo "== 九宫切片"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_slice_test.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    -o "$OUT_DIR/slice_test.exe" || exit 1
"$OUT_DIR/slice_test.exe" || STATUS=1

echo "== 文本对齐"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_text_test.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    -o "$OUT_DIR/text_test.exe" || exit 1
"$OUT_DIR/text_test.exe" || STATUS=1

echo "== 交互"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/ui_interaction_test.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    "$ROOT/native/ui/script/lua_lexer.cpp" \
    "$ROOT/native/ui/script/lua_parser.cpp" \
    "$ROOT/native/ui/script/lua_value.cpp" \
    "$ROOT/native/ui/script/lua_vm.cpp" \
    "$ROOT/native/ui/script/widget_host.cpp" \
    "$ROOT/scripts/tests/log_stub.cpp" \
    -o "$OUT_DIR/interaction_test.exe" || exit 1
"$OUT_DIR/interaction_test.exe" || STATUS=1

echo "== 输入映射"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/ui_input_test.cpp" \
    "$ROOT/native/ui/ui_input.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    "$ROOT/scripts/tests/log_stub.cpp" \
    -o "$OUT_DIR/input_test.exe" || exit 1
"$OUT_DIR/input_test.exe" || STATUS=1

echo "== 管理器单例"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/ui_manager_test.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    "$ROOT/native/ui/script/lua_lexer.cpp" \
    "$ROOT/native/ui/script/lua_parser.cpp" \
    "$ROOT/native/ui/script/lua_value.cpp" \
    "$ROOT/native/ui/script/lua_vm.cpp" \
    "$ROOT/native/ui/script/widget_host.cpp" \
    "$ROOT/scripts/tests/log_stub.cpp" \
    -o "$OUT_DIR/manager_test.exe" || exit 1
"$OUT_DIR/manager_test.exe" || STATUS=1

echo "== 转储工具"
# 转储工具是核对链路的一部分, 它自己坏掉会让"数据没变"的结论失真, 因此一并断言.
if bash "$ROOT/scripts/dump_screen.sh" "$ROOT/../../assets/project/data/xml/ui/mobile/account.xml" \
        "$OUT_DIR/screen_check.tsv" --frame AccountLoginFrame > "$OUT_DIR/dump_screen_check.log" 2>&1; then
    SCREEN_ROWS=$(wc -l < "$OUT_DIR/screen_check.tsv")
    echo "屏幕转储行数 $SCREEN_ROWS"
    if [ "$SCREEN_ROWS" -gt 1 ]; then
        echo "ok   屏幕转储可用"
    else
        echo "FAIL 屏幕转储为空"
        STATUS=1
    fi
else
    echo "FAIL 屏幕转储编不过"
    tail -5 "$OUT_DIR/dump_screen_check.log"
    STATUS=1
fi

if bash "$ROOT/scripts/dump_layout.sh" "$ROOT/../../assets/project/data/xml/ui/mobile/account.xml" \
        "$OUT_DIR/layout_check.tsv" --all > "$OUT_DIR/dump_layout_check.log" 2>&1; then
    LAYOUT_ROWS=$(wc -l < "$OUT_DIR/layout_check.tsv")
    echo "全量转储行数 $LAYOUT_ROWS"
    if [ "$LAYOUT_ROWS" -gt 1 ]; then
        echo "ok   全量转储可用"
    else
        echo "FAIL 全量转储为空"
        STATUS=1
    fi
else
    echo "FAIL 全量转储编不过"
    tail -5 "$OUT_DIR/dump_layout_check.log"
    STATUS=1
fi

echo "== 模板实例化"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_template_test.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    -o "$OUT_DIR/template_test.exe" || exit 1
"$OUT_DIR/template_test.exe" || STATUS=1

echo "== 多行文本"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_multiline_test.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    -o "$OUT_DIR/multiline_test.exe" || exit 1
"$OUT_DIR/multiline_test.exe" || STATUS=1

echo "== 文字描边"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_outline_test.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    -o "$OUT_DIR/outline_test.exe" || exit 1
"$OUT_DIR/outline_test.exe" || STATUS=1

echo "== 文字裁剪"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_clip_test.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    -o "$OUT_DIR/clip_test.exe" || exit 1
"$OUT_DIR/clip_test.exe" || STATUS=1

echo "== 翻转与描边"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_flip_outline_test.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    -o "$OUT_DIR/flip_outline_test.exe" || exit 1
"$OUT_DIR/flip_outline_test.exe" || STATUS=1

echo "== 布局数学"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_math2_test.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    -o "$OUT_DIR/math2_test.exe" || exit 1
"$OUT_DIR/math2_test.exe" || STATUS=1

echo "== 锚点数学"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_anchor_math_test.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    -o "$OUT_DIR/anchor_math_test.exe" || exit 1
"$OUT_DIR/anchor_math_test.exe" || STATUS=1

echo "== 绘制数学"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_math_test.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    -o "$OUT_DIR/math_test.exe" || exit 1
"$OUT_DIR/math_test.exe" || STATUS=1

echo "== 九宫 UV"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_slice_uv_test.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    -o "$OUT_DIR/slice_uv_test.exe" || exit 1
"$OUT_DIR/slice_uv_test.exe" || STATUS=1

echo "== 注释帧"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_comment_test.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    -o "$OUT_DIR/comment_test.exe" || exit 1
"$OUT_DIR/comment_test.exe" || STATUS=1

echo "== 流式布局"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_flowbox_test.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    -o "$OUT_DIR/flowbox_test.exe" || exit 1
"$OUT_DIR/flowbox_test.exe" || STATUS=1

echo "== 选中态"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_checked_test.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    -o "$OUT_DIR/checked_test.exe" || exit 1
"$OUT_DIR/checked_test.exe" || STATUS=1

echo "== 绘制路径同步"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_path_sync_test.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    -o "$OUT_DIR/path_sync_test.exe" || exit 1
"$OUT_DIR/path_sync_test.exe" || STATUS=1

echo "== 滑动裁剪"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_clip_sliding_test.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    -o "$OUT_DIR/clip_sliding_test.exe" || exit 1
"$OUT_DIR/clip_sliding_test.exe" || STATUS=1

echo "== 命中测试"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/layout_hit_test.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    -o "$OUT_DIR/hit_test.exe" || exit 1
"$OUT_DIR/hit_test.exe" || STATUS=1

echo "== 页签切换"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/ui_tab_test.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    "$ROOT/native/ui/layout_xml.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    "$ROOT/native/ui/widget_tree.cpp" \
    "$ROOT/native/ui/script/lua_lexer.cpp" \
    "$ROOT/native/ui/script/lua_parser.cpp" \
    "$ROOT/native/ui/script/lua_value.cpp" \
    "$ROOT/native/ui/script/lua_vm.cpp" \
    "$ROOT/native/ui/script/widget_host.cpp" \
    "$ROOT/scripts/tests/log_stub.cpp" \
    -o "$OUT_DIR/tab_test.exe" || exit 1
"$OUT_DIR/tab_test.exe" "$ROOT/../../assets/project/data/xml/ui/mobile" "$ROOT/../../assets/project/ui/mobile" || STATUS=1

echo "== 六屏验收"
bash "$ROOT/scripts/check_ui_screens.sh" || STATUS=1

echo "== 界面精灵索引"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/ui_sprite_index_test.cpp" \
    "$ROOT/native/ui/layout_render.cpp" \
    "$ROOT/native/ui/hud_font.cpp" \
    "$ROOT/native/ui/layout_draw.cpp" \
    "$ROOT/native/ui/layout_frame.cpp" \
    -o "$OUT_DIR/sprite_test.exe" || exit 1
"$OUT_DIR/sprite_test.exe" || STATUS=1

echo "== 界面纹理容器"
g++ -std=c++17 -I "$ROOT/native" \
    "$ROOT/scripts/tests/ui_texture_ktx_test.cpp" \
    "$ROOT/native/ui/ui_texture_ktx.cpp" \
    -o "$OUT_DIR/ktx_test.exe" || exit 1
"$OUT_DIR/ktx_test.exe" "$ROOT/../../assets/project/ui/mobile/texture2" || STATUS=1

exit $STATUS
