/**
 * @file main_simple.cpp
 * @brief Simplified Python bindings for backtrader-cpp
 * 
 * This is a minimal working version focusing on core functionality
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <memory>
#include <vector>

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(backtrader_cpp, m) {
    m.doc() = "Backtrader C++ - High-performance quantitative trading framework (Simplified Version)";
    
    // Module metadata
    m.attr("__version__") = "0.1.0";
    
    // Simple test function to verify module loading
    m.def("test", []() {
        return "Backtrader C++ module loaded successfully!";
    }, "Test function to verify module loading");
    
    // Version information
    m.def("get_version", []() {
        return py::dict(
            "version"_a="0.1.0",
            "build_date"_a=__DATE__,
            "build_time"_a=__TIME__,
            "compiler"_a="C++20"
        );
    }, "Get version and build information");
    
    // Simple indicator example - Moving Average
    py::class_<std::vector<double>>(m, "SimpleMA")
        .def(py::init<>())
        .def("calculate", [](std::vector<double>& self, py::array_t<double> data, int period) {
            auto buf = data.request();
            double* ptr = static_cast<double*>(buf.ptr);
            size_t size = buf.size;
            
            self.clear();
            self.reserve(size);
            
            for (size_t i = 0; i < size; ++i) {
                if (i < period - 1) {
                    self.push_back(std::numeric_limits<double>::quiet_NaN());
                } else {
                    double sum = 0.0;
                    for (int j = 0; j < period; ++j) {
                        sum += ptr[i - j];
                    }
                    self.push_back(sum / period);
                }
            }
            return py::array_t<double>(self.size(), self.data());
        }, "Calculate simple moving average", "data"_a, "period"_a)
        .def("__len__", [](const std::vector<double>& self) { return self.size(); });
    
    // Simple data container
    py::class_<std::vector<std::vector<double>>>(m, "SimpleData")
        .def(py::init<>())
        .def("add_series", [](std::vector<std::vector<double>>& self, py::array_t<double> series) {
            auto buf = series.request();
            double* ptr = static_cast<double*>(buf.ptr);
            size_t size = buf.size;
            
            std::vector<double> data(ptr, ptr + size);
            self.push_back(data);
        }, "Add a data series", "series"_a)
        .def("get_series", [](const std::vector<std::vector<double>>& self, int index) {
            if (index >= 0 && index < self.size()) {
                return py::array_t<double>(self[index].size(), self[index].data());
            }
            throw std::out_of_range("Series index out of range");
        }, "Get a data series by index", "index"_a)
        .def("__len__", [](const std::vector<std::vector<double>>& self) { return self.size(); });
    
    // Simple performance calculator
    m.def("calculate_returns", [](py::array_t<double> prices) {
        auto buf = prices.request();
        double* ptr = static_cast<double*>(buf.ptr);
        size_t size = buf.size;
        
        if (size < 2) {
            return py::array_t<double>(0);
        }
        
        std::vector<double> returns;
        returns.reserve(size - 1);
        
        for (size_t i = 1; i < size; ++i) {
            double ret = (ptr[i] - ptr[i-1]) / ptr[i-1];
            returns.push_back(ret);
        }
        
        return py::array_t<double>(returns.size(), returns.data());
    }, "Calculate simple returns from price series", "prices"_a);
    
    m.def("calculate_sharpe", [](py::array_t<double> returns, double risk_free_rate = 0.0) {
        auto buf = returns.request();
        double* ptr = static_cast<double*>(buf.ptr);
        size_t size = buf.size;
        
        if (size == 0) return 0.0;
        
        // Calculate mean return
        double sum = 0.0;
        for (size_t i = 0; i < size; ++i) {
            sum += ptr[i];
        }
        double mean = sum / size;
        
        // Calculate standard deviation
        double variance = 0.0;
        for (size_t i = 0; i < size; ++i) {
            double diff = ptr[i] - mean;
            variance += diff * diff;
        }
        double std_dev = std::sqrt(variance / size);
        
        if (std_dev == 0.0) return 0.0;
        
        // Calculate Sharpe ratio (annualized assuming daily returns)
        double sharpe = (mean - risk_free_rate/252) / std_dev * std::sqrt(252);
        return sharpe;
    }, "Calculate Sharpe ratio from returns", "returns"_a, "risk_free_rate"_a = 0.0);
    
    // Simple benchmark function
    m.def("benchmark_calculation", [](int iterations = 1000000) {
        auto start = std::chrono::high_resolution_clock::now();
        
        double sum = 0.0;
        for (int i = 0; i < iterations; ++i) {
            sum += std::sin(i) * std::cos(i);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return py::dict(
            "iterations"_a=iterations,
            "time_us"_a=duration.count(),
            "time_ms"_a=duration.count() / 1000.0,
            "ops_per_second"_a=iterations * 1000000.0 / duration.count()
        );
    }, "Benchmark calculation performance", "iterations"_a = 1000000);
    
    // Add module docstring
    m.def("__doc__", []() {
        return R"(
Backtrader C++ - Simplified Python Bindings
===========================================

This is a simplified version of the backtrader-cpp Python bindings
focusing on core functionality and demonstrating the performance
advantages of the C++ implementation.

Available Functions:
-------------------
- test(): Verify module loading
- get_version(): Get version information
- calculate_returns(prices): Calculate returns from price series
- calculate_sharpe(returns): Calculate Sharpe ratio
- benchmark_calculation(): Performance benchmark

Available Classes:
-----------------
- SimpleMA: Simple moving average calculator
- SimpleData: Simple data container

Example Usage:
-------------
>>> import backtrader_cpp as bt
>>> print(bt.test())
>>> version = bt.get_version()
>>> print(f"Version: {version['version']}")
)";
    });
}