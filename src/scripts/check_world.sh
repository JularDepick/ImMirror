#!/usr/bin/env bash
# 世界域离线断言: 地形转储与群系覆盖.
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CLIENT="$(cd "$ROOT/../.." && pwd)"
OUT="$CLIENT/temp"
BIN="$OUT/world_dump.exe"
STATUS=0

fail() {
    echo "FAIL $1"
    STATUS=1
}

echo "== 编译世界转储"
g++ -std=c++17 -I "$ROOT/native" -O1 \
    "$ROOT/scripts/tests/world_dump.cpp" \
    "$ROOT/native/voxel/"*.cpp \
    "$ROOT/native/tests/host_log_stub.cpp" \
    -o "$BIN" || exit 1

echo "== 群系覆盖"
SCAN=$("$BIN" 1 0 --biome-scan 2>&1)
echo "$SCAN" | head -8
for band in Snow Desert Swamp Taiga Plains Forest; do
    n=$(echo "$SCAN" | grep " $band " | grep -oE '[0-9]+ \(' | grep -oE '^[0-9]+' | head -1)
    if [ -z "${n:-}" ] || [ "${n:-0}" -le 0 ]; then
        fail "群系带 $band 从未出现"
    fi
done

echo "== 地形转储"
DUMP=$("$BIN" 20260101 2 2>&1)
echo "$DUMP" | head -12
blocks=$(echo "$DUMP" | grep -oE '总方块 [0-9]+' | grep -oE '[0-9]+' | head -1)
kinds=$(echo "$DUMP" | grep -oE '方块种类 [0-9]+' | grep -oE '[0-9]+' | head -1)
surface=$(echo "$DUMP" | grep -oE '最低 [0-9]+ 最高 [0-9]+' | head -1)
if [ "${blocks:-0}" -le 0 ]; then
    fail "地形没有生成任何方块"
fi
if [ "${kinds:-0}" -lt 10 ]; then
    fail "方块种类过少: ${kinds:-0}"
fi
minh=$(echo "$surface" | grep -oE '最低 [0-9]+' | grep -oE '[0-9]+')
maxh=$(echo "$surface" | grep -oE '最高 [0-9]+' | grep -oE '[0-9]+')
if [ -n "${minh:-}" ] && [ "$minh" -lt 10 ]; then
    fail "表面高度过低: $minh"
fi
if [ -n "${maxh:-}" ] && [ "$maxh" -gt 200 ]; then
    fail "表面高度过高: $maxh"
fi

echo "== 天气参数"
PYTHONIOENCODING=utf-8 python "$ROOT/scripts/check_weather.py" | tail -8
if [ "$?" -ne 0 ]; then
    fail "天气参数与 biomedef.csv 不匹配"
fi

echo "== 地下结构"
cave=$(echo "$DUMP" | grep "地下格" | grep -oE "\([0-9.]+%\)" | head -1 | tr -d "()%")
water=$(echo "$DUMP" | grep -oE "液体格 [0-9]+" | grep -oE "[0-9]+" | head -1)
echo "  洞穴比例 ${cave:-?}% 液体格 ${water:-?}"
if [ -n "${cave:-}" ]; then
    if awk "BEGIN{exit !($cave < 0.2)}"; then
        fail "洞穴比例过低: $cave%"
    fi
    if awk "BEGIN{exit !($cave > 20)}"; then
        fail "洞穴比例过高: $cave%"
    fi
fi
if [ -z "${water:-}" ] || [ "${water:-0}" -le 0 ]; then
    fail "没有生成液体"
fi

echo "== 确定性"

A=$("$BIN" 12345 1 2>&1 | md5sum)
B=$("$BIN" 12345 1 2>&1 | md5sum)
if [ "$A" != "$B" ]; then
    fail "同种子两次转储不一致"
fi
echo "确定性: 一致"

if [ "$STATUS" -eq 0 ]; then
    echo "世界域断言全部通过"
fi
exit "$STATUS"
