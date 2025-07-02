#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include "core/CircularBuffer.h"
#include "core/LineRoot.h"
#include "core/Common.h"

namespace py = pybind11;

void bind_core(py::module& m) {
    // 绑定常量
    m.attr("NaN") = backtrader::NaN;
    
    // 实用函数
    m.def("isNaN", &backtrader::isNaN, "Check if value is NaN");
    m.def("isFinite", &backtrader::isFinite, "Check if value is finite");
    m.def("isValid", &backtrader::isValid, "Check if value is valid (finite and not NaN)");
    
    // CircularBuffer绑定
    py::class_<backtrader::CircularBuffer<double>>(m, "CircularBuffer")
        .def(py::init<size_t>(), py::arg("capacity") = 1000)
        .def("__len__", &backtrader::CircularBuffer<double>::len)
        .def("empty", &backtrader::CircularBuffer<double>::empty)
        .def("capacity", &backtrader::CircularBuffer<double>::capacity)
        .def("__getitem__", [](const backtrader::CircularBuffer<double>& self, int ago) {
            return self[ago];
        })
        .def("get", &backtrader::CircularBuffer<double>::get, py::arg("ago") = 0)
        .def("forward", &backtrader::CircularBuffer<double>::forward,
             py::arg("value") = std::numeric_limits<double>::quiet_NaN(),
             py::arg("size") = 1)
        .def("backward", &backtrader::CircularBuffer<double>::backward, py::arg("size") = 1)
        .def("home", &backtrader::CircularBuffer<double>::home)
        .def("getBatch", [](const backtrader::CircularBuffer<double>& self, int start_ago, size_t count) {
            std::vector<double> result(count);
            self.getBatch(start_ago, count, result.data());
            return result;
        })
        .def("getContinuousView", [](const backtrader::CircularBuffer<double>& self, int start_ago, size_t count) -> py::object {
            const double* view = self.getContinuousView(start_ago, count);
            if (view) {
                return py::cast(std::vector<double>(view, view + count));
            }
            return py::none();
        });
    
    // LineRoot绑定
    py::class_<backtrader::LineRoot, std::shared_ptr<backtrader::LineRoot>>(m, "LineRoot")
        .def(py::init<size_t, const std::string&>(), 
             py::arg("capacity") = 1000, py::arg("name") = "")
        .def("__len__", &backtrader::LineRoot::len)
        .def("buflen", &backtrader::LineRoot::buflen)
        .def("empty", &backtrader::LineRoot::empty)
        .def("__getitem__", [](const backtrader::LineRoot& self, int ago) {
            return self[ago];
        })
        .def("__call__", [](const backtrader::LineRoot& self, int ago) {
            return self(ago);
        })
        .def("get", &backtrader::LineRoot::get, py::arg("ago") = 0)
        .def("forward", &backtrader::LineRoot::forward,
             py::arg("value") = std::numeric_limits<double>::quiet_NaN(),
             py::arg("size") = 1)
        .def("backward", &backtrader::LineRoot::backward, py::arg("size") = 1)
        .def("home", &backtrader::LineRoot::home)
        .def("getMinPeriod", &backtrader::LineRoot::getMinPeriod)
        .def("setMinPeriod", &backtrader::LineRoot::setMinPeriod)
        .def("getName", &backtrader::LineRoot::getName)
        .def("setName", &backtrader::LineRoot::setName)
        
        // 运算符重载
        .def("__add__", [](const backtrader::LineRoot& self, double other) {
            return self + other;
        })
        .def("__radd__", [](const backtrader::LineRoot& self, double other) {
            return other + self;
        })
        .def("__sub__", [](const backtrader::LineRoot& self, double other) {
            return self - other;
        })
        .def("__rsub__", [](const backtrader::LineRoot& self, double other) {
            return other - self;
        })
        .def("__mul__", [](const backtrader::LineRoot& self, double other) {
            return self * other;
        })
        .def("__rmul__", [](const backtrader::LineRoot& self, double other) {
            return other * self;
        })
        .def("__truediv__", [](const backtrader::LineRoot& self, double other) {
            return self / other;
        })
        .def("__rtruediv__", [](const backtrader::LineRoot& self, double other) {
            return other / self;
        })
        .def("__gt__", [](const backtrader::LineRoot& self, double other) {
            return self > other;
        })
        .def("__lt__", [](const backtrader::LineRoot& self, double other) {
            return self < other;
        })
        .def("__ge__", [](const backtrader::LineRoot& self, double other) {
            return self >= other;
        })
        .def("__le__", [](const backtrader::LineRoot& self, double other) {
            return self <= other;
        })
        .def("__eq__", [](const backtrader::LineRoot& self, double other) {
            return self == other;
        })
        .def("__ne__", [](const backtrader::LineRoot& self, double other) {
            return self != other;
        })
        
        // LineRoot之间的运算
        .def("__add__", [](const backtrader::LineRoot& self, const backtrader::LineRoot& other) {
            return self + other;
        })
        .def("__sub__", [](const backtrader::LineRoot& self, const backtrader::LineRoot& other) {
            return self - other;
        })
        .def("__mul__", [](const backtrader::LineRoot& self, const backtrader::LineRoot& other) {
            return self * other;
        })
        .def("__truediv__", [](const backtrader::LineRoot& self, const backtrader::LineRoot& other) {
            return self / other;
        })
        .def("__gt__", [](const backtrader::LineRoot& self, const backtrader::LineRoot& other) {
            return self > other;
        })
        .def("__lt__", [](const backtrader::LineRoot& self, const backtrader::LineRoot& other) {
            return self < other;
        });
}