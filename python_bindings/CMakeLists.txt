cmake_minimum_required(VERSION 3.16)
project(backtrader_cpp_extended_bindings VERSION 0.4.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(pybind11 REQUIRED HINTS ${PYBIND11_ROOT})

# Include directories for backtrader headers
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Create Python module - fully backtrader-compatible
pybind11_add_module(backtrader_cpp src/backtrader_cpp_bindings.cpp)

# Create fixed indicators module for corrected implementations
pybind11_add_module(fixed_indicators src/fixed_indicators.cpp)

# For now, we'll build without linking to the core library
# TODO: Link with backtrader core library once it's built
# target_link_libraries(backtrader_cpp PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../build/libbacktrader_core.a
# )

# Configure fixed indicators module with same settings
target_compile_definitions(fixed_indicators PRIVATE 
    VERSION_INFO="${PROJECT_VERSION}"
)

set_target_properties(fixed_indicators PROPERTIES
    CXX_VISIBILITY_PRESET "hidden"
    VISIBILITY_INLINES_HIDDEN YES
    LINK_FLAGS "-static-libgcc -static-libstdc++"
)

target_compile_options(fixed_indicators PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -DNDEBUG>
    $<$<CONFIG:Debug>:-g -O0 -DDEBUG>
)

# Compiler-specific options
target_compile_definitions(backtrader_cpp PRIVATE 
    VERSION_INFO="${PROJECT_VERSION}"
)

# Set properties for better compatibility
set_target_properties(backtrader_cpp PROPERTIES
    CXX_VISIBILITY_PRESET "hidden"
    VISIBILITY_INLINES_HIDDEN YES
    LINK_FLAGS "-static-libgcc -static-libstdc++"
)

# Optimization flags
target_compile_options(backtrader_cpp PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -DNDEBUG>
    $<$<CONFIG:Debug>:-g -O0 -DDEBUG>
)

message(STATUS "=== Backtrader C++ Extended Bindings ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Features: 20+ Technical Indicators")
message(STATUS "=========================================")