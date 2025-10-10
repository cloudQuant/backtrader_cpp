# Backtrader C++ 跨平台编译和测试指南

本文档说明如何在 Mac 和 Ubuntu 上编译和运行 Backtrader C++ 测试。

## 支持的平台

- **Ubuntu/Linux**: Ubuntu 20.04+, Debian 11+, 及其他现代 Linux 发行版
- **macOS**: macOS 11+ (Big Sur及以后版本)

## 系统要求

### 通用要求

- CMake >= 3.16
- C++ 编译器支持 C++20 标准
- Google Test 库

### Ubuntu/Linux

```bash
# 安装依赖
sudo apt-get update
sudo apt-get install -y cmake g++ libgtest-dev pkg-config

# 如果使用的是旧版Ubuntu，可能需要编译安装GTest
cd /usr/src/gtest
sudo cmake CMakeLists.txt
sudo make
sudo cp lib/*.a /usr/lib
```

### macOS

```bash
# 安装 Homebrew (如果还没安装)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install cmake googletest coreutils

# coreutils 提供 gtimeout 命令（用于测试超时控制）
```

## 编译和运行测试

### 方法一：使用 run_tests.sh（推荐）

这个脚本会自动检测平台并使用正确的编译选项。

```bash
cd backtrader_cpp

# 默认模式：使用CMake编译和运行所有测试（推荐）
./run_tests.sh

# 仅使用CMake编译和测试
./run_tests.sh --cmake-only

# 查看帮助
./run_tests.sh --help

# 生成JSON格式的报告
./run_tests.sh --json-report

# 设置超时时间（例如30秒）
./run_tests.sh --timeout 30
```

### 方法二：手动使用 CMake

```bash
cd backtrader_cpp

# 1. 清理旧的构建文件
rm -rf CMakeCache.txt cmake_install.cmake Makefile CMakeFiles/

# 2. 配置项目
cmake . -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=OFF

# 3. 编译核心库（使用8核并行）
cmake --build . --config Debug --parallel 8

# 4. 进入测试目录
cd tests

# 5. 清理测试构建目录
rm -rf build
mkdir -p build
cd build

# 6. 配置测试
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 7. 编译测试（使用8核并行）
cmake --build . --config Debug --parallel 8

# 8. 运行所有测试
ctest --output-on-failure

# 9. 或者运行单个测试
./test_sma
```

## 跨平台差异处理

### 1. 可执行文件检测

脚本自动检测平台并使用正确的 `find` 命令选项：
- **Linux**: 使用 `-executable` 标志
- **macOS**: 使用 `-perm +111` 标志（BSD风格）

### 2. 动态库路径

脚本自动设置正确的库路径环境变量：
- **Linux**: 使用 `LD_LIBRARY_PATH`
- **macOS**: 使用 `DYLD_LIBRARY_PATH`

库路径会自动检测并包含：
- Linux: `/usr/lib/x86_64-linux-gnu`, `/usr/lib`, `/usr/local/lib`
- Mac: `$HOME/opt/anaconda3/lib`, `/opt/homebrew/lib`, `/usr/local/lib`

### 3. 超时命令

- **Linux**: 使用系统自带的 `timeout` 命令
- **macOS**: 使用 `gtimeout`（来自 coreutils），如果未安装则不使用超时

### 4. 编译器选项

CMake 自动检测编译器并应用正确的选项：
- **GCC**: 包含 `-Wno-unused-but-set-variable`
- **Clang/AppleClang**: 不包含GCC特定的选项
- **macOS**: 添加 `-stdlib=libc++`

### 5. 线程库链接

使用 CMake 的 `find_package(Threads)` 自动处理：
- **Linux**: 链接 `pthread`
- **macOS**: 使用系统自带的线程库

## 故障排除

### Ubuntu

**问题**: 找不到 Google Test
```bash
# 解决方案
sudo apt-get install libgtest-dev
# 或者从源码编译
cd /usr/src/gtest
sudo cmake CMakeLists.txt
sudo make
sudo cp lib/*.a /usr/lib
```

**问题**: 编译器不支持 C++20
```bash
# 安装较新的 GCC
sudo apt-get install g++-11
export CXX=g++-11
```

### macOS

**问题**: 找不到 gtimeout
```bash
# 安装 coreutils
brew install coreutils
```

**问题**: 找不到 Google Test
```bash
# 使用 Homebrew 安装
brew install googletest
```

**问题**: 链接错误 (PIE related)
```bash
# CMakeLists.txt 已经包含了 -Wl,-no_pie 选项
# 如果仍有问题，尝试：
export LDFLAGS="-Wl,-no_pie"
```

## 验证安装

运行以下命令验证所有依赖都已正确安装：

```bash
# 检查 CMake
cmake --version

# 检查编译器
g++ --version  # 或 clang++ --version

# 检查 Google Test (Ubuntu)
pkg-config --modversion gtest

# 检查 Google Test (Mac)
brew list googletest

# 检查线程支持
ldd $(which cmake) | grep pthread  # Linux
otool -L $(which cmake) | grep pthread  # Mac

# 运行测试脚本进行完整验证
cd backtrader_cpp
./run_tests.sh --cmake-only
```

## 性能优化建议

1. **并行编译**: 使用 `-parallel 8` 选项（或根据CPU核心数调整）
2. **增量编译**: CMake 支持增量编译，只重新编译修改的文件
3. **Release 模式**: 对于生产环境，使用 `-DCMAKE_BUILD_TYPE=Release`
4. **缓存**: 保留 CMake 缓存以加快重新配置速度

## 持续集成 (CI)

可以在 CI 环境中使用以下配置：

### GitHub Actions 示例

```yaml
name: Cross-Platform Build

on: [push, pull_request]

jobs:
  build:
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest]
        
    runs-on: ${{ matrix.os }}
    
    steps:
    - uses: actions/checkout@v2
    
    - name: Install dependencies (Ubuntu)
      if: runner.os == 'Linux'
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake g++ libgtest-dev
        
    - name: Install dependencies (macOS)
      if: runner.os == 'macOS'
      run: |
        brew install cmake googletest coreutils
        
    - name: Build and Test
      run: |
        cd backtrader_cpp
        ./run_tests.sh --cmake-only --json-report
```

## 贡献

如果你在其他平台上测试成功，或者发现平台特定的问题，请提交 Issue 或 Pull Request。

## 许可证

与主项目相同。

