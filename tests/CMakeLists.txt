cmake_minimum_required(VERSION 3.16)
project(BacktraderCppTests)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Google Test
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTEST REQUIRED gtest)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/original_tests
    ${CMAKE_SOURCE_DIR}/../include
    ${GTEST_INCLUDE_DIRS}
)

# Link directories
link_directories(
    ${GTEST_LIBRARY_DIRS}
    ${CMAKE_BINARY_DIR}
)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GTEST_CFLAGS_OTHER}")

# Find all test source files in original_tests subdirectory
file(GLOB TEST_SOURCES "original_tests/test_*.cpp")

# Create test executables
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    # Skip test_common.h as it's a header file
    if(NOT TEST_NAME STREQUAL "test_common")
        add_executable(${TEST_NAME} ${TEST_SOURCE})
        
        # Link to pre-built library - using direct path since build_tests contains the library
        set(BACKTRADER_LIB_PATH "${CMAKE_BINARY_DIR}/../libbacktrader_core.a")
        if(EXISTS "${BACKTRADER_LIB_PATH}")
            target_link_libraries(${TEST_NAME} 
                "${BACKTRADER_LIB_PATH}"
                ${GTEST_LIBRARIES}
                gtest_main
                pthread
            )
        else()
            message(FATAL_ERROR "backtrader_core library not found at ${BACKTRADER_LIB_PATH}")
        endif()
        
        # Add to CTest
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endif()
endforeach()

# Enable testing
enable_testing()