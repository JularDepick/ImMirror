# 编辑器命令层的单元测试, 只在主机工具链开启时参与构建:
#   cmake -S src/native/tests -B build/tests && cmake --build build/tests && ctest --test-dir build/tests
# 不进入 Android 库, 构建入口由测试目录自己的 CMakeLists 承担.
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

add_executable(immirror-editor-tests
    test_main.cpp
    test_editor_command.cpp
    android_log_stub.cpp
    ../editor/editor_command.cpp
    ../editor/project_format.cpp
    ../editor/editor_types.cpp
    ../editor/editor_json.cpp
    ../editor/trigger_model.cpp
    ../editor/undo_stack.cpp
    ../editor/generated/editor_data_gen.cpp
    ../voxel/block_registry.cpp
    ../voxel/chunk_column.cpp
    ../voxel/terrain_generator.cpp
    ../voxel/chunk_mesher.cpp
    ../world/world.cpp
)

target_include_directories(immirror-editor-tests PRIVATE
    ${IMMIRROR_NATIVE_ROOT}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Windows 主机编译需要 area 与 timezone 修正, 与 NDK 行为一致的宽宏处理.
target_compile_options(immirror-editor-tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wno-unused-parameter>
)

enable_testing()
add_test(NAME editor_command_tests COMMAND immirror-editor-tests)
