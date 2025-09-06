/**
 * @file analyzer_bindings.cpp
 * @brief Analyzer bindings for backtrader-cpp
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "analyzer.h"

namespace py = pybind11;
using namespace pybind11::literals;

void bind_analyzers(py::module& m) {
    // Analyzer base class
    py::class_<backtrader::Analyzer, std::shared_ptr<backtrader::Analyzer>>(m, "Analyzer",
        "Base class for strategy analyzers")
        .def("get_analysis", &backtrader::Analyzer::get_analysis,
             "Get analysis results");

    // Add specific analyzer bindings here
    // TODO: Implement specific analyzers like SharpeRatio, DrawDown, etc.
}