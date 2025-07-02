#!/bin/bash

echo "=== Building Backtrader C++ Tests ==="

# 进入项目根目录
cd "$(dirname "$0")"

# 创建并进入构建目录
if [ -d "build" ]; then
    echo "Removing existing build directory..."
    rm -rf build
fi

echo "Creating build directory..."
mkdir build
cd build

# 配置CMake
echo "Configuring CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

# 构建核心库
echo "Building core library..."
make backtrader_core -j4

# 构建简单测试
echo "Building simple test..."
make simple_test -j4

# 构建原始测试（如果GTest可用）
echo "Building original tests..."
if [ -d "tests/original_tests" ]; then
    cd tests/original_tests
    make test_ind_sma_simple -j4 2>/dev/null || echo "GTest tests not available"
    cd ../..
fi

echo "=== Build completed ==="

# 运行简单测试
echo "Running simple test..."
if [ -f "tests/simple_test" ]; then
    ./tests/simple_test
else
    echo "Simple test not found"
fi

echo "=== Test execution completed ==="