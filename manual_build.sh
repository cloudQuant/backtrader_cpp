#!/bin/bash

echo "Starting C++ project build process..."

# 1. 设置工作目录
cd /home/yun/Documents/refactor_backtrader/backtrader_cpp || exit 1

# 2. 清理现有构建文件
echo "Cleaning existing build files..."
rm -rf build
mkdir -p build

# 3. 进入构建目录
cd build || exit 1

# 4. 运行 cmake 配置
echo "Running cmake configuration..."
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DBUILD_PYTHON_BINDINGS=OFF

# 5. 编译核心库
echo "Building core library..."
make -j$(nproc) backtrader_core

# 6. 编译测试
echo "Building tests..."
make -j$(nproc) simple_test

# 7. 运行测试
echo "Running simple test..."
./tests/simple_test

echo "Build process completed."