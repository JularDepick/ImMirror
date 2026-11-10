# 单元测试与性能基准, 只在主机工具开启时参与构建
add_executable(immirror-script-tests
    test_main.cpp
    test_language.cpp
    test_sandbox.cpp
    test_bytecode.cpp
    bench.cpp
)
target_link_libraries(immirror-script-tests PRIVATE immirror_script)
target_include_directories(immirror-script-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME script_unit_tests COMMAND immirror-script-tests)
