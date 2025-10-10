# 跨平台快速开始指南

## 一键测试

### Ubuntu/Linux
```bash
# 安装依赖
sudo apt-get update
sudo apt-get install -y cmake g++ libgtest-dev pkg-config

# 检查平台
cd backtrader_cpp
./check_platform.sh

# 运行测试
./run_tests.sh --cmake-only
```

### macOS
```bash
# 安装依赖
brew install cmake googletest coreutils

# 检查平台
cd backtrader_cpp
./check_platform.sh

# 运行测试
./run_tests.sh --cmake-only
```

## 关键改进

### 自动平台检测
脚本会自动检测您的平台并使用正确的配置：

| 功能 | Linux | macOS |
|------|-------|-------|
| 可执行文件检测 | `-executable` | `-perm +111` |
| 库路径变量 | `LD_LIBRARY_PATH` | `DYLD_LIBRARY_PATH` |
| 超时命令 | `timeout` | `gtimeout` |
| 编译器 | GCC/Clang | AppleClang |

### 智能库路径检测

**Linux** 会自动检测:
- `/usr/lib/x86_64-linux-gnu`
- `/usr/lib`
- `/usr/local/lib`

**macOS** 会自动检测:
- `$HOME/opt/anaconda3/lib`
- `/opt/homebrew/lib`
- `/usr/local/lib`

## 常用命令

```bash
# 1. 检查平台兼容性
./check_platform.sh

# 2. 快速验证跨平台功能
./test_cross_platform.sh

# 3. 运行完整测试（推荐）
./run_tests.sh --cmake-only

# 4. 查看详细文档
cat CROSS_PLATFORM.md

# 5. 查看更新日志
cat CHANGELOG_CROSS_PLATFORM.md
```

## 故障排除

### Linux: 找不到 Google Test
```bash
sudo apt-get install libgtest-dev
```

### macOS: 找不到 gtimeout
```bash
brew install coreutils
```

### 编译器不支持 C++20
```bash
# Linux
sudo apt-get install g++-11
export CXX=g++-11

# macOS
brew install llvm
export CXX=/opt/homebrew/opt/llvm/bin/clang++
```

## 验证安装

运行以下命令确保一切正常：
```bash
./check_platform.sh && ./test_cross_platform.sh
```

如果两个脚本都成功，您就可以开始运行测试了！

## 获取帮助

```bash
# 查看 run_tests.sh 的所有选项
./run_tests.sh --help

# 查看平台检测结果
./check_platform.sh

# 查看详细的跨平台文档
cat CROSS_PLATFORM.md
```

## 反馈

遇到问题？请提供以下信息：
1. 平台: `uname -a`
2. 检测结果: `./check_platform.sh`
3. 测试结果: `./test_cross_platform.sh`

