/**
 * @file core_bindings.cpp
 * @brief Core type bindings for backtrader-cpp
 * 
 * This file defines bindings for fundamental types and enums used throughout
 * the backtrader framework.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

// Core headers
#include "lineroot.h"
#include "linebuffer.h" 
#include "lineseries.h"
#include "dataseries.h"
#include "order.h"
#include "trade.h"
#include "position.h"

namespace py = pybind11;
using namespace pybind11::literals;

void bind_core_types(py::module& m) {
    // ==================== LineRoot and Line System ====================
    
    // LineSingle base class
    py::class_<backtrader::LineSingle>(m, "LineSingle", 
        "Base class for single-line data series")
        .def("size", &backtrader::LineSingle::size,
             "Get the number of data points")
        .def("__len__", &backtrader::LineSingle::size)
        .def("get", &backtrader::LineSingle::get,
             "Get value at specified offset",
             "ago"_a = 0)
        .def("__call__", &backtrader::LineSingle::get,
             "Get value at specified offset",
             "ago"_a = 0)
        .def("__getitem__", [](const backtrader::LineSingle& self, int index) {
            if (index < 0) {
                // Python-style negative indexing
                return self.get(index);
            } else {
                // Convert positive index to ago value
                int ago = static_cast<int>(self.size()) - 1 - index;
                return self.get(-ago);
            }
        }, "Get value by index (supports negative indexing)")
        .def("set", &backtrader::LineSingle::set,
             "Set value at specified offset",
             "ago"_a, "value"_a)
        .def("__setitem__", [](backtrader::LineSingle& self, int index, double value) {
            if (index < 0) {
                self.set(index, value);
            } else {
                int ago = static_cast<int>(self.size()) - 1 - index;
                self.set(-ago, value);
            }
        }, "Set value by index");

    // LineBuffer - high-performance circular buffer
    py::class_<backtrader::LineBuffer, backtrader::LineSingle>(m, "LineBuffer",
        "High-performance circular buffer for time series data")
        .def(py::init<>())
        .def("append", &backtrader::LineBuffer::append,
             "Append a new value to the buffer",
             "value"_a)
        .def("data_size", &backtrader::LineBuffer::data_size,
             "Get actual data size")
        .def("get_idx", &backtrader::LineBuffer::get_idx,
             "Get current buffer index")
        .def("set_idx", &backtrader::LineBuffer::set_idx,
             "Set current buffer index",
             "index"_a)
        .def("reset", &backtrader::LineBuffer::reset,
             "Reset buffer to initial state")
        .def("to_numpy", [](const backtrader::LineBuffer& self) {
             // Convert buffer to NumPy array
             auto array = self.array();
             return py::array_t<double>(
                 array.size(),
                 array.data(),
                 py::cast(self, py::return_value_policy::reference_internal)
             );
         }, "Convert to NumPy array (zero-copy when possible)")
        .def("from_numpy", [](backtrader::LineBuffer& self, py::array_t<double> arr) {
             auto buf = arr.request();
             if (buf.ndim != 1) {
                 throw std::runtime_error("Input array must be 1-dimensional");
             }
             double* ptr = static_cast<double*>(buf.ptr);
             for (ssize_t i = 0; i < buf.shape[0]; ++i) {
                 self.append(ptr[i]);
             }
         }, "Load data from NumPy array", py::arg("array"));

    // LineSeries - container for multiple lines
    py::class_<backtrader::LineSeries>(m, "LineSeries",
        "Container for multiple related data lines")
        .def(py::init<>())
        .def("size", &backtrader::LineSeries::size,
             "Get number of lines")
        .def("__len__", &backtrader::LineSeries::size)
        .def("add_line", [](backtrader::LineSeries& self, std::shared_ptr<backtrader::LineSingle> line) {
             self.lines->add_line(line);
         }, "Add a line to the series", py::arg("line"))
        .def("get_line", [](const backtrader::LineSeries& self, size_t index) {
             return self.lines->getline(index);
         }, "Get line by index", py::arg("index"))
        .def("__getitem__", [](const backtrader::LineSeries& self, size_t index) {
             return self.lines->getline(index);
         }, "Get line by index")
        .def("add_alias", [](backtrader::LineSeries& self, const std::string& name, size_t index) {
             self.lines->add_alias(name, index);
         }, "Add name alias for a line", py::arg("name"), py::arg("index"));

    // ==================== Order System ====================
    
    // Order status enumeration
    py::enum_<backtrader::OrderStatus>(m, "OrderStatus", "Order execution status")
        .value("Created", backtrader::OrderStatus::Created, "Order created but not submitted")
        .value("Submitted", backtrader::OrderStatus::Submitted, "Order submitted to broker")
        .value("Accepted", backtrader::OrderStatus::Accepted, "Order accepted by broker")
        .value("Partial", backtrader::OrderStatus::Partial, "Order partially filled")
        .value("Completed", backtrader::OrderStatus::Completed, "Order completely filled")
        .value("Canceled", backtrader::OrderStatus::Canceled, "Order canceled")
        .value("Expired", backtrader::OrderStatus::Expired, "Order expired")
        .value("Rejected", backtrader::OrderStatus::Rejected, "Order rejected by broker")
        .export_values();

    // Order type enumeration
    py::enum_<backtrader::OrderType>(m, "OrderType", "Order type specification")
        .value("Market", backtrader::OrderType::Market, "Market order")
        .value("Limit", backtrader::OrderType::Limit, "Limit order")
        .value("Stop", backtrader::OrderType::Stop, "Stop order")
        .value("StopLimit", backtrader::OrderType::StopLimit, "Stop-limit order")
        .value("Close", backtrader::OrderType::Close, "Close position order")
        .export_values();

    // Execution information
    py::class_<backtrader::ExecutionInfo>(m, "ExecutionInfo", "Order execution details")
        .def_readonly("price", &backtrader::ExecutionInfo::price, "Execution price")
        .def_readonly("size", &backtrader::ExecutionInfo::size, "Executed size")
        .def_readonly("commission", &backtrader::ExecutionInfo::commission, "Commission paid")
        .def_readonly("datetime", &backtrader::ExecutionInfo::datetime, "Execution datetime")
        .def("__repr__", [](const backtrader::ExecutionInfo& self) {
             return "<ExecutionInfo price=" + std::to_string(self.price) + 
                    " size=" + std::to_string(self.size) + 
                    " commission=" + std::to_string(self.commission) + ">";
         });

    // Order class
    py::class_<backtrader::Order, std::shared_ptr<backtrader::Order>>(m, "Order", "Trading order")
        .def_readonly("ref", &backtrader::Order::ref, "Order reference ID")
        .def_readonly("status", &backtrader::Order::status, "Current order status")
        .def_readonly("order_type", &backtrader::Order::order_type, "Order type")
        .def_readonly("size", &backtrader::Order::size, "Order size")
        .def_readonly("price", &backtrader::Order::price, "Order price")
        .def_readonly("created", &backtrader::Order::created, "Creation datetime")
        .def_readonly("executed", &backtrader::Order::executed, "Execution information")
        .def("isbuy", &backtrader::Order::isbuy, "Check if buy order")
        .def("issell", &backtrader::Order::issell, "Check if sell order")
        .def("alive", &backtrader::Order::alive, "Check if order is still active")
        .def("__repr__", [](const backtrader::Order& self) {
             return "<Order ref=" + std::to_string(self.ref) + 
                    " status=" + std::to_string(static_cast<int>(self.status)) +
                    " size=" + std::to_string(self.size) + 
                    " price=" + std::to_string(self.price) + ">";
         });

    // ==================== Trade System ====================
    
    // Trade class
    py::class_<backtrader::Trade, std::shared_ptr<backtrader::Trade>>(m, "Trade", "Completed trade")
        .def_readonly("ref", &backtrader::Trade::ref, "Trade reference ID")
        .def_readonly("size", &backtrader::Trade::size, "Trade size")
        .def_readonly("price", &backtrader::Trade::price, "Trade price")
        .def_readonly("value", &backtrader::Trade::value, "Trade value")
        .def_readonly("commission", &backtrader::Trade::commission, "Total commission")
        .def_readonly("pnl", &backtrader::Trade::pnl, "Profit and loss")
        .def_readonly("pnlcomm", &backtrader::Trade::pnlcomm, "PnL including commission")
        .def_readonly("dtopen", &backtrader::Trade::dtopen, "Open datetime")
        .def_readonly("dtclose", &backtrader::Trade::dtclose, "Close datetime")
        .def_readonly("baropen", &backtrader::Trade::baropen, "Open bar number")
        .def_readonly("barclose", &backtrader::Trade::barclose, "Close bar number")
        .def("isopen", &backtrader::Trade::isopen, "Check if trade is open")
        .def("isclosed", &backtrader::Trade::isclosed, "Check if trade is closed")
        .def("__repr__", [](const backtrader::Trade& self) {
             return "<Trade ref=" + std::to_string(self.ref) + 
                    " size=" + std::to_string(self.size) +
                    " pnl=" + std::to_string(self.pnl) + ">";
         });

    // ==================== Position System ====================
    
    // Position class
    py::class_<backtrader::Position, std::shared_ptr<backtrader::Position>>(m, "Position", "Position information")
        .def_readonly("size", &backtrader::Position::size, "Position size")
        .def_readonly("price", &backtrader::Position::price, "Average entry price")
        .def_readonly("datetime", &backtrader::Position::datetime, "Position datetime")
        .def("clone", &backtrader::Position::clone, "Create a copy of position")
        .def("__bool__", [](const backtrader::Position& self) {
             return self.size != 0.0;
         }, "Check if position exists")
        .def("__repr__", [](const backtrader::Position& self) {
             return "<Position size=" + std::to_string(self.size) + 
                    " price=" + std::to_string(self.price) + ">";
         });

    // ==================== Utility Functions ====================
    
    // Date/time utilities
    m.def("num2date", [](double num) {
        // Convert numeric date to string representation
        // TODO: Implement proper date conversion
        return std::to_string(num);
    }, "Convert numeric date to string", py::arg("num"));

    m.def("date2num", [](const std::string& date_str) {
        // Convert string date to numeric representation  
        // TODO: Implement proper date parsing
        return 0.0;
    }, "Convert string date to numeric", py::arg("date_str"));

    // Performance utilities
    m.def("benchmark_operation", [](py::function func, int iterations) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            func();
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        return duration.count() / static_cast<double>(iterations);
    }, "Benchmark a function call", py::arg("func"), py::arg("iterations") = 1000);

    // Memory utilities
    m.def("sizeof_linebuffer", []() {
        return sizeof(backtrader::LineBuffer);
    }, "Get size of LineBuffer in bytes");

    m.def("sizeof_order", []() {
        return sizeof(backtrader::Order);
    }, "Get size of Order in bytes");
}