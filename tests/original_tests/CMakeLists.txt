cmake_minimum_required(VERSION 3.16)
project(BacktraderOriginalTests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找Google Test
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/../../include
    ${CMAKE_SOURCE_DIR}/..
    ${GTEST_INCLUDE_DIRS}
)

# 获取backtrader核心库
if(TARGET backtrader_core)
    set(BACKTRADER_CORE_LIB backtrader_core)
else()
    # 如果没有现成的库，收集源文件
    set(BACKTRADER_SRC_DIR ${CMAKE_SOURCE_DIR}/../../src)
    
    # 收集基础源文件
    file(GLOB BACKTRADER_CORE_SOURCES
        "${BACKTRADER_SRC_DIR}/*.cpp"
        "${BACKTRADER_SRC_DIR}/feeds/*.cpp"
        "${BACKTRADER_SRC_DIR}/analyzers/*.cpp"
    )
    
    # 只包含基本的工作指标，排除有问题的指标
    set(BACKTRADER_INDICATOR_SOURCES
        "${BACKTRADER_SRC_DIR}/indicators/IndicatorBase.cpp"
        "${BACKTRADER_SRC_DIR}/indicators/SMA.cpp"
        "${BACKTRADER_SRC_DIR}/indicators/EMA.cpp"
        "${BACKTRADER_SRC_DIR}/indicators/RSI.cpp"
        "${BACKTRADER_SRC_DIR}/indicators/MACD.cpp"
        "${BACKTRADER_SRC_DIR}/indicators/BollingerBands.cpp"
        "${BACKTRADER_SRC_DIR}/indicators/CrossOver.cpp"
    )
    
    # 添加策略源文件
    set(BACKTRADER_STRATEGY_SOURCES
        "${BACKTRADER_SRC_DIR}/strategy/Strategy.cpp"
        "${BACKTRADER_SRC_DIR}/broker/Order.cpp"
    )
    
    list(APPEND BACKTRADER_SOURCES ${BACKTRADER_CORE_SOURCES} ${BACKTRADER_INDICATOR_SOURCES} ${BACKTRADER_STRATEGY_SOURCES})
    
    add_library(backtrader_original_core STATIC ${BACKTRADER_SOURCES})
    target_link_libraries(backtrader_original_core Threads::Threads)
    set(BACKTRADER_CORE_LIB backtrader_original_core)
endif()

# 收集所有原始测试文件
file(GLOB ORIGINAL_TEST_SOURCES
    "test_ind_*.cpp"
)

# 创建SMA简单测试目标
add_executable(test_sma_simple test_sma_simple.cpp)
target_link_libraries(test_sma_simple
    ${BACKTRADER_CORE_LIB}
    ${GTEST_LIBRARIES}
    gtest_main
    Threads::Threads
)

add_test(NAME test_sma_simple COMMAND test_sma_simple)

# 创建EMA简单测试目标
add_executable(test_ema_simple test_ema_simple.cpp)
target_link_libraries(test_ema_simple
    ${BACKTRADER_CORE_LIB}
    ${GTEST_LIBRARIES}
    gtest_main
    Threads::Threads
)

add_test(NAME test_ema_simple COMMAND test_ema_simple)

# 创建RSI简单测试目标
add_executable(test_rsi_simple test_ind_rsi_simple.cpp)
target_link_libraries(test_rsi_simple
    ${BACKTRADER_CORE_LIB}
    ${GTEST_LIBRARIES}
    gtest_main
    Threads::Threads
)

add_test(NAME test_rsi_simple COMMAND test_rsi_simple)

# 创建MACD简单测试目标
add_executable(test_macd_simple test_ind_macd_simple.cpp)
target_link_libraries(test_macd_simple
    ${BACKTRADER_CORE_LIB}
    ${GTEST_LIBRARIES}
    gtest_main
    Threads::Threads
)

add_test(NAME test_macd_simple COMMAND test_macd_simple)

# 创建BollingerBands简单测试目标
add_executable(test_bbands_simple test_ind_bbands_simple.cpp)
target_link_libraries(test_bbands_simple
    ${BACKTRADER_CORE_LIB}
    ${GTEST_LIBRARIES}
    gtest_main
    Threads::Threads
)

add_test(NAME test_bbands_simple COMMAND test_bbands_simple)

# 创建CrossOver简单测试目标
add_executable(test_crossover_simple test_crossover_simple.cpp)
target_link_libraries(test_crossover_simple
    ${BACKTRADER_CORE_LIB}
    ${GTEST_LIBRARIES}
    gtest_main
    Threads::Threads
)

add_test(NAME test_crossover_simple COMMAND test_crossover_simple)

# 创建策略测试目标
add_executable(test_strategy_unoptimized test_strategy_unoptimized.cpp)
target_link_libraries(test_strategy_unoptimized
    ${BACKTRADER_CORE_LIB}
    ${GTEST_LIBRARIES}
    gtest_main
    Threads::Threads
)

add_test(NAME test_strategy_unoptimized COMMAND test_strategy_unoptimized)

# 为每个测试文件创建单独的可执行文件
foreach(test_source ${ORIGINAL_TEST_SOURCES})
    # 获取文件名（不含扩展名）
    get_filename_component(test_name ${test_source} NAME_WE)
    
    # 创建可执行文件
    add_executable(${test_name} ${test_source})
    
    # 链接库
    target_link_libraries(${test_name}
        ${BACKTRADER_CORE_LIB}
        ${GTEST_LIBRARIES}
        Threads::Threads
    )
    
    # 添加到测试套件
    add_test(NAME ${test_name} COMMAND ${test_name})
    
    # 设置测试属性
    set_target_properties(${test_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/original_tests
    )
endforeach()

# 创建运行所有原始测试的目标
add_custom_target(run_original_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "test_ind_"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all original indicator tests"
)

# 创建一个包含所有测试的单一可执行文件
add_executable(all_original_tests 
    ${ORIGINAL_TEST_SOURCES}
    main_original_tests.cpp
)

target_link_libraries(all_original_tests
    ${BACKTRADER_CORE_LIB}
    ${GTEST_LIBRARIES}
    Threads::Threads
)

# 添加调试版本的测试目标
add_custom_target(run_original_tests_debug
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose -R "test_ind_"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running original tests with debug output"
)

# 创建特定指标的测试目标
add_custom_target(run_sma_test
    COMMAND ./original_tests/test_ind_sma
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS test_ind_sma
    COMMENT "Running SMA indicator test"
)

add_custom_target(run_rsi_test
    COMMAND ./original_tests/test_ind_rsi
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS test_ind_rsi
    COMMENT "Running RSI indicator test"
)

add_custom_target(run_bbands_test
    COMMAND ./original_tests/test_ind_bbands
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS test_ind_bbands
    COMMENT "Running Bollinger Bands indicator test"
)

add_custom_target(run_atr_test
    COMMAND ./original_tests/test_ind_atr
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS test_ind_atr
    COMMENT "Running ATR indicator test"
)

add_custom_target(run_stochastic_test
    COMMAND ./original_tests/test_ind_stochastic
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS test_ind_stochastic
    COMMENT "Running Stochastic indicator test"
)

# 打印配置信息
message(STATUS "=== Original Tests Configuration ===")
message(STATUS "Found ${ORIGINAL_TEST_SOURCES} test files")
message(STATUS "Google Test: ${GTEST_FOUND}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "=====================================")

# 帮助信息
message(STATUS "Available original test targets:")
message(STATUS "  make run_original_tests      - Run all original tests")
message(STATUS "  make run_original_tests_debug - Run with debug output")
message(STATUS "  make run_sma_test           - Run SMA test only")
message(STATUS "  make run_rsi_test           - Run RSI test only")
message(STATUS "  make run_bbands_test        - Run BBands test only")
message(STATUS "  make run_atr_test           - Run ATR test only")
message(STATUS "  make run_stochastic_test    - Run Stochastic test only")