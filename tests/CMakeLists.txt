cmake_minimum_required(VERSION 3.16)
project(BacktraderCppTests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译器选项
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# Simple test without external dependencies
add_executable(simple_test 
    simple_test.cpp
)

target_link_libraries(simple_test
    PRIVATE
        backtrader_core
)

target_include_directories(simple_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/../include
)

# Add test to CTest
add_test(NAME SimpleTest COMMAND simple_test)

# 查找Google Test (includes GMock) - optional
find_package(GTest QUIET)

# 查找Threads库（多线程支持）
find_package(Threads REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}
    ${GTEST_INCLUDE_DIRS}
)

# 添加源代码目录
set(BACKTRADER_SRC_DIR ${CMAKE_SOURCE_DIR}/../src)

# Note: backtrader_core library is already defined in src/CMakeLists.txt

# 定义测试可执行文件宏
macro(add_backtrader_test test_name test_sources)
    if(GTEST_FOUND)
        add_executable(${test_name} ${test_sources})
        target_link_libraries(${test_name}
            backtrader_core
            ${GTEST_LIBRARIES}
            ${GTEST_MAIN_LIBRARIES}
            Threads::Threads
        )
        
        target_include_directories(${test_name}
            PRIVATE
                ${GTEST_INCLUDE_DIRS}
                ${CMAKE_SOURCE_DIR}/../include
        )
        
        # 添加到测试套件
        add_test(NAME ${test_name} COMMAND ${test_name})
        
        # 设置测试属性
        set_target_properties(${test_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
        )
    else()
        message(STATUS "Skipping GTest-based test ${test_name} - GTest not found")
    endif()
endmacro()

# 启用测试
enable_testing()

# 单元测试
add_backtrader_test(test_metaclass test_metaclass.cpp)

# Note: Other tests will be added as files are created

# 创建运行所有测试的目标
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --parallel 4
    DEPENDS test_metaclass
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests"
)

# 创建快速测试目标（仅单元测试）
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "test_metaclass"
    DEPENDS test_metaclass
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running unit tests only"
)

# 代码覆盖率支持（如果使用GCC或Clang）
if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
    option(ENABLE_COVERAGE "Enable code coverage" OFF)
    
    if(ENABLE_COVERAGE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
        
        # 添加覆盖率报告目标
        find_program(GCOV_PATH gcov)
        find_program(LCOV_PATH lcov)
        find_program(GENHTML_PATH genhtml)
        
        if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
            add_custom_target(coverage
                # 清理之前的覆盖率数据
                COMMAND ${LCOV_PATH} --directory . --zerocounters
                
                # 运行测试
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                
                # 捕获覆盖率信息
                COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
                
                # 过滤掉系统文件和测试文件
                COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '*test*' '*gtest*' '*gmock*' --output-file coverage.info.cleaned
                
                # 生成HTML报告
                COMMAND ${GENHTML_PATH} -o coverage_html coverage.info.cleaned
                
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating code coverage report"
            )
        endif()
    endif()
endif()

# 静态分析支持
find_program(CPPCHECK_PATH cppcheck)
if(CPPCHECK_PATH)
    add_custom_target(static_analysis
        COMMAND ${CPPCHECK_PATH}
            --enable=all
            --std=c++17
            --verbose
            --quiet
            --error-exitcode=1
            --suppress=missingIncludeSystem
            --suppress=unmatchedSuppression
            --suppress=unusedFunction
            ${CMAKE_SOURCE_DIR}/../include
            ${CMAKE_SOURCE_DIR}/../src
        COMMENT "Running static analysis with cppcheck"
    )
endif()

# 内存检查支持（Valgrind）
find_program(VALGRIND_PATH valgrind)
if(VALGRIND_PATH)
    add_custom_target(memcheck
        COMMAND ${CMAKE_CTEST_COMMAND}
            --output-on-failure
            --overwrite MemoryCheckCommand=${VALGRIND_PATH}
            --overwrite MemoryCheckCommandOptions="--tool=memcheck --leak-check=full --show-reachable=yes --num-callers=20 --track-fds=yes"
            -T memcheck
        DEPENDS test_indicators test_integration
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running tests with Valgrind memory checker"
    )
endif()

# 安装测试二进制文件（可选）
install(TARGETS test_metaclass
    RUNTIME DESTINATION bin/tests
    COMPONENT tests
)

# 打印配置信息
message(STATUS "=== Backtrader C++ Test Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "CXX Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CXX Flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "Google Test found: ${GTEST_FOUND}")
message(STATUS "GMock found: Available in GTest")
message(STATUS "Coverage enabled: ${ENABLE_COVERAGE}")
message(STATUS "Cppcheck found: ${CPPCHECK_PATH}")
message(STATUS "Valgrind found: ${VALGRIND_PATH}")
message(STATUS "==========================================")

# 帮助信息
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Available targets:")
    message(STATUS "  make run_all_tests      - Run all tests")
    message(STATUS "  make run_unit_tests     - Run unit tests only")
    message(STATUS "  make run_performance_tests - Run performance tests")
    message(STATUS "  make run_stress_tests   - Run stress tests")
    if(ENABLE_COVERAGE)
        message(STATUS "  make coverage           - Generate coverage report")
    endif()
    if(CPPCHECK_PATH)
        message(STATUS "  make static_analysis    - Run static analysis")
    endif()
    if(VALGRIND_PATH)
        message(STATUS "  make memcheck           - Run memory checks")
    endif()
endif()