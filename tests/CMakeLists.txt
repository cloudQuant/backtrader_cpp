cmake_minimum_required(VERSION 3.16)
project(backtrader_tests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 跨平台线程库支持
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# 使用 Git Submodule 管理 GoogleTest
set(GOOGLETEST_DIR "${CMAKE_SOURCE_DIR}/../external/googletest")

if(EXISTS "${GOOGLETEST_DIR}/CMakeLists.txt")
    message(STATUS "使用已存在的 GoogleTest submodule: ${GOOGLETEST_DIR}")
    
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    
    # 直接添加 googletest 子目录
    add_subdirectory(${GOOGLETEST_DIR} ${CMAKE_BINARY_DIR}/googletest EXCLUDE_FROM_ALL)
    
    message(STATUS "GoogleTest 已加载")
else()
    message(FATAL_ERROR "GoogleTest submodule 不存在。请运行以下命令初始化：\n"
                       "  cd ${CMAKE_SOURCE_DIR}/..\n"
                       "  git submodule update --init --recursive external/googletest\n"
                       "或者使用脚本会自动初始化")
endif()

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/../
    ${CMAKE_SOURCE_DIR}/original_tests
)

# 查找父目录的库
find_library(BACKTRADER_LIB backtrader PATHS ${CMAKE_SOURCE_DIR}/.. NO_DEFAULT_PATH)

# 如果找不到库文件，直接使用路径
if(NOT BACKTRADER_LIB)
    set(BACKTRADER_LIB ${CMAKE_SOURCE_DIR}/../libbacktrader.a)
    message(STATUS "Using direct path for backtrader: ${BACKTRADER_LIB}")
else()
    message(STATUS "Found backtrader library: ${BACKTRADER_LIB}")
endif()

# 验证核心库存在
if(NOT EXISTS ${BACKTRADER_LIB})
    message(FATAL_ERROR "Backtrader library not found at: ${BACKTRADER_LIB}\n"
                       "Please build the library first by running 'make' in the parent directory.")
endif()

# 获取所有测试文件
file(GLOB TEST_SOURCES "original_tests/test_*.cpp")

# 为每个测试文件创建可执行文件
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    # 链接库 - 跨平台兼容
    target_link_libraries(${TEST_NAME}
        ${BACKTRADER_LIB}
        GTest::gtest
        GTest::gtest_main
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
message(STATUS "Core Library: ${BACKTRADER_LIB}")
message(STATUS "Test Sources: ${CMAKE_SOURCE_DIR}/original_tests")
message(STATUS "GoogleTest: Downloaded as external package")
message(STATUS "==========================================")