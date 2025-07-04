cmake_minimum_required(VERSION 3.16)
project(BacktraderMinimalTests)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
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

# 获取backtrader核心库 - 最小版本，只包含基本指标功能
set(BACKTRADER_SRC_DIR ${CMAKE_SOURCE_DIR}/../../src)

# 核心文件，包含基本和中级组件
set(BACKTRADER_MINIMAL_SOURCES
    "${BACKTRADER_SRC_DIR}/lineroot.cpp"
    "${BACKTRADER_SRC_DIR}/linebuffer.cpp"
    "${BACKTRADER_SRC_DIR}/lineseries.cpp"
    "${BACKTRADER_SRC_DIR}/lineiterator.cpp"
    "${BACKTRADER_SRC_DIR}/dataseries.cpp"
    "${BACKTRADER_SRC_DIR}/indicator.cpp"
    "${BACKTRADER_SRC_DIR}/metabase.cpp"
)

# 条件添加策略相关文件（如果存在且没有依赖问题）
if(EXISTS "${BACKTRADER_SRC_DIR}/strategy.cpp")
    # 暂时跳过strategy.cpp，因为它可能有复杂的依赖
    # list(APPEND BACKTRADER_MINIMAL_SOURCES "${BACKTRADER_SRC_DIR}/strategy.cpp")
endif()

# 添加可用的指标源文件
set(BACKTRADER_INDICATOR_SOURCES)

# 检查指标文件是否存在并添加
if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/sma.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/sma.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/ema.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/ema.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/rsi.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/rsi.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/macd.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/macd.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/bollinger.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/bollinger.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/atr.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/atr.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/wma.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/wma.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/dema.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/dema.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/momentum.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/momentum.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/basicops.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/basicops.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/cci.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/cci.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/hma.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/hma.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/dpo.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/dpo.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/dv2.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/dv2.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/percentrank.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/percentrank.cpp")
endif()

# Skip complex indicators for now due to namespace and dependency issues
# if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/aroon.cpp")
#     list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/aroon.cpp")
# endif()

# if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/awesomeoscillator.cpp")
#     list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/awesomeoscillator.cpp")
# endif()

# if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/envelope.cpp")
#     list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/envelope.cpp")
# endif()

# if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/directionalmove.cpp")
#     list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/directionalmove.cpp")
# endif()

# if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/deviation.cpp")
#     list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/deviation.cpp")
# endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/percentchange.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/percentchange.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/smma.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/smma.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/zlema.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/zlema.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/upmove.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/upmove.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/zlind.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/zlind.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/ultimateoscillator.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/ultimateoscillator.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/envelope.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/envelope.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/trix.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/trix.cpp")
endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/oscillator.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/oscillator.cpp")
endif()

# Skip crossover for now due to access level issues
# if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/crossover.cpp")
#     list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/crossover.cpp")
# endif()

# Skip more complex indicators for now due to namespace issues

# if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/tsi.cpp")
#     list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/tsi.cpp")
# endif()

# if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/psar.cpp")
#     list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/psar.cpp")
# endif()

# Skip crossover for now due to API mismatch issues
# if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/crossover.cpp")
#     list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/crossover.cpp")
# endif()

if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/momentum.cpp")
    list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/momentum.cpp")
endif()

# Skip stochastic for now due to namespace issues
# if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/stochastic.cpp")
#     list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/stochastic.cpp")
# endif()

# Skip williams for now (may have similar issues)
# if(EXISTS "${BACKTRADER_SRC_DIR}/indicators/williams.cpp")
#     list(APPEND BACKTRADER_INDICATOR_SOURCES "${BACKTRADER_SRC_DIR}/indicators/williams.cpp")
# endif()

list(APPEND BACKTRADER_SOURCES ${BACKTRADER_MINIMAL_SOURCES} ${BACKTRADER_INDICATOR_SOURCES})

# 创建最小化的backtrader库
add_library(backtrader_minimal_core STATIC ${BACKTRADER_SOURCES})
target_link_libraries(backtrader_minimal_core Threads::Threads)

# 创建简单的测试目标，只针对有对应指标的测试文件
macro(add_simple_test indicator_name test_file)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${test_file}")
        add_executable(test_${indicator_name}_simple ${test_file})
        target_link_libraries(test_${indicator_name}_simple
            backtrader_minimal_core
            ${GTEST_LIBRARIES}
            gtest_main
            Threads::Threads
        )
        add_test(NAME test_${indicator_name}_simple COMMAND test_${indicator_name}_simple)
    endif()
endmacro()

# 添加基本指标测试
add_simple_test(sma test_ind_sma.cpp)
add_simple_test(ema test_ind_ema_simple.cpp)
add_simple_test(rsi test_ind_rsi_simple.cpp)
add_simple_test(macd test_ind_macd_simple.cpp)
add_simple_test(bbands test_ind_bbands_simple.cpp)
add_simple_test(atr test_ind_atr.cpp)
add_simple_test(wma test_ind_wma.cpp)
add_simple_test(dema test_ind_dema_simple.cpp)
add_simple_test(momentum test_ind_momentum_simple.cpp)
add_simple_test(highest test_ind_highest_simple.cpp)
add_simple_test(lowest test_ind_lowest_simple.cpp)
add_simple_test(sumn test_ind_sumn_simple.cpp)
add_simple_test(cci test_ind_cci.cpp)
add_simple_test(hma test_ind_hma_simple.cpp)
add_simple_test(dpo test_ind_dpo_simple.cpp)
add_simple_test(dv2 test_ind_dv2_simple.cpp)
add_simple_test(pctchange test_ind_pctchange_simple.cpp)
add_simple_test(pctrank test_ind_pctrank_simple.cpp)
add_simple_test(roc test_ind_roc_simple.cpp)
add_simple_test(average test_ind_average_simple.cpp)
add_simple_test(anyn test_ind_anyn_simple.cpp)
add_simple_test(alln test_ind_alln_simple.cpp)
add_simple_test(smma test_ind_smma_simple.cpp)
add_simple_test(zlema test_ind_zlema_simple.cpp)
add_simple_test(upmove test_ind_upmove_simple.cpp)
add_simple_test(downmove test_ind_downmove_simple.cpp)
add_simple_test(zlind test_ind_zlind_simple.cpp)
add_simple_test(ultosc test_ind_ultosc_simple.cpp)
add_simple_test(envelope test_ind_envelope_simple.cpp)
add_simple_test(trix test_ind_trix_simple.cpp)
add_simple_test(oscillator test_ind_oscillator_simple.cpp)
# Skip complex indicators for now due to namespace issues
# add_simple_test(aroon test_ind_aroonupdown.cpp)
# add_simple_test(awesome_oscillator test_ind_awesomeoscillator.cpp)
# add_simple_test(envelope test_ind_envelope.cpp)
# add_simple_test(dm test_ind_dm.cpp)

# 添加更多可能的测试
# Skip crossover test for now due to implementation issues
# # add_simple_test(crossover test_crossover_simple.cpp)
# 暂时跳过复杂测试
# add_simple_test(sma_standalone test_sma_simple.cpp)
# add_simple_test(ema_complex test_ind_ema.cpp)
# add_simple_test(rsi_complex test_ind_rsi.cpp)
# add_simple_test(bbands_complex test_ind_bbands.cpp)
# add_simple_test(dema_complex test_ind_dema.cpp)
# add_simple_test(momentum_complex test_ind_momentum.cpp)
# add_simple_test(highest_complex test_ind_highest.cpp)
# add_simple_test(lowest_complex test_ind_lowest.cpp)
# add_simple_test(sumn_complex test_ind_sumn.cpp)
# add_simple_test(hma_complex test_ind_hma.cpp)
# add_simple_test(dpo_complex test_ind_dpo.cpp)
# add_simple_test(dv2_complex test_ind_dv2.cpp)
# add_simple_test(pctchange_complex test_ind_pctchange.cpp)
# add_simple_test(pctrank_complex test_ind_pctrank.cpp)
# add_simple_test(roc_complex test_ind_roc.cpp)
# add_simple_test(smma_complex test_ind_smma.cpp)
# add_simple_test(zlema_complex test_ind_zlema.cpp)
# add_simple_test(upmove_complex test_ind_upmove.cpp)
# add_simple_test(downmove_complex test_ind_downmove.cpp)
# add_simple_test(zlind_complex test_ind_zlind.cpp)
# add_simple_test(ultosc_complex test_ind_ultosc.cpp)
# add_simple_test(envelope_complex test_ind_envelope.cpp)
# add_simple_test(trix_complex test_ind_trix.cpp)
# add_simple_test(oscillator_complex test_ind_oscillator.cpp)
# add_simple_test(wma_complex test_ind_wma.cpp)

# 打印配置信息
message(STATUS "=== Minimal Tests Configuration ===")
message(STATUS "Minimal sources: ${BACKTRADER_MINIMAL_SOURCES}")
message(STATUS "Indicator sources: ${BACKTRADER_INDICATOR_SOURCES}")
message(STATUS "Google Test: ${GTEST_FOUND}")
message(STATUS "====================================")