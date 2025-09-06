/**
 * @file broker_bindings.cpp
 * @brief Broker bindings for backtrader-cpp
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "broker.h"

namespace py = pybind11;
using namespace pybind11::literals;

void bind_broker(py::module& m) {
    // BrokerBase class
    py::class_<backtrader::BrokerBase, std::shared_ptr<backtrader::BrokerBase>>(m, "BrokerBase",
        "Base broker for order execution and portfolio management")
        .def("setcash", &backtrader::BrokerBase::setcash, "cash"_a)
        .def("getcash", &backtrader::BrokerBase::getcash)
        .def("getvalue", &backtrader::BrokerBase::getvalue);

    // BackBroker class 
    py::class_<backtrader::BackBroker, backtrader::BrokerBase, std::shared_ptr<backtrader::BackBroker>>(m, "Broker",
        "Backtesting broker implementation")
        .def(py::init<>());
}