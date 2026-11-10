# 本目录源文件由构建时收集, 新增 .cpp 无需修改本文件
# main.cpp 是主机命令行工具, 不进入 Android 库, 因此从模块源文件中剔除

file(GLOB IMMIRROR_LOCAL_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(REMOVE_ITEM IMMIRROR_LOCAL_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

# 作为 native 子目录时把源文件汇总给上层, 独立配置时不需要
if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(IMMIRROR_MODULE_SOURCES ${IMMIRROR_MODULE_SOURCES} ${IMMIRROR_LOCAL_SOURCES})
    set(IMMIRROR_MODULE_SOURCES "${IMMIRROR_MODULE_SOURCES}" PARENT_SCOPE)
endif()

# 独立配置时 (cmake -S src/native/script -B ...) 默认构建主机工具与单元测试,
# 作为 native 子目录被包含时默认关闭, 保证 Android 产物只含运行期代码
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.16)
    project(immirror_script LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release)
    endif()
    set(IMMIRROR_SCRIPT_HOST_DEFAULT ON)
else()
    set(IMMIRROR_SCRIPT_HOST_DEFAULT OFF)
endif()

option(IMMIRROR_SCRIPT_HOST_TOOLS "构建脚本 CLI 与单元测试" ${IMMIRROR_SCRIPT_HOST_DEFAULT})

if(IMMIRROR_SCRIPT_HOST_TOOLS)
    add_library(immirror_script STATIC ${IMMIRROR_LOCAL_SOURCES})
    target_include_directories(immirror_script PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_features(immirror_script PUBLIC cxx_std_17)

    add_executable(immirror-script main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/tests/bench.cpp)
    target_link_libraries(immirror-script PRIVATE immirror_script)

    enable_testing()
    add_subdirectory(tests)
endif()
