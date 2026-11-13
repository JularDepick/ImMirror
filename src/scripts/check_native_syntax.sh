#!/usr/bin/env bash
# 原生源码语法检查: 用 NDK 的 clang 对全部 .cpp 做 -fsyntax-only, 不产生构建产物.
# 比整包构建快一到两个数量级, 用于每轮改动后的快速回归.
# 用法: bash scripts/check_native_syntax.sh [额外的 -I 路径 ...]
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
NATIVE_ROOT="$SRC_ROOT/native"
NDK_ROOT="${NDK_ROOT:-D:/AppWithoutInstall/android-ndk-r30}"
CLANG="$NDK_ROOT/toolchains/llvm/prebuilt/windows-x86_64/bin/clang++.exe"
SYSROOT="$NDK_ROOT/toolchains/llvm/prebuilt/windows-x86_64/sysroot"

if [ ! -x "$CLANG" ]; then
  echo "找不到 clang: $CLANG" >&2
  exit 2
fi

EXTRA_INCLUDES=("-I" "$NATIVE_ROOT" "-I" "$NATIVE_ROOT/third_party/android_native_app_glue/include" "-I" "$NATIVE_ROOT/tests")
for extra in "$@"; do
  EXTRA_INCLUDES+=("-I" "$extra")
done

cd "$NATIVE_ROOT" || exit 2
failed=0
checked=0
while IFS= read -r file; do
  checked=$((checked + 1))
  output=$("$CLANG" --target=aarch64-none-linux-android24 --sysroot="$SYSROOT" \
    -std=c++17 -fsyntax-only -Wno-unused-parameter -Wno-deprecated-declarations \
    "${EXTRA_INCLUDES[@]}" "$file" 2>&1 | head -4)
  if [ -n "$output" ]; then
    echo "== $file"
    echo "$output"
    failed=$((failed + 1))
  fi
done < <(find . -name '*.cpp' -not -path './third_party/*' -not -path './build/*' -not -path './temp/*' | sort)

echo "检查 $checked 个文件, 失败 $failed 个"
[ "$failed" -eq 0 ]
