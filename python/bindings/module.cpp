#include <pybind11/pybind11.h>

namespace py = pybind11;

// 声明各个子模块绑定函数
void bind_core(py::module& m);
void bind_indicators(py::module& m);

PYBIND11_MODULE(backtrader_cpp_native, m) {
    m.doc() = "High-performance C++ backend for backtrader (Phase 0 prototype)";
    m.attr("__version__") = VERSION_INFO;
    
    // 绑定核心模块
    auto core_module = m.def_submodule("core", "Core data structures");
    bind_core(core_module);
    
    // 绑定指标模块
    auto indicators_module = m.def_submodule("indicators", "Technical indicators");
    bind_indicators(indicators_module);
}