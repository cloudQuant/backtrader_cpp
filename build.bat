@echo off
REM ============================================================================
REM Backtrader C++ Core Library Build Script for Windows
REM 
REM Purpose: Build libbacktrader_core library (supports static and shared libs)
REM Usage: build.bat [--shared] [--static] [--both] [--clean] [--help]
REM ============================================================================

REM Switch to UTF-8 code page for proper Chinese character display
chcp 65001 >nul 2>&1

setlocal enabledelayedexpansion

REM ========== Global Variables ==========
set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%SCRIPT_DIR%build"
set "INCLUDE_DIR=%SCRIPT_DIR%include"
set "SRC_DIR=%SCRIPT_DIR%src"

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Setup log file with timestamp
for /f "tokens=2 delims==" %%a in ('wmic OS Get localdatetime /value') do set "dt=%%a"
set "YYYY=%dt:~0,4%"
set "MM=%dt:~4,2%"
set "DD=%dt:~6,2%"
set "HH=%dt:~8,2%"
set "Min=%dt:~10,2%"
set "Sec=%dt:~12,2%"

set "LOG_FILE=%BUILD_DIR%\build_%YYYY%%MM%%DD%_%HH%%Min%%Sec%.log"
echo ===================================== > "%LOG_FILE%"
echo Backtrader C++ Core Library 构建日志 >> "%LOG_FILE%"
echo 开始时间: %date% %time% >> "%LOG_FILE%"
echo ===================================== >> "%LOG_FILE%"
echo. >> "%LOG_FILE%"

REM ========== Parse Arguments ==========
set "CLEAN_BUILD=0"
set "BUILD_MODE=static"
set "BUILD_BOTH=0"

:parse_args
if "%~1"=="" goto :args_done
if /i "%~1"=="--shared" (
    set "BUILD_MODE=shared"
    shift
    goto :parse_args
)
if /i "%~1"=="--static" (
    set "BUILD_MODE=static"
    shift
    goto :parse_args
)
if /i "%~1"=="--both" (
    set "BUILD_BOTH=1"
    shift
    goto :parse_args
)
if /i "%~1"=="--clean" (
    set "CLEAN_BUILD=1"
    shift
    goto :parse_args
)
if /i "%~1"=="--help" (
    call :show_help
    exit /b 0
)
call :print_error "未知选项: %~1"
call :show_help
exit /b 1

:args_done

REM ========== Main Script ==========
call :print_info "Backtrader C++ 核心库构建脚本"
call :print_info "脚本位置: %SCRIPT_DIR%"
call :print_info "构建目录: %BUILD_DIR%"
call :print_separator

REM Check dependencies
call :check_dependencies
if !errorlevel! neq 0 exit /b 1

REM Clean build if requested
if "!CLEAN_BUILD!"=="1" (
    call :clean_build
)

REM Build libraries
if "!BUILD_BOTH!"=="1" (
    call :print_info "构建模式: 同时编译静态库和动态库"
    
    REM Build static library
    call :print_separator
    call :print_info "=== 编译静态库 ==="
    call :build_library "static"
    if !errorlevel! neq 0 (
        call :print_error "静态库编译失败"
        exit /b 1
    )
    
    REM Build shared library
    call :print_separator
    call :print_info "=== 编译动态库 ==="
    call :build_library "shared"
    if !errorlevel! neq 0 (
        call :print_error "动态库编译失败"
        exit /b 1
    )
) else (
    if "!BUILD_MODE!"=="shared" (
        call :print_info "构建模式: 动态库"
    ) else (
        call :print_info "构建模式: 静态库"
    )
    call :build_library "!BUILD_MODE!"
    if !errorlevel! neq 0 exit /b 1
)

REM Log completion
echo. >> "%LOG_FILE%"
echo ===================================== >> "%LOG_FILE%"
echo 结束时间: %date% %time% >> "%LOG_FILE%"
echo ===================================== >> "%LOG_FILE%"

call :print_separator
call :print_success "核心库构建完成"
call :show_build_results
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
call :print_info "build.bat - Backtrader C++ Core Library Build Script"
echo.
echo Usage: build.bat [OPTIONS]
echo.
echo Options:
echo   --static       Build static library (.a) [Default]
echo   --shared       Build shared library (.dll + .dll.a)
echo   --both         Build both static and shared libraries
echo   --clean        Clean previous build and rebuild
echo   --help         Show this help message
echo.
echo Description:
echo   This script builds the Backtrader C++ core library
echo   - Static library: libbacktrader_core.a
echo   - Shared library: libbacktrader_core.dll + libbacktrader_core.dll.a
echo   - All warnings are enabled (no warnings suppressed)
echo.
echo Build Output:
echo   - Static: build/static/libbacktrader_core.a
echo   - Shared: build/shared/libbacktrader_core.dll
echo   - Root: libbacktrader_core.* (for testing)
echo.
echo Examples:
echo   build.bat                # Build static library
echo   build.bat --shared       # Build shared library
echo   build.bat --both         # Build both libraries
echo   build.bat --clean        # Clean and rebuild
echo.
exit /b 0

:check_dependencies
call :print_info "检查依赖..."
echo. >> "%LOG_FILE%"
echo ========== Dependencies Check ========== >> "%LOG_FILE%"

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
        call :print_error "未找到 mingw32-make 或 make，请安装 MinGW-w64"
        exit /b 1
    )
)

call :print_success "依赖检查完成"
call :print_info "使用编译器: g++"
call :print_info "使用构建工具: !MAKE_CMD!"
exit /b 0

:clean_build
call :print_info "清理构建目录..."
cd /d "%SCRIPT_DIR%"

REM Clean build directory
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%" >> "%LOG_FILE%" 2>&1
    mkdir "%BUILD_DIR%"
)

REM Clean old library files in root
if exist "%SCRIPT_DIR%libbacktrader_core.a" (
    del /f "%SCRIPT_DIR%libbacktrader_core.a" >> "%LOG_FILE%" 2>&1
)
if exist "%SCRIPT_DIR%libbacktrader_core.dll" (
    del /f "%SCRIPT_DIR%libbacktrader_core.dll" >> "%LOG_FILE%" 2>&1
)
if exist "%SCRIPT_DIR%libbacktrader_core.dll.a" (
    del /f "%SCRIPT_DIR%libbacktrader_core.dll.a" >> "%LOG_FILE%" 2>&1
)

call :print_success "清理完成"
exit /b 0

:build_library
set "lib_type=%~1"

REM Determine build directory and CMake options
if "%lib_type%"=="shared" (
    set "BUILD_SUBDIR=%BUILD_DIR%\shared"
    set "CMAKE_SHARED_OPT=-DBUILD_SHARED_LIBS=ON"
    call :print_info "目标: 动态库 (.dll)"
) else (
    set "BUILD_SUBDIR=%BUILD_DIR%\static"
    set "CMAKE_SHARED_OPT=-DBUILD_SHARED_LIBS=OFF"
    call :print_info "目标: 静态库 (.a)"
)

REM Create build subdirectory
if not exist "%BUILD_SUBDIR%" mkdir "%BUILD_SUBDIR%"
cd /d "%BUILD_SUBDIR%"

REM Configure with CMake
call :print_info "配置项目（使用 GCC 和 MinGW Makefiles）..."
echo. >> "%LOG_FILE%"
echo ========== CMake Configuration (%lib_type%) ========== >> "%LOG_FILE%"
cmake ..\.. -G "MinGW Makefiles" ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DBUILD_TESTS=OFF ^
    -DCMAKE_C_COMPILER=gcc ^
    -DCMAKE_CXX_COMPILER=g++ ^
    %CMAKE_SHARED_OPT% >> "%LOG_FILE%" 2>&1

if !errorlevel! neq 0 (
    call :print_error "CMake 配置失败"
    call :print_info "请查看日志: %LOG_FILE%"
    cd /d "%SCRIPT_DIR%"
    exit /b 1
)
call :print_success "项目配置成功"

REM Compile
call :print_info "编译所有源文件（并行编译，显示所有警告）..."
echo. >> "%LOG_FILE%"
echo ========== Compilation (%lib_type%) ========== >> "%LOG_FILE%"
!MAKE_CMD! -j%NUMBER_OF_PROCESSORS% >> "%LOG_FILE%" 2>&1

if !errorlevel! neq 0 (
    call :print_error "编译失败"
    call :print_warning "请查看日志文件中的警告和错误信息"
    call :print_info "完整日志: %LOG_FILE%"
    cd /d "%SCRIPT_DIR%"
    exit /b 1
)
call :print_success "编译成功"
call :print_info "提示: 所有编译警告已记录到日志文件"

REM Copy library files to root directory
call :print_info "复制库文件到根目录..."
if "%lib_type%"=="shared" (
    REM Copy DLL and import library
    if exist "libbacktrader_core.dll" (
        copy /y "libbacktrader_core.dll" "%SCRIPT_DIR%libbacktrader_core.dll" >> "%LOG_FILE%" 2>&1
        if !errorlevel! neq 0 (
            call :print_error "复制 DLL 失败"
            cd /d "%SCRIPT_DIR%"
            exit /b 1
        )
    )
    if exist "libbacktrader_core.dll.a" (
        copy /y "libbacktrader_core.dll.a" "%SCRIPT_DIR%libbacktrader_core.dll.a" >> "%LOG_FILE%" 2>&1
        if !errorlevel! neq 0 (
            call :print_error "复制导入库失败"
            cd /d "%SCRIPT_DIR%"
            exit /b 1
        )
    )
    call :print_success "动态库文件已复制到根目录"
) else (
    REM Copy static library
    if exist "libbacktrader_core.a" (
        copy /y "libbacktrader_core.a" "%SCRIPT_DIR%libbacktrader_core.a" >> "%LOG_FILE%" 2>&1
        if !errorlevel! neq 0 (
            call :print_error "复制静态库失败"
            cd /d "%SCRIPT_DIR%"
            exit /b 1
        )
        call :print_success "静态库文件已复制到根目录"
    ) else (
        call :print_error "未找到编译的静态库文件"
        cd /d "%SCRIPT_DIR%"
        exit /b 1
    )
)

cd /d "%SCRIPT_DIR%"
exit /b 0

:show_build_results
echo.
call :print_info "构建产物:"
if exist "%SCRIPT_DIR%libbacktrader_core.a" (
    echo   √ 静态库: libbacktrader_core.a
    echo   √ 静态库: libbacktrader_core.a >> "%LOG_FILE%"
)
if exist "%SCRIPT_DIR%libbacktrader_core.dll" (
    echo   √ 动态库: libbacktrader_core.dll
    echo   √ 动态库: libbacktrader_core.dll >> "%LOG_FILE%"
)
if exist "%SCRIPT_DIR%libbacktrader_core.dll.a" (
    echo   √ 导入库: libbacktrader_core.dll.a
    echo   √ 导入库: libbacktrader_core.dll.a >> "%LOG_FILE%"
)
echo.
exit /b 0
