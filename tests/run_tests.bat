@echo off
REM ============================================================================
REM Backtrader C++ Test Runner Script for Windows
REM 
REM 功能: 编译并运行所有测试
REM 用法: run_tests.bat [--run-only] [--clean] [--help]
REM ============================================================================

REM Switch to UTF-8 code page for proper Chinese character display
chcp 65001 >nul 2>&1

setlocal enabledelayedexpansion

REM ========== Global Variables ==========
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."
set "BUILD_DIR=%SCRIPT_DIR%build_tests"
set "CORE_LIB=%PROJECT_ROOT%\libbacktrader_core.a"

REM Result counters
set "COMPILED_SUCCESS=0"
set "COMPILED_FAILED=0"
set "TEST_SUCCESS=0"
set "TEST_FAILED=0"
set "TEST_TIMEOUT=0"
set "TEST_CRASHED=0"

REM Create build directory and temp directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
set "TEMP_RESULTS=%BUILD_DIR%\temp_results"
if not exist "%TEMP_RESULTS%" mkdir "%TEMP_RESULTS%"

REM Setup log file with timestamp
for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value') do set "dt=%%a"
set "YYYY=%dt:~0,4%"
set "MM=%dt:~4,2%"
set "DD=%dt:~6,2%"
set "HH=%dt:~8,2%"
set "Min=%dt:~10,2%"
set "Sec=%dt:~12,2%"

set "LOG_FILE=%BUILD_DIR%\test_run_%YYYY%%MM%%DD%_%HH%%Min%%Sec%.log"
echo ===================================== > "%LOG_FILE%"
echo Backtrader C++ 测试运行日志 >> "%LOG_FILE%"
echo 开始时间: %date% %time% >> "%LOG_FILE%"
echo ===================================== >> "%LOG_FILE%"
echo. >> "%LOG_FILE%"

REM ========== Parse Arguments ==========
set "RUN_ONLY=0"
set "CLEAN_BUILD=0"

:parse_args
if "%~1"=="" goto :args_done
if "%~1"=="--run-only" (
    set "RUN_ONLY=1"
    shift
    goto :parse_args
)
if "%~1"=="--clean" (
    set "CLEAN_BUILD=1"
    shift
    goto :parse_args
)
if "%~1"=="--help" (
    call :show_help
    exit /b 0
)
call :print_error "未知选项: %~1"
call :show_help
exit /b 1

:args_done

REM ========== Main Script ==========
call :print_info "Backtrader C++ 测试运行脚本"
call :print_info "脚本位置: %SCRIPT_DIR%"
call :print_info "构建目录: %BUILD_DIR%"
call :print_info "项目根目录: %PROJECT_ROOT%"
call :print_separator

REM Check dependencies
call :check_dependencies
if !errorlevel! neq 0 exit /b 1

REM Check core library
call :check_core_library
if !errorlevel! neq 0 exit /b 1

REM Clean build if requested
if "!CLEAN_BUILD!"=="1" (
    call :clean_build
)

REM Build tests if not in run-only mode
if "!RUN_ONLY!"=="0" (
    call :build_tests
    if !errorlevel! neq 0 (
        call :print_warning "部分测试编译失败，但继续运行已编译的测试"
    )
)

REM Run all tests
call :run_all_tests

REM Generate report
call :generate_report

REM Save report
call :save_report

REM Log completion
echo. >> "%LOG_FILE%"
echo ===================================== >> "%LOG_FILE%"
echo 结束时间: %date% %time% >> "%LOG_FILE%"
echo ===================================== >> "%LOG_FILE%"

call :print_separator
call :print_success "测试运行完成"
call :print_info "完整日志已保存到: %LOG_FILE%"

exit /b 0

REM ========== Function Definitions ==========

:print_info
set "msg=[INFO] %~1"
echo !msg!
echo %time% !msg! >> "%LOG_FILE%"
exit /b 0

:print_success
set "msg=[SUCCESS] %~1"
echo !msg!
echo %time% !msg! >> "%LOG_FILE%"
exit /b 0

:print_warning
set "msg=[WARNING] %~1"
echo !msg!
echo %time% !msg! >> "%LOG_FILE%"
exit /b 0

:print_error
set "msg=[ERROR] %~1"
echo !msg!
echo %time% !msg! >> "%LOG_FILE%"
exit /b 0

:print_separator
echo ===============================================
echo %time% =============================================== >> "%LOG_FILE%"
exit /b 0

:show_help
echo.
echo 用法: run_tests.bat [选项]
echo.
echo 选项:
echo   --run-only    只运行已编译的测试，不重新编译
echo   --clean       清理之前的构建并重新编译
echo   --help        显示此帮助信息
echo.
echo 说明:
echo   此脚本用于编译并运行所有 Backtrader C++ 测试
echo   - 测试超时: 30秒
echo   - 日志位置: build_tests\test_run_*.log
echo   - 测试报告: build_tests\test_report.txt
echo.
echo 前提条件:
echo   - 核心库必须已编译 (libbacktrader_core.a)
echo   - 如果核心库不存在，请先运行: ..\build.bat
echo.
echo 示例:
echo   run_tests.bat              # 编译并运行所有测试
echo   run_tests.bat --run-only   # 只运行已编译的测试
echo   run_tests.bat --clean      # 清理后重新编译并运行
echo.
exit /b 0

:check_dependencies
call :print_info "检查依赖..."

REM Check cmake
cmake --version >> "%LOG_FILE%" 2>&1
if !errorlevel! neq 0 (
    call :print_error "未找到 cmake，请安装 CMake 并添加到 PATH"
    exit /b 1
)

REM Check g++
g++ --version >> "%LOG_FILE%" 2>&1
if !errorlevel! neq 0 (
    call :print_error "未找到 g++，请安装 MinGW-w64 并添加到 PATH"
    exit /b 1
)

REM Check make command
set "MAKE_CMD="
mingw32-make --version >> "%LOG_FILE%" 2>&1
if !errorlevel! equ 0 (
    set "MAKE_CMD=mingw32-make"
) else (
    make --version >> "%LOG_FILE%" 2>&1
    if !errorlevel! equ 0 (
        set "MAKE_CMD=make"
    ) else (
        call :print_error "未找到 mingw32-make 或 make"
        exit /b 1
    )
)

REM Check git
git --version >> "%LOG_FILE%" 2>&1
if !errorlevel! equ 0 (
    call :print_info "Git 已安装"
) else (
    call :print_warning "未找到 Git，GoogleTest submodule 功能可能不可用"
)

call :print_success "依赖检查完成"
call :print_info "使用编译器: g++"
call :print_info "使用构建工具: !MAKE_CMD!"
exit /b 0

:check_core_library
call :print_info "检查核心库..."
echo. >> "%LOG_FILE%"
echo ========== Core Library Check ========== >> "%LOG_FILE%"
echo 核心库路径: %CORE_LIB% >> "%LOG_FILE%"

if not exist "%CORE_LIB%" (
    call :print_error "核心库不存在: %CORE_LIB%"
    call :print_error "请先运行: ..\build.bat 编译核心库"
    echo. >> "%LOG_FILE%"
    echo 错误: 核心库不存在 >> "%LOG_FILE%"
    echo 请先编译核心库: >> "%LOG_FILE%"
    echo   cd .. >> "%LOG_FILE%"
    echo   build.bat >> "%LOG_FILE%"
    exit /b 1
)

call :print_success "核心库存在: %CORE_LIB%"
exit /b 0

:check_googletest_submodule
call :print_info "检查 GoogleTest submodule..."

set "GOOGLETEST_DIR=%PROJECT_ROOT%\external\googletest"

REM Check if googletest submodule exists and has content
if exist "%GOOGLETEST_DIR%\CMakeLists.txt" (
    call :print_success "GoogleTest submodule 已存在"
    echo GoogleTest 路径: %GOOGLETEST_DIR% >> "%LOG_FILE%"
    exit /b 0
)

REM GoogleTest submodule doesn't exist, need to initialize
call :print_info "GoogleTest submodule 不存在，正在初始化..."

REM Check if we're in a git repository
if not exist "%PROJECT_ROOT%\.git" (
    call :print_warning "不在 Git 仓库中，CMake 将自动下载 GoogleTest"
    exit /b 0
)

REM Check if .gitmodules exists
if not exist "%PROJECT_ROOT%\.gitmodules" (
    call :print_warning ".gitmodules 文件不存在，CMake 将自动下载 GoogleTest"
    exit /b 0
)

REM Initialize and update the submodule
cd /d "%PROJECT_ROOT%"
call :print_info "运行: git submodule update --init --recursive external/googletest"
echo. >> "%LOG_FILE%"
echo ========== Git Submodule Init ========== >> "%LOG_FILE%"
git submodule update --init --recursive external/googletest >> "%LOG_FILE%" 2>&1

if !errorlevel! neq 0 (
    call :print_error "GoogleTest submodule 初始化失败"
    call :print_warning "CMake 将尝试自动下载 GoogleTest"
    cd /d "%SCRIPT_DIR%"
    exit /b 1
)

call :print_success "GoogleTest submodule 初始化成功"
cd /d "%SCRIPT_DIR%"
exit /b 0

:clean_build
call :print_info "清理构建目录..."

if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%" >> "%LOG_FILE%" 2>&1
    mkdir "%BUILD_DIR%"
)

if not exist "%TEMP_RESULTS%" mkdir "%TEMP_RESULTS%"

call :print_success "清理完成"
exit /b 0

:build_tests
call :print_info "开始编译测试..."

REM Check and initialize GoogleTest submodule if needed
call :check_googletest_submodule

cd /d "%BUILD_DIR%"

REM Configure with CMake
call :print_info "配置测试项目（使用 GCC 和 MinGW Makefiles）..."
echo. >> "%LOG_FILE%"
echo ========== CMake Configuration ========== >> "%LOG_FILE%"
cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_C_COMPILER=gcc ^
    -DCMAKE_CXX_COMPILER=g++ >> "%LOG_FILE%" 2>&1

if !errorlevel! neq 0 (
    call :print_error "CMake 配置失败"
    call :print_info "请查看日志: %LOG_FILE%"
    cd /d "%SCRIPT_DIR%"
    exit /b 1
)
call :print_success "测试配置成功"

REM Compile all test files
call :print_info "编译所有测试文件（并行编译）..."
echo. >> "%LOG_FILE%"
echo ========== Test Compilation ========== >> "%LOG_FILE%"
!MAKE_CMD! -j%NUMBER_OF_PROCESSORS% >> "%LOG_FILE%" 2>&1

set "BUILD_RESULT=!errorlevel!"
if !BUILD_RESULT! neq 0 (
    call :print_warning "部分测试编译失败"
) else (
    call :print_success "所有测试编译成功"
)

REM Collect compilation results
call :collect_test_results

cd /d "%SCRIPT_DIR%"
exit /b 0

:collect_test_results
call :print_info "收集测试编译结果..."
echo. >> "%LOG_FILE%"
echo ========== Compilation Results ========== >> "%LOG_FILE%"

REM Clear previous result files
if exist "%TEMP_RESULTS%\compile_success.txt" del /f /q "%TEMP_RESULTS%\compile_success.txt"
if exist "%TEMP_RESULTS%\compile_failed.txt" del /f /q "%TEMP_RESULTS%\compile_failed.txt"

set "COMPILED_SUCCESS=0"
set "COMPILED_FAILED=0"

REM Check compilation status of each test
for %%f in ("%SCRIPT_DIR%original_tests\test_*.cpp") do (
    set "filename=%%~nf"
    
    REM Skip test_common
    if not "!filename!"=="test_common" (
        REM Check if executable exists
        if exist "%BUILD_DIR%\!filename!.exe" (
            set /a COMPILED_SUCCESS+=1
            echo !filename!>> "%TEMP_RESULTS%\compile_success.txt"
            echo [SUCCESS] !filename! >> "%LOG_FILE%"
        ) else (
            set /a COMPILED_FAILED+=1
            echo !filename!>> "%TEMP_RESULTS%\compile_failed.txt"
            echo [FAILED] !filename! >> "%LOG_FILE%"
        )
    )
)

call :print_separator
call :print_info "编译结果统计:"
call :print_success "成功编译: !COMPILED_SUCCESS! 个测试"
if !COMPILED_FAILED! gtr 0 (
    call :print_error "编译失败: !COMPILED_FAILED! 个测试"
    
    call :print_info "编译失败的测试："
    if exist "%TEMP_RESULTS%\compile_failed.txt" (
        for /f "delims=" %%a in (%TEMP_RESULTS%\compile_failed.txt) do (
            echo   × %%a
            echo   × %%a >> "%LOG_FILE%"
        )
    )
)

exit /b 0

:run_all_tests
call :print_separator
call :print_info "开始运行所有测试..."
echo. >> "%LOG_FILE%"
echo ========== Test Execution ========== >> "%LOG_FILE%"

REM Clear test result files
if exist "%TEMP_RESULTS%\test_success.txt" del /f /q "%TEMP_RESULTS%\test_success.txt"
if exist "%TEMP_RESULTS%\test_failed.txt" del /f /q "%TEMP_RESULTS%\test_failed.txt"
if exist "%TEMP_RESULTS%\test_timeout.txt" del /f /q "%TEMP_RESULTS%\test_timeout.txt"
if exist "%TEMP_RESULTS%\test_crashed.txt" del /f /q "%TEMP_RESULTS%\test_crashed.txt"

set "TEST_SUCCESS=0"
set "TEST_FAILED=0"
set "TEST_TIMEOUT=0"
set "TEST_CRASHED=0"

REM Find all test executables
set "test_count=0"
for %%f in ("%BUILD_DIR%\test_*.exe") do (
    set /a test_count+=1
)

if !test_count! equ 0 (
    call :print_error "未找到可执行的测试文件"
    exit /b 1
)

call :print_info "找到 !test_count! 个测试可执行文件"

REM Run each test
set "current=0"
for %%f in ("%BUILD_DIR%\test_*.exe") do (
    set /a current+=1
    call :run_single_test "%%f" !current! !test_count!
)

call :print_separator
call :print_info "测试运行完成统计:"
call :print_success "测试通过: !TEST_SUCCESS! 个"
if !TEST_FAILED! gtr 0 call :print_error "测试失败: !TEST_FAILED! 个"
if !TEST_TIMEOUT! gtr 0 call :print_warning "测试超时: !TEST_TIMEOUT! 个"
if !TEST_CRASHED! gtr 0 call :print_warning "测试崩溃: !TEST_CRASHED! 个"

exit /b 0

:run_single_test
set "test_exe=%~1"
set "current=%~2"
set "total=%~3"
set "test_name=%~n1"

call :print_info "[!current!/!total!] 运行测试: !test_name!"
echo. >> "%LOG_FILE%"
echo ========== Test: !test_name! ========== >> "%LOG_FILE%"

REM Create temporary files for test output
set "output_file=%TEMP_RESULTS%\output_!test_name!.txt"
set "xml_output=%BUILD_DIR%\test_!test_name!.xml"
set "exitcode_file=%TEMP_RESULTS%\exitcode_!test_name!.txt"

REM Timeout in seconds
set "timeout_seconds=30"

REM Run test with timeout
call :run_with_timeout "%test_exe%" "%output_file%" "%xml_output%" "%exitcode_file%" !timeout_seconds!
set "run_result=!errorlevel!"

REM Check test result
if "!run_result!"=="124" (
    REM Timeout
    call :print_error "  └─ 测试超时 (30秒)"
    echo [TIMEOUT] !test_name! >> "%LOG_FILE%"
    set /a TEST_TIMEOUT+=1
    set /a TEST_FAILED+=1
    echo !test_name!>> "%TEMP_RESULTS%\test_timeout.txt"
    echo !test_name!>> "%TEMP_RESULTS%\test_failed.txt"
) else if "!run_result!"=="125" (
    REM Crashed
    call :print_error "  └─ 测试崩溃"
    echo [CRASHED] !test_name! >> "%LOG_FILE%"
    set /a TEST_CRASHED+=1
    set /a TEST_FAILED+=1
    echo !test_name!>> "%TEMP_RESULTS%\test_crashed.txt"
    echo !test_name!>> "%TEMP_RESULTS%\test_failed.txt"
) else (
    REM Check exit code
    if exist "%xml_output%" (
        REM Check if there are failures in XML
        findstr /c:"failures=\"0\"" "%xml_output%" >nul
        if !errorlevel! equ 0 (
            findstr /c:"errors=\"0\"" "%xml_output%" >nul
            if !errorlevel! equ 0 (
                REM Success
                call :print_success "  └─ 测试通过"
                echo [PASSED] !test_name! >> "%LOG_FILE%"
                set /a TEST_SUCCESS+=1
                echo !test_name!>> "%TEMP_RESULTS%\test_success.txt"
            ) else (
                REM Has errors
                call :print_error "  └─ 测试失败 (有错误)"
                echo [FAILED] !test_name! (errors) >> "%LOG_FILE%"
                set /a TEST_FAILED+=1
                echo !test_name!>> "%TEMP_RESULTS%\test_failed.txt"
            )
        ) else (
            REM Has failures
            call :print_error "  └─ 测试失败"
            echo [FAILED] !test_name! >> "%LOG_FILE%"
            set /a TEST_FAILED+=1
            echo !test_name!>> "%TEMP_RESULTS%\test_failed.txt"
        )
    ) else if exist "%output_file%" (
        REM No XML output, check output file
        findstr /c:"[  PASSED  ]" "%output_file%" >nul
        if !errorlevel! equ 0 (
            findstr /c:"[  FAILED  ]" "%output_file%" >nul
            if !errorlevel! equ 0 (
                call :print_error "  └─ 测试失败"
                echo [FAILED] !test_name! >> "%LOG_FILE%"
                set /a TEST_FAILED+=1
                echo !test_name!>> "%TEMP_RESULTS%\test_failed.txt"
            ) else (
                call :print_success "  └─ 测试通过"
                echo [PASSED] !test_name! >> "%LOG_FILE%"
                set /a TEST_SUCCESS+=1
                echo !test_name!>> "%TEMP_RESULTS%\test_success.txt"
            )
        ) else (
            call :print_error "  └─ 测试失败 (无输出)"
            echo [FAILED] !test_name! (no output) >> "%LOG_FILE%"
            set /a TEST_FAILED+=1
            echo !test_name!>> "%TEMP_RESULTS%\test_failed.txt"
        )
    ) else (
        call :print_error "  └─ 测试失败 (无输出文件)"
        echo [FAILED] !test_name! (no output file) >> "%LOG_FILE%"
        set /a TEST_FAILED+=1
        echo !test_name!>> "%TEMP_RESULTS%\test_failed.txt"
    )
)

REM Log test output if it exists
if exist "%output_file%" (
    echo. >> "%LOG_FILE%"
    echo --- Test Output Start --- >> "%LOG_FILE%"
    type "%output_file%" >> "%LOG_FILE%" 2>&1
    echo --- Test Output End --- >> "%LOG_FILE%"
)

exit /b 0

:run_with_timeout
set "exe_path=%~1"
set "output_file=%~2"
set "xml_output=%~3"
set "exitcode_file=%~4"
set "timeout_sec=%~5"

REM Create VBScript for timeout control
set "vbs_file=%TEMP_RESULTS%\run_timeout.vbs"
(
echo Set WshShell = CreateObject^("WScript.Shell"^)
echo Set objFSO = CreateObject^("Scripting.FileSystemObject"^)
echo.
echo strCommand = "%exe_path% --gtest_output=xml:%xml_output% > %output_file% 2>&1"
echo Set objProcess = WshShell.Exec^(strCommand^)
echo.
echo ' Wait for process with timeout
echo intTimeout = %timeout_sec% * 1000
echo intElapsed = 0
echo intInterval = 100
echo.
echo Do While objProcess.Status = 0 And intElapsed ^< intTimeout
echo     WScript.Sleep intInterval
echo     intElapsed = intElapsed + intInterval
echo Loop
echo.
echo ' Check if process is still running
echo If objProcess.Status = 0 Then
echo     ' Timeout - terminate process
echo     WshShell.Run "taskkill /F /PID " ^& objProcess.ProcessID, 0, False
echo     WScript.Sleep 500
echo     WScript.Quit 124  ' Timeout exit code
echo Else
echo     ' Process finished normally
echo     intExitCode = objProcess.ExitCode
echo     If intExitCode ^< 0 Or intExitCode ^>= 3221225472 Then
echo         WScript.Quit 125  ' Crash exit code
echo     Else
echo         WScript.Quit intExitCode
echo     End If
echo End If
) > "%vbs_file%"

REM Run VBScript
cscript //Nologo "%vbs_file%"
set "result=!errorlevel!"

exit /b !result!

:generate_report
call :print_separator
call :print_info "生成详细报告..."
echo. >> "%LOG_FILE%"
echo ========== Report Generation ========== >> "%LOG_FILE%"

set "REPORT_FILE=%BUILD_DIR%\test_report.txt"

echo ============================================= > "%REPORT_FILE%"
echo Backtrader C++ 测试报告 >> "%REPORT_FILE%"
echo ============================================= >> "%REPORT_FILE%"
echo 生成时间: %date% %time% >> "%REPORT_FILE%"
echo. >> "%REPORT_FILE%"

echo ========================= 编译结果 ========================= >> "%REPORT_FILE%"
echo 成功编译的测试 (!COMPILED_SUCCESS!个): >> "%REPORT_FILE%"
if exist "%TEMP_RESULTS%\compile_success.txt" (
    for /f "delims=" %%a in (%TEMP_RESULTS%\compile_success.txt) do (
        echo   √ %%a >> "%REPORT_FILE%"
    )
) else (
    echo   (无数据) >> "%REPORT_FILE%"
)

echo. >> "%REPORT_FILE%"
echo 编译失败的测试 (!COMPILED_FAILED!个): >> "%REPORT_FILE%"
if exist "%TEMP_RESULTS%\compile_failed.txt" (
    for /f "delims=" %%a in (%TEMP_RESULTS%\compile_failed.txt) do (
        echo   × %%a >> "%REPORT_FILE%"
    )
) else (
    echo   (无数据) >> "%REPORT_FILE%"
)

echo. >> "%REPORT_FILE%"
echo ========================= 测试结果 ========================= >> "%REPORT_FILE%"
echo 测试通过 (!TEST_SUCCESS!个): >> "%REPORT_FILE%"
if exist "%TEMP_RESULTS%\test_success.txt" (
    for /f "delims=" %%a in (%TEMP_RESULTS%\test_success.txt) do (
        echo   √ %%a >> "%REPORT_FILE%"
    )
) else (
    echo   (无数据) >> "%REPORT_FILE%"
)

echo. >> "%REPORT_FILE%"
echo 测试失败 (!TEST_FAILED!个): >> "%REPORT_FILE%"
if exist "%TEMP_RESULTS%\test_failed.txt" (
    for /f "delims=" %%a in (%TEMP_RESULTS%\test_failed.txt) do (
        echo   × %%a >> "%REPORT_FILE%"
    )
) else (
    echo   (无数据) >> "%REPORT_FILE%"
)

if !TEST_TIMEOUT! gtr 0 (
    echo. >> "%REPORT_FILE%"
    echo 测试超时 (!TEST_TIMEOUT!个): >> "%REPORT_FILE%"
    if exist "%TEMP_RESULTS%\test_timeout.txt" (
        for /f "delims=" %%a in (%TEMP_RESULTS%\test_timeout.txt) do (
            echo   [超时] %%a >> "%REPORT_FILE%"
        )
    )
)

if !TEST_CRASHED! gtr 0 (
    echo. >> "%REPORT_FILE%"
    echo 测试崩溃 (!TEST_CRASHED!个): >> "%REPORT_FILE%"
    if exist "%TEMP_RESULTS%\test_crashed.txt" (
        for /f "delims=" %%a in (%TEMP_RESULTS%\test_crashed.txt) do (
            echo   [崩溃] %%a >> "%REPORT_FILE%"
        )
    )
)

echo. >> "%REPORT_FILE%"
echo ============================================= >> "%REPORT_FILE%"
echo 报告结束 >> "%REPORT_FILE%"
echo ============================================= >> "%REPORT_FILE%"

call :print_success "报告已生成: %REPORT_FILE%"
exit /b 0

:save_report
REM Display report to console
echo.
type "%REPORT_FILE%"
echo.
exit /b 0

