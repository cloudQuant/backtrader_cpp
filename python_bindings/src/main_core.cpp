/**
 * @file main_core.cpp
 * @brief Core backtrader-cpp Python bindings with real library integration
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <memory>
#include <vector>
#include <string>

// Include backtrader-cpp headers
#include "linebuffer.h"
#include "lineseries.h"  
#include "dataseries.h"
#include "indicator.h"
#include "indicators/sma.h"
#include "indicators/ema.h"
#include "indicators/rsi.h"

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(backtrader_cpp, m) {
    m.doc() = "Backtrader C++ - Core Python Bindings with Real Library Integration";
    
    // Module metadata
    m.attr("__version__") = "0.2.0";
    m.attr("__author__") = "Backtrader C++ Team";
    
    // ==================== CORE TYPES ====================
    
    // LineSingle base class
    py::class_<backtrader::LineSingle, std::shared_ptr<backtrader::LineSingle>>(m, "LineSingle",
        "Base class for single-line data series")
        .def("size", &backtrader::LineSingle::size, "Get number of data points")
        .def("__len__", &backtrader::LineSingle::size)
        .def("get", &backtrader::LineSingle::get, "Get value at offset", "ago"_a = 0)
        .def("__call__", &backtrader::LineSingle::get, "Get value at offset", "ago"_a = 0)
        .def("__getitem__", [](const backtrader::LineSingle& self, int index) {
            // Python-style indexing: [0] = current, [-1] = previous
            if (index < 0) {
                return self.get(-index);
            } else {
                int ago = static_cast<int>(self.size()) - 1 - index;
                return self.get(-ago);
            }
        }, "Get value by index")
        .def("set", &backtrader::LineSingle::set, "Set value at offset", "ago"_a, "value"_a);

    // LineBuffer - high-performance circular buffer
    py::class_<backtrader::LineBuffer, backtrader::LineSingle, std::shared_ptr<backtrader::LineBuffer>>(m, "LineBuffer",
        "High-performance circular buffer for time series data")
        .def(py::init<>())
        .def("append", &backtrader::LineBuffer::append, "Append value to buffer", "value"_a)
        .def("data_size", &backtrader::LineBuffer::data_size, "Get actual data size")
        .def("get_idx", &backtrader::LineBuffer::get_idx, "Get current index")
        .def("set_idx", &backtrader::LineBuffer::set_idx, "Set current index", "index"_a)
        .def("reset", &backtrader::LineBuffer::reset, "Reset buffer")
        .def("to_numpy", [](const backtrader::LineBuffer& self) {
            // Zero-copy conversion to NumPy array
            size_t size = self.data_size();
            if (size == 0) {
                return py::array_t<double>();
            }
            return py::array_t<double>(size, self.data_ptr(), py::cast(self));
        }, "Convert to NumPy array (zero-copy)");

    // LineSeries - multiple line container
    py::class_<backtrader::LineSeries, std::shared_ptr<backtrader::LineSeries>>(m, "LineSeries",
        "Container for multiple data lines")
        .def(py::init<>())
        .def("size", &backtrader::LineSeries::size, "Get number of data points");

    // DataSeries - OHLCV data container
    py::class_<backtrader::DataSeries, backtrader::LineSeries, std::shared_ptr<backtrader::DataSeries>>(m, "DataSeries",
        "OHLCV data series container")
        .def(py::init<>())
        .def("size", &backtrader::DataSeries::size)
        .def("__len__", &backtrader::DataSeries::size)
        .def("open", [](const backtrader::DataSeries& self, int ago) { return self.open(ago); }, "Get open price", "ago"_a = 0)
        .def("high", [](const backtrader::DataSeries& self, int ago) { return self.high(ago); }, "Get high price", "ago"_a = 0)
        .def("low", [](const backtrader::DataSeries& self, int ago) { return self.low(ago); }, "Get low price", "ago"_a = 0)
        .def("close", [](const backtrader::DataSeries& self, int ago) { return self.close(ago); }, "Get close price", "ago"_a = 0)
        .def("volume", [](const backtrader::DataSeries& self, int ago) { return self.volume(ago); }, "Get volume", "ago"_a = 0);

    // ==================== INDICATORS ====================
    
    // Indicator base class
    py::class_<backtrader::Indicator, std::shared_ptr<backtrader::Indicator>>(m, "Indicator",
        "Base class for technical indicators")
        .def("size", &backtrader::Indicator::size, "Get number of calculated values")
        .def("__len__", &backtrader::Indicator::size)
        .def("__getitem__", [](const backtrader::Indicator& self, int index) {
            if (index < 0) {
                return self.get(-index);
            } else {
                int ago = static_cast<int>(self.size()) - 1 - index;
                return self.get(-ago);
            }
        }, "Get indicator value by index");

    // Simple Moving Average
    py::class_<backtrader::SMA, backtrader::Indicator, std::shared_ptr<backtrader::SMA>>(m, "SMA",
        "Simple Moving Average indicator")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(), 
             "Create SMA indicator", "data"_a, "period"_a)
        .def("__getitem__", [](const backtrader::SMA& self, int ago) {
            return self.get(ago);
        }, "Get SMA value");

    // Exponential Moving Average
    py::class_<backtrader::EMA, backtrader::Indicator, std::shared_ptr<backtrader::EMA>>(m, "EMA",
        "Exponential Moving Average indicator")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create EMA indicator", "data"_a, "period"_a);

    // Relative Strength Index
    py::class_<backtrader::RSI, backtrader::Indicator, std::shared_ptr<backtrader::RSI>>(m, "RSI",
        "Relative Strength Index indicator")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create RSI indicator", "data"_a, "period"_a = 14);

    // ==================== DATA FEEDS ====================
    
    // Data feed factory functions
    m.def("CSVData", [](const std::string& filename) {
        auto data = std::make_shared<backtrader::DataSeries>();
        // TODO: Implement CSV loading
        return data;
    }, "Load data from CSV file", "filename"_a);
    
    m.def("PandasData", [](py::object dataframe) {
        auto data = std::make_shared<backtrader::DataSeries>();
        // TODO: Implement Pandas DataFrame conversion
        return data;
    }, "Load data from Pandas DataFrame", "dataframe"_a);

    // ==================== UTILITY FUNCTIONS ====================
    
    m.def("test", []() {
        return "Backtrader C++ core module loaded successfully!";
    }, "Test function");
    
    m.def("get_version", []() {
        return py::dict(
            "version"_a="0.2.0",
            "build_date"_a=__DATE__,
            "build_time"_a=__TIME__,
            "compiler"_a="C++20",
            "features"_a=py::list(py::make_tuple("LineSeries", "Indicators", "DataSeries", "NumPy"))
        );
    }, "Get version and feature information");

    // Simple math functions for compatibility
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
    }, "Calculate Simple Moving Average", "prices"_a, "period"_a);

    m.def("calculate_returns", [](const std::vector<double>& prices) {
        std::vector<double> returns;
        returns.reserve(prices.size() - 1);
        
        for (size_t i = 1; i < prices.size(); ++i) {
            double ret = (prices[i] - prices[i-1]) / prices[i-1];
            returns.push_back(ret);
        }
        return returns;
    }, "Calculate returns from prices", "prices"_a);

    m.def("benchmark", [](int iterations = 1000000) {
        auto start = std::chrono::high_resolution_clock::now();
        
        double sum = 0.0;
        for (int i = 0; i < iterations; ++i) {
            sum += std::sin(i * 0.001) * std::cos(i * 0.001);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        return py::dict(
            "result"_a=sum,
            "time_us"_a=duration.count(),
            "iterations"_a=iterations,
            "ops_per_second"_a=static_cast<double>(iterations) * 1000000.0 / duration.count()
        );
    }, "Performance benchmark", "iterations"_a = 1000000);
}