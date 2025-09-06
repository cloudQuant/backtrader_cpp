/**
 * @file main_minimal.cpp
 * @brief Minimal Python bindings for backtrader-cpp without external dependencies
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <memory>
#include <vector>
#include <cmath>
#include <numeric>
#include <chrono>

namespace py = pybind11;

PYBIND11_MODULE(backtrader_cpp, m) {
    m.doc() = "Backtrader C++ - Minimal Python Bindings";
    
    m.attr("__version__") = "0.1.0";
    
    // Simple test function
    m.def("test", []() {
        return "Backtrader C++ module loaded successfully!";
    });
    
    // Version information
    m.def("get_version", []() {
        return py::dict(
            py::arg("version") = "0.1.0",
            py::arg("build_date") = __DATE__,
            py::arg("compiler") = "C++20"
        );
    });
    
    // Simple mathematical functions without numpy dependency
    m.def("calculate_sma", [](const std::vector<double>& prices, int period) {
        std::vector<double> result;
        result.reserve(prices.size());
        
        for (size_t i = 0; i < prices.size(); ++i) {
            if (i < period - 1) {
                result.push_back(std::numeric_limits<double>::quiet_NaN());
            } else {
                double sum = 0.0;
                for (int j = 0; j < period; ++j) {
                    sum += prices[i - j];
                }
                result.push_back(sum / period);
            }
        }
        return result;
    }, "Calculate Simple Moving Average", py::arg("prices"), py::arg("period"));
    
    m.def("calculate_returns", [](const std::vector<double>& prices) {
        std::vector<double> returns;
        returns.reserve(prices.size() - 1);
        
        for (size_t i = 1; i < prices.size(); ++i) {
            double ret = (prices[i] - prices[i-1]) / prices[i-1];
            returns.push_back(ret);
        }
        return returns;
    }, "Calculate returns from prices", py::arg("prices"));
    
    m.def("performance_test", [](int iterations) {
        auto start = std::chrono::high_resolution_clock::now();
        
        double sum = 0.0;
        for (int i = 0; i < iterations; ++i) {
            sum += std::sin(i * 0.01) * std::cos(i * 0.01);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return py::dict(
            py::arg("result") = sum,
            py::arg("time_us") = duration.count(),
            py::arg("iterations") = iterations
        );
    }, "Performance test", py::arg("iterations") = 1000000);
    
    // Simple data container
    py::class_<std::vector<double>>(m, "DoubleVector")
        .def(py::init<>())
        .def("push_back", [](std::vector<double>& v, double val) { v.push_back(val); })
        .def("size", &std::vector<double>::size)
        .def("__len__", &std::vector<double>::size)
        .def("__getitem__", [](const std::vector<double>& v, size_t i) {
            if (i >= v.size()) throw py::index_error();
            return v[i];
        })
        .def("__setitem__", [](std::vector<double>& v, size_t i, double value) {
            if (i >= v.size()) throw py::index_error();
            v[i] = value;
        });
}