#!/bin/bash

# Backtrader C++ 测试运行脚本
# 该脚本会编译和运行 backtrader_cpp/tests/original_tests 目录下的所有测试文件
# 支持独立编译各个测试文件，记录编译成功/失败的情况
# 支持运行测试用例，记录运行成功/失败的情况
# 支持超时保护，防止编译或测试过程卡住（默认10秒超时）
# 优化版本：使用智能源文件选择，避免编译所有156个源文件
# 推荐使用 --cmake-only 选项获得最佳性能和稳定性

set -e  # 遇到错误时不退出，而是记录错误继续执行

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 全局变量
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR/tests"
BUILD_DIR="$SCRIPT_DIR/build_tests"
INCLUDE_DIR="$SCRIPT_DIR/include"
SRC_DIR="$SCRIPT_DIR/src"

# 结果统计
COMPILED_SUCCESS=()
COMPILED_FAILED=()
COMPILE_ERRORS=()
TEST_SUCCESS=()
TEST_FAILED=()
TEST_ERRORS=()
TEST_DURATIONS=()
TEST_DETAILS=()
TEST_CASE_STATS=()

# 详细的测试用例统计
TEST_CASES_PASSED=()
TEST_CASES_FAILED=()
TEST_CASES_SKIPPED=()
TEST_CASES_DETAILS=()
TEST_SUITES_INFO=()

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

print_separator() {
    echo -e "${BLUE}===============================================${NC}"
}

# 检查依赖
check_dependencies() {
    print_info "检查依赖..."
    
    # 检查 CMake
    if ! command -v cmake &> /dev/null; then
        print_error "未找到 cmake。请先安装 cmake。"
        exit 1
    fi
    
    # 检查 C++ 编译器
    if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
        print_error "未找到 C++ 编译器。请安装 g++ 或 clang++。"
        exit 1
    fi
    
    # 检查 Google Test
    if ! pkg-config --exists gtest 2>/dev/null; then
        print_warning "未找到 Google Test。将尝试使用 FetchContent 自动获取。"
    fi
    
    print_success "依赖检查完成"
}

# 创建构建目录
create_build_dir() {
    print_info "创建构建目录..."
    
    if [ -d "$BUILD_DIR" ]; then
        print_warning "构建目录已存在，正在清理..."
        rm -rf "$BUILD_DIR"
    fi
    
    mkdir -p "$BUILD_DIR"
    print_success "构建目录创建完成: $BUILD_DIR"
}

# 获取所有测试文件
get_test_files() {
    local test_files=()
    
    if [ -d "$TEST_DIR" ]; then
        # 获取所有 .cpp 文件，排除 main_original_tests.cpp 和 test_common.h
        while IFS= read -r -d '' file; do
            local filename=$(basename "$file")
            if [[ "$filename" != "main_original_tests.cpp" && "$filename" != "test_common.h" ]]; then
                test_files+=("$file")
            fi
        done < <(find "$TEST_DIR" -name "*.cpp" -print0)
    fi
    
    echo "${test_files[@]}"
}

# 使用 CMake 构建所有测试
build_with_cmake() {
    print_info "使用 CMake 构建测试..."
    
    # 首先在主目录构建核心库
    cd "$SCRIPT_DIR"
    
    # 清除可能存在的旧的CMake缓存文件（避免跨平台路径冲突）
    print_info "清除旧的CMake缓存文件..."
    rm -f CMakeCache.txt cmake_install.cmake Makefile
    rm -rf CMakeFiles
    
    # 配置项目 (带超时)
    local cmake_config_output
    # 首先构建所有backtrader_cpp源文件
    print_info "配置backtrader_cpp项目（使用8核并行编译）..."
    if cmake . -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=OFF; then
        print_success "项目配置成功"
    else
        print_error "项目配置失败"
        cd "$SCRIPT_DIR"
        return 1
    fi
    
    print_info "编译所有backtrader_cpp源文件（8核并行）..."
    if cmake --build . --config Debug --parallel 8; then
        print_success "backtrader_cpp编译成功"
    else
        print_error "backtrader_cpp编译失败"
        cd "$SCRIPT_DIR"
        return 1
    fi
    
    # 然后构建所有测试文件（使用tests目录的构建目录）
    local test_build_dir="$TEST_DIR/build"
    if [ -d "$test_build_dir" ]; then
        print_info "清理旧的测试构建目录..."
        rm -rf "$test_build_dir"
    fi
    mkdir -p "$test_build_dir"
    
    # 同时清除tests目录中的CMake缓存文件
    cd "$TEST_DIR"
    print_info "清除tests目录的CMake缓存文件..."
    rm -f cmake_install.cmake CTestTestfile.cmake Makefile
    
    cd "$test_build_dir"
    
    print_info "配置测试文件（8核并行编译）..."
    # 确保核心库存在于主目录
    local core_lib_path="$SCRIPT_DIR/libbacktrader_core.a"
    if [ ! -f "$core_lib_path" ]; then
        print_error "核心库不存在: $core_lib_path"
        cd "$SCRIPT_DIR"
        return 1
    fi
    
    if cmake_config_output=$(run_with_timeout 60 "CMake 配置测试" "cmake .. -DCMAKE_BUILD_TYPE=Debug"); then
        print_success "测试配置成功"
    else
        local exit_code=$?
        if [ $exit_code -eq 124 ]; then
            print_error "测试配置超时 (60秒)"
        else
            print_error "测试配置失败"
            echo "$cmake_config_output"
        fi
        cd "$SCRIPT_DIR"
        return 1
    fi
    
    # 编译所有测试文件 (8核并行，继续模式)
    print_info "编译所有测试文件（8核并行，失败不停止）..."
    local cmake_build_output
    if cmake_build_output=$(run_with_timeout 300 "CMake 编译测试" "cmake --build . --config Debug --parallel 8"); then
        print_success "测试编译完成"
    else
        local exit_code=$?
        if [ $exit_code -eq 124 ]; then
            print_warning "测试编译超时 (300秒)，但可能有部分文件编译成功"
        else
            print_warning "测试编译过程中有失败，但已继续编译其他文件"
        fi
    fi
    
    # 收集编译结果
    collect_cmake_test_results "$test_build_dir" "$cmake_build_output"
    
    cd "$SCRIPT_DIR"
    return 0
}

# 收集CMake测试编译结果
collect_cmake_test_results() {
    local test_build_dir="$1"
    local build_output="$2"
    
    print_info "收集测试编译结果..."
    
    # 清空之前的结果
    COMPILED_SUCCESS=()
    COMPILED_FAILED=()
    COMPILE_ERRORS=()
    
    # 查找所有可执行的测试文件
    local success_count=0
    local failed_count=0
    
    # 检查实际的编译输出目录 - 同时检查三个可能的位置
    # 跨平台兼容：Mac使用 -perm +111，Linux使用 -executable
    local actual_test_dir=""
    local find_exec_flag="-executable"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        find_exec_flag="-perm +111"
    fi
    
    if [ -d "$test_build_dir" ] && [ "$(find "$test_build_dir" -maxdepth 1 -name "test_*" -type f $find_exec_flag 2>/dev/null | wc -l)" -gt 0 ]; then
        actual_test_dir="$test_build_dir"
        print_info "在 $test_build_dir 目录中找到编译的测试文件"
    elif [ -d "$TEST_DIR/build" ] && [ "$(find "$TEST_DIR/build" -maxdepth 1 -name "test_*" -type f $find_exec_flag 2>/dev/null | wc -l)" -gt 0 ]; then
        actual_test_dir="$TEST_DIR/build"
        print_info "在 $TEST_DIR/build 目录中找到编译的测试文件"
    elif [ -d "$TEST_DIR" ] && [ "$(find "$TEST_DIR" -maxdepth 1 -name "test_*" -type f $find_exec_flag 2>/dev/null | wc -l)" -gt 0 ]; then
        actual_test_dir="$TEST_DIR"
        print_info "在 $TEST_DIR 目录中找到编译的测试文件"
    else
        print_warning "未找到编译的测试文件"
    fi
    
    # 检查每个测试的编译状态
    for test_file in "$TEST_DIR/original_tests"/test_*.cpp; do
        local filename=$(basename "$test_file" .cpp)
        
        # 跳过test_common
        if [ "$filename" = "test_common" ]; then
            continue
        fi
        
        # 检查可执行文件是否存在 - 跨平台兼容
        local found=false
        if [ -n "$actual_test_dir" ]; then
            if find "$actual_test_dir" -maxdepth 1 -name "$filename" -type f $find_exec_flag 2>/dev/null | grep -q .; then
                found=true
            fi
        fi
        
        if [ "$found" = true ]; then
            COMPILED_SUCCESS+=("$filename")
            success_count=$((success_count + 1))
        else
            COMPILED_FAILED+=("$filename")
            failed_count=$((failed_count + 1))
            
            # 从构建输出中提取编译错误信息
            local error_msg=""
            if [ -n "$build_output" ]; then
                # 提取与该测试文件相关的错误信息
                error_msg=$(echo "$build_output" | grep -A 10 -B 2 "$filename\.cpp" | head -15 | tr '\n' ' ' | sed 's/^[[:space:]]*//g')
                if [ -z "$error_msg" ]; then
                    error_msg="编译失败，未在构建输出中找到具体错误信息"
                fi
            else
                error_msg="编译失败，未找到构建输出"
            fi
            COMPILE_ERRORS+=("$filename: $error_msg")
        fi
    done
    
    print_separator
    print_info "CMake测试编译结果统计:"
    print_success "成功编译: $success_count 个测试"
    print_error "编译失败: $failed_count 个测试"
    
    if [ $success_count -gt 0 ]; then
        print_info "成功编译的测试："
        for test in "${COMPILED_SUCCESS[@]}"; do
            echo "  ✓ $test"
        done
    fi
    
    if [ $failed_count -gt 0 ]; then
        print_info "编译失败的测试："
        for test in "${COMPILED_FAILED[@]}"; do
            echo "  ✗ $test"
        done
    fi
    
    # 保存详细的编译错误信息到文件
    save_cmake_compilation_errors "$test_build_dir" "$build_output"
}

# 保存CMake编译错误信息到文件
save_cmake_compilation_errors() {
    local test_build_dir="$1"
    local build_output="$2"
    local error_log="$BUILD_DIR/cmake_compilation_errors.log"
    
    print_info "保存编译错误信息到: $error_log"
    
    {
        echo "========================================"
        echo "CMake 测试编译错误报告"
        echo "生成时间: $(date)"
        echo "构建目录: $test_build_dir"
        echo "========================================"
        echo
        
        echo "编译成功的测试 (${#COMPILED_SUCCESS[@]}个):"
        for test in "${COMPILED_SUCCESS[@]}"; do
            echo "  ✓ $test"
        done
        echo
        
        echo "编译失败的测试 (${#COMPILED_FAILED[@]}个):"
        for test in "${COMPILED_FAILED[@]}"; do
            echo "  ✗ $test"
        done
        echo
        
        echo "========================================"
        echo "详细错误信息:"
        echo "========================================"
        
        # 显示完整的构建输出（前100行）
        if [ -n "$build_output" ]; then
            echo "构建输出摘要:"
            echo "$build_output" | head -100
            echo
        fi
        
        # 查找并提取每个失败测试的详细错误信息
        for test in "${COMPILED_FAILED[@]}"; do
            echo
            echo "--- $test ---"
            
            # 从构建输出中提取特定于该测试的错误
            if [ -n "$build_output" ]; then
                echo "该测试的编译错误:"
                echo "$build_output" | grep -A 15 -B 5 "$test\.cpp" | head -25 || echo "未在构建输出中找到该测试的具体错误"
            else
                echo "未找到构建输出"
            fi
            
            # 查看是否有对象文件生成失败
            local obj_file="$test_build_dir/CMakeFiles/${test}.dir/${test}.cpp.o"
            if [ ! -f "$obj_file" ]; then
                echo "对象文件未生成: $obj_file"
            fi
            
            echo
        done
        
        echo "========================================"
        echo "调试建议:"
        echo "1. 手动编译单个测试文件进行调试"
        echo "2. 检查 test_common.h 中的API一致性问题"
        echo "3. 确认所有必要的头文件和库已正确包含"
        echo "4. 运行 'make VERBOSE=1' 查看详细编译命令"
        echo "========================================"
        
    } > "$error_log"
    
    print_success "编译错误信息已保存到: $error_log"
}

# 带超时的命令执行函数
run_with_timeout() {
    local timeout_seconds="$1"
    local description="$2"
    shift 2
    local cmd="$@"
    
    # 创建临时文件来存储输出
    local temp_output=$(mktemp)
    local temp_status=$(mktemp)
    
    # 在后台运行命令，设置正确的库路径（兼容Linux和Mac）
    (
        # Linux使用LD_LIBRARY_PATH，Mac使用DYLD_LIBRARY_PATH
        if [[ "$OSTYPE" == "darwin"* ]]; then
            # Mac: 检测并添加常用库路径
            local lib_paths=""
            [ -d "$HOME/opt/anaconda3/lib" ] && lib_paths="$HOME/opt/anaconda3/lib"
            [ -d "/opt/homebrew/lib" ] && lib_paths="${lib_paths:+$lib_paths:}/opt/homebrew/lib"
            [ -d "/usr/local/lib" ] && lib_paths="${lib_paths:+$lib_paths:}/usr/local/lib"
            export DYLD_LIBRARY_PATH="${lib_paths}:$DYLD_LIBRARY_PATH"
        else
            # Linux: 检测并添加常用库路径
            local lib_paths="/usr/lib/x86_64-linux-gnu"
            [ -d "/usr/lib" ] && lib_paths="${lib_paths}:/usr/lib"
            [ -d "/usr/local/lib" ] && lib_paths="${lib_paths}:/usr/local/lib"
            export LD_LIBRARY_PATH="${lib_paths}:$LD_LIBRARY_PATH"
        fi
        eval "$cmd" > "$temp_output" 2>&1
        echo $? > "$temp_status"
    ) &
    
    local bg_pid=$!
    
    # 等待命令完成或超时
    local count=0
    while [ $count -lt $timeout_seconds ]; do
        if ! kill -0 $bg_pid 2>/dev/null; then
            # 进程已完成
            wait $bg_pid 2>/dev/null
            local exit_code=$(cat "$temp_status" 2>/dev/null || echo "1")
            local output=$(cat "$temp_output" 2>/dev/null || echo "")
            
            rm -f "$temp_output" "$temp_status"
            
            if [ "$exit_code" -eq 0 ]; then
                echo "$output"
                return 0
            else
                echo "$output"
                return 1
            fi
        fi
        sleep 1
        count=$((count + 1))
    done
    
    # 超时，终止进程
    kill -TERM $bg_pid 2>/dev/null
    sleep 1
    kill -KILL $bg_pid 2>/dev/null
    wait $bg_pid 2>/dev/null
    
    rm -f "$temp_output" "$temp_status"
    
    print_error "$description 超时 (${timeout_seconds}s)"
    return 124  # 返回超时错误码
}

# 构建核心库（类似CMake方式）
build_core_library() {
    local core_lib="$BUILD_DIR/libbacktrader_core.a"
    
    print_info "构建核心库: $core_lib"
    
    # 检查是否已存在
    if [ -f "$core_lib" ]; then
        print_info "核心库已存在，跳过构建"
        return 0
    fi
    
    # 获取源文件
    local minimal_sources=(
        "$SRC_DIR/lineroot.cpp"
        "$SRC_DIR/linebuffer.cpp" 
        "$SRC_DIR/lineseries.cpp"
        "$SRC_DIR/lineiterator.cpp"
        "$SRC_DIR/dataseries.cpp"
        "$SRC_DIR/indicator.cpp"
        "$SRC_DIR/metabase.cpp"
        "$SRC_DIR/errors.cpp"
        "$SRC_DIR/mathsupport.cpp"
        "$SRC_DIR/version.cpp"
    )
    
    local indicator_sources=(
        "$SRC_DIR/indicators/sma.cpp"
        "$SRC_DIR/indicators/ema.cpp"
        "$SRC_DIR/indicators/wma.cpp"
        "$SRC_DIR/indicators/basicops.cpp"
        "$SRC_DIR/indicators/dm.cpp"
        "$SRC_DIR/indicators/williamsad.cpp"
        "$SRC_DIR/indicators/envelope.cpp"
        "$SRC_DIR/indicators/lowest.cpp"
        "$SRC_DIR/indicators/zlema.cpp"
        "$SRC_DIR/indicators/zlind.cpp"
        "$SRC_DIR/indicators/ichimoku.cpp"
        "$SRC_DIR/indicators/rsi.cpp"
        "$SRC_DIR/indicators/macd.cpp"
        "$SRC_DIR/indicators/bollinger.cpp"
        "$SRC_DIR/indicators/atr.cpp"
        "$SRC_DIR/indicators/dema.cpp"
        "$SRC_DIR/indicators/momentum.cpp"
        "$SRC_DIR/indicators/cci.cpp"
        "$SRC_DIR/indicators/hma.cpp"
        "$SRC_DIR/indicators/dpo.cpp"
        "$SRC_DIR/indicators/dv2.cpp"
        "$SRC_DIR/indicators/percentrank.cpp"
        "$SRC_DIR/indicators/percentchange.cpp"
        "$SRC_DIR/indicators/smma.cpp"
        "$SRC_DIR/indicators/upmove.cpp"
        "$SRC_DIR/indicators/ultimateoscillator.cpp"
        "$SRC_DIR/indicators/trix.cpp"
        "$SRC_DIR/indicators/oscillator.cpp"
    )
    
    # 检查文件是否存在
    local existing_sources=()
    for src_file in "${minimal_sources[@]}" "${indicator_sources[@]}"; do
        if [ -f "$src_file" ]; then
            existing_sources+=("$src_file")
        fi
    done
    
    print_info "编译核心库 (${#existing_sources[@]} 个源文件)..."
    
    # 编译为对象文件
    local obj_files=()
    local obj_dir="$BUILD_DIR/obj"
    mkdir -p "$obj_dir"
    
    for src_file in "${existing_sources[@]}"; do
        local filename=$(basename "$src_file" .cpp)
        local obj_file="$obj_dir/${filename}.o"
        obj_files+=("$obj_file")
        
        print_info "编译: $filename.cpp -> $filename.o"
        if ! g++ -std=c++20 -I"$INCLUDE_DIR" -O2 -c "$src_file" -o "$obj_file"; then
            print_error "编译核心库失败: $filename.cpp"
            return 1
        fi
    done
    
    # 创建静态库
    print_info "创建静态库..."
    if ar rcs "$core_lib" "${obj_files[@]}"; then
        print_success "核心库构建成功: $core_lib"
        ls -lh "$core_lib"
        return 0
    else
        print_error "创建静态库失败"
        return 1
    fi
}

# 独立编译单个测试文件 - 使用预建核心库
compile_single_test() {
    local test_file="$1"
    local timeout_seconds="$2"
    local verbose_mode="$3"
    local filename=$(basename "$test_file" .cpp)
    local output_file="$BUILD_DIR/$filename"
    local core_lib="$BUILD_DIR/libbacktrader_core.a"
    
    if [ "$verbose_mode" = true ]; then
        print_info "编译测试文件: $filename (使用预建核心库)"
    else
        print_info "编译测试文件: $filename"
    fi
    
    # 检查核心库是否存在
    if [ ! -f "$core_lib" ]; then
        print_error "核心库不存在: $core_lib"
        return 1
    fi
    
    # 使用预建核心库的快速编译
    local compile_cmd="g++ -std=c++20 -I$INCLUDE_DIR -I$TEST_DIR -O0 -g"
    
    # 添加 Google Test 编译标志
    if pkg-config --exists gtest; then
        compile_cmd="$compile_cmd $(pkg-config --cflags gtest)"
    fi
    
    # 只编译测试文件并链接核心库
    compile_cmd="$compile_cmd $test_file $core_lib -o $output_file"
    
    # 添加 Google Test 链接库
    if pkg-config --exists gtest; then
        compile_cmd="$compile_cmd $(pkg-config --libs gtest) -lgtest_main -pthread"
    else
        compile_cmd="$compile_cmd -lgtest -lgtest_main -pthread"
    fi
    
    # 执行编译（带超时）
    local compile_output
    if [ "$verbose_mode" = true ]; then
        print_info "执行编译命令: $compile_cmd"
    fi
    
    if compile_output=$(run_with_timeout $timeout_seconds "编译 $filename" "$compile_cmd"); then
        COMPILED_SUCCESS+=("$filename")
        print_success "编译成功: $filename"
        if [ "$verbose_mode" = true ] && [ -n "$compile_output" ]; then
            echo "编译输出: $compile_output"
        fi
        return 0
    else
        local exit_code=$?
        COMPILED_FAILED+=("$filename")
        if [ $exit_code -eq 124 ]; then
            COMPILE_ERRORS+=("$filename: 编译超时 (${timeout_seconds}秒)")
            print_error "编译超时: $filename"
        else
            COMPILE_ERRORS+=("$filename: $compile_output")
            print_error "编译失败: $filename"
            if [ "$verbose_mode" = true ]; then
                echo "错误详情: $compile_output"
            fi
        fi
        return 1
    fi
}

# 并行编译单个测试文件 - 用于并行模式
compile_single_test_parallel() {
    local test_file="$1"
    local timeout_seconds="$2"
    local verbose_mode="$3"
    local filename=$(basename "$test_file" .cpp)
    local output_file="$BUILD_DIR/$filename"
    local core_lib="$BUILD_DIR/libbacktrader_core.a"
    local result_file="$BUILD_DIR/compile_result_$filename.txt"
    
    # 检查核心库是否存在
    if [ ! -f "$core_lib" ]; then
        echo "ERROR: 核心库不存在: $core_lib" > "$result_file"
        return 1
    fi
    
    # 使用预建核心库的快速编译
    local compile_cmd="g++ -std=c++20 -I$INCLUDE_DIR -I$TEST_DIR -O0 -g"
    
    # 添加 Google Test 编译标志
    if pkg-config --exists gtest; then
        compile_cmd="$compile_cmd $(pkg-config --cflags gtest)"
    fi
    
    # 只编译测试文件并链接核心库
    compile_cmd="$compile_cmd $test_file $core_lib -o $output_file"
    
    # 添加 Google Test 链接库
    if pkg-config --exists gtest; then
        compile_cmd="$compile_cmd $(pkg-config --libs gtest) -lgtest_main -pthread"
    else
        compile_cmd="$compile_cmd -lgtest -lgtest_main -pthread"
    fi
    
    # 执行编译（带超时）
    local compile_output
    if compile_output=$(run_with_timeout $timeout_seconds "编译 $filename" "$compile_cmd"); then
        echo "SUCCESS: $filename" > "$result_file"
        if [ "$verbose_mode" = true ] && [ -n "$compile_output" ]; then
            echo "COMPILE_OUTPUT: $compile_output" >> "$result_file"
        fi
        return 0
    else
        local exit_code=$?
        if [ $exit_code -eq 124 ]; then
            echo "TIMEOUT: $filename - 编译超时 (${timeout_seconds}秒)" > "$result_file"
        else
            echo "FAILED: $filename" > "$result_file"
            echo "ERROR_OUTPUT: $compile_output" >> "$result_file"
        fi
        return 1
    fi
}

# 运行单个测试
run_single_test() {
    local test_file="$1"
    local timeout_seconds="$2"
    local verbose_mode="$3"
    local filename=$(basename "$test_file" .cpp)
    local executable="$BUILD_DIR/$filename"
    
    if [ ! -f "$executable" ]; then
        print_warning "测试可执行文件不存在: $filename"
        return 1
    fi
    
    print_info "运行测试: $filename"
    
    # 执行测试（带超时）
    local test_output
    local start_time=$(date +%s.%N)
    
    # 同时生成XML和获取标准输出
    if test_output=$(run_with_timeout $timeout_seconds "运行测试 $filename" "$executable --gtest_output=xml:$BUILD_DIR/test_${filename}.xml"); then
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "0")
        
        TEST_SUCCESS+=("$filename")
        TEST_DURATIONS+=("$filename:$duration")
        print_success "测试通过: $filename (耗时: ${duration}s)"
        
        if [ "$verbose_mode" = true ]; then
            echo "测试输出: $test_output"
        fi
        
        # 优先解析XML，回退到标准输出
        if [ -f "$BUILD_DIR/test_${filename}.xml" ]; then
            parse_gtest_xml "$BUILD_DIR/test_${filename}.xml" "$filename"
        else
            parse_gtest_output "$test_output" "$filename"
        fi
        
        return 0
    else
        local exit_code=$?
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "0")
        
        TEST_FAILED+=("$filename")
        TEST_DURATIONS+=("$filename:$duration")
        
        if [ $exit_code -eq 124 ]; then
            TEST_ERRORS+=("$filename: 测试运行超时 (${timeout_seconds}秒)")
            print_error "测试超时: $filename (耗时: ${duration}s)"
        else
            TEST_ERRORS+=("$filename: $test_output")
            print_error "测试失败: $filename (耗时: ${duration}s)"
            
            if [ "$verbose_mode" = true ]; then
                echo "错误输出: $test_output"
            fi
            
            # 尝试解析失败测试的输出以获取详细信息
            if [ -f "$BUILD_DIR/test_${filename}.xml" ]; then
                parse_gtest_xml "$BUILD_DIR/test_${filename}.xml" "$filename"
            else
                parse_gtest_output "$test_output" "$filename"
            fi
        fi
        
        # 保存失败的测试详细信息
        save_test_failure_details "$filename" "$test_output" "$duration"
        return 1
    fi
}

# 使用 CTest 运行测试
run_with_gtest() {
    print_info "直接运行Google Test测试..."
    
    # 跨平台兼容：Mac使用 -perm +111，Linux使用 -executable
    local find_exec_flag="-executable"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        find_exec_flag="-perm +111"
    fi
    
    # 检查实际的测试目录
    local test_run_dir=""
    if [ -d "$TEST_DIR/build" ] && [ "$(find "$TEST_DIR/build" -maxdepth 1 -name "test_*" -type f $find_exec_flag 2>/dev/null | wc -l)" -gt 0 ]; then
        test_run_dir="$TEST_DIR/build"
        print_info "使用 tests/build 目录运行测试"
    elif [ -d "$TEST_DIR" ] && [ "$(find "$TEST_DIR" -maxdepth 1 -name "test_*" -type f $find_exec_flag 2>/dev/null | wc -l)" -gt 0 ]; then
        test_run_dir="$TEST_DIR"
        print_info "使用 tests 目录运行测试"
    elif [ -d "$BUILD_DIR/tests" ] && [ "$(find "$BUILD_DIR/tests" -maxdepth 1 -name "test_*" -type f $find_exec_flag 2>/dev/null | wc -l)" -gt 0 ]; then
        test_run_dir="$BUILD_DIR/tests"
        print_info "使用 build_tests/tests 目录运行测试"
    elif [ -d "$BUILD_DIR" ] && [ "$(find "$BUILD_DIR" -maxdepth 1 -name "test_*" -type f $find_exec_flag 2>/dev/null | wc -l)" -gt 0 ]; then
        test_run_dir="$BUILD_DIR"
        print_info "使用 build_tests 目录运行测试"
    else  
        print_error "未找到编译的测试文件"
        return 1
    fi
    
    cd "$test_run_dir"
    
    # 清空测试结果数组
    TEST_SUCCESS=()
    TEST_FAILED=()
    TEST_ERRORS=()
    TEST_DURATIONS=()
    TEST_CASES_PASSED=()
    TEST_CASES_FAILED=()
    TEST_CASES_SKIPPED=()
    TEST_CASES_DETAILS=()
    TEST_SUITES_INFO=()
    
    # 找到所有测试可执行文件
    local test_executables=()
    for exe in test_*; do
        if [ -x "$exe" ] && [ -f "$exe" ]; then
            test_executables+=("$exe")
        fi
    done
    
    if [ ${#test_executables[@]} -eq 0 ]; then
        print_error "未找到可执行的测试文件"
        cd "$SCRIPT_DIR"
        return 1
    fi
    
    print_info "找到 ${#test_executables[@]} 个测试可执行文件"
    
    # 运行每个测试
    local count=0
    for exe in "${test_executables[@]}"; do
        count=$((count + 1))
        # 运行测试进度: $count/${#test_executables[@]} - $exe
        
        run_single_gtest "$exe"
    done
    
    cd "$SCRIPT_DIR"
    
    # 汇总统计
    local total_tests=$((${#TEST_SUCCESS[@]} + ${#TEST_FAILED[@]}))
    local total_cases=$((${#TEST_CASES_PASSED[@]} + ${#TEST_CASES_FAILED[@]} + ${#TEST_CASES_SKIPPED[@]}))
    
    print_info "Google Test结果统计: 总计 $total_tests 个测试文件"
    print_success "通过: ${#TEST_SUCCESS[@]} 个测试文件"
    print_error "失败: ${#TEST_FAILED[@]} 个测试文件"
    
    if [ $total_cases -gt 0 ]; then
        print_info "测试用例统计: 总计 $total_cases 个测试用例"
        print_success "通过: ${#TEST_CASES_PASSED[@]} 个测试用例"
        print_error "失败: ${#TEST_CASES_FAILED[@]} 个测试用例"
        if [ ${#TEST_CASES_SKIPPED[@]} -gt 0 ]; then
            print_info "跳过: ${#TEST_CASES_SKIPPED[@]} 个测试用例"
        fi
    fi
}

# 解析CTest结果
# 运行单个Google Test可执行文件
run_single_gtest() {
    local exe="$1"
    local start_time=$(date +%s.%N)
    
    # 创建失败日志文件
    local failure_log="$BUILD_DIR/failure_${exe}.log"
    
    # 运行测试并捕获输出
    local test_output
    local exit_code=0
    
    # Mac和Linux兼容的运行方式
    local xml_output="$BUILD_DIR/test_${exe}.xml"
    local run_cmd
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # Mac: 使用DYLD_LIBRARY_PATH，检测常用库路径
        local lib_paths=""
        # 检测Anaconda路径
        if [ -d "$HOME/opt/anaconda3/lib" ]; then
            lib_paths="$HOME/opt/anaconda3/lib"
        fi
        # 检测Homebrew路径
        if [ -d "/opt/homebrew/lib" ]; then
            lib_paths="${lib_paths:+$lib_paths:}/opt/homebrew/lib"
        fi
        if [ -d "/usr/local/lib" ]; then
            lib_paths="${lib_paths:+$lib_paths:}/usr/local/lib"
        fi
        
        # Mac可能需要gtimeout（从coreutils）
        if command -v gtimeout &> /dev/null; then
            run_cmd="DYLD_LIBRARY_PATH=${lib_paths}:\$DYLD_LIBRARY_PATH gtimeout 30 ./$exe --gtest_output=xml:$xml_output"
        else
            # Mac没有timeout命令，直接运行
            run_cmd="DYLD_LIBRARY_PATH=${lib_paths}:\$DYLD_LIBRARY_PATH ./$exe --gtest_output=xml:$xml_output"
        fi
    else
        # Linux: 使用LD_LIBRARY_PATH和timeout
        local lib_paths="/usr/lib/x86_64-linux-gnu"
        # 检测其他可能的库路径
        if [ -d "/usr/lib" ]; then
            lib_paths="${lib_paths}:/usr/lib"
        fi
        if [ -d "/usr/local/lib" ]; then
            lib_paths="${lib_paths}:/usr/local/lib"
        fi
        run_cmd="LD_LIBRARY_PATH=${lib_paths}:\$LD_LIBRARY_PATH timeout 30 ./$exe --gtest_output=xml:$xml_output"
    fi
    
    if test_output=$(eval "$run_cmd" 2>&1); then
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "0")
        
        TEST_SUCCESS+=("$exe")
        TEST_DURATIONS+=("$exe:$duration")
        
        # 优先解析XML，回退到标准输出
        if [ -f "$xml_output" ]; then
            parse_gtest_xml "$xml_output" "$exe"
        else
            parse_gtest_output "$test_output" "$exe" "PASSED"
        fi
    else
        exit_code=$?
        local end_time=$(date +%s.%N)
        local duration=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "0")
        
        TEST_FAILED+=("$exe")
        TEST_DURATIONS+=("$exe:$duration")
        
        # 保存失败详情到文件
        {
            echo "测试文件: $exe"
            echo "失败时间: $(date)"
            echo "退出代码: $exit_code"
            echo "运行时间: ${duration}s"
            echo "="
            echo "输出内容:"
            echo "$test_output"
        } > "$failure_log"
        
        # 确定失败类型
        local error_type="测试失败"
        if [ $exit_code -eq 124 ]; then
            error_type="超时"
        elif echo "$test_output" | grep -q "Segmentation fault\|core dumped"; then
            error_type="段错误"
        fi
        
        TEST_ERRORS+=("$exe: $error_type")
        
        # 优先解析XML，回退到标准输出
        if [ -f "$xml_output" ]; then
            parse_gtest_xml "$xml_output" "$exe"
        else
            parse_gtest_output "$test_output" "$exe" "FAILED"
        fi
    fi
}

# 解析Google Test输出
parse_gtest_output() {
    local test_output="$1"
    local test_name="$2"
    local overall_status="$3"
    
    # 解析Google Test的总体统计
    local total_tests=0
    local passed_tests=0
    local failed_tests=0
    local total_time=0
    
    # 查找总结行，如："[  PASSED  ] 4 tests."
    local passed_line=$(echo "$test_output" | grep "\\[ *PASSED *\\].*tests")
    if [ -n "$passed_line" ]; then
        passed_tests=$(echo "$passed_line" | grep -o '[0-9]\+' | head -1)
        [ -z "$passed_tests" ] && passed_tests=0
    fi
    
    # 查找失败行，如："[  FAILED  ] 6 tests"
    local failed_line=$(echo "$test_output" | grep "\\[ *FAILED *\\].*tests")
    if [ -n "$failed_line" ]; then
        failed_tests=$(echo "$failed_line" | grep -o '[0-9]\+' | head -1)
        [ -z "$failed_tests" ] && failed_tests=0
    fi
    
    total_tests=$((passed_tests + failed_tests))
    
    # 查找运行时间
    local time_line=$(echo "$test_output" | grep "tests ran.*ms total" | tail -1)
    if [ -n "$time_line" ]; then
        local ms_time=$(echo "$time_line" | grep -o '[0-9]\+' | tail -1)
        if [ -n "$ms_time" ] && command -v bc &> /dev/null; then
            total_time=$(echo "scale=3; $ms_time / 1000" | bc -l 2>/dev/null || echo "0")
        fi
    fi
    
    # 存储测试套件信息
    TEST_SUITES_INFO+=("$test_name:total=$total_tests,passed=$passed_tests,failures=$failed_tests,errors=0,disabled=0,time=$total_time")
    
    # 统计测试用例
    for ((i=0; i<passed_tests; i++)); do
        TEST_CASES_PASSED+=("${test_name}_case_$i")
    done
    
    for ((i=0; i<failed_tests; i++)); do
        TEST_CASES_FAILED+=("${test_name}_case_$i")
    done
}


# 解析Google Test XML输出
parse_gtest_xml() {
    local xml_file="$1"
    local test_name="$2"
    
    if [ ! -f "$xml_file" ]; then
        print_warning "XML文件不存在: $xml_file"
        return 1
    fi
    
    # 解析测试XML: $test_name
    
    # 解析测试套件级别信息
    local total_tests=$(grep -o 'tests="[0-9]*"' "$xml_file" | head -1 | sed 's/tests="//' | sed 's/"//')
    local failures=$(grep -o 'failures="[0-9]*"' "$xml_file" | head -1 | sed 's/failures="//' | sed 's/"//')
    local errors=$(grep -o 'errors="[0-9]*"' "$xml_file" | head -1 | sed 's/errors="//' | sed 's/"//')
    local disabled=$(grep -o 'disabled="[0-9]*"' "$xml_file" | head -1 | sed 's/disabled="//' | sed 's/"//')
    local time=$(grep -o 'time="[0-9.]*"' "$xml_file" | head -1 | sed 's/time="//' | sed 's/"//')
    
    # 默认值处理
    total_tests=${total_tests:-0}
    failures=${failures:-0}
    errors=${errors:-0}
    disabled=${disabled:-0}
    time=${time:-0}
    
    # 存储测试套件信息
    TEST_SUITES_INFO+=("$test_name:total=$total_tests,passed=$((total_tests-failures-errors)),failures=$failures,errors=$errors,disabled=$disabled,time=$time")
    
    # 解析每个测试用例的详细信息
    local temp_file=$(mktemp)
    
    # 提取所有测试用例标签
    grep -o '<testcase[^>]*>' "$xml_file" > "$temp_file"
    
    while IFS= read -r testcase_line; do
        if [ -z "$testcase_line" ]; then
            continue
        fi
        
        # 提取测试用例属性
        local case_name=$(echo "$testcase_line" | grep -o 'name="[^"]*"' | sed 's/name="//' | sed 's/"//')
        local suite_name=$(echo "$testcase_line" | grep -o 'classname="[^"]*"' | sed 's/classname="//' | sed 's/"//')
        local case_time=$(echo "$testcase_line" | grep -o 'time="[^"]*"' | sed 's/time="//' | sed 's/"//')
        local case_status=$(echo "$testcase_line" | grep -o 'status="[^"]*"' | sed 's/status="//' | sed 's/"//')
        
        case_time=${case_time:-0}
        case_status=${case_status:-"run"}
        
        # 构建完整的测试用例名称
        local full_case_name="${test_name}.${suite_name}.${case_name}"
        
        # 检查是否有失败或错误信息
        local case_line_num=$(grep -n "<testcase.*name=\"$case_name\"" "$xml_file" | cut -d: -f1)
        local has_failure=false
        local has_error=false
        local failure_message=""
        
        if [ -n "$case_line_num" ]; then
            # 查找这个测试用例的失败或错误信息
            local case_content=$(awk -v start="$case_line_num" 'NR >= start && /<\/testcase>/ {print; exit} NR >= start' "$xml_file")
            
            if echo "$case_content" | grep -q "<failure"; then
                has_failure=true
                failure_message=$(echo "$case_content" | grep -o '<failure[^>]*>.*</failure>' | sed 's/<[^>]*>//g' | head -3 | tr '\n' ' ')
            elif echo "$case_content" | grep -q "<error"; then
                has_error=true
                failure_message=$(echo "$case_content" | grep -o '<error[^>]*>.*</error>' | sed 's/<[^>]*>//g' | head -3 | tr '\n' ' ')
            fi
        fi
        
        # 分类存储测试用例结果
        if [ "$has_failure" = true ] || [ "$has_error" = true ]; then
            TEST_CASES_FAILED+=("$full_case_name")
            TEST_CASES_DETAILS+=("$full_case_name:FAILED:$case_time:$failure_message")
        elif [ "$case_status" = "notrun" ] || echo "$testcase_line" | grep -q "disabled"; then
            TEST_CASES_SKIPPED+=("$full_case_name")
            TEST_CASES_DETAILS+=("$full_case_name:SKIPPED:$case_time:Disabled or not run")
        else
            TEST_CASES_PASSED+=("$full_case_name")
            TEST_CASES_DETAILS+=("$full_case_name:PASSED:$case_time:")
        fi
        
    done < "$temp_file"
    
    rm -f "$temp_file"
    
    # 更新统计
    TEST_CASE_STATS+=("$test_name:tests=$total_tests,failures=$failures,errors=$errors,time=$time")
    
    print_info "解析完成: $test_name - 总计 $total_tests 个测试用例"
}

compile_all_tests_parallel() {
    local timeout_seconds="$1"
    local verbose_mode="$2"
    local compile_all_mode="$3"
    
    print_info "使用8个CPU并行编译测试文件..."
    print_info "超时设置: ${timeout_seconds}秒每个文件"
    
    local test_files=($(get_test_files))
    
    if [ ${#test_files[@]} -eq 0 ]; then
        print_warning "未找到测试文件"
        return 1
    fi
    
    # 导出需要的变量和函数给子进程
    export BUILD_DIR INCLUDE_DIR TEST_DIR
    export -f compile_single_test_parallel run_with_timeout
    
    # 创建临时文件列表
    local temp_list=$(mktemp)
    printf '%s\n' "${test_files[@]}" > "$temp_list"
    
    local start_time=$(date +%s)
    
    # 使用GNU parallel或xargs进行并行编译
    if command -v parallel &> /dev/null; then
        print_info "使用GNU parallel进行并行编译..."
        parallel -j8 --timeout ${timeout_seconds} --colsep ' ' \
            compile_single_test_parallel {} "$timeout_seconds" "$verbose_mode" \
            < "$temp_list"
    else
        print_info "使用xargs进行并行编译..."
        cat "$temp_list" | xargs -n 1 -P 8 -I {} bash -c \
            "compile_single_test_parallel '{}' '$timeout_seconds' '$verbose_mode'"
    fi
    
    local end_time=$(date +%s)
    local elapsed=$((end_time - start_time))
    
    # 清理临时文件
    rm -f "$temp_list"
    
    # 收集并行编译结果
    collect_parallel_results "$verbose_mode"
    
    print_separator
    print_info "并行编译完成统计 (耗时: ${elapsed}秒):"
    print_success "成功编译: ${#COMPILED_SUCCESS[@]} 个文件"
    print_error "编译失败: ${#COMPILED_FAILED[@]} 个文件"
    
    if [ ${#COMPILED_SUCCESS[@]} -gt 0 ]; then
        local avg_time=$((elapsed / ${#test_files[@]}))
        print_info "平均每个文件耗时: ${avg_time}秒"
    fi
}

# 收集并行编译结果
collect_parallel_results() {
    local verbose_mode="$1"
    
    print_info "收集并行编译结果..."
    
    # 清空之前的结果
    COMPILED_SUCCESS=()
    COMPILED_FAILED=()
    COMPILE_ERRORS=()
    TEST_CASES_PASSED=()
    TEST_CASES_FAILED=()
    TEST_CASES_SKIPPED=()
    TEST_CASES_DETAILS=()
    TEST_SUITES_INFO=()
    
    # 查找所有结果文件
    for result_file in "$BUILD_DIR"/compile_result_*.txt; do
        if [ -f "$result_file" ]; then
            local filename=$(basename "$result_file" .txt | sed 's/compile_result_//')
            
            # 读取结果
            local first_line=$(head -n1 "$result_file" 2>/dev/null)
            
            if [[ "$first_line" =~ ^SUCCESS: ]]; then
                local test_name=$(echo "$first_line" | sed 's/SUCCESS: //')
                COMPILED_SUCCESS+=("$test_name")
                if [ "$verbose_mode" = true ]; then
                    print_success "编译成功: $test_name"
                fi
            elif [[ "$first_line" =~ ^FAILED: ]] || [[ "$first_line" =~ ^TIMEOUT: ]] || [[ "$first_line" =~ ^ERROR: ]]; then
                local test_name=$(echo "$first_line" | sed 's/^[A-Z]*: //')
                COMPILED_FAILED+=("$test_name")
                
                # 收集错误信息
                local error_output=$(grep "^ERROR_OUTPUT:" "$result_file" 2>/dev/null | sed 's/ERROR_OUTPUT: //')
                if [ -n "$error_output" ]; then
                    COMPILE_ERRORS+=("$test_name: $error_output")
                else
                    COMPILE_ERRORS+=("$first_line")
                fi
                
                if [ "$verbose_mode" = true ]; then
                    print_error "编译失败: $test_name"
                fi
            fi
            
            # 清理结果文件
            rm -f "$result_file"
        fi
    done
}

# 运行所有测试
run_all_tests() {
    local timeout_seconds="$1"
    local verbose_mode="$2"
    
    print_info "开始运行所有测试..."
    
    if [ ${#COMPILED_SUCCESS[@]} -eq 0 ]; then
        print_warning "没有成功编译的测试文件"
        return 1
    fi
    
    local test_files=($(get_test_files))
    
    local count=0
    for test_file in "${test_files[@]}"; do
        local filename=$(basename "$test_file" .cpp)
        
        # 只运行成功编译的测试
        if [[ " ${COMPILED_SUCCESS[@]} " =~ " $filename " ]]; then
            count=$((count + 1))
            run_single_test "$test_file" "$timeout_seconds" "$verbose_mode" || true  # 继续运行其他测试
        fi
    done
    
    print_separator
    print_info "测试运行完成统计:"
    print_success "测试通过: ${#TEST_SUCCESS[@]} 个"
    print_error "测试失败: ${#TEST_FAILED[@]} 个"
}

# 生成详细报告
generate_report() {
    print_separator
    print_info "生成详细报告..."
    
    echo
    echo "========================= 编译结果 ========================="
    echo "成功编译的文件 (${#COMPILED_SUCCESS[@]}个):"
    for file in "${COMPILED_SUCCESS[@]}"; do
        echo "  ✓ $file"
    done
    
    echo
    echo "编译失败的文件 (${#COMPILED_FAILED[@]}个):"
    for file in "${COMPILED_FAILED[@]}"; do
        echo "  ✗ $file"
    done
    
    echo
    echo "========================= 测试结果 ========================="
    echo "测试通过的文件 (${#TEST_SUCCESS[@]}个):"
    for file in "${TEST_SUCCESS[@]}"; do
        echo "  ✓ $file"
    done
    
    echo
    echo "测试失败的文件 (${#TEST_FAILED[@]}个):"
    for file in "${TEST_FAILED[@]}"; do
        echo "  ✗ $file"
    done
    
    echo
    echo "========================= 错误详情 ========================="
    if [ ${#COMPILE_ERRORS[@]} -gt 0 ]; then
        echo "编译错误详情:"
        for error in "${COMPILE_ERRORS[@]}"; do
            echo "  $error"
            echo
        done
    fi
    
    if [ ${#TEST_ERRORS[@]} -gt 0 ]; then
        echo "测试错误详情:"
        for error in "${TEST_ERRORS[@]}"; do
            echo "  $error"
            echo
        done
    fi
    
    echo
    echo "========================= 性能统计 ========================="
    if [ ${#TEST_DURATIONS[@]} -gt 0 ]; then
        echo "测试运行时间统计:"
        for duration_info in "${TEST_DURATIONS[@]}"; do
            local test_name="${duration_info%%:*}"
            local duration="${duration_info##*:}"
            printf "  %-30s %8ss\n" "$test_name" "$duration"
        done
        echo
        
        # 计算平均运行时间
        local total_time=0
        local count=0
        for duration_info in "${TEST_DURATIONS[@]}"; do
            local duration="${duration_info##*:}"
            if command -v bc &> /dev/null; then
                total_time=$(echo "$total_time + $duration" | bc -l)
            fi
            count=$((count + 1))
        done
        
        if [ $count -gt 0 ] && command -v bc &> /dev/null; then
            local avg_time=$(echo "scale=3; $total_time / $count" | bc -l)
            echo "平均运行时间: ${avg_time}s"
            echo "总运行时间: ${total_time}s"
        fi
    fi
    
    echo
    echo "========================= 测试套件统计 ========================="
    if [ ${#TEST_SUITES_INFO[@]} -gt 0 ]; then
        echo "详细测试套件统计:"
        printf "  %-30s %8s %8s %8s %8s %8s %10s\n" "测试文件" "总计" "通过" "失败" "错误" "跳过" "时间(s)"
        echo "  $(printf '%*s' 90 '' | tr ' ' '-')"
        
        for suite_info in "${TEST_SUITES_INFO[@]}"; do
            local test_name="${suite_info%%:*}"
            local stats="${suite_info##*:}"
            
            # 解析统计信息
            local total=$(echo "$stats" | grep -o 'total=[0-9]*' | sed 's/total=//')
            local passed=$(echo "$stats" | grep -o 'passed=[0-9]*' | sed 's/passed=//')
            local failures=$(echo "$stats" | grep -o 'failures=[0-9]*' | sed 's/failures=//')
            local errors=$(echo "$stats" | grep -o 'errors=[0-9]*' | sed 's/errors=//')
            local disabled=$(echo "$stats" | grep -o 'disabled=[0-9]*' | sed 's/disabled=//')
            local time=$(echo "$stats" | grep -o 'time=[0-9.]*' | sed 's/time=//')
            
            printf "  %-30s %8s %8s %8s %8s %8s %10s\n" "$test_name" "$total" "$passed" "$failures" "$errors" "$disabled" "$time"
        done
    fi
    
    echo
    echo "========================= 测试用例详情 ========================="
    local total_cases=$((${#TEST_CASES_PASSED[@]} + ${#TEST_CASES_FAILED[@]} + ${#TEST_CASES_SKIPPED[@]}))
    echo "总测试用例数: $total_cases"
    echo "通过测试用例: ${#TEST_CASES_PASSED[@]} 个"
    echo "失败测试用例: ${#TEST_CASES_FAILED[@]} 个"
    echo "跳过测试用例: ${#TEST_CASES_SKIPPED[@]} 个"
    
    if [ ${#TEST_CASES_PASSED[@]} -gt 0 ]; then
        echo
        # 如果通过的测试用例太多，只显示前10个和后10个
        if [ ${#TEST_CASES_PASSED[@]} -gt 50 ]; then
            echo "通过的测试用例: ${#TEST_CASES_PASSED[@]} 个 (显示前10个和后10个):"
            local count=0
            local shown=0
            for case_detail in "${TEST_CASES_DETAILS[@]}"; do
                local case_name="${case_detail%%:*}"
                local remainder="${case_detail#*:}"
                local status="${remainder%%:*}"
                remainder="${remainder#*:}"
                local time="${remainder%%:*}"
                
                if [ "$status" = "PASSED" ]; then
                    count=$((count + 1))
                    if [ $count -le 10 ] || [ $count -gt $((${#TEST_CASES_PASSED[@]} - 10)) ]; then
                        printf "  ✓ %-80s %8ss\n" "$case_name" "$time"
                        shown=$((shown + 1))
                    elif [ $shown -eq 10 ]; then
                        echo "  ... (省略 $((${#TEST_CASES_PASSED[@]} - 20)) 个通过的测试用例) ..."
                        shown=$((shown + 1))
                    fi
                fi
            done
        else
            echo "通过的测试用例:"
            for case_detail in "${TEST_CASES_DETAILS[@]}"; do
                local case_name="${case_detail%%:*}"
                local remainder="${case_detail#*:}"
                local status="${remainder%%:*}"
                remainder="${remainder#*:}"
                local time="${remainder%%:*}"
                
                if [ "$status" = "PASSED" ]; then
                    printf "  ✓ %-80s %8ss\n" "$case_name" "$time"
                fi
            done
        fi
    fi
    
    if [ ${#TEST_CASES_FAILED[@]} -gt 0 ]; then
        echo
        echo "失败的测试用例:"
        for case_detail in "${TEST_CASES_DETAILS[@]}"; do
            local case_name="${case_detail%%:*}"
            local remainder="${case_detail#*:}"
            local status="${remainder%%:*}"
            remainder="${remainder#*:}"
            local time="${remainder%%:*}"
            local message="${remainder#*:}"
            
            if [ "$status" = "FAILED" ]; then
                printf "  ✗ %-50s %8ss\n" "$case_name" "$time"
                if [ -n "$message" ]; then
                    echo "    错误: $(echo "$message" | cut -c1-100)..."
                fi
            fi
        done
    fi
    
    if [ ${#TEST_CASES_SKIPPED[@]} -gt 0 ]; then
        echo
        echo "跳过的测试用例:"
        for case_detail in "${TEST_CASES_DETAILS[@]}"; do
            local case_name="${case_detail%%:*}"
            local remainder="${case_detail#*:}"
            local status="${remainder%%:*}"
            remainder="${remainder#*:}"
            local time="${remainder%%:*}"
            
            if [ "$status" = "SKIPPED" ]; then
                printf "  ⊘ %-50s %8ss\n" "$case_name" "$time"
            fi
        done
    fi
    
    echo
    echo "========================= 失败详情文件 ========================="
    if [ ${#TEST_DETAILS[@]} -gt 0 ]; then
        echo "失败详情文件位置:"
        for detail in "${TEST_DETAILS[@]}"; do
            local test_name="${detail%%:*}"
            local log_file="${detail##*:}"
            echo "  $test_name: $log_file"
        done
    fi
    
    echo
    echo "========================= 汇总统计 ========================="
    echo "测试文件级别统计:"
    echo "  总测试文件数: $(get_test_files | wc -w)"
    echo "  成功编译: ${#COMPILED_SUCCESS[@]}"
    echo "  编译失败: ${#COMPILED_FAILED[@]}"
    echo "  测试通过: ${#TEST_SUCCESS[@]}"
    echo "  测试失败: ${#TEST_FAILED[@]}"
    
    echo
    echo "测试用例级别统计:"
    local total_cases=$((${#TEST_CASES_PASSED[@]} + ${#TEST_CASES_FAILED[@]} + ${#TEST_CASES_SKIPPED[@]}))
    echo "  总测试用例数: $total_cases"
    echo "  通过测试用例: ${#TEST_CASES_PASSED[@]}"
    echo "  失败测试用例: ${#TEST_CASES_FAILED[@]}"
    echo "  跳过测试用例: ${#TEST_CASES_SKIPPED[@]}"
    
    echo
    echo "成功率统计:"
    
    # 计算文件级别成功率
    local total_compiled=$((${#COMPILED_SUCCESS[@]} + ${#COMPILED_FAILED[@]}))
    local total_tested=$((${#TEST_SUCCESS[@]} + ${#TEST_FAILED[@]}))
    
    if [ $total_compiled -gt 0 ]; then
        local compile_rate=$(printf "%.1f" $(echo "scale=2; ${#COMPILED_SUCCESS[@]} * 100 / $total_compiled" | bc -l 2>/dev/null || echo "0"))
        echo "  编译成功率: ${compile_rate}%"
    fi
    
    if [ $total_tested -gt 0 ]; then
        local test_pass_rate=$(printf "%.1f" $(echo "scale=2; ${#TEST_SUCCESS[@]} * 100 / $total_tested" | bc -l 2>/dev/null || echo "0"))
        echo "  测试文件通过率: ${test_pass_rate}%"
    fi
    
    # 计算测试用例级别成功率
    if [ $total_cases -gt 0 ]; then
        local case_pass_rate=$(printf "%.1f" $(echo "scale=2; ${#TEST_CASES_PASSED[@]} * 100 / $total_cases" | bc -l 2>/dev/null || echo "0"))
        echo "  测试用例通过率: ${case_pass_rate}%"
        
        if [ ${#TEST_CASES_FAILED[@]} -gt 0 ]; then
            local case_fail_rate=$(printf "%.1f" $(echo "scale=2; ${#TEST_CASES_FAILED[@]} * 100 / $total_cases" | bc -l 2>/dev/null || echo "0"))
            echo "  测试用例失败率: ${case_fail_rate}%"
        fi
        
        if [ ${#TEST_CASES_SKIPPED[@]} -gt 0 ]; then
            local case_skip_rate=$(printf "%.1f" $(echo "scale=2; ${#TEST_CASES_SKIPPED[@]} * 100 / $total_cases" | bc -l 2>/dev/null || echo "0"))
            echo "  测试用例跳过率: ${case_skip_rate}%"
        fi
    fi
    
    echo "========================================================="
}

# 保存报告到文件
save_report() {
    local report_file="$BUILD_DIR/test_report.txt"
    
    print_info "保存报告到: $report_file"
    
    {
        echo "Backtrader C++ 测试运行报告"
        echo "生成时间: $(date)"
        echo "脚本位置: $SCRIPT_DIR"
        echo
        generate_report
    } > "$report_file"
    
    print_success "报告已保存到: $report_file"
}

# 生成JSON格式的报告
generate_json_report() {
    local json_file="$BUILD_DIR/test_report.json"
    
    print_info "生成JSON报告到: $json_file"
    
    {
        echo "{"
        echo "  \"timestamp\": \"$(date -Iseconds)\","
        echo "  \"script_location\": \"$SCRIPT_DIR\","
        echo "  \"build_directory\": \"$BUILD_DIR\","
        echo "  \"summary\": {"
        echo "    \"total_test_files\": $(get_test_files | wc -w),"
        echo "    \"compiled_success\": ${#COMPILED_SUCCESS[@]},"
        echo "    \"compiled_failed\": ${#COMPILED_FAILED[@]},"
        echo "    \"tests_passed\": ${#TEST_SUCCESS[@]},"
        echo "    \"tests_failed\": ${#TEST_FAILED[@]}"
        echo "  },"
        echo "  \"compilation\": {"
        echo "    \"successful\": ["
        for i in "${!COMPILED_SUCCESS[@]}"; do
            echo -n "      \"${COMPILED_SUCCESS[$i]}\""
            if [ $i -lt $((${#COMPILED_SUCCESS[@]} - 1)) ]; then
                echo ","
            else
                echo ""
            fi
        done
        echo "    ],"
        echo "    \"failed\": ["
        for i in "${!COMPILED_FAILED[@]}"; do
            echo -n "      \"${COMPILED_FAILED[$i]}\""
            if [ $i -lt $((${#COMPILED_FAILED[@]} - 1)) ]; then
                echo ","
            else
                echo ""
            fi
        done
        echo "    ],"
        echo "    \"errors\": ["
        for i in "${!COMPILE_ERRORS[@]}"; do
            local error_escaped=$(echo "${COMPILE_ERRORS[$i]}" | sed 's/"/\\"/g' | sed 's/\\/\\\\/g')
            echo -n "      \"$error_escaped\""
            if [ $i -lt $((${#COMPILE_ERRORS[@]} - 1)) ]; then
                echo ","
            else
                echo ""
            fi
        done
        echo "    ]"
        echo "  },"
        echo "  \"testing\": {"
        echo "    \"successful\": ["
        for i in "${!TEST_SUCCESS[@]}"; do
            echo -n "      \"${TEST_SUCCESS[$i]}\""
            if [ $i -lt $((${#TEST_SUCCESS[@]} - 1)) ]; then
                echo ","
            else
                echo ""
            fi
        done
        echo "    ],"
        echo "    \"failed\": ["
        for i in "${!TEST_FAILED[@]}"; do
            echo -n "      \"${TEST_FAILED[$i]}\""
            if [ $i -lt $((${#TEST_FAILED[@]} - 1)) ]; then
                echo ","
            else
                echo ""
            fi
        done
        echo "    ],"
        echo "    \"errors\": ["
        for i in "${!TEST_ERRORS[@]}"; do
            local error_escaped=$(echo "${TEST_ERRORS[$i]}" | sed 's/"/\\"/g' | sed 's/\\/\\\\/g')
            echo -n "      \"$error_escaped\""
            if [ $i -lt $((${#TEST_ERRORS[@]} - 1)) ]; then
                echo ","
            else
                echo ""
            fi
        done
        echo "    ],"
        echo "    \"durations\": {"
        for i in "${!TEST_DURATIONS[@]}"; do
            local duration_info="${TEST_DURATIONS[$i]}"
            local test_name="${duration_info%%:*}"
            local duration="${duration_info##*:}"
            echo -n "      \"$test_name\": $duration"
            if [ $i -lt $((${#TEST_DURATIONS[@]} - 1)) ]; then
                echo ","
            else
                echo ""
            fi
        done
        echo "    },"
        echo "    \"case_stats\": {"
        for i in "${!TEST_CASE_STATS[@]}"; do
            local case_stat="${TEST_CASE_STATS[$i]}"
            local test_name="${case_stat%%:*}"
            local stats="${case_stat##*:}"
            echo -n "      \"$test_name\": \"$stats\""
            if [ $i -lt $((${#TEST_CASE_STATS[@]} - 1)) ]; then
                echo ","
            else
                echo ""
            fi
        done
        echo "    },"
        echo "    \"failure_details\": {"
        for i in "${!TEST_DETAILS[@]}"; do
            local detail="${TEST_DETAILS[$i]}"
            local test_name="${detail%%:*}"
            local log_file="${detail##*:}"
            echo -n "      \"$test_name\": \"$log_file\""
            if [ $i -lt $((${#TEST_DETAILS[@]} - 1)) ]; then
                echo ","
            else
                echo ""
            fi
        done
        echo "    }"
        echo "  },"
        echo "  \"test_cases\": {"
        echo "    \"summary\": {"
        local total_cases=$((${#TEST_CASES_PASSED[@]} + ${#TEST_CASES_FAILED[@]} + ${#TEST_CASES_SKIPPED[@]}))
        echo "      \"total\": $total_cases,"
        echo "      \"passed\": ${#TEST_CASES_PASSED[@]},"
        echo "      \"failed\": ${#TEST_CASES_FAILED[@]},"
        echo "      \"skipped\": ${#TEST_CASES_SKIPPED[@]}"
        echo "    },"
        echo "    \"test_suites\": ["
        for i in "${!TEST_SUITES_INFO[@]}"; do
            local suite_info="${TEST_SUITES_INFO[$i]}"
            local test_name="${suite_info%%:*}"
            local stats="${suite_info##*:}"
            
            # 解析统计信息
            local total=$(echo "$stats" | grep -o 'total=[0-9]*' | sed 's/total=//')
            local passed=$(echo "$stats" | grep -o 'passed=[0-9]*' | sed 's/passed=//')
            local failures=$(echo "$stats" | grep -o 'failures=[0-9]*' | sed 's/failures=//')
            local errors=$(echo "$stats" | grep -o 'errors=[0-9]*' | sed 's/errors=//')
            local disabled=$(echo "$stats" | grep -o 'disabled=[0-9]*' | sed 's/disabled=//')
            local time=$(echo "$stats" | grep -o 'time=[0-9.]*' | sed 's/time=//')
            
            echo "      {"
            echo "        \"name\": \"$test_name\","
            echo "        \"total\": $total,"
            echo "        \"passed\": $passed,"
            echo "        \"failed\": $failures,"
            echo "        \"errors\": $errors,"
            echo "        \"disabled\": $disabled,"
            echo "        \"time\": $time"
            echo -n "      }"
            if [ $i -lt $((${#TEST_SUITES_INFO[@]} - 1)) ]; then
                echo ","
            else
                echo ""
            fi
        done
        echo "    ],"
        echo "    \"passed_cases\": ["
        for i in "${!TEST_CASES_PASSED[@]}"; do
            echo -n "      \"${TEST_CASES_PASSED[$i]}\""
            if [ $i -lt $((${#TEST_CASES_PASSED[@]} - 1)) ]; then
                echo ","
            else
                echo ""
            fi
        done
        echo "    ],"
        echo "    \"failed_cases\": ["
        for i in "${!TEST_CASES_FAILED[@]}"; do
            echo -n "      \"${TEST_CASES_FAILED[$i]}\""
            if [ $i -lt $((${#TEST_CASES_FAILED[@]} - 1)) ]; then
                echo ","
            else
                echo ""
            fi
        done
        echo "    ],"
        echo "    \"skipped_cases\": ["
        for i in "${!TEST_CASES_SKIPPED[@]}"; do
            echo -n "      \"${TEST_CASES_SKIPPED[$i]}\""
            if [ $i -lt $((${#TEST_CASES_SKIPPED[@]} - 1)) ]; then
                echo ","
            else
                echo ""
            fi
        done
        echo "    ],"
        echo "    \"case_details\": ["
        for i in "${!TEST_CASES_DETAILS[@]}"; do
            local case_detail="${TEST_CASES_DETAILS[$i]}"
            local case_name="${case_detail%%:*}"
            local remainder="${case_detail#*:}"
            local status="${remainder%%:*}"
            remainder="${remainder#*:}"
            local time="${remainder%%:*}"
            local message="${remainder#*:}"
            
            # 转义JSON字符串
            case_name=$(echo "$case_name" | sed 's/"/\\"/g' | sed 's/\\/\\\\/g')
            message=$(echo "$message" | sed 's/"/\\"/g' | sed 's/\\/\\\\/g')
            
            echo "      {"
            echo "        \"name\": \"$case_name\","
            echo "        \"status\": \"$status\","
            echo "        \"time\": $time,"
            echo "        \"message\": \"$message\""
            echo -n "      }"
            if [ $i -lt $((${#TEST_CASES_DETAILS[@]} - 1)) ]; then
                echo ","
            else
                echo ""
            fi
        done
        echo "    ]"
        echo "  }"
        echo "}"
    } > "$json_file"
    
    print_success "JSON报告已保存到: $json_file"
}

# 分析测试失败原因

# 显示帮助
show_help() {
    echo "Backtrader C++ 测试运行脚本"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  --cmake-only     仅使用 CMake 构建和测试（推荐）"
    echo "  --compile-only   仅编译测试文件"
    echo "  --compile-all    强制编译所有115个测试文件（用于全面bug检测）"
    echo "  --parallel       使用8个CPU并行编译（推荐与--compile-all配合使用）"
    echo "  --run-only       仅运行已编译的测试"
    echo "  --no-report      不生成详细报告"
    echo "  --json-report    生成JSON格式的报告"
    echo "  --timeout N      设置超时时间（秒，默认10秒）"
    echo "  --prefer-cmake   优先使用 CMake 而不是直接编译"
    echo "  --verbose        显示详细的编译输出"
    echo "  --help           显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0                           # 默认：使用CMake方式，8核并行编译（推荐）"
    echo "  $0 --cmake-only              # 仅使用CMake构建和测试"
    echo "  $0 --compile-all --parallel --timeout 10 # 直接编译：8个CPU并行编译所有115个测试文件"
    echo "  $0 --compile-all --parallel --verbose # 直接编译，显示详细输出"
    echo "  $0 --compile-only --timeout 30 # 仅编译，30秒超时"
    echo "  $0 --no-report               # 不生成详细报告"
    echo ""
    echo "说明:"
    echo "  默认使用CMake进行构建，使用8核并行编译，提供最佳性能"
    echo "  CMake模式会编译backtrader_cpp的所有源文件，支持增量编译"
    echo "  该脚本会编译 tests/original_tests 目录下的所有 .cpp 测试文件"
    echo "  即使某个文件编译失败，脚本也会继续编译其他文件"
    echo "  测试失败时，脚本会继续运行其他测试"
    echo "  编译和测试都有超时保护，默认10秒超时"
    echo "  --compile-all 使用两阶段编译：先构建核心库，再快速编译测试文件"
    echo "  --parallel 使用8个CPU并行编译，大幅提升编译速度"
    echo "  脚本会收集详细的测试用例级别统计信息"
    echo "  自动解析Google Test的XML和标准输出格式"
    echo "  生成全面的测试报告，包括失败原因分析和修复建议"
    echo "  支持JSON格式报告，便于程序化处理和集成"
}

# 主函数
main() {
    local cmake_only=false
    local compile_only=false
    local compile_all=false
    local run_only=false
    local no_report=false
    local json_report=false
    local prefer_cmake=false
    local verbose=false
    local parallel=false
    local timeout_seconds=10
    
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            --cmake-only)
                cmake_only=true
                shift
                ;;
            --compile-only)
                compile_only=true
                shift
                ;;
            --compile-all)
                compile_all=true
                compile_only=true  # --compile-all 包含 --compile-only
                shift
                ;;
            --parallel)
                parallel=true
                shift
                ;;
            --run-only)
                run_only=true
                shift
                ;;
            --no-report)
                no_report=true
                shift
                ;;
            --json-report)
                json_report=true
                shift
                ;;
            --verbose)
                verbose=true
                shift
                ;;
            --timeout)
                if [[ $# -gt 1 && $2 =~ ^[0-9]+$ ]]; then
                    timeout_seconds=$2
                    shift 2
                else
                    print_error "--timeout 需要一个数字参数"
                    exit 1
                fi
                ;;
            --prefer-cmake)
                prefer_cmake=true
                shift
                ;;
            --help)
                show_help
                exit 0
                ;;
            *)
                print_error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    print_separator
    print_info "Backtrader C++ 测试运行脚本"
    print_info "脚本位置: $SCRIPT_DIR"
    print_info "测试目录: $TEST_DIR"
    print_info "构建目录: $BUILD_DIR"
    print_info "超时设置: ${timeout_seconds}秒"
    if [ "$compile_all" = true ]; then
        print_info "编译模式: 全面编译所有测试文件"
    fi
    if [ "$parallel" = true ]; then
        print_info "并行模式: 使用8个CPU并行编译"
    fi
    if [ "$verbose" = true ]; then
        print_info "详细输出: 启用"
    fi
    print_separator
    
    # 检查测试目录是否存在
    if [ ! -d "$TEST_DIR" ]; then
        print_error "测试目录不存在: $TEST_DIR"
        exit 1
    fi
    
    # 检查依赖
    check_dependencies
    
    # 创建构建目录
    create_build_dir
    
    if [ "$cmake_only" = true ] || [ "$prefer_cmake" = true ]; then
        # 使用 CMake 构建和测试
        print_info "使用 CMake 构建模式（推荐）"
        build_with_cmake
        run_with_gtest
    elif [ "$compile_only" = true ]; then
        # 仅编译
        if [ "$compile_all" = true ]; then
            print_info "全面编译模式，将编译所有115个测试文件，超时设置: ${timeout_seconds}秒"
        else
            print_info "仅编译模式，超时设置: ${timeout_seconds}秒"
        fi
        if [ "$parallel" = true ]; then
            compile_all_tests_parallel "$timeout_seconds" "$verbose" "$compile_all"
        else
            # 需要实现非并行编译函数或者直接使用并行版本
            compile_all_tests_parallel "$timeout_seconds" "$verbose" "$compile_all"
        fi
    elif [ "$run_only" = true ]; then
        # 仅运行测试
        print_info "仅运行测试模式，超时设置: ${timeout_seconds}秒"
        run_all_tests "$timeout_seconds" "$verbose"
    else
        # 默认使用 CMake 模式（推荐）
        print_info "默认使用 CMake 构建模式（推荐），使用8核并行编译"
        print_info "如需使用直接编译模式，请使用 --compile-all 选项"
        build_with_cmake
        run_with_gtest
    fi
    
    # 生成报告
    if [ "$no_report" = false ]; then
        generate_report
        save_report
        
        # 生成JSON报告
        if [ "$json_report" = true ]; then
            generate_json_report
        fi
        
        # 失败详情已写入单独的日志文件
    fi
    
    print_separator
    print_success "测试运行脚本执行完成"
}

# 运行主函数
main "$@"
