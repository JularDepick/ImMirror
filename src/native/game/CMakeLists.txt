# 本目录源文件由构建时收集, 新增 .cpp 无需修改本文件
file(GLOB IMMIRROR_LOCAL_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
set(IMMIRROR_MODULE_SOURCES ${IMMIRROR_MODULE_SOURCES} ${IMMIRROR_LOCAL_SOURCES})
# 子模块: 物品定义与表 (item), 配方与匹配 (crafting)
add_subdirectory(item)
add_subdirectory(crafting)
set(IMMIRROR_MODULE_SOURCES "${IMMIRROR_MODULE_SOURCES}" PARENT_SCOPE)
