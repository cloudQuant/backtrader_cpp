# Examples CMakeLists.txt for Backtrader C++

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Create examples directory output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)

# Helper function to create examples
function(add_backtrader_example name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} 
        backtrader_cpp
        Threads::Threads
    )
    target_compile_features(${name} PRIVATE cxx_std_20)
    
    # Set properties
    set_target_properties(${name} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    
    # Add to examples group
    set_target_properties(${name} PROPERTIES FOLDER "Examples")
endfunction()

# Basic Examples
add_backtrader_example(simple_sma_strategy)

# Advanced Examples (to be implemented)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/rsi_divergence_strategy.cpp)
    add_backtrader_example(rsi_divergence_strategy)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/pairs_trading_strategy.cpp)
    add_backtrader_example(pairs_trading_strategy)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/mean_reversion_strategy.cpp)
    add_backtrader_example(mean_reversion_strategy)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/momentum_strategy.cpp)
    add_backtrader_example(momentum_strategy)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/multi_timeframe_strategy.cpp)
    add_backtrader_example(multi_timeframe_strategy)
endif()

# Data feed examples
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/csv_data_example.cpp)
    add_backtrader_example(csv_data_example)
endif()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/live_data_example.cpp)
    add_backtrader_example(live_data_example)
endif()

# Analyzer examples
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/analyzer_example.cpp)
    add_backtrader_example(analyzer_example)
endif()

# Performance comparison examples
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/performance_benchmark.cpp)
    add_backtrader_example(performance_benchmark)
endif()

# Custom examples directory for user additions
file(GLOB CUSTOM_EXAMPLES "${CMAKE_CURRENT_SOURCE_DIR}/custom/*.cpp")
foreach(example_file ${CUSTOM_EXAMPLES})
    get_filename_component(example_name ${example_file} NAME_WE)
    add_backtrader_example(${example_name})
    set_target_properties(${example_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/custom
    )
endforeach()

# Installation
install(TARGETS simple_sma_strategy
    RUNTIME DESTINATION examples
    COMPONENT Examples
)

# Install example source files
install(FILES 
    simple_sma_strategy.cpp
    DESTINATION examples/src
    COMPONENT Examples
)

# Create example data directory
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/examples/data)

# Copy sample data if it exists
if(EXISTS ${CMAKE_SOURCE_DIR}/data/sample_data.csv)
    configure_file(
        ${CMAKE_SOURCE_DIR}/data/sample_data.csv
        ${CMAKE_BINARY_DIR}/examples/data/sample_data.csv
        COPYONLY
    )
endif()

# Print examples information
message(STATUS "Backtrader C++ Examples:")
message(STATUS "  Examples will be built in: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "  Basic examples: simple_sma_strategy")

# Add custom target for running all examples
add_custom_target(run_examples
    COMMAND ${CMAKE_COMMAND} -E echo "Running Backtrader C++ Examples..."
    DEPENDS simple_sma_strategy
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Individual run targets
add_custom_target(run_sma_example
    COMMAND $<TARGET_FILE:simple_sma_strategy>
    DEPENDS simple_sma_strategy
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Running Simple SMA Strategy Example"
)

# Documentation target for examples
if(DOXYGEN_FOUND)
    set(EXAMPLES_DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.examples.in)
    set(EXAMPLES_DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.examples)
    
    if(EXISTS ${EXAMPLES_DOXYFILE_IN})
        configure_file(${EXAMPLES_DOXYFILE_IN} ${EXAMPLES_DOXYFILE_OUT} @ONLY)
        
        add_custom_target(examples_docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${EXAMPLES_DOXYFILE_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating examples documentation with Doxygen"
            VERBATIM
        )
    endif()
endif()

# Testing for examples (basic compilation tests)
if(BUILD_TESTING)
    add_test(
        NAME examples_compile_test
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target simple_sma_strategy
    )
    
    set_tests_properties(examples_compile_test PROPERTIES
        LABELS "Examples;Compilation"
    )
endif()