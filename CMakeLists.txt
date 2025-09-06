cmake_minimum_required(VERSION 3.16)
project(backtrader_cpp VERSION 0.4.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(include)

# Get all source files
file(GLOB_RECURSE BACKTRADER_SOURCES src/*.cpp)

# Create core library
add_library(backtrader_core STATIC ${BACKTRADER_SOURCES})

# Set properties
set_target_properties(backtrader_core PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Compile options
target_compile_options(backtrader_core PRIVATE
    $<$<CONFIG:Release>:-O3 -march=native -DNDEBUG>
    $<$<CONFIG:Debug>:-g -O0 -DDEBUG>
)

# Link libraries
target_link_libraries(backtrader_core
    Threads::Threads
)

# Install library
install(TARGETS backtrader_core
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

# Add subdirectories
add_subdirectory(examples)
add_subdirectory(tests)

# Print build info
message(STATUS "=== Backtrader C++ Core Library ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Source files: ${BACKTRADER_SOURCES}")
message(STATUS "===================================")
