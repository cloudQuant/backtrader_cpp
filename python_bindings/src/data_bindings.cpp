/**
 * @file data_bindings.cpp
 * @brief Data source bindings for backtrader-cpp
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "dataseries.h"

namespace py = pybind11;
using namespace pybind11::literals;

void bind_data(py::module& m) {
    // DataSeries class
    py::class_<backtrader::DataSeries, std::shared_ptr<backtrader::DataSeries>>(m, "DataSeries",
        "OHLCV data series container")
        .def(py::init<>())
        .def("size", &backtrader::DataSeries::size)
        .def("__len__", &backtrader::DataSeries::size)
        .def("open", [](const backtrader::DataSeries& self, int ago) {
             return self.open(ago);
         }, py::arg("ago") = 0)
        .def("high", [](const backtrader::DataSeries& self, int ago) {
             return self.high(ago);
         }, py::arg("ago") = 0)
        .def("low", [](const backtrader::DataSeries& self, int ago) {
             return self.low(ago);
         }, py::arg("ago") = 0)
        .def("close", [](const backtrader::DataSeries& self, int ago) {
             return self.close(ago);
         }, py::arg("ago") = 0)
        .def("volume", [](const backtrader::DataSeries& self, int ago) {
             return self.volume(ago);
         }, py::arg("ago") = 0)
        .def("datetime", [](const backtrader::DataSeries& self, int ago) {
             return self.datetime(ago);
         }, py::arg("ago") = 0);

    // Data feed factory functions
    m.def("CSVData", [](const std::string& dataname, py::kwargs kwargs) {
        auto csv_data = std::make_shared<backtrader::DataSeries>();
        // TODO: Implement CSV loading
        return csv_data;
    }, "Create CSV data feed", py::arg("dataname"));

    m.def("PandasData", [](py::object dataframe, py::kwargs kwargs) {
        auto data_series = std::make_shared<backtrader::DataSeries>();
        // TODO: Implement Pandas DataFrame conversion
        return data_series;
    }, "Create Pandas data feed", py::arg("dataframe"));
}