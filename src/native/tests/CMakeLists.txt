# 主机单元测试 (编辑器命令层与体素光照), 只在主机工具链开启时参与构建:
#   cmake -S src/native/tests -B native/temp/editor-tests-build
#   cmake --build native/temp/editor-tests-build
#   ctest --test-dir native/temp/editor-tests-build
# 不进入 Android 库, 构建入口由本目录自己的 CMakeLists 承担.
cmake_minimum_required(VERSION 3.16)
project(immirror-editor-tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 原生源码根目录, 以本目录的上一级为准.
get_filename_component(IMMIRROR_NATIVE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# 用例与被测源码一起编进测试可执行文件.
# host_log_stub.cpp 顶替 core/log.cpp: 后者依赖 android/log.h, 主机没有 NDK 头.
# host_mingw_compat.h 通过 -include 注入: MinGW 的 mkdir 只有单参, 工程格式层按 POSIX 两参调用.
add_executable(immirror-editor-tests
    test_main.cpp
    test_editor_command.cpp
    test_light_field.cpp
    test_block_shape.cpp
    host_log_stub.cpp
    ../editor/editor_command.cpp
    ../editor/project_format.cpp
    ../editor/editor_session.cpp
    ../editor/editor_types.cpp
    ../editor/editor_json.cpp
    ../editor/trigger_model.cpp
    ../editor/undo_stack.cpp
    ../editor/generated/editor_data_gen.cpp
    ../voxel/block_registry.cpp
    ../voxel/chunk_column.cpp
    ../voxel/terrain_generator.cpp
    ../voxel/chunk_mesher.cpp
    ../voxel/light_field.cpp
    ../world/world.cpp
)

target_include_directories(immirror-editor-tests PRIVATE
    ${IMMIRROR_NATIVE_ROOT}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# MinGW 主机注入 mkdir 兼容宏, 其他工具链不需要.
if(MINGW)
    target_compile_options(immirror-editor-tests PRIVATE
        -include ${CMAKE_CURRENT_SOURCE_DIR}/host_mingw_compat.h)
endif()

target_compile_options(immirror-editor-tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wno-unused-parameter>
)

enable_testing()
add_test(NAME editor_command_tests COMMAND immirror-editor-tests)
