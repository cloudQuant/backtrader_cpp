#!/bin/bash

# Backtrader C++ 修复后的构建脚本

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

# 项目根目录
PROJECT_ROOT="/home/yun/Documents/refactor_backtrader/backtrader_cpp"
BUILD_DIR="${PROJECT_ROOT}/build"

print_info "Starting Backtrader C++ build process..."
print_info "Project root: $PROJECT_ROOT"

# 检查项目目录
if [ ! -d "$PROJECT_ROOT" ]; then
    print_error "Project directory not found: $PROJECT_ROOT"
    exit 1
fi

if [ ! -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
    print_error "CMakeLists.txt not found in project root"
    exit 1
fi

cd "$PROJECT_ROOT"

# 检查依赖
print_info "Checking dependencies..."

# 检查cmake
if ! command -v cmake &> /dev/null; then
    print_error "cmake not found. Please install cmake first."
    print_info "On Ubuntu/Debian: sudo apt install cmake"
    print_info "On CentOS/RHEL: sudo yum install cmake"
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1 | awk '{print $3}')
print_success "CMake found: version $CMAKE_VERSION"

# 检查编译器
if command -v g++ &> /dev/null; then
    GCC_VERSION=$(g++ --version | head -n1)
    print_success "GCC found: $GCC_VERSION"
    COMPILER="g++"
elif command -v clang++ &> /dev/null; then
    CLANG_VERSION=$(clang++ --version | head -n1)
    print_success "Clang found: $CLANG_VERSION"
    COMPILER="clang++"
else
    print_error "No C++ compiler found. Please install g++ or clang++."
    print_info "On Ubuntu/Debian: sudo apt install build-essential"
    print_info "On CentOS/RHEL: sudo yum groupinstall 'Development Tools'"
    exit 1
fi

# 清理构建目录
print_info "Cleaning build directory..."
if [ -d "$BUILD_DIR" ]; then
    rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"

# 配置项目
print_info "Configuring project with CMake..."
cd "$BUILD_DIR"

# 基本配置
BUILD_TYPE=${BUILD_TYPE:-"Debug"}
BUILD_TESTS=${BUILD_TESTS:-"ON"}

print_info "Build configuration:"
print_info "  - BUILD_TYPE: $BUILD_TYPE"
print_info "  - BUILD_TESTS: $BUILD_TESTS"
print_info "  - COMPILER: $COMPILER"

# 运行 CMake 配置
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_TESTS="$BUILD_TESTS" \
    -DBUILD_PYTHON_BINDINGS=OFF \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_CXX_COMPILER="$COMPILER"

if [ $? -eq 0 ]; then
    print_success "CMake configuration successful"
else
    print_error "CMake configuration failed"
    exit 1
fi

# 构建核心库
print_info "Building core library..."

# 检测CPU核心数
if [[ "$OSTYPE" == "darwin"* ]]; then
    JOBS=$(sysctl -n hw.ncpu)
else
    JOBS=$(nproc 2>/dev/null || echo "4")
fi

print_info "Using $JOBS parallel jobs"

# 构建核心库
make backtrader_core -j "$JOBS"

if [ $? -eq 0 ]; then
    print_success "Core library built successfully"
else
    print_error "Core library build failed"
    exit 1
fi

# 构建测试
if [ "$BUILD_TESTS" = "ON" ]; then
    print_info "Building tests..."
    
    make simple_test -j "$JOBS"
    
    if [ $? -eq 0 ]; then
        print_success "Tests built successfully"
        
        # 运行测试
        print_info "Running simple test..."
        if [ -f "tests/simple_test" ]; then
            ./tests/simple_test
            if [ $? -eq 0 ]; then
                print_success "All tests passed!"
            else
                print_warning "Some tests failed, but build completed"
            fi
        else
            print_warning "Test executable not found"
        fi
    else
        print_warning "Test build failed, but core library is available"
    fi
fi

# 显示构建结果
print_info "Build artifacts:"
find "$BUILD_DIR" -name "*.a" -o -name "*.so" -o -name "*test*" -type f | while read file; do
    size=$(du -h "$file" | cut -f1)
    print_info "  $file ($size)"
done

print_success "Build process completed successfully!"
print_info "Core library location: $BUILD_DIR/src/libbacktrader_core.a"

# 显示下一步操作
echo
print_info "Next steps:"
print_info "1. Run tests: cd $BUILD_DIR && make test"
print_info "2. Install: cd $BUILD_DIR && make install"
print_info "3. Build documentation: cd $BUILD_DIR && make doc"