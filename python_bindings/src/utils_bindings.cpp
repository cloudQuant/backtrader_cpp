/**
 * @file utils_bindings.cpp
 * @brief Utility bindings for backtrader-cpp
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

void bind_utils(py::module& m) {
    // Utility functions
    m.def("version", []() {
        return "1.0.0";
    }, "Get backtrader-cpp version");

    m.def("build_info", []() {
        return py::dict(
            "version"_a="1.0.0",
            "build_date"_a=__DATE__,
            "compiler"_a="C++20"
        );
    }, "Get build information");
}