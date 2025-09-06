/**
 * @file indicator_bindings.cpp
 * @brief Technical indicator bindings for backtrader-cpp
 * 
 * This file defines Python bindings for all technical indicators available
 * in the C++ implementation.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include "indicator.h"
#include "dataseries.h"

// Include all indicator headers
#include "indicators/sma.h"
#include "indicators/ema.h"
#include "indicators/wma.h"
#include "indicators/rsi.h"
#include "indicators/macd.h"
#include "indicators/bollinger.h"
#include "indicators/stochastic.h"
#include "indicators/cci.h"
#include "indicators/atr.h"
#include "indicators/aroon.h"
#include "indicators/awesomeoscillator.h"
#include "indicators/crossover.h"
#include "indicators/envelope.h"
#include "indicators/momentum.h"
#include "indicators/roc.h"
#include "indicators/williamsad.h"
#include "indicators/williamsr.h"
#include "indicators/fractal.h"
#include "indicators/highest.h"
#include "indicators/lowest.h"
#include "indicators/sumn.h"

namespace py = pybind11;
using namespace pybind11::literals;

// Helper function to convert Python data input to C++ DataSeries
std::shared_ptr<backtrader::DataSeries> convert_python_data_input(py::object data_input) {
    if (py::isinstance<backtrader::DataSeries>(data_input)) {
        return data_input.cast<std::shared_ptr<backtrader::DataSeries>>();
    } else if (py::isinstance<backtrader::Indicator>(data_input)) {
        // TODO: Create indicator-to-dataseries adapter
        throw std::runtime_error("Indicator-to-indicator chaining not yet implemented");
    } else {
        throw std::runtime_error("Invalid data input type");
    }
}

// Template for binding simple indicators
template<typename IndicatorType>
void bind_simple_indicator(py::module& m, const std::string& name, const std::string& description) {
    py::class_<IndicatorType, backtrader::Indicator, std::shared_ptr<IndicatorType>>(m, name.c_str(), description.c_str())
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create indicator with data and period",
             py::arg("data"), py::arg("period"))
        .def(py::init([](py::object data, int period) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<IndicatorType>(cpp_data, period);
             }),
             py::arg("data"), py::arg("period"));
}

void bind_indicators(py::module& m) {
    // Indicator base class
    py::class_<backtrader::Indicator, std::shared_ptr<backtrader::Indicator>>(m, "Indicator",
        "Base class for all technical indicators")
        .def("get", &backtrader::Indicator::get,
             "Get indicator value at specified offset",
             py::arg("ago") = 0)
        .def("__call__", &backtrader::Indicator::get,
             "Get indicator value at specified offset", 
             py::arg("ago") = 0)
        .def("__len__", &backtrader::Indicator::size,
             "Get number of calculated values")
        .def("__getitem__", [](const backtrader::Indicator& self, int index) {
             if (index < 0) {
                 return self.get(index);
             } else {
                 int ago = static_cast<int>(self.size()) - 1 - index;
                 return self.get(-ago);
             }
         }, "Get value by index (supports negative indexing)")
        .def("size", &backtrader::Indicator::size,
             "Get number of calculated values")
        .def("getMinPeriod", &backtrader::Indicator::getMinPeriod,
             "Get minimum period required for calculation")
        .def("calculate", &backtrader::Indicator::calculate,
             "Force calculation of all values")
        .def("to_numpy", [](const backtrader::Indicator& self) {
             // Convert indicator values to NumPy array
             std::vector<double> values;
             for (size_t i = 0; i < self.size(); ++i) {
                 values.push_back(self.get(-static_cast<int>(i)));
             }
             return py::array_t<double>(values.size(), values.data());
         }, "Convert indicator values to NumPy array")
        .def("__repr__", [](const backtrader::Indicator& self) {
             return "<Indicator size=" + std::to_string(self.size()) + 
                    " minperiod=" + std::to_string(self.getMinPeriod()) + ">";
         });

    // ==================== Moving Averages ====================
    
    // Simple Moving Average
    py::class_<backtrader::indicators::SMA, backtrader::Indicator, std::shared_ptr<backtrader::indicators::SMA>>(m, "SMA",
        "Simple Moving Average")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create SMA with data and period",
             py::arg("data"), py::arg("period") = 20)
        .def(py::init([](py::object data, int period) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::SMA>(cpp_data, period);
             }),
             py::arg("data"), py::arg("period") = 20);

    // Exponential Moving Average
    py::class_<backtrader::indicators::EMA, backtrader::Indicator, std::shared_ptr<backtrader::indicators::EMA>>(m, "EMA",
        "Exponential Moving Average")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create EMA with data and period",
             py::arg("data"), py::arg("period") = 20)
        .def(py::init([](py::object data, int period) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::EMA>(cpp_data, period);
             }),
             py::arg("data"), py::arg("period") = 20);

    // Weighted Moving Average
    py::class_<backtrader::indicators::WMA, backtrader::Indicator, std::shared_ptr<backtrader::indicators::WMA>>(m, "WMA",
        "Weighted Moving Average")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create WMA with data and period",
             py::arg("data"), py::arg("period") = 20)
        .def(py::init([](py::object data, int period) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::WMA>(cpp_data, period);
             }),
             py::arg("data"), py::arg("period") = 20);

    // ==================== Oscillators ====================

    // Relative Strength Index
    py::class_<backtrader::indicators::RSI, backtrader::Indicator, std::shared_ptr<backtrader::indicators::RSI>>(m, "RSI",
        "Relative Strength Index")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create RSI with data and period",
             py::arg("data"), py::arg("period") = 14)
        .def(py::init([](py::object data, int period) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::RSI>(cpp_data, period);
             }),
             py::arg("data"), py::arg("period") = 14);

    // Commodity Channel Index
    py::class_<backtrader::indicators::CCI, backtrader::Indicator, std::shared_ptr<backtrader::indicators::CCI>>(m, "CCI",
        "Commodity Channel Index")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create CCI with data and period",
             py::arg("data"), py::arg("period") = 20)
        .def(py::init([](py::object data, int period) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::CCI>(cpp_data, period);
             }),
             py::arg("data"), py::arg("period") = 20);

    // Stochastic Oscillator
    py::class_<backtrader::indicators::Stochastic, backtrader::Indicator, std::shared_ptr<backtrader::indicators::Stochastic>>(m, "Stochastic",
        "Stochastic Oscillator")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int, int>(),
             "Create Stochastic with data, K period, and D period",
             py::arg("data"), py::arg("period_k") = 14, py::arg("period_d") = 3)
        .def(py::init([](py::object data, int period_k, int period_d) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::Stochastic>(cpp_data, period_k, period_d);
             }),
             py::arg("data"), py::arg("period_k") = 14, py::arg("period_d") = 3)
        .def_property_readonly("percK", [](const backtrader::indicators::Stochastic& self) {
             return self.getLine(0);
         }, "Get %K line")
        .def_property_readonly("percD", [](const backtrader::indicators::Stochastic& self) {
             return self.getLine(1);
         }, "Get %D line");

    // Williams %R
    py::class_<backtrader::indicators::WilliamsR, backtrader::Indicator, std::shared_ptr<backtrader::indicators::WilliamsR>>(m, "WilliamsR",
        "Williams %R Oscillator")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create Williams %R with data and period",
             py::arg("data"), py::arg("period") = 14)
        .def(py::init([](py::object data, int period) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::WilliamsR>(cpp_data, period);
             }),
             py::arg("data"), py::arg("period") = 14);

    // ==================== Trend Indicators ====================

    // MACD
    py::class_<backtrader::indicators::MACD, backtrader::Indicator, std::shared_ptr<backtrader::indicators::MACD>>(m, "MACD",
        "Moving Average Convergence Divergence")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int, int, int>(),
             "Create MACD with data and periods",
             py::arg("data"), py::arg("period_me1") = 12, py::arg("period_me2") = 26, py::arg("period_signal") = 9)
        .def(py::init([](py::object data, int period_me1, int period_me2, int period_signal) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::MACD>(cpp_data, period_me1, period_me2, period_signal);
             }),
             py::arg("data"), py::arg("period_me1") = 12, py::arg("period_me2") = 26, py::arg("period_signal") = 9)
        .def_property_readonly("macd", [](const backtrader::indicators::MACD& self) {
             return self.getLine(0);
         }, "Get MACD line")
        .def_property_readonly("signal", [](const backtrader::indicators::MACD& self) {
             return self.getLine(1);
         }, "Get signal line")
        .def_property_readonly("histogram", [](const backtrader::indicators::MACD& self) {
             return self.getLine(2);
         }, "Get histogram line");

    // Bollinger Bands
    py::class_<backtrader::indicators::BollingerBands, backtrader::Indicator, std::shared_ptr<backtrader::indicators::BollingerBands>>(m, "BollingerBands",
        "Bollinger Bands")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int, double>(),
             "Create Bollinger Bands with data, period, and deviation factor",
             py::arg("data"), py::arg("period") = 20, py::arg("devfactor") = 2.0)
        .def(py::init([](py::object data, int period, double devfactor) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::BollingerBands>(cpp_data, period, devfactor);
             }),
             py::arg("data"), py::arg("period") = 20, py::arg("devfactor") = 2.0)
        .def_property_readonly("top", [](const backtrader::indicators::BollingerBands& self) {
             return self.getLine(0);
         }, "Get upper band")
        .def_property_readonly("mid", [](const backtrader::indicators::BollingerBands& self) {
             return self.getLine(1);
         }, "Get middle band (SMA)")
        .def_property_readonly("bot", [](const backtrader::indicators::BollingerBands& self) {
             return self.getLine(2);
         }, "Get lower band");

    // Average True Range
    py::class_<backtrader::indicators::ATR, backtrader::Indicator, std::shared_ptr<backtrader::indicators::ATR>>(m, "ATR",
        "Average True Range")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create ATR with data and period",
             py::arg("data"), py::arg("period") = 14)
        .def(py::init([](py::object data, int period) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::ATR>(cpp_data, period);
             }),
             py::arg("data"), py::arg("period") = 14);

    // Aroon Indicator
    py::class_<backtrader::indicators::Aroon, backtrader::Indicator, std::shared_ptr<backtrader::indicators::Aroon>>(m, "Aroon",
        "Aroon Indicator")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create Aroon with data and period",
             py::arg("data"), py::arg("period") = 14)
        .def(py::init([](py::object data, int period) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::Aroon>(cpp_data, period);
             }),
             py::arg("data"), py::arg("period") = 14)
        .def_property_readonly("aroonup", [](const backtrader::indicators::Aroon& self) {
             return self.getLine(0);
         }, "Get Aroon Up line")
        .def_property_readonly("aroondown", [](const backtrader::indicators::Aroon& self) {
             return self.getLine(1);
         }, "Get Aroon Down line");

    // ==================== Signal Indicators ====================

    // CrossOver
    py::class_<backtrader::indicators::CrossOver, backtrader::Indicator, std::shared_ptr<backtrader::indicators::CrossOver>>(m, "CrossOver",
        "CrossOver Signal")
        .def(py::init([](py::object data1, py::object data2) {
                 auto cpp_data1 = convert_python_data_input(data1);
                 auto cpp_data2 = convert_python_data_input(data2);
                 return std::make_shared<backtrader::indicators::CrossOver>(cpp_data1, cpp_data2);
             }),
             "Create CrossOver signal between two data series",
             py::arg("data1"), py::arg("data2"));

    // ==================== Utility Indicators ====================

    // Highest
    py::class_<backtrader::indicators::Highest, backtrader::Indicator, std::shared_ptr<backtrader::indicators::Highest>>(m, "Highest",
        "Highest Value over Period")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create Highest with data and period",
             py::arg("data"), py::arg("period") = 20)
        .def(py::init([](py::object data, int period) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::Highest>(cpp_data, period);
             }),
             py::arg("data"), py::arg("period") = 20);

    // Lowest
    py::class_<backtrader::indicators::Lowest, backtrader::Indicator, std::shared_ptr<backtrader::indicators::Lowest>>(m, "Lowest",
        "Lowest Value over Period")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create Lowest with data and period",
             py::arg("data"), py::arg("period") = 20)
        .def(py::init([](py::object data, int period) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::Lowest>(cpp_data, period);
             }),
             py::arg("data"), py::arg("period") = 20);

    // Sum
    py::class_<backtrader::indicators::SumN, backtrader::Indicator, std::shared_ptr<backtrader::indicators::SumN>>(m, "SumN",
        "Sum over N periods")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create SumN with data and period",
             py::arg("data"), py::arg("period") = 20)
        .def(py::init([](py::object data, int period) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::SumN>(cpp_data, period);
             }),
             py::arg("data"), py::arg("period") = 20);

    // Momentum
    py::class_<backtrader::indicators::Momentum, backtrader::Indicator, std::shared_ptr<backtrader::indicators::Momentum>>(m, "Momentum",
        "Momentum Indicator")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create Momentum with data and period",
             py::arg("data"), py::arg("period") = 12)
        .def(py::init([](py::object data, int period) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::Momentum>(cpp_data, period);
             }),
             py::arg("data"), py::arg("period") = 12);

    // Rate of Change
    py::class_<backtrader::indicators::ROC, backtrader::Indicator, std::shared_ptr<backtrader::indicators::ROC>>(m, "ROC",
        "Rate of Change")
        .def(py::init<std::shared_ptr<backtrader::DataSeries>, int>(),
             "Create ROC with data and period",
             py::arg("data"), py::arg("period") = 12)
        .def(py::init([](py::object data, int period) {
                 auto cpp_data = convert_python_data_input(data);
                 return std::make_shared<backtrader::indicators::ROC>(cpp_data, period);
             }),
             py::arg("data"), py::arg("period") = 12);

    // ==================== Indicator Factory Functions ====================

    // Generic indicator factory
    m.def("create_indicator", [](const std::string& indicator_type, py::object data, py::kwargs kwargs) {
        auto cpp_data = convert_python_data_input(data);
        
        // Parse common parameters
        int period = kwargs.contains("period") ? kwargs["period"].cast<int>() : 20;
        
        // Create appropriate indicator based on type
        if (indicator_type == "SMA") {
            return std::static_pointer_cast<backtrader::Indicator>(
                std::make_shared<backtrader::indicators::SMA>(cpp_data, period));
        } else if (indicator_type == "EMA") {
            return std::static_pointer_cast<backtrader::Indicator>(
                std::make_shared<backtrader::indicators::EMA>(cpp_data, period));
        } else if (indicator_type == "RSI") {
            return std::static_pointer_cast<backtrader::Indicator>(
                std::make_shared<backtrader::indicators::RSI>(cpp_data, period));
        }
        // Add more indicators as needed
        
        throw std::runtime_error("Unknown indicator type: " + indicator_type);
    }, "Create indicator by type name", py::arg("indicator_type"), py::arg("data"));

    // Batch indicator calculation
    m.def("calculate_indicators", [](py::list indicators) {
        for (auto& ind : indicators) {
            auto indicator = ind.cast<std::shared_ptr<backtrader::Indicator>>();
            indicator->calculate();
        }
    }, "Calculate multiple indicators efficiently", py::arg("indicators"));
}