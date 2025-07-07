cmake_minimum_required(VERSION 3.16)
project(BacktraderCpp)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# Core source files - minimal working set to avoid compilation errors
set(BACKTRADER_CORE_SOURCES
    src/lineroot.cpp
    src/linebuffer.cpp
    src/lineseries.cpp
    src/lineiterator.cpp
    src/dataseries.cpp
    src/indicator.cpp
    src/metabase.cpp
    src/errors.cpp
    src/mathsupport.cpp
    src/version.cpp
    src/comminfo.cpp
    src/position.cpp
    src/feed.cpp
    src/cerebro.cpp
    src/strategy.cpp
    src/order.cpp
    src/trade.cpp
    src/analyzer.cpp
    src/utils/date.cpp
    src/analyzers/sqn.cpp
    src/analyzers/timereturn.cpp
)

# Indicator source files - expanded set for tests
set(BACKTRADER_INDICATOR_SOURCES
    src/indicators/sma.cpp
    src/indicators/ema.cpp
    src/indicators/wma.cpp
    src/indicators/crossover.cpp
    src/indicators/basicops.cpp
    src/indicators/dm.cpp
    src/indicators/williamsad.cpp
    src/indicators/envelope.cpp
    src/indicators/lowest.cpp
    src/indicators/zlema.cpp
    src/indicators/zlind.cpp
    src/indicators/ichimoku.cpp
    src/indicators/heikinashi.cpp
    src/indicators/kama.cpp
    src/indicators/rsi.cpp
    src/indicators/macd.cpp
    src/indicators/bollinger.cpp
    src/indicators/atr.cpp
    src/indicators/dema.cpp
    src/indicators/momentum.cpp
    src/indicators/cci.cpp
    src/indicators/hma.cpp
    src/indicators/dpo.cpp
    src/indicators/dv2.cpp
    src/indicators/percentrank.cpp
    src/indicators/percentchange.cpp
    src/indicators/smma.cpp
    src/indicators/upmove.cpp
    src/indicators/ultimateoscillator.cpp
    src/indicators/trix.cpp
    src/indicators/oscillator.cpp
    src/indicators/accdecoscillator.cpp
    src/indicators/awesomeoscillator.cpp
    src/indicators/fractal.cpp
    src/indicators/aroon.cpp
    src/indicators/demaosc.cpp
    src/indicators/dma.cpp
    src/indicators/emaosc.cpp
    src/indicators/smaosc.cpp
    src/indicators/temaosc.cpp
    src/indicators/roc.cpp
    src/indicators/kamaenvelope.cpp
    src/indicators/kamaosc.cpp
    src/indicators/lrsi.cpp
    src/indicators/kst.cpp
    src/indicators/stochastic.cpp
)

# Skip complex components for now to get basic build working
# These can be added back once core compilation issues are resolved

# Create the core library with minimal components
add_library(backtrader_core STATIC
    ${BACKTRADER_CORE_SOURCES}
    ${BACKTRADER_INDICATOR_SOURCES}
)

target_include_directories(backtrader_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Create an interface library for header-only parts
add_library(backtrader_headers INTERFACE)
target_include_directories(backtrader_headers
    INTERFACE
        ${CMAKE_SOURCE_DIR}/include
)

# Enable testing (if building tests)
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS backtrader_core backtrader_headers
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Print configuration
message(STATUS "=== Backtrader C++ Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Source files: ${CMAKE_SOURCE_DIR}/src")
message(STATUS "Include files: ${CMAKE_SOURCE_DIR}/include")
message(STATUS "======================================")