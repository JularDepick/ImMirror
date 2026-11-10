cmake_minimum_required(VERSION 3.22)

# 工程名与最终共享库同名, 方便在日志与打包结果中一一对应
project(immirror_core LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 胶水层以目标文件形式编入主库, 确保 ANativeActivity_onCreate 一定被导出,
# 同时不依赖 AGP 的外部原生构建, 由 scripts/build-native.sh 负责调用 CMake
add_library(immirror_app_glue OBJECT
    third_party/android_native_app_glue/src/android_native_app_glue.c
)
target_include_directories(immirror_app_glue PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/android_native_app_glue/include
)

# 各子系统在自己的目录里收集源文件, 汇总后统一编入库
set(IMMIRROR_MODULE_SOURCES "")
add_subdirectory(voxel)
add_subdirectory(audio)
add_subdirectory(ui)
add_subdirectory(world)
add_subdirectory(render)
add_subdirectory(player)
add_subdirectory(game)

# 引擎运行时: 基础库, 平台适配, 渲染后端与 JNI 导出
add_library(immirror_core SHARED
    $<TARGET_OBJECTS:immirror_app_glue>
    core/app_loop.cpp
    core/frame_clock.cpp
    core/frame_stats.cpp
    core/log.cpp
    jni/engine_host.cpp
    jni/native_entry.cpp
    platform/android_runtime.cpp
    platform/app_bridge.cpp
    ${IMMIRROR_MODULE_SOURCES}
)

target_include_directories(immirror_core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/android_native_app_glue/include
)

# android 提供 ANativeActivity/ALooper, log 提供日志, EGL 与 GLESv3 提供图形接口
target_link_libraries(immirror_core PRIVATE
    android
    log
    EGL
    GLESv3
)

target_compile_options(immirror_core PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-deprecated-declarations
)

set_target_properties(immirror_core PROPERTIES
    OUTPUT_NAME immirror_core
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
)