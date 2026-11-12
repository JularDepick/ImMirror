#!/usr/bin/env bash
#
# ImMirror 原生库外置构建脚本.
# 用外部 CMake 加 NDK 工具链逐个 ABI 构建 libimmirror_core.so,
# 再复制到 Android 工程的 jniLibs 目录, 由 Gradle 直接打包进 APK.
# 不使用 AGP 的 externalNativeBuild, 原因见 .agent/BUILD_RECIPE.md.
#
# 用法:
#   ./scripts/build-native.sh                     # 构建默认三 ABI
#   ./scripts/build-native.sh arm64-v8a           # 只构建指定 ABI (可空格分隔多个)
#   CLEAN=1 ./scripts/build-native.sh             # 先删除各 ABI 构建目录
#
set -euo pipefail

# ---------------------------------------------------------------- 可改常量
# NDK 根目录, 内含 build/cmake/android.toolchain.cmake 与预编译 make
NDK_ROOT="${NDK_ROOT:-D:/AppWithoutInstall/android-ndk-r30}"
# CMake 主程序, 留默认值表示由 PATH 解析
CMAKE_BIN="${CMAKE_BIN:-cmake}"
# 构建类型与目标平台级别
BUILD_TYPE="${BUILD_TYPE:-Release}"
ANDROID_PLATFORM="${ANDROID_PLATFORM:-android-24}"
# 静态链接 C++ 运行时, 让 APK 只需要打包一个自有 so
ANDROID_STL="${ANDROID_STL:-c++_static}"
# 生成器必须是 MinGW Makefiles, 用 Ninja 在本机会卡死
CMAKE_GENERATOR="${CMAKE_GENERATOR:-MinGW Makefiles}"
# NDK 自带的 make, 作为 MinGW Makefiles 生成器的构建程序
MAKE_PROGRAM="${MAKE_PROGRAM:-$NDK_ROOT/prebuilt/windows-x86_64/bin/make.exe}"
# 默认 ABI 列表, 调试阶段三个 ABI 全覆盖
DEFAULT_ABIS="arm64-v8a armeabi-v7a x86_64"
# 目标库名, 与 CMakeLists.txt 中 OUTPUT_NAME 保持一致
LIBRARY_NAME="immirror_core"
# 并行编译任务数
JOBS="${JOBS:-8}"
# 是否先清理构建目录, 置 1 生效
CLEAN="${CLEAN:-0}"

# ---------------------------------------------------------------- 路径推导
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
NATIVE_DIR="$SRC_ROOT/native"
JNI_LIBS_DIR="$SRC_ROOT/android/app/src/main/jniLibs"
BUILD_ROOT="$NATIVE_DIR/build"

# CMake 是原生 Windows 程序, 需要 Windows 风格路径, 用 cygpath 转换
to_host_path() {
    if command -v cygpath >/dev/null 2>&1; then
        cygpath -m "$1"
    else
        printf '%s' "$1"
    fi
}

require_path() {
    local label="$1"
    local path="$2"
    if [ ! -e "$path" ]; then
        printf '缺少%s: %s\n' "$label" "$path" >&2
        exit 1
    fi
}

# ---------------------------------------------------------------- 前置检查
require_path " NDK 目录" "$NDK_ROOT"
require_path " NDK 工具链文件" "$NDK_ROOT/build/cmake/android.toolchain.cmake"
require_path " NDK 自带的 make" "$MAKE_PROGRAM"
require_path " 原生源码目录" "$NATIVE_DIR"
if ! command -v "$CMAKE_BIN" >/dev/null 2>&1; then
    printf '缺少 cmake 可执行文件: %s\n' "$CMAKE_BIN" >&2
    exit 1
fi

ABIS="${*:-$DEFAULT_ABIS}"

# ---------------------------------------------------------------- 界面参数表
# HUD 的九宫边界与素材尺寸来自生成头文件, 编译前先生成, 避免 Java 与 C++ 各写一份坐标
PYTHON_BIN="${PYTHON_BIN:-python}"
if ! command -v "$PYTHON_BIN" >/dev/null 2>&1; then
    printf '缺少 python, 无法生成界面素材参数表: %s\n' "$PYTHON_BIN" >&2
    exit 1
fi
"$PYTHON_BIN" "$SCRIPT_DIR/gen_ui_resources.py" --no-assets

# ---------------------------------------------------------------- 方块数据表
# 方块中文名与掉落物只在素材仓库的 blockdef.csv 维护, 构建前生成原生头文件
"$PYTHON_BIN" "$SCRIPT_DIR/gen_block_data.py"

# ---------------------------------------------------------------- 物品与配方数据表
# 物品名称, 堆叠上限, 关联方块与合成配方只在素材仓库的 itemdef 与 crafting 表维护,
# 构建前生成原生头文件; 表内只收录与客户端方块相关的物品与可完整映射的配方
# ---------------------------------------------------------------- 方块光照表
# 光照三列与方块 id 映射必须与 gen_block_data.py 同源, 因此在它之后立即生成
"$PYTHON_BIN" "$SCRIPT_DIR/gen_block_light.py"

"$PYTHON_BIN" "$SCRIPT_DIR/gen_item_data.py"

# ---------------------------------------------------------------- 生物数据表
# 生物名称, 生命, 攻击, 尺寸与感知距离只在素材仓库的 monster 表维护, 构建前生成原生头文件
"$PYTHON_BIN" "$SCRIPT_DIR/gen_monster_data.py"

# ---------------------------------------------------------------- 熔炉数据表
# 熔炼配方与燃料只在素材仓库的 furnace 表维护, 构建前生成原生头文件;
# 材料与产出按 item 表的 sourceId 映射折算, 依赖 gen_item_data.py 先行执行
"$PYTHON_BIN" "$SCRIPT_DIR/gen_furnace_data.py"

# ---------------------------------------------------------------- 群系数据表
# 群系带只在素材仓库的 biomedef 表维护, 构建前按温度湿度折算成生成表
"$PYTHON_BIN" "$SCRIPT_DIR/gen_biome_data.py"

# ---------------------------------------------------------------- 矿脉数据表
# 矿脉的高度区间, 概率与替换方块只在素材仓库的 oredef 表维护;
# 客户端方块 id 复用 gen_block_data.py 的分配结果, 因此必须在它之后执行
"$PYTHON_BIN" "$SCRIPT_DIR/gen_ore_data.py"

# ---------------------------------------------------------------- 生物音效表
# 生物音效按生物 ID 分目录, 构建前折算成 路径对照表
"$PYTHON_BIN" "$SCRIPT_DIR/gen_entity_sound.py"

# ---------------------------------------------------------------- 工具属性表
# 工具表与物品表靠中文名对应, 因此必须在 gen_item_data.py 之后执行
"$PYTHON_BIN" "$SCRIPT_DIR/gen_tool_data.py"

# ---------------------------------------------------------------- 状态效果与食物
# 两张表都按原版物品 ID 挂到客户端物品, 因此必须在 gen_item_data.py 之后执行
"$PYTHON_BIN" "$SCRIPT_DIR/gen_buff_data.py"
"$PYTHON_BIN" "$SCRIPT_DIR/gen_food_data.py"

printf '原生构建配置\n'
printf '  源码目录     : %s\n' "$NATIVE_DIR"
printf '  构建根目录   : %s\n' "$BUILD_ROOT"
printf '  输出目录     : %s\n' "$JNI_LIBS_DIR"
printf '  NDK          : %s\n' "$NDK_ROOT"
printf '  构建类型     : %s\n' "$BUILD_TYPE"
printf '  目标平台     : %s\n' "$ANDROID_PLATFORM"
printf '  ABI 列表     : %s\n' "$ABIS"

# ---------------------------------------------------------------- 逐 ABI 构建
for abi in $ABIS; do
    abi_build_dir="$BUILD_ROOT/$abi"
    if [ "$CLEAN" = "1" ]; then
        rm -rf "$abi_build_dir"
    fi
    mkdir -p "$abi_build_dir"

    printf '==> 配置 %s\n' "$abi"
    "$CMAKE_BIN" \
        -S "$(to_host_path "$NATIVE_DIR")" \
        -B "$(to_host_path "$abi_build_dir")" \
        -G "$CMAKE_GENERATOR" \
        -DCMAKE_TOOLCHAIN_FILE="$(to_host_path "$NDK_ROOT/build/cmake/android.toolchain.cmake")" \
        -DCMAKE_MAKE_PROGRAM="$(to_host_path "$MAKE_PROGRAM")" \
        -DANDROID_ABI="$abi" \
        -DANDROID_PLATFORM="$ANDROID_PLATFORM" \
        -DANDROID_STL="$ANDROID_STL" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

    printf '==> 编译 %s\n' "$abi"
    "$CMAKE_BIN" --build "$(to_host_path "$abi_build_dir")" --parallel "$JOBS"

    built_library="$abi_build_dir/lib$LIBRARY_NAME.so"
    require_path " 构建产物" "$built_library"

    mkdir -p "$JNI_LIBS_DIR/$abi"
    cp "$built_library" "$JNI_LIBS_DIR/$abi/lib$LIBRARY_NAME.so"
    printf '==> 已复制 %s\n' "$JNI_LIBS_DIR/$abi/lib$LIBRARY_NAME.so"
done

# ---------------------------------------------------------------- 结果汇总
printf '原生库构建完成\n'
for abi in $ABIS; do
    target="$JNI_LIBS_DIR/$abi/lib$LIBRARY_NAME.so"
    if [ -f "$target" ]; then
        size_bytes="$(wc -c < "$target" | tr -d ' ')"
        printf '  %-12s %8s 字节\n' "$abi" "$size_bytes"
    fi
done
