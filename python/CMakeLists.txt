# Python绑定
if(NOT pybind11_FOUND)
    message(WARNING "pybind11 not found, skipping Python bindings")
    return()
endif()

# Python绑定源文件
set(PYTHON_BINDING_SOURCES
    bindings/core_bindings.cpp
    bindings/indicator_bindings.cpp
    bindings/module.cpp
)

# 创建Python模块
pybind11_add_module(backtrader_cpp_native ${PYTHON_BINDING_SOURCES})

# 链接核心库
target_link_libraries(backtrader_cpp_native PRIVATE backtrader_core)

# 设置模块属性
set_target_properties(backtrader_cpp_native PROPERTIES
    CXX_VISIBILITY_PRESET "hidden"
    INTERPROCEDURAL_OPTIMIZATION TRUE
)

# 编译定义
target_compile_definitions(backtrader_cpp_native
    PRIVATE
        VERSION_INFO=${PROJECT_VERSION}
)

# 包含目录
target_include_directories(backtrader_cpp_native
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)