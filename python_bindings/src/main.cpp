/**
 * @file main.cpp
 * @brief Main pybind11 module definition for backtrader-cpp
 * 
 * This file defines the main Python module and coordinates all sub-modules.
 * It provides the entry point for the Python bindings.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/chrono.h>
#include <pybind11/functional.h>

// C++ headers
#include <memory>
#include <string>
#include <vector>
#include <chrono>

// Binding declarations
void bind_core_types(pybind11::module& m);
void bind_cerebro(pybind11::module& m);
void bind_strategy(pybind11::module& m);
void bind_indicators(pybind11::module& m);
void bind_data(pybind11::module& m);
void bind_analyzers(pybind11::module& m);
void bind_broker(pybind11::module& m);
void bind_utils(pybind11::module& m);

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(backtrader_cpp, m) {
    m.doc() = "Backtrader C++ - High-performance quantitative trading framework";
    
    // Module metadata
    m.attr("__version__") = "1.0.0";
    m.attr("__author__") = "Backtrader C++ Team";
    m.attr("__email__") = "team@backtrader-cpp.org";
    
    // Performance information
    m.attr("__build_date__") = __DATE__ " " __TIME__;
    m.attr("__compiler__") = 
#ifdef __clang__
        "Clang " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__);
#elif defined(__GNUC__)
        "GCC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__);
#elif defined(_MSC_VER)
        "MSVC " + std::to_string(_MSC_VER);
#else
        "Unknown";
#endif

    // C++ standard information
    m.attr("__cpp_std__") = __cplusplus;
    
    // Optimization information
#ifdef NDEBUG
    m.attr("__optimized__") = true;
#else
    m.attr("__optimized__") = false;
#endif

    // SIMD support information
    m.attr("__simd_support__") = py::dict();
#ifdef __AVX2__
    m.attr("__simd_support__")["AVX2"] = true;
#endif
#ifdef __AVX512F__
    m.attr("__simd_support__")["AVX512"] = true;
#endif
#ifdef __SSE4_1__
    m.attr("__simd_support__")["SSE4.1"] = true;
#endif

    // Exception handling
    py::register_exception<std::runtime_error>(m, "BacktraderError");
    py::register_exception<std::invalid_argument>(m, "InvalidArgumentError");
    py::register_exception<std::out_of_range>(m, "OutOfRangeError");
    
    // Global utility functions
    m.def("get_version", []() {
        return py::make_tuple(1, 0, 0);
    }, "Get version as tuple (major, minor, patch)");
    
    m.def("get_build_info", []() {
        return py::dict(
            "version"_a="1.0.0",
            "build_date"_a=__DATE__ " " __TIME__,
            "compiler"_a=
#ifdef __clang__
                "Clang " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__),
#elif defined(__GNUC__)
                "GCC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__),
#elif defined(_MSC_VER)
                "MSVC " + std::to_string(_MSC_VER),
#else
                "Unknown",
#endif
            "cpp_std"_a=__cplusplus,
            "optimized"_a=
#ifdef NDEBUG
                true
#else
                false
#endif
        );
    }, "Get detailed build information");

    // Performance monitoring
    m.def("enable_performance_monitoring", [](bool enable) {
        // TODO: Implement performance monitoring toggle
        return enable;
    }, "Enable/disable performance monitoring", py::arg("enable") = true);

    // Memory statistics  
    m.def("get_memory_stats", []() {
        // TODO: Implement memory statistics collection
        return py::dict(
            "total_allocated"_a=0,
            "current_usage"_a=0,
            "peak_usage"_a=0
        );
    }, "Get memory usage statistics");

    // Bind all sub-modules
    try {
        bind_core_types(m);
        bind_cerebro(m);
        bind_strategy(m);
        bind_indicators(m);
        bind_data(m);
        bind_analyzers(m);
        bind_broker(m);
        bind_utils(m);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to initialize backtrader_cpp module: " + std::string(e.what()));
    }
    
    // Module initialization complete message
    m.attr("__initialized__") = true;
}