#!/bin/bash

# 跨平台功能快速测试脚本

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_separator() {
    echo -e "${BLUE}===============================================${NC}"
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

print_separator
print_info "Backtrader C++ 跨平台功能测试"
print_separator
echo

# 测试1: 平台检测
print_info "测试1: 平台检测"
if [[ "$OSTYPE" == "darwin"* ]]; then
    print_success "检测到 macOS 平台"
    PLATFORM="mac"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    print_success "检测到 Linux 平台"
    PLATFORM="linux"
else
    print_error "未知平台: $OSTYPE"
    exit 1
fi
echo

# 测试2: find 命令可执行文件检测
print_info "测试2: find 命令可执行文件检测"
TEMP_DIR=$(mktemp -d)
TEMP_EXEC="$TEMP_DIR/test_exec"
touch "$TEMP_EXEC"
chmod +x "$TEMP_EXEC"

if [[ "$PLATFORM" == "mac" ]]; then
    FIND_EXEC_FLAG="-perm +111"
else
    FIND_EXEC_FLAG="-executable"
fi

if find "$TEMP_DIR" -type f $FIND_EXEC_FLAG 2>/dev/null | grep -q "$TEMP_EXEC"; then
    print_success "find 命令测试通过 (使用 $FIND_EXEC_FLAG)"
else
    print_error "find 命令测试失败"
    rm -rf "$TEMP_DIR"
    exit 1
fi
rm -rf "$TEMP_DIR"
echo

# 测试3: 库路径环境变量
print_info "测试3: 库路径环境变量设置"
if [[ "$PLATFORM" == "mac" ]]; then
    # Mac: 检测库路径
    LIB_PATHS=""
    [ -d "$HOME/opt/anaconda3/lib" ] && LIB_PATHS="$HOME/opt/anaconda3/lib"
    [ -d "/opt/homebrew/lib" ] && LIB_PATHS="${LIB_PATHS:+$LIB_PATHS:}/opt/homebrew/lib"
    [ -d "/usr/local/lib" ] && LIB_PATHS="${LIB_PATHS:+$LIB_PATHS:}/usr/local/lib"
    
    if [ -n "$LIB_PATHS" ]; then
        export DYLD_LIBRARY_PATH="${LIB_PATHS}:$DYLD_LIBRARY_PATH"
        print_success "DYLD_LIBRARY_PATH 设置成功: $DYLD_LIBRARY_PATH"
    else
        print_error "未找到 macOS 库路径"
        exit 1
    fi
else
    # Linux: 检测库路径
    LIB_PATHS="/usr/lib/x86_64-linux-gnu"
    [ -d "/usr/lib" ] && LIB_PATHS="${LIB_PATHS}:/usr/lib"
    [ -d "/usr/local/lib" ] && LIB_PATHS="${LIB_PATHS}:/usr/local/lib"
    
    export LD_LIBRARY_PATH="${LIB_PATHS}:$LD_LIBRARY_PATH"
    print_success "LD_LIBRARY_PATH 设置成功: $LD_LIBRARY_PATH"
fi
echo

# 测试4: timeout 命令
print_info "测试4: timeout 命令可用性"
if [[ "$PLATFORM" == "mac" ]]; then
    if command -v gtimeout &> /dev/null; then
        print_success "gtimeout 命令可用 (macOS)"
        TIMEOUT_CMD="gtimeout"
    else
        print_error "gtimeout 未安装，建议运行: brew install coreutils"
        TIMEOUT_CMD=""
    fi
else
    if command -v timeout &> /dev/null; then
        print_success "timeout 命令可用 (Linux)"
        TIMEOUT_CMD="timeout"
    else
        print_error "timeout 未安装"
        exit 1
    fi
fi
echo

# 测试5: CMake 配置
print_info "测试5: CMake 配置测试"
cd "$SCRIPT_DIR"

# 清理旧的缓存
rm -f CMakeCache.txt cmake_install.cmake Makefile
rm -rf CMakeFiles

# 测试 CMake 配置
if cmake . -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=OFF > /tmp/cmake_config.log 2>&1; then
    print_success "CMake 配置成功"
    
    # 检查配置输出
    if grep -q "Platform:" /tmp/cmake_config.log; then
        DETECTED_PLATFORM=$(grep "Platform:" /tmp/cmake_config.log | head -1)
        print_info "检测到平台: $DETECTED_PLATFORM"
    fi
    
    if grep -q "Compiler:" /tmp/cmake_config.log; then
        DETECTED_COMPILER=$(grep "Compiler:" /tmp/cmake_config.log | head -1)
        print_info "检测到编译器: $DETECTED_COMPILER"
    fi
else
    print_error "CMake 配置失败"
    cat /tmp/cmake_config.log
    exit 1
fi
echo

# 测试6: 编译一个源文件
print_info "测试6: 编译测试 (编译一个核心源文件)"
if cmake --build . --config Debug --parallel 1 --target backtrader_core > /tmp/cmake_build.log 2>&1; then
    print_success "编译测试成功"
    
    # 检查库文件是否生成
    if [ -f "libbacktrader_core.a" ]; then
        FILESIZE=$(ls -lh libbacktrader_core.a | awk '{print $5}')
        print_success "核心库已生成: libbacktrader_core.a ($FILESIZE)"
    else
        print_error "核心库未生成"
        exit 1
    fi
else
    print_error "编译测试失败"
    cat /tmp/cmake_build.log
    exit 1
fi
echo

# 清理
print_info "清理测试文件..."
rm -f /tmp/cmake_config.log /tmp/cmake_build.log
print_success "清理完成"
echo

# 总结
print_separator
print_success "所有跨平台功能测试通过！"
print_separator
echo
print_info "平台总结:"
print_info "  操作系统: $PLATFORM"
print_info "  find 标志: $FIND_EXEC_FLAG"
if [[ "$PLATFORM" == "mac" ]]; then
    print_info "  库路径变量: DYLD_LIBRARY_PATH"
    print_info "  超时命令: ${TIMEOUT_CMD:-无}"
else
    print_info "  库路径变量: LD_LIBRARY_PATH"
    print_info "  超时命令: $TIMEOUT_CMD"
fi
echo
print_info "下一步:"
print_info "  1. 运行完整测试: ./run_tests.sh --cmake-only"
print_info "  2. 查看跨平台文档: cat CROSS_PLATFORM.md"
print_info "  3. 在另一个平台上测试以验证跨平台兼容性"
echo

