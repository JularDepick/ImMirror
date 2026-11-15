#!/usr/bin/env bash
# 六屏验收: 编译一次转储工具, 逐屏跑脚本与绘制, 断言每屏都有输出且无脚本错误.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$(cd "$ROOT/../.." && pwd)/temp"
mkdir -p "$OUT_DIR"
cd "$ROOT" || exit 1

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
STATUS=0
SCREEN_INDEX=0

for spec in "大厅:lobby:LobbyFrame BackgroundFrame GongNengFrame" "登录:account:AccountLoginFrame" "单人:createworld:CreateWorldFrame" "设置:setting:SetMenuFrame GameSetFrame" "背包:backpack:BackpackFrame" "联机大厅:multiplayerlobby:MultiplayerLobbyFrame" "联机房间:room:RoomFrame" "选角:selectrole:SelectRoleFrame"; do
    label="${spec%%:*}"
    rest="${spec#*:}"
    file="${rest%%:*}"
    rest="${rest#*:}"
    frames="${rest%%:*}"
    # 第四段可选: 布局文件名; 不给时用 file 本身.
    layout="${rest#*:}"
    only=""
    if [ "$layout" = "$frames" ]; then
        layout="$file"
    elif [ "${layout#*:}" != "$layout" ]; then
        # 第五段: 只统计该子树(核对子界面时主帧仍打开提供参照物).
        only="${layout#*:}"
        layout="${layout%%:*}"
    fi
    args=()
    for frame in $frames; do
        args+=(--frame "$frame")
    done
    # 共享脚本: 界面脚本依赖 uicommon/globaldata 里的公共函数.
    # uitemplate.lua 提供 TemplateTabBtn_Init 这类模板初始化函数, 缺了列表类控件不会排布.
    args+=(--prelude uicommon.lua --prelude globaldata.lua --prelude uiconfig/uitemplate.lua)
    if [ -n "$only" ]; then
        args+=(--only "$only")
    fi
    # 打开主帧走脚本路径, 由引擎的生命周期事件链触发 OnLoad/OnShow, 贴近真实运行.
    args+=(--use-events)
    # 点击冒烟: 每屏点前 20 个带 OnClick 的可见元素, 验证交互链路通.
    args+=(--click-all 20)
    # 同一布局文件的不同主帧用不同 stem 区分, 日志名才不会互相覆盖.
    summary="$OUT_DIR/screen-$file.log"
    "$BIN" "$TEMPLATE_DIR/$layout.xml" "$SCRIPT_DIR" "$TEMPLATE_DIR" "${args[@]}" \
        > "$OUT_DIR/screen-$file.tsv" 2> "$summary" || { echo "FAIL $label 转储失败"; STATUS=1; continue; }
    line=$(grep '^布局' "$summary" || true)
    count=$(echo "$line" | grep -oE '可见命令 [0-9]+' | grep -oE '[0-9]+' || echo 0)
    hit=$(echo "$line" | grep -oE '精灵命中 [0-9]+' | grep -oE '[0-9]+' || echo 0)
    sprite=$(echo "$line" | grep -oE '带精灵 [0-9]+' | grep -oE '[0-9]+' || echo 0)
    errors=$(grep -c '脚本错误' "$summary" || true)
    printf '%-8s 命令 %-4s 精灵 %-3s 命中 %-3s 脚本错误 %s\n' "$label" "$count" "$sprite" "$hit" "$errors"
    if [ "$count" -le 0 ]; then
        echo "FAIL $label 没有可见命令"
        STATUS=1
    fi
    if [ "$errors" -ne 0 ]; then
        echo "FAIL $label 脚本报错"
        STATUS=1
    fi
    if [ "$sprite" -gt 0 ] && [ "$hit" -lt "$sprite" ]; then
        echo "提示 $label 有 $((sprite - hit)) 条精灵未命中"
    fi
    # 覆盖完整性: 应绘制的元素必须都产出绘制命令.
    undrawn=$(grep -oE '应绘制未绘制 [0-9]+' "$summary" | grep -oE '[0-9]+' || echo 0)
    if [ "${undrawn:-0}" -gt 0 ]; then
        echo "FAIL $label 有 $undrawn 个应绘制元素未绘制"
        STATUS=1
    fi
    # 交互冒烟: 尝试的每个按钮都必须真的执行到处理函数.
    tried=$(grep -oE '尝试 [0-9]+ 个' "$summary" | grep -oE '[0-9]+' | head -1 || echo 0)
    handled=$(grep -oE '执行成功 [0-9]+ 个' "$summary" | grep -oE '[0-9]+' | head -1 || echo 0)
    if [ "${tried:-0}" -lt 1 ]; then
        echo "FAIL $label 点击冒烟没有可点的元素"
        STATUS=1
    elif [ "${handled:-0}" -ne "${tried:-0}" ]; then
        echo "FAIL $label 点击冒烟 $tried 个里只成功 $handled 个"
        STATUS=1
    fi
    missing=$(grep -oE '锚点参照物缺失 [0-9]+' "$summary" | grep -oE '[0-9]+' || echo 0)
    if [ "${missing:-0}" -gt 0 ]; then
        echo "提示 $label 有 $missing 处锚点参照物缺失"
    fi
done

# 顺带把每屏的绘制清单导出成可读表格, 供与原版截图逐项对照.
BASELINE="$ROOT/../../../.agent/ui-baseline"
mkdir -p "$BASELINE"
for spec in "大厅:lobby" "登录:account" "单人:createworld" "设置:setting" "背包:backpack" "联机大厅:multiplayerlobby" "联机房间:room" "选角:selectrole" "存档筛选:lobby-archivefilter" "存档信息:lobby-archiveinfo" "世界备份:lobby-worldbackup" "多语言:lobby-multilang" "作品存档:lobby-miniworksarchive" "作品分享图:lobby-miniworksshare" "帮助提示:lobby-msgbox"; do
    label="${spec%%:*}"
    file="${spec#*:}"
    if [ -f "$OUT_DIR/screen-$file.tsv" ]; then
        PYTHONIOENCODING=utf-8 python "$ROOT/scripts/gen_drawlist.py" \
            "$OUT_DIR/screen-$file.tsv" "$label" "$BASELINE/drawlist-$file.md" > /dev/null
    fi
done
echo "绘制清单已导出到 .agent/ui-baseline/drawlist-*.md"

# 目标覆盖自查: 目标点名的每个界面都要有布局, 主帧与脚本.
PYTHONIOENCODING=utf-8 python "$ROOT/scripts/check_screen_coverage.py" | tail -1 || STATUS=1

# 字体表: 从 fonts.xml 重新生成, 保证字号与素材源同步.
PYTHONIOENCODING=utf-8 python "$ROOT/scripts/gen_font_index.py" | tail -1

# 精灵缺口普查: 布局引用的精灵名在所有图集里都找不到的, 单独成表.
PYTHONIOENCODING=utf-8 python "$ROOT/scripts/check_sprite_gaps.py" | tail -1

# 逐屏验收表: 每轮自动刷新, 避免口头数字与实测脱节.
PYTHONIOENCODING=utf-8 python "$ROOT/scripts/gen_acceptance.py"

# 布局健康度: 必须放在绘制清单生成之后, 否则读到的是上一轮的数据.
PYTHONIOENCODING=utf-8 python "$ROOT/scripts/check_layout_health.py" | tail -1

# 脚本影响面: 设置屏的脚本会创建页签内容, 有脚本必须比无脚本多出元素.
# 若脚本失效, 两者会相等 —— 这是第 43 轮 _G 表缺失那类问题的守门.
S_NS=$("$BIN" "$TEMPLATE_DIR/setting.xml" "$SCRIPT_DIR" "$TEMPLATE_DIR" \
    --frame SetMenuFrame --frame GameSetFrame > /dev/null 2> "$OUT_DIR/ns.log" ; \
    grep -oE '可见命令 [0-9]+' "$OUT_DIR/ns.log" | grep -oE '[0-9]+' | head -1)
S_WS=$("$BIN" "$TEMPLATE_DIR/setting.xml" "$SCRIPT_DIR" "$TEMPLATE_DIR" \
    --frame SetMenuFrame --frame GameSetFrame \
    --prelude uicommon.lua --prelude globaldata.lua --prelude uiconfig/uitemplate.lua \
    > /dev/null 2> "$OUT_DIR/ws.log" ; \
    grep -oE '可见命令 [0-9]+' "$OUT_DIR/ws.log" | grep -oE '[0-9]+' | head -1)
if [ "${S_WS:-0}" -le "${S_NS:-0}" ]; then
    echo "FAIL 设置屏脚本没有带来额外元素(无脚本 ${S_NS:-0} / 有脚本 ${S_WS:-0})"
    STATUS=1
else
    echo "脚本影响面: 设置屏 ${S_NS:-0} -> ${S_WS:-0}"
fi

# 确定性: 同一输入转储两次, 结果必须完全一致(防止引入随机性).
for det in lobby setting; do
    "$BIN" "$TEMPLATE_DIR/$det.xml" "$SCRIPT_DIR" "$TEMPLATE_DIR" \
        --prelude uicommon.lua --prelude globaldata.lua --prelude uiconfig/uitemplate.lua \
        > "$OUT_DIR/det-a.tsv" 2> /dev/null
    "$BIN" "$TEMPLATE_DIR/$det.xml" "$SCRIPT_DIR" "$TEMPLATE_DIR" \
        --prelude uicommon.lua --prelude globaldata.lua --prelude uiconfig/uitemplate.lua \
        > "$OUT_DIR/det-b.tsv" 2> /dev/null
    if ! diff -q "$OUT_DIR/det-a.tsv" "$OUT_DIR/det-b.tsv" > /dev/null 2>&1; then
        echo "FAIL $det 两次转储结果不一致"
        STATUS=1
    fi
done
echo "确定性: 两次转储一致"

# 标签覆盖: 布局里出现的标签必须被解析器处理, 或明确属于帧类型与脚本事件.
PYTHONIOENCODING=utf-8 python "$ROOT/scripts/check_tag_coverage.py" | tail -1

# 模板引用: inherits 引用的模板必须有定义.
PYTHONIOENCODING=utf-8 python "$ROOT/scripts/check_template_refs.py" | tail -1

# 属性覆盖: 布局里出现的属性名必须被解析器读取, 或明确不需要.
PYTHONIOENCODING=utf-8 python "$ROOT/scripts/check_attr_coverage.py" | tail -1

# 几何属性扫描: 尺寸与间距不能为负, 坐标不能超 4096.
PYTHONIOENCODING=utf-8 python "$ROOT/scripts/check_geometry_values.py" | tail -1

# 图集越界检查: 精灵矩形必须落在页面内.
PYTHONIOENCODING=utf-8 python "$ROOT/scripts/check_atlas_bounds.py" | tail -1

# 悬空引用清单: 汇总锚点指向但从未定义的帧名.
PYTHONIOENCODING=utf-8 python "$ROOT/scripts/gen_dangling_refs.py" | tail -1

exit $STATUS
