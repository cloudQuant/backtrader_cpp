cmake_minimum_required(VERSION 3.16)
project(backtrader_tests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找GoogleTest
find_package(GTest REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/../
    ${CMAKE_SOURCE_DIR}/original_tests
    ${GTEST_INCLUDE_DIRS}
)

# 查找父目录的库
find_library(BACKTRADER_CORE_LIB backtrader_core PATHS ${CMAKE_SOURCE_DIR}/.. NO_DEFAULT_PATH)

# 如果找不到库文件，直接使用路径
if(NOT BACKTRADER_CORE_LIB)
    set(BACKTRADER_CORE_LIB ${CMAKE_SOURCE_DIR}/../libbacktrader_core.a)
    message(STATUS "Using direct path for backtrader_core: ${BACKTRADER_CORE_LIB}")
else()
    message(STATUS "Found backtrader_core library: ${BACKTRADER_CORE_LIB}")
endif()

# 获取所有测试文件
file(GLOB TEST_SOURCES "original_tests/test_*.cpp")

# 为每个测试文件创建可执行文件
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    # 链接库 - 使用找到的库路径
    target_link_libraries(${TEST_NAME}
        ${BACKTRADER_CORE_LIB}
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
        pthread
    )
    
    # 添加到CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# 启用测试
enable_testing()