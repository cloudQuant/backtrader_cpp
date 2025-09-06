# Backtrader C++ 构建系统和开发环境设计

本文档详细设计了backtrader C++版本的完整构建系统、开发环境和CI/CD流水线。

## 🏗️ CMake构建系统设计

### 主CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(BacktraderCpp 
    VERSION 1.0.0
    DESCRIPTION "High-performance C++ backtesting framework"
    LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 编译选项
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")

# 编译器特定优化
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
endif()

# 选项
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_BENCHMARKS "Build benchmark tests" ON)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers" OFF)
option(USE_CONAN "Use Conan package manager" OFF)

# 依赖管理
if(USE_CONAN)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS)
endif()

# 查找依赖包
find_package(Threads REQUIRED)

# OpenMP支持
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(backtrader_core PUBLIC OpenMP::OpenMP_CXX)
endif()

# Google Test
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
endif()

# Google Benchmark
if(BUILD_BENCHMARKS)
    find_package(benchmark REQUIRED)
endif()

# Python绑定
if(BUILD_PYTHON_BINDINGS)
    find_package(pybind11 REQUIRED)
endif()

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# 添加子目录
add_subdirectory(src)
add_subdirectory(include)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(BUILD_PYTHON_BINDINGS)
    add_subdirectory(python)
endif()

# 安装配置
include(GNUInstallDirs)
install(TARGETS backtrader_core
    EXPORT BacktraderTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 导出配置
install(EXPORT BacktraderTargets
    FILE BacktraderTargets.cmake
    NAMESPACE Backtrader::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Backtrader
)

# 创建配置文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    BacktraderConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/BacktraderConfig.cmake.in
    BacktraderConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Backtrader
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/BacktraderConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/BacktraderConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Backtrader
)
```

### 模块化CMake结构

#### src/CMakeLists.txt
```cmake
# 核心库源文件
set(BACKTRADER_CORE_SOURCES
    core/MetaBase.cpp
    core/ParameterRegistry.cpp
    core/LineRoot.cpp
    core/LineBuffer.cpp
    core/LineIterator.cpp
    
    indicators/IndicatorBase.cpp
    indicators/SMA.cpp
    indicators/EMA.cpp
    indicators/RSI.cpp
    indicators/BollingerBands.cpp
    indicators/MACD.cpp
    indicators/Stochastic.cpp
    indicators/Ichimoku.cpp
    
    data/DataFeed.cpp
    data/CSVDataFeed.cpp
    data/Resampler.cpp
    
    broker/Order.cpp
    broker/Position.cpp
    broker/Trade.cpp
    broker/BrokerBase.cpp
    broker/BacktestBroker.cpp
    
    strategy/StrategyBase.cpp
    
    engine/Cerebro.cpp
    engine/OptimizationEngine.cpp
    
    utils/DateTime.cpp
    utils/Logger.cpp
    utils/ThreadPool.cpp
)

# 创建核心库
add_library(backtrader_core ${BACKTRADER_CORE_SOURCES})

# 设置目标属性
set_target_properties(backtrader_core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

# 链接依赖
target_link_libraries(backtrader_core
    PUBLIC
        Threads::Threads
    PRIVATE
        $<$<BOOL:${OpenMP_CXX_FOUND}>:OpenMP::OpenMP_CXX>
)

# 编译定义
target_compile_definitions(backtrader_core
    PRIVATE
        BACKTRADER_EXPORTS
    PUBLIC
        $<$<CONFIG:Debug>:BACKTRADER_DEBUG>
)

# 包含目录
target_include_directories(backtrader_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# 编译特性
target_compile_features(backtrader_core
    PUBLIC
        cxx_std_17
)

# 预编译头
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(backtrader_core
        PRIVATE
            <memory>
            <vector>
            <unordered_map>
            <string>
            <chrono>
            <thread>
            <future>
            <algorithm>
    )
endif()
```

#### tests/CMakeLists.txt
```cmake
# 测试源文件
set(TEST_SOURCES
    core/test_MetaBase.cpp
    core/test_ParameterRegistry.cpp
    core/test_LineBuffer.cpp
    core/test_LineIterator.cpp
    
    indicators/test_SMA.cpp
    indicators/test_EMA.cpp
    indicators/test_RSI.cpp
    indicators/test_BollingerBands.cpp
    indicators/test_MACD.cpp
    
    data/test_DataFeed.cpp
    data/test_Resampler.cpp
    
    broker/test_Order.cpp
    broker/test_Position.cpp
    broker/test_BacktestBroker.cpp
    
    strategy/test_StrategyBase.cpp
    
    engine/test_Cerebro.cpp
    
    integration/test_EndToEnd.cpp
    
    utils/TestDataProvider.cpp
    utils/PrecisionMatcher.cpp
)

# 创建测试可执行文件
add_executable(backtrader_tests ${TEST_SOURCES})

# 链接依赖
target_link_libraries(backtrader_tests
    PRIVATE
        backtrader_core
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
)

# 测试数据
configure_file(
    ${CMAKE_SOURCE_DIR}/tests/data/test_data.csv
    ${CMAKE_BINARY_DIR}/tests/data/test_data.csv
    COPYONLY
)

# 添加测试
include(GoogleTest)
gtest_discover_tests(backtrader_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    PROPERTIES TIMEOUT 600
)

# 覆盖率支持
if(ENABLE_COVERAGE)
    target_compile_options(backtrader_tests PRIVATE --coverage)
    target_link_options(backtrader_tests PRIVATE --coverage)
endif()

# 内存检查
if(ENABLE_SANITIZERS)
    target_compile_options(backtrader_tests PRIVATE 
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(backtrader_tests PRIVATE 
        -fsanitize=address,undefined
    )
endif()
```

#### benchmarks/CMakeLists.txt
```cmake
# 基准测试源文件
set(BENCHMARK_SOURCES
    indicators/benchmark_SMA.cpp
    indicators/benchmark_EMA.cpp
    indicators/benchmark_RSI.cpp
    
    data/benchmark_LineBuffer.cpp
    data/benchmark_DataProcessing.cpp
    
    strategy/benchmark_StrategyExecution.cpp
    
    engine/benchmark_Cerebro.cpp
    
    main.cpp
)

# 创建基准测试可执行文件
add_executable(backtrader_benchmarks ${BENCHMARK_SOURCES})

# 链接依赖
target_link_libraries(backtrader_benchmarks
    PRIVATE
        backtrader_core
        benchmark::benchmark
        benchmark::benchmark_main
)

# 基准测试数据
configure_file(
    ${CMAKE_SOURCE_DIR}/benchmarks/data/large_dataset.csv
    ${CMAKE_BINARY_DIR}/benchmarks/data/large_dataset.csv
    COPYONLY
)

# 设置基准测试属性
set_target_properties(backtrader_benchmarks PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks
)
```

## 🐍 Python绑定设计

### python/CMakeLists.txt
```cmake
# Python绑定源文件
set(PYTHON_BINDING_SOURCES
    bindings/core_bindings.cpp
    bindings/indicator_bindings.cpp
    bindings/data_bindings.cpp
    bindings/broker_bindings.cpp
    bindings/strategy_bindings.cpp
    bindings/engine_bindings.cpp
    bindings/module.cpp
)

# 创建Python模块
pybind11_add_module(backtrader_cpp ${PYTHON_BINDING_SOURCES})

# 链接核心库
target_link_libraries(backtrader_cpp PRIVATE backtrader_core)

# 设置模块属性
set_target_properties(backtrader_cpp PROPERTIES
    CXX_VISIBILITY_PRESET "hidden"
    INTERPROCEDURAL_OPTIMIZATION TRUE
)

# 编译定义
target_compile_definitions(backtrader_cpp
    PRIVATE
        VERSION_INFO=${PROJECT_VERSION}
)

# 安装Python模块
install(TARGETS backtrader_cpp
    DESTINATION ${CMAKE_INSTALL_PREFIX}/python/backtrader_cpp
)

# Python包文件
configure_file(
    ${CMAKE_SOURCE_DIR}/python/setup.py.in
    ${CMAKE_BINARY_DIR}/python/setup.py
    @ONLY
)

configure_file(
    ${CMAKE_SOURCE_DIR}/python/__init__.py
    ${CMAKE_BINARY_DIR}/python/backtrader_cpp/__init__.py
    COPYONLY
)
```

### Python绑定实现示例

#### bindings/core_bindings.cpp
```cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/chrono.h>
#include <pybind11/operators.h>

#include "core/LineRoot.h"
#include "core/LineBuffer.h"
#include "core/ParameterRegistry.h"

namespace py = pybind11;

void bind_core(py::module& m) {
    // ParameterRegistry绑定
    py::class_<ParameterRegistry>(m, "ParameterRegistry")
        .def(py::init<>())
        .def("set", [](ParameterRegistry& self, const std::string& name, py::object value) {
            // 处理Python类型到C++类型的转换
            if (py::isinstance<py::int_>(value)) {
                self.set(name, value.cast<int>());
            } else if (py::isinstance<py::float_>(value)) {
                self.set(name, value.cast<double>());
            } else if (py::isinstance<py::str>(value)) {
                self.set(name, value.cast<std::string>());
            } else if (py::isinstance<py::bool_>(value)) {
                self.set(name, value.cast<bool>());
            }
        })
        .def("get", [](const ParameterRegistry& self, const std::string& name) -> py::object {
            // 返回Python对象
            try {
                return py::cast(self.get<int>(name));
            } catch (...) {}
            try {
                return py::cast(self.get<double>(name));
            } catch (...) {}
            try {
                return py::cast(self.get<std::string>(name));
            } catch (...) {}
            try {
                return py::cast(self.get<bool>(name));
            } catch (...) {}
            return py::none();
        })
        .def("has", &ParameterRegistry::has);
    
    // CircularBuffer绑定
    py::class_<CircularBuffer<double>>(m, "CircularBuffer")
        .def(py::init<size_t>(), py::arg("capacity") = 1000)
        .def("__getitem__", [](const CircularBuffer<double>& self, int ago) {
            return self[ago];
        })
        .def("forward", &CircularBuffer<double>::forward,
             py::arg("value") = std::numeric_limits<double>::quiet_NaN(),
             py::arg("size") = 1)
        .def("backward", &CircularBuffer<double>::backward, py::arg("size") = 1)
        .def("home", &CircularBuffer<double>::home)
        .def("len", &CircularBuffer<double>::len)
        .def("empty", &CircularBuffer<double>::empty);
    
    // LineRoot绑定
    py::class_<LineRoot>(m, "LineRoot")
        .def("__getitem__", [](const LineRoot& self, int ago) {
            return self[ago];
        })
        .def("__call__", [](const LineRoot& self, int ago) {
            return self(ago);
        })
        .def("get", &LineRoot::get, py::arg("ago") = 0)
        .def("len", &LineRoot::len)
        .def("buflen", &LineRoot::buflen)
        .def("forward", &LineRoot::forward,
             py::arg("value") = std::numeric_limits<double>::quiet_NaN(),
             py::arg("size") = 1)
        .def("getMinPeriod", &LineRoot::getMinPeriod)
        // 运算符重载
        .def("__add__", [](const LineRoot& self, double other) {
            return self + other;
        })
        .def("__radd__", [](const LineRoot& self, double other) {
            return other + self;
        })
        .def("__sub__", [](const LineRoot& self, double other) {
            return self - other;
        })
        .def("__rsub__", [](const LineRoot& self, double other) {
            return other - self;
        })
        .def("__mul__", [](const LineRoot& self, double other) {
            return self * other;
        })
        .def("__rmul__", [](const LineRoot& self, double other) {
            return other * self;
        })
        .def("__truediv__", [](const LineRoot& self, double other) {
            return self / other;
        })
        .def("__rtruediv__", [](const LineRoot& self, double other) {
            return other / self;
        });
}
```

## 🔄 CI/CD 流水线设计

### GitHub Actions工作流

#### .github/workflows/ci.yml
```yaml
name: CI

on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

env:
  BUILD_TYPE: Release

jobs:
  # 代码质量检查
  code-quality:
    runs-on: ubuntu-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install clang-format
      run: sudo apt-get install clang-format
    
    - name: Check code formatting
      run: |
        find src include tests -name "*.cpp" -o -name "*.h" | xargs clang-format --dry-run --Werror
    
    - name: Install cppcheck
      run: sudo apt-get install cppcheck
    
    - name: Run static analysis
      run: |
        cppcheck --enable=all --error-exitcode=1 --suppress=missingIncludeSystem src/ include/

  # Linux构建和测试
  linux-build:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        compiler: [gcc-11, clang-14]
        build_type: [Debug, Release]
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake ninja-build libgtest-dev libbenchmark-dev
        
        # 安装编译器
        if [[ "${{ matrix.compiler }}" == "gcc-11" ]]; then
          sudo apt-get install -y gcc-11 g++-11
          echo "CC=gcc-11" >> $GITHUB_ENV
          echo "CXX=g++-11" >> $GITHUB_ENV
        elif [[ "${{ matrix.compiler }}" == "clang-14" ]]; then
          sudo apt-get install -y clang-14
          echo "CC=clang-14" >> $GITHUB_ENV
          echo "CXX=clang++-14" >> $GITHUB_ENV
        fi
    
    - name: Configure CMake
      run: |
        cmake -B ${{github.workspace}}/build \
              -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
              -DBUILD_TESTS=ON \
              -DBUILD_BENCHMARKS=ON \
              -DENABLE_COVERAGE=${{ matrix.build_type == 'Debug' && matrix.compiler == 'gcc-11' }} \
              -G Ninja
    
    - name: Build
      run: cmake --build ${{github.workspace}}/build --config ${{ matrix.build_type }}
    
    - name: Test
      working-directory: ${{github.workspace}}/build
      run: ctest --output-on-failure
    
    - name: Upload coverage to Codecov
      if: matrix.build_type == 'Debug' && matrix.compiler == 'gcc-11'
      uses: codecov/codecov-action@v3
      with:
        directory: ${{github.workspace}}/build
        flags: unittests
        name: codecov-umbrella

  # macOS构建
  macos-build:
    runs-on: macos-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        brew install cmake ninja googletest google-benchmark
    
    - name: Configure CMake
      run: |
        cmake -B ${{github.workspace}}/build \
              -DCMAKE_BUILD_TYPE=${{env.BUILD_TYPE}} \
              -DBUILD_TESTS=ON \
              -DBUILD_BENCHMARKS=ON \
              -G Ninja
    
    - name: Build
      run: cmake --build ${{github.workspace}}/build --config ${{env.BUILD_TYPE}}
    
    - name: Test
      working-directory: ${{github.workspace}}/build
      run: ctest --output-on-failure

  # Windows构建
  windows-build:
    runs-on: windows-latest
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Configure CMake
      run: |
        cmake -B ${{github.workspace}}/build ^
              -DCMAKE_BUILD_TYPE=${{env.BUILD_TYPE}} ^
              -DBUILD_TESTS=ON ^
              -DBUILD_BENCHMARKS=ON
    
    - name: Build
      run: cmake --build ${{github.workspace}}/build --config ${{env.BUILD_TYPE}}
    
    - name: Test
      working-directory: ${{github.workspace}}/build
      run: ctest --output-on-failure -C ${{env.BUILD_TYPE}}

  # Python绑定测试
  python-bindings:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        python-version: [3.8, 3.9, '3.10', '3.11']
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Set up Python ${{ matrix.python-version }}
      uses: actions/setup-python@v4
      with:
        python-version: ${{ matrix.python-version }}
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake ninja-build
        python -m pip install --upgrade pip
        pip install pybind11[global] pytest numpy pandas
    
    - name: Configure and build
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_PYTHON_BINDINGS=ON -G Ninja
        cmake --build build
    
    - name: Install Python package
      run: |
        cd build/python
        pip install .
    
    - name: Test Python bindings
      run: |
        python -c "import backtrader_cpp; print('Import successful')"
        pytest tests/python/ -v

  # 性能基准测试
  benchmarks:
    runs-on: ubuntu-latest
    if: github.event_name == 'push' && github.ref == 'refs/heads/main'
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake ninja-build libbenchmark-dev
    
    - name: Configure CMake
      run: |
        cmake -B ${{github.workspace}}/build \
              -DCMAKE_BUILD_TYPE=Release \
              -DBUILD_BENCHMARKS=ON \
              -G Ninja
    
    - name: Build
      run: cmake --build ${{github.workspace}}/build --config Release
    
    - name: Run benchmarks
      working-directory: ${{github.workspace}}/build
      run: |
        ./benchmarks/backtrader_benchmarks --benchmark_format=json > benchmark_results.json
    
    - name: Store benchmark result
      uses: benchmark-action/github-action-benchmark@v1
      with:
        tool: 'googlecpp'
        output-file-path: ${{github.workspace}}/build/benchmark_results.json
        github-token: ${{ secrets.GITHUB_TOKEN }}
        auto-push: true
```

### Docker支持

#### Dockerfile.dev (开发环境)
```dockerfile
FROM ubuntu:22.04

# 设置环境变量
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# 安装基础依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    curl \
    wget \
    pkg-config \
    libgtest-dev \
    libbenchmark-dev \
    python3-dev \
    python3-pip \
    clang-format \
    cppcheck \
    valgrind \
    gdb \
    && rm -rf /var/lib/apt/lists/*

# 安装现代编译器
RUN apt-get update && apt-get install -y \
    gcc-11 \
    g++-11 \
    clang-14 \
    && rm -rf /var/lib/apt/lists/*

# 安装Python依赖
RUN pip3 install --no-cache-dir \
    pybind11[global] \
    numpy \
    pandas \
    pytest \
    conan

# 设置工作目录
WORKDIR /workspace

# 创建非root用户
RUN useradd -m -s /bin/bash developer && \
    chown -R developer:developer /workspace

USER developer

# 设置默认编译器
ENV CC=gcc-11
ENV CXX=g++-11

# 启动命令
CMD ["/bin/bash"]
```

#### Dockerfile.prod (生产环境)
```dockerfile
FROM ubuntu:22.04 AS builder

# 构建阶段
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    && rm -rf /var/lib/apt/lists/*

COPY . /src
WORKDIR /src

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release -G Ninja && \
    cmake --build build --config Release

# 运行时阶段
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libgomp1 \
    python3 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/lib* /usr/local/lib/
COPY --from=builder /src/build/bin* /usr/local/bin/
COPY --from=builder /src/include /usr/local/include/

RUN ldconfig

WORKDIR /app
CMD ["/bin/bash"]
```

### Conan包管理配置

#### conanfile.py
```python
from conans import ConanFile, CMake, tools

class BacktraderCppConan(ConanFile):
    name = "backtrader-cpp"
    version = "1.0.0"
    license = "Apache-2.0"
    author = "Backtrader Team"
    url = "https://github.com/your-org/backtrader-cpp"
    description = "High-performance C++ backtesting framework"
    topics = ("backtesting", "trading", "finance", "cpp")
    
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_python_bindings": [True, False],
        "with_benchmarks": [True, False]
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_python_bindings": True,
        "with_benchmarks": False
    }
    
    generators = "cmake"
    exports_sources = "src/*", "include/*", "CMakeLists.txt", "cmake/*"
    
    def requirements(self):
        if self.options.with_benchmarks:
            self.requires("benchmark/1.7.1")
        
        if self.options.with_python_bindings:
            self.requires("pybind11/2.10.1")
    
    def build_requirements(self):
        self.build_requires("cmake/3.25.0")
        self.build_requires("gtest/1.12.1")
    
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC
    
    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
    
    def build(self):
        cmake = CMake(self)
        cmake.definitions["BUILD_TESTS"] = "OFF"
        cmake.definitions["BUILD_BENCHMARKS"] = self.options.with_benchmarks
        cmake.definitions["BUILD_PYTHON_BINDINGS"] = self.options.with_python_bindings
        cmake.configure()
        cmake.build()
    
    def package(self):
        self.copy("*.h", dst="include", src="include")
        self.copy("*.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.dylib*", dst="lib", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)
    
    def package_info(self):
        self.cpp_info.libs = ["backtrader_core"]
        self.cpp_info.cppflags = ["-std=c++17"]
        
        if self.settings.os in ["Linux", "FreeBSD"]:
            self.cpp_info.system_libs = ["pthread", "m"]
```

## 📊 开发工具集成

### VS Code配置

#### .vscode/settings.json
```json
{
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "C_Cpp.default.cppStandard": "c++17",
    "files.associations": {
        "*.h": "cpp",
        "*.cpp": "cpp"
    },
    "clang-format.executable": "clang-format",
    "editor.formatOnSave": true,
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.generator": "Ninja",
    "python.defaultInterpreterPath": "./venv/bin/python"
}
```

#### .vscode/tasks.json
```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "CMake Configure",
            "type": "shell",
            "command": "cmake",
            "args": [
                "-B", "build",
                "-DCMAKE_BUILD_TYPE=Debug",
                "-DBUILD_TESTS=ON",
                "-DBUILD_BENCHMARKS=ON",
                "-G", "Ninja"
            ],
            "group": "build",
            "problemMatcher": []
        },
        {
            "label": "Build All",
            "type": "shell",
            "command": "cmake",
            "args": ["--build", "build"],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": "$gcc"
        },
        {
            "label": "Run Tests",
            "type": "shell",
            "command": "ctest",
            "args": ["--output-on-failure"],
            "options": {
                "cwd": "${workspaceFolder}/build"
            },
            "group": "test",
            "dependsOn": "Build All"
        },
        {
            "label": "Run Benchmarks",
            "type": "shell",
            "command": "./benchmarks/backtrader_benchmarks",
            "options": {
                "cwd": "${workspaceFolder}/build"
            },
            "group": "test",
            "dependsOn": "Build All"
        }
    ]
}
```

### 代码质量工具

#### .clang-format
```yaml
---
Language: Cpp
BasedOnStyle: Google
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 100
BreakBeforeBraces: Attach
AllowShortIfStatementsOnASingleLine: false
AllowShortLoopsOnASingleLine: false
AllowShortFunctionsOnASingleLine: None
IndentCaseLabels: true
AccessModifierOffset: -2
NamespaceIndentation: None
```

#### .clang-tidy
```yaml
---
Checks: '
  *,
  -abseil-*,
  -altera-*,
  -android-*,
  -fuchsia-*,
  -google-*,
  -llvm-*,
  -zircon-*,
  -readability-magic-numbers,
  -cppcoreguidelines-avoid-magic-numbers,
  -cppcoreguidelines-pro-bounds-pointer-arithmetic,
  -cppcoreguidelines-pro-bounds-array-to-pointer-decay,
  -hicpp-no-array-decay
'
WarningsAsErrors: ''
HeaderFilterRegex: '.*'
FormatStyle: file
```

这个完整的构建系统设计提供了：

1. **现代化CMake**: 模块化、可扩展的构建配置
2. **多平台支持**: Linux、macOS、Windows全覆盖
3. **完整CI/CD**: 自动化测试、静态分析、性能基准
4. **Python绑定**: 无缝的Python接口
5. **包管理**: Conan集成，简化依赖管理
6. **开发工具**: VS Code、Docker、代码质量工具

这为backtrader C++重构项目提供了坚实的基础设施支持。