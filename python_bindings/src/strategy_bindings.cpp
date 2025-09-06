/**
 * @file strategy_bindings.cpp
 * @brief Strategy class bindings for backtrader-cpp
 * 
 * This file defines Python bindings for the Strategy base class and
 * related functionality for creating trading strategies.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "strategy.h"
#include "broker.h"
#include "dataseries.h"
#include "order.h"
#include "trade.h"
#include "position.h"

namespace py = pybind11;
using namespace pybind11::literals;

void bind_strategy(py::module& m) {
    // Strategy base class
    py::class_<backtrader::Strategy, std::shared_ptr<backtrader::Strategy>>(m, "Strategy",
        "Base class for trading strategies")
        
        // Core lifecycle methods
        .def(py::init<>(),
             "Create a new strategy instance")
        
        .def("init", &backtrader::Strategy::init,
             "Initialize strategy (called once before execution)")
        
        .def("start", &backtrader::Strategy::start,
             "Called when strategy execution starts")
        
        .def("next", &backtrader::Strategy::next,
             "Called for each new bar of data")
        
        .def("stop", &backtrader::Strategy::stop,
             "Called when strategy execution stops")
        
        // Data access methods
        .def("len", &backtrader::Strategy::len,
             "Get current bar index")
        
        .def("__len__", &backtrader::Strategy::len,
             "Get current bar index")
        
        // Order management methods
        .def("buy", 
             py::overload_cast<>(&backtrader::Strategy::buy),
             "Place market buy order with default size")
        
        .def("buy", 
             py::overload_cast<double>(&backtrader::Strategy::buy),
             "Place market buy order with specified size",
             py::arg("size"))
        
        .def("buy", 
             py::overload_cast<double, double>(&backtrader::Strategy::buy),
             "Place limit buy order with size and price",
             py::arg("size"), py::arg("price"))
        
        .def("sell", 
             py::overload_cast<>(&backtrader::Strategy::sell),
             "Place market sell order with default size")
        
        .def("sell", 
             py::overload_cast<double>(&backtrader::Strategy::sell),
             "Place market sell order with specified size",
             py::arg("size"))
        
        .def("sell", 
             py::overload_cast<double, double>(&backtrader::Strategy::sell),
             "Place limit sell order with size and price",
             py::arg("size"), py::arg("price"))
        
        .def("close", 
             py::overload_cast<>(&backtrader::Strategy::close),
             "Close current position")
        
        .def("cancel", &backtrader::Strategy::cancel,
             "Cancel an order",
             py::arg("order"))
        
        // Position and portfolio methods
        .def("getposition", 
             py::overload_cast<>(&backtrader::Strategy::getposition, py::const_),
             "Get position size for default data")
        
        .def("getposition", 
             py::overload_cast<std::shared_ptr<backtrader::DataSeries>>(&backtrader::Strategy::getposition, py::const_),
             "Get position size for specific data",
             py::arg("data"))
        
        // Broker access
        .def("broker", &backtrader::Strategy::broker_ptr,
             "Get broker instance",
             py::return_value_policy::reference)
        
        // Notification methods (can be overridden in Python)
        .def("notify_order", &backtrader::Strategy::notify_order,
             "Called when order status changes",
             py::arg("order"))
        
        .def("notify_trade", &backtrader::Strategy::notify_trade,
             "Called when trade is completed",
             py::arg("trade"))
        
        .def("notify_cashvalue", &backtrader::Strategy::notify_cashvalue,
             "Called with cash and value updates",
             py::arg("cash"), py::arg("value"))
        
        // Utility methods
        .def("log", [](backtrader::Strategy& self, const std::string& message, py::object dt) {
             if (dt.is_none()) {
                 self.log(message);
             } else {
                 self.log(message, dt.cast<double>());
             }
         }, "Log a message with optional datetime",
         py::arg("message"), py::arg("dt") = py::none())
        
        // Properties for accessing data and indicators
        .def_property_readonly("data", [](const backtrader::Strategy& self) {
             return self.datas.empty() ? nullptr : self.datas[0];
         }, "Primary data feed")
        
        .def_property_readonly("datas", [](const backtrader::Strategy& self) {
             py::list data_list;
             for (const auto& data : self.datas) {
                 data_list.append(data);
             }
             return data_list;
         }, "List of all data feeds")
        
        // String representation
        .def("__repr__", [](const backtrader::Strategy& self) {
             return "<Strategy datas=" + std::to_string(self.datas.size()) + ">";
         });

    // Strategy parameter system
    py::class_<backtrader::StrategyParams>(m, "StrategyParams",
        "Strategy parameter container")
        .def(py::init<>())
        .def("set", [](backtrader::StrategyParams& self, const std::string& name, py::object value) {
             // TODO: Implement parameter setting
         }, "Set parameter value", py::arg("name"), py::arg("value"))
        .def("get", [](const backtrader::StrategyParams& self, const std::string& name) {
             // TODO: Implement parameter getting
             return py::none();
         }, "Get parameter value", py::arg("name"));

    // Convenience functions for strategy creation
    m.def("create_strategy", [](py::object strategy_class, py::kwargs kwargs) {
        // Factory function to create strategies with parameters
        py::dict params;
        for (auto item : kwargs) {
            params[item.first] = item.second;
        }
        
        // This would typically be used internally by Cerebro
        return strategy_class(**params);
    }, "Create strategy instance with parameters",
    py::arg("strategy_class"));

    // Strategy utilities
    m.def("validate_strategy", [](py::object strategy_instance) {
        // Validate that strategy has required methods
        bool valid = true;
        std::vector<std::string> missing_methods;
        
        std::vector<std::string> required_methods = {"init", "next"};
        for (const auto& method : required_methods) {
            if (!py::hasattr(strategy_instance, method.c_str())) {
                valid = false;
                missing_methods.push_back(method);
            }
        }
        
        if (!valid) {
            std::string error = "Strategy missing required methods: ";
            for (const auto& method : missing_methods) {
                error += method + " ";
            }
            throw std::runtime_error(error);
        }
        
        return valid;
    }, "Validate strategy implementation", py::arg("strategy"));

    // Strategy performance tracking
    py::class_<backtrader::StrategyStats>(m, "StrategyStats",
        "Strategy performance statistics")
        .def(py::init<>())
        .def_readonly("total_orders", &backtrader::StrategyStats::total_orders)
        .def_readonly("total_trades", &backtrader::StrategyStats::total_trades)
        .def_readonly("winning_trades", &backtrader::StrategyStats::winning_trades)
        .def_readonly("losing_trades", &backtrader::StrategyStats::losing_trades)
        .def_readonly("total_pnl", &backtrader::StrategyStats::total_pnl)
        .def_readonly("max_drawdown", &backtrader::StrategyStats::max_drawdown)
        .def("win_rate", &backtrader::StrategyStats::win_rate,
             "Calculate win rate percentage")
        .def("profit_factor", &backtrader::StrategyStats::profit_factor,
             "Calculate profit factor")
        .def("__repr__", [](const backtrader::StrategyStats& self) {
             return "<StrategyStats orders=" + std::to_string(self.total_orders) +
                    " trades=" + std::to_string(self.total_trades) +
                    " pnl=" + std::to_string(self.total_pnl) + ">";
         });
}