#!/bin/bash

# 平台检测和兼容性检查脚本

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_separator() {
    echo -e "${BLUE}===============================================${NC}"
}

# 检测操作系统
print_separator
print_info "检测操作系统..."
print_separator

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS_NAME="Linux"
    print_success "检测到 Linux 系统"
    
    # 检测发行版
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        print_info "发行版: $NAME $VERSION"
    fi
    
    # 检测架构
    print_info "架构: $(uname -m)"
    print_info "内核: $(uname -r)"
    
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS_NAME="macOS"
    print_success "检测到 macOS 系统"
    print_info "版本: $(sw_vers -productVersion)"
    print_info "架构: $(uname -m)"
    
else
    OS_NAME="Unknown"
    print_error "未知的操作系统: $OSTYPE"
    exit 1
fi

echo

# 检查必需工具
print_separator
print_info "检查必需工具..."
print_separator

check_tool() {
    local tool=$1
    local optional=$2
    
    if command -v $tool &> /dev/null; then
        local version=$($tool --version 2>&1 | head -n1)
        print_success "$tool: $version"
        return 0
    else
        if [ "$optional" = "optional" ]; then
            print_warning "$tool: 未安装（可选）"
        else
            print_error "$tool: 未安装（必需）"
        fi
        return 1
    fi
}

# 检查必需工具
MISSING_REQUIRED=0

check_tool "cmake" || MISSING_REQUIRED=1
check_tool "make" || MISSING_REQUIRED=1

# 检查C++编译器
if check_tool "g++"; then
    :
elif check_tool "clang++"; then
    :
else
    print_error "未找到C++编译器 (g++ 或 clang++)"
    MISSING_REQUIRED=1
fi

# 检查可选工具
print_info ""
print_info "检查可选工具..."

if [[ "$OS_NAME" == "macOS" ]]; then
    check_tool "gtimeout" "optional"
    if ! command -v gtimeout &> /dev/null; then
        print_warning "建议安装 coreutils: brew install coreutils"
    fi
else
    check_tool "timeout" "optional"
fi

check_tool "pkg-config" "optional"
check_tool "bc" "optional"

echo

# 检查Google Test
print_separator
print_info "检查 Google Test..."
print_separator

GTEST_FOUND=0

if pkg-config --exists gtest 2>/dev/null; then
    GTEST_VERSION=$(pkg-config --modversion gtest 2>/dev/null)
    print_success "通过 pkg-config 找到 Google Test: $GTEST_VERSION"
    GTEST_FOUND=1
fi

# 手动查找 gtest
if [ $GTEST_FOUND -eq 0 ]; then
    for include_path in /usr/include /usr/local/include /opt/homebrew/include; do
        if [ -f "$include_path/gtest/gtest.h" ]; then
            print_success "找到 Google Test 头文件: $include_path/gtest/gtest.h"
            GTEST_FOUND=1
            break
        fi
    done
fi

if [ $GTEST_FOUND -eq 0 ]; then
    print_error "未找到 Google Test"
    if [[ "$OS_NAME" == "Linux" ]]; then
        print_info "安装命令: sudo apt-get install libgtest-dev"
    elif [[ "$OS_NAME" == "macOS" ]]; then
        print_info "安装命令: brew install googletest"
    fi
    MISSING_REQUIRED=1
fi

echo

# 测试 find 命令的可执行文件检测
print_separator
print_info "测试 find 命令的可执行文件检测..."
print_separator

# 创建临时测试文件
TEMP_DIR=$(mktemp -d)
TEMP_EXEC="$TEMP_DIR/test_executable"
touch "$TEMP_EXEC"
chmod +x "$TEMP_EXEC"

if [[ "$OS_NAME" == "macOS" ]]; then
    FIND_EXEC_FLAG="-perm +111"
    print_info "使用 macOS 风格的 find: -perm +111"
else
    FIND_EXEC_FLAG="-executable"
    print_info "使用 Linux 风格的 find: -executable"
fi

if find "$TEMP_DIR" -type f $FIND_EXEC_FLAG 2>/dev/null | grep -q "$TEMP_EXEC"; then
    print_success "find 命令测试成功"
else
    print_error "find 命令测试失败"
fi

# 清理临时文件
rm -rf "$TEMP_DIR"

echo

# 测试库路径环境变量
print_separator
print_info "检查库路径..."
print_separator

if [[ "$OS_NAME" == "macOS" ]]; then
    print_info "使用 DYLD_LIBRARY_PATH (macOS)"
    
    # 检测常用库路径
    FOUND_PATHS=()
    for lib_path in "$HOME/opt/anaconda3/lib" "/opt/homebrew/lib" "/usr/local/lib"; do
        if [ -d "$lib_path" ]; then
            FOUND_PATHS+=("$lib_path")
            print_success "找到库路径: $lib_path"
        fi
    done
    
    if [ ${#FOUND_PATHS[@]} -eq 0 ]; then
        print_warning "未找到标准库路径"
    fi
    
else
    print_info "使用 LD_LIBRARY_PATH (Linux)"
    
    # 检测常用库路径
    FOUND_PATHS=()
    for lib_path in "/usr/lib/x86_64-linux-gnu" "/usr/lib" "/usr/local/lib"; do
        if [ -d "$lib_path" ]; then
            FOUND_PATHS+=("$lib_path")
            print_success "找到库路径: $lib_path"
        fi
    done
    
    if [ ${#FOUND_PATHS[@]} -eq 0 ]; then
        print_error "未找到标准库路径"
    fi
fi

echo

# 总结
print_separator
print_info "平台兼容性检查总结"
print_separator

print_info "操作系统: $OS_NAME"
print_info "find 标志: $FIND_EXEC_FLAG"

if [[ "$OS_NAME" == "macOS" ]]; then
    print_info "库路径变量: DYLD_LIBRARY_PATH"
else
    print_info "库路径变量: LD_LIBRARY_PATH"
fi

echo

if [ $MISSING_REQUIRED -eq 0 ]; then
    print_success "所有必需工具都已安装"
    print_success "系统已准备好编译和运行测试"
    echo
    print_info "可以运行以下命令开始测试:"
    print_info "  cd backtrader_cpp"
    print_info "  ./run_tests.sh --cmake-only"
    exit 0
else
    print_error "缺少必需的工具"
    print_info "请根据上述错误信息安装缺失的工具"
    exit 1
fi

