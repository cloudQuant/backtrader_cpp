#!/bin/bash

# Backtrader C++ Phase 0 构建脚本
# 用于快速构建和测试原型

set -e  # 遇到错误时退出

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印函数
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

# 检查依赖
check_dependencies() {
    print_info "Checking dependencies..."
    
    # 检查cmake
    if ! command -v cmake &> /dev/null; then
        print_error "cmake not found. Please install cmake first."
        exit 1
    fi
    
    # 检查编译器
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        print_error "No C++ compiler found. Please install g++ or clang++."
        exit 1
    fi
    
    # 检查Python
    if ! command -v python3 &> /dev/null; then
        print_error "python3 not found. Please install Python 3."
        exit 1
    fi
    
    print_success "All dependencies found"
}

# 创建构建目录
create_build_dir() {
    print_info "Creating build directory..."
    
    if [ -d "build" ]; then
        print_warning "Build directory exists, cleaning..."
        rm -rf build
    fi
    
    mkdir build
    print_success "Build directory created"
}

# 配置项目
configure_project() {
    print_info "Configuring project with CMake..."
    
    cd build
    
    # 默认构建配置
    BUILD_TYPE=${BUILD_TYPE:-"Debug"}
    BUILD_TESTS=${BUILD_TESTS:-"ON"}
    BUILD_PYTHON_BINDINGS=${BUILD_PYTHON_BINDINGS:-"ON"}
    
    print_info "Build configuration:"
    print_info "  - BUILD_TYPE: $BUILD_TYPE"
    print_info "  - BUILD_TESTS: $BUILD_TESTS"
    print_info "  - BUILD_PYTHON_BINDINGS: $BUILD_PYTHON_BINDINGS"
    
    cmake .. \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DBUILD_TESTS=$BUILD_TESTS \
        -DBUILD_PYTHON_BINDINGS=$BUILD_PYTHON_BINDINGS \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    
    if [ $? -eq 0 ]; then
        print_success "Project configured successfully"
    else
        print_error "Configuration failed"
        exit 1
    fi
    
    cd ..
}

# 编译项目
build_project() {
    print_info "Building project..."
    
    cd build
    
    # 检测CPU核心数
    if [[ "$OSTYPE" == "darwin"* ]]; then
        JOBS=$(sysctl -n hw.ncpu)
    else
        JOBS=$(nproc)
    fi
    
    print_info "Using $JOBS parallel jobs"
    
    cmake --build . --config $BUILD_TYPE -j $JOBS
    
    if [ $? -eq 0 ]; then
        print_success "Build completed successfully"
    else
        print_error "Build failed"
        exit 1
    fi
    
    cd ..
}

# 运行测试
run_tests() {
    if [ "$BUILD_TESTS" != "ON" ]; then
        print_warning "Tests not built, skipping test execution"
        return
    fi
    
    print_info "Running C++ tests..."
    
    cd build
    
    if [ -f "backtrader_tests" ]; then
        ./backtrader_tests
        if [ $? -eq 0 ]; then
            print_success "C++ tests passed"
        else
            print_error "C++ tests failed"
            cd ..
            return 1
        fi
    else
        print_warning "Test executable not found"
    fi
    
    cd ..
}

# 运行Python绑定测试
run_python_tests() {
    if [ "$BUILD_PYTHON_BINDINGS" != "ON" ]; then
        print_warning "Python bindings not built, skipping Python tests"
        return
    fi
    
    print_info "Running Python binding tests..."
    
    # 设置Python路径
    export PYTHONPATH="$PWD/build:$PYTHONPATH"
    
    # 运行Python测试
    cd python
    
    if python3 test_bindings.py; then
        print_success "Python binding tests passed"
    else
        print_error "Python binding tests failed"
        cd ..
        return 1
    fi
    
    cd ..
}

# 安装开发模式
install_dev() {
    if [ "$BUILD_PYTHON_BINDINGS" != "ON" ]; then
        print_warning "Python bindings not built, skipping installation"
        return
    fi
    
    print_info "Installing in development mode..."
    
    # 创建符号链接或复制文件
    if [ -f "build/backtrader_cpp_native.so" ] || [ -f "build/backtrader_cpp_native.dylib" ] || [ -f "build/backtrader_cpp_native.dll" ]; then
        # 复制Python模块文件
        mkdir -p python/backtrader_cpp
        cp python/__init__.py python/backtrader_cpp/
        cp python/utils.py python/backtrader_cpp/
        
        # 复制或链接C++模块
        find build -name "backtrader_cpp_native.*" -exec cp {} python/backtrader_cpp/ \;
        
        print_success "Development installation completed"
        print_info "You can now import backtrader_cpp from the python/ directory"
    else
        print_error "Python module not found in build directory"
        return 1
    fi
}

# 清理构建
clean() {
    print_info "Cleaning build artifacts..."
    
    if [ -d "build" ]; then
        rm -rf build
        print_success "Build directory removed"
    fi
    
    if [ -d "python/backtrader_cpp" ]; then
        rm -rf python/backtrader_cpp
        print_success "Python installation cleaned"
    fi
    
    # 清理其他临时文件
    find . -name "*.pyc" -delete
    find . -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
    
    print_success "Cleanup completed"
}

# 快速测试
quick_test() {
    print_info "Running quick functionality test..."
    
    export PYTHONPATH="$PWD/python:$PYTHONPATH"
    
    python3 -c "
import sys
sys.path.insert(0, 'python')

try:
    import backtrader_cpp as btcpp
    print('✓ Import successful')
    
    # 简单测试
    line = btcpp.LineRoot(10, 'test')
    line.forward(1.0)
    line.forward(2.0)
    
    assert line.len() == 2
    assert line.get(0) == 2.0
    print('✓ LineRoot works')
    
    # SMA测试
    data = [1.0, 2.0, 3.0, 4.0, 5.0]
    test_line = btcpp.create_line_from_list(data)
    sma = btcpp.SMA(test_line, 3)
    
    # 移动到有效位置
    test_line.forward()
    test_line.forward()
    sma.calculate()
    
    result = sma.get(0)
    expected = 2.0  # (1+2+3)/3
    assert abs(result - expected) < 1e-10
    print('✓ SMA calculation works')
    
    print('🎉 Quick test passed!')
    
except Exception as e:
    print(f'❌ Quick test failed: {e}')
    sys.exit(1)
"
    
    if [ $? -eq 0 ]; then
        print_success "Quick test passed"
    else
        print_error "Quick test failed"
        return 1
    fi
}

# 显示帮助
show_help() {
    echo "Backtrader C++ Phase 0 Build Script"
    echo ""
    echo "Usage: $0 [command] [options]"
    echo ""
    echo "Commands:"
    echo "  build       - Full build (configure + compile + test)"
    echo "  configure   - Configure project with CMake"
    echo "  compile     - Compile the project"
    echo "  test        - Run C++ tests"
    echo "  python-test - Run Python binding tests"
    echo "  install-dev - Install in development mode"
    echo "  quick-test  - Run a quick functionality test"
    echo "  clean       - Clean build artifacts"
    echo "  help        - Show this help"
    echo ""
    echo "Environment variables:"
    echo "  BUILD_TYPE              - Debug|Release (default: Debug)"
    echo "  BUILD_TESTS             - ON|OFF (default: ON)"
    echo "  BUILD_PYTHON_BINDINGS   - ON|OFF (default: ON)"
    echo ""
    echo "Examples:"
    echo "  $0 build                    # Full debug build with tests"
    echo "  BUILD_TYPE=Release $0 build # Release build"
    echo "  $0 clean && $0 build        # Clean build"
    echo "  $0 quick-test               # Quick functionality check"
}

# 主函数
main() {
    local command=${1:-"build"}
    
    case $command in
        "build")
            check_dependencies
            create_build_dir
            configure_project
            build_project
            run_tests
            run_python_tests
            install_dev
            quick_test
            print_success "Build completed successfully!"
            ;;
        "configure")
            check_dependencies
            create_build_dir
            configure_project
            ;;
        "compile")
            build_project
            ;;
        "test")
            run_tests
            ;;
        "python-test")
            run_python_tests
            ;;
        "install-dev")
            install_dev
            ;;
        "quick-test")
            quick_test
            ;;
        "clean")
            clean
            ;;
        "help"|"-h"|"--help")
            show_help
            ;;
        *)
            print_error "Unknown command: $command"
            show_help
            exit 1
            ;;
    esac
}

# 运行主函数
main "$@"