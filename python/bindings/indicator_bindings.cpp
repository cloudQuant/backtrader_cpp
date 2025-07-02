#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "indicators/IndicatorBase.h"
#include "indicators/SMA.h"
#include "indicators/EMA.h"

namespace py = pybind11;

void bind_indicators(py::module& m) {
    // IndicatorBase绑定
    py::class_<backtrader::IndicatorBase, backtrader::LineRoot, std::shared_ptr<backtrader::IndicatorBase>>(m, "IndicatorBase")
        .def("addInput", &backtrader::IndicatorBase::addInput)
        .def("getInput", &backtrader::IndicatorBase::getInput, py::arg("index") = 0)
        .def("getInputCount", &backtrader::IndicatorBase::getInputCount)
        .def("getOutput", &backtrader::IndicatorBase::getOutput, py::arg("index") = 0)
        .def("getOutputCount", &backtrader::IndicatorBase::getOutputCount)
        .def("setParam", &backtrader::IndicatorBase::setParam)
        .def("getParam", &backtrader::IndicatorBase::getParam, py::arg("name"), py::arg("default_value") = 0.0)
        .def("hasParam", &backtrader::IndicatorBase::hasParam)
        .def("getParams", &backtrader::IndicatorBase::getParams)
        .def("initialize", &backtrader::IndicatorBase::initialize)
        .def("isInitialized", &backtrader::IndicatorBase::isInitialized)
        .def("calculate", &backtrader::IndicatorBase::calculate)
        .def("calculateBatch", &backtrader::IndicatorBase::calculateBatch)
        .def("reset", &backtrader::IndicatorBase::reset)
        .def("hasValidInput", &backtrader::IndicatorBase::hasValidInput)
        .def("getRequiredDataCount", &backtrader::IndicatorBase::getRequiredDataCount)
        .def("hasEnoughData", &backtrader::IndicatorBase::hasEnoughData);
    
    // SMA绑定
    py::class_<backtrader::SMA, backtrader::IndicatorBase, std::shared_ptr<backtrader::SMA>>(m, "SMA")
        .def(py::init<std::shared_ptr<backtrader::LineRoot>, size_t, bool>(),
             py::arg("input"), py::arg("period") = 30, py::arg("use_incremental") = true)
        .def("getPeriod", &backtrader::SMA::getPeriod)
        .def("setPeriod", &backtrader::SMA::setPeriod)
        .def("getCurrentWindow", &backtrader::SMA::getCurrentWindow)
        .def("getCurrentSum", &backtrader::SMA::getCurrentSum)
        .def("isUsingIncremental", &backtrader::SMA::isUsingIncremental);
    
    // EMA绑定
    py::class_<backtrader::EMA, backtrader::IndicatorBase, std::shared_ptr<backtrader::EMA>>(m, "EMA")
        .def(py::init<std::shared_ptr<backtrader::LineRoot>, size_t>(),
             py::arg("input"), py::arg("period") = 30)
        .def("getPeriod", &backtrader::EMA::getPeriod)
        .def("getAlpha", &backtrader::EMA::getAlpha)
        .def("setPeriod", &backtrader::EMA::setPeriod)
        .def("setAlpha", &backtrader::EMA::setAlpha)
        .def("getPreviousEMA", &backtrader::EMA::getPreviousEMA)
        .def("hasPreviousValue", &backtrader::EMA::hasPreviousValue)
        .def("setInitialValue", &backtrader::EMA::setInitialValue)
        .def("getStabilizationPeriod", &backtrader::EMA::getStabilizationPeriod)
        .def("getWeights", &backtrader::EMA::getWeights)
        .def("getTotalWeight", &backtrader::EMA::getTotalWeight);
}