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

# Core source files
set(BACKTRADER_CORE_SOURCES
    src/lineroot.cpp
    src/linebuffer.cpp
    src/lineseries.cpp
    src/lineiterator.cpp
    src/dataseries.cpp
    src/indicator.cpp
    src/strategy.cpp
    src/cerebro.cpp
    src/broker.cpp
    src/order.cpp
    src/position.cpp
    src/comminfo.cpp
    src/trade.cpp
    src/sizer.cpp
    src/timer.cpp
    src/errors.cpp
    src/mathsupport.cpp
    src/functions.cpp
    src/store.cpp
    src/version.cpp
    src/signal.cpp
    src/fillers.cpp
    src/flt.cpp
    src/writer.cpp
    src/resamplerfilter.cpp
    src/tradingcal.cpp
    src/talib.cpp
    src/feed.cpp
    src/observer.cpp
    src/analyzer.cpp
    src/metabase.cpp
)

# Indicator source files
set(BACKTRADER_INDICATOR_SOURCES
    src/indicators/sma.cpp
    src/indicators/ema.cpp
    src/indicators/rsi.cpp
    src/indicators/macd.cpp
    src/indicators/bollinger.cpp
    src/indicators/atr.cpp
    src/indicators/crossover.cpp
    src/indicators/momentum.cpp
    src/indicators/stochastic.cpp
    src/indicators/aroon.cpp
    src/indicators/williams.cpp
    src/indicators/dema.cpp
    src/indicators/cci.cpp
    src/indicators/ichimoku.cpp
    src/indicators/kama.cpp
    src/indicators/lrsi.cpp
    src/indicators/tsi.cpp
    src/indicators/trix.cpp
    src/indicators/ultimateoscillator.cpp
    src/indicators/vortex.cpp
    src/indicators/priceoscillator.cpp
    src/indicators/psar.cpp
    src/indicators/hma.cpp
    src/indicators/smma.cpp
    src/indicators/directionalmove.cpp
    src/indicators/awesomeoscillator.cpp
    src/indicators/envelope.cpp
    src/indicators/percentchange.cpp
    src/indicators/percentrank.cpp
    src/indicators/pivotpoint.cpp
    src/indicators/basicops.cpp
    src/indicators/deviation.cpp
    src/indicators/dpo.cpp
    src/indicators/dma.cpp
    src/indicators/heikinashi.cpp
    src/indicators/dv2.cpp
    src/indicators/accdecoscillator.cpp
    src/indicators/wma.cpp
    src/indicators/zlema.cpp
    src/indicators/zlind.cpp
    src/indicators/kst.cpp
    src/indicators/rmi.cpp
    src/indicators/prettygoodoscillator.cpp
    src/indicators/oscillator.cpp
    src/indicators/hurst.cpp
    src/indicators/ols.cpp
)

# Analyzer source files
set(BACKTRADER_ANALYZER_SOURCES
    src/analyzers/returns.cpp
    src/analyzers/drawdown.cpp
    src/analyzers/sharpe.cpp
    src/analyzers/timereturn.cpp
    src/analyzers/annualreturn.cpp
    src/analyzers/tradeanalyzer.cpp
    src/analyzers/calmar.cpp
    src/analyzers/sqn.cpp
)

# Feed source files
set(BACKTRADER_FEED_SOURCES
    src/feeds/btcsv.cpp
    src/feeds/csvgeneric.cpp
    src/feeds/yahoo.cpp
    src/feeds/pandafeed.cpp
)

# Broker source files
set(BACKTRADER_BROKER_SOURCES
    src/brokers/bbroker.cpp
    src/brokers/ibbroker.cpp
)

# Observer source files
set(BACKTRADER_OBSERVER_SOURCES
    src/observers/broker.cpp
    src/observers/trades.cpp
)

# Create the core library
add_library(backtrader_core STATIC
    ${BACKTRADER_CORE_SOURCES}
    ${BACKTRADER_INDICATOR_SOURCES}
    ${BACKTRADER_ANALYZER_SOURCES}
    ${BACKTRADER_FEED_SOURCES}
    ${BACKTRADER_BROKER_SOURCES}
    ${BACKTRADER_OBSERVER_SOURCES}
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

# Enable testing
enable_testing()
add_subdirectory(tests)

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