cmake_minimum_required(VERSION 3.16)
project(backtrader_tests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 跨平台线程库支持
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# 查找GoogleTest - 跨平台兼容
find_package(GTest QUIET)
if(NOT GTest_FOUND)
    # 尝试使用pkg-config查找GTest (适用于Linux)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(GTEST gtest)
        pkg_check_modules(GTEST_MAIN gtest_main)
    endif()
    
    # 如果还是找不到，尝试手动查找
    if(NOT GTEST_FOUND)
        find_path(GTEST_INCLUDE_DIR gtest/gtest.h
            PATHS /usr/include /usr/local/include /opt/homebrew/include)
        find_library(GTEST_LIBRARY gtest
            PATHS /usr/lib /usr/local/lib /opt/homebrew/lib)
        find_library(GTEST_MAIN_LIBRARY gtest_main
            PATHS /usr/lib /usr/local/lib /opt/homebrew/lib)
        
        if(GTEST_INCLUDE_DIR AND GTEST_LIBRARY AND GTEST_MAIN_LIBRARY)
            set(GTEST_FOUND TRUE)
            set(GTEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIR})
            set(GTEST_LIBRARIES ${GTEST_LIBRARY})
            set(GTEST_MAIN_LIBRARIES ${GTEST_MAIN_LIBRARY})
            message(STATUS "Found GTest manually: ${GTEST_LIBRARIES}")
        else()
            message(FATAL_ERROR "Could not find GTest. Please install it using:\n"
                               "  Ubuntu: sudo apt-get install libgtest-dev\n"
                               "  Mac: brew install googletest")
        endif()
    endif()
endif()

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

# 验证核心库存在
if(NOT EXISTS ${BACKTRADER_CORE_LIB})
    message(FATAL_ERROR "Backtrader core library not found at: ${BACKTRADER_CORE_LIB}\n"
                       "Please build the core library first by running 'make' in the parent directory.")
endif()

# 获取所有测试文件
file(GLOB TEST_SOURCES "original_tests/test_*.cpp")

# 为每个测试文件创建可执行文件
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    # 链接库 - 跨平台兼容
    target_link_libraries(${TEST_NAME}
        ${BACKTRADER_CORE_LIB}
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
        Threads::Threads
    )
    
    # Mac特定的链接选项
    if(APPLE)
        target_link_options(${TEST_NAME} PRIVATE
            -Wl,-no_pie
        )
    endif()
    
    # 添加到CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    
    # 设置测试超时（30秒）
    set_tests_properties(${TEST_NAME} PROPERTIES TIMEOUT 30)
endforeach()

# 启用测试
enable_testing()

# 打印配置信息
message(STATUS "=== Backtrader C++ Tests Configuration ===")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "GTest Include: ${GTEST_INCLUDE_DIRS}")
message(STATUS "GTest Libraries: ${GTEST_LIBRARIES}")
message(STATUS "Core Library: ${BACKTRADER_CORE_LIB}")
message(STATUS "Test Sources: ${CMAKE_SOURCE_DIR}/original_tests")
message(STATUS "Number of tests: ${CMAKE_MATCH_COUNT}")
message(STATUS "==========================================")