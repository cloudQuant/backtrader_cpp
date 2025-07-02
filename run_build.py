#!/usr/bin/env python3
"""
Manual build script for backtrader C++ project
"""

import os
import subprocess
import sys
import shutil

def run_command(cmd, cwd=None, check=True):
    """Run a command and return output"""
    print(f"Running: {cmd}")
    if cwd:
        print(f"Working directory: {cwd}")
    
    try:
        result = subprocess.run(
            cmd, 
            shell=True, 
            cwd=cwd, 
            capture_output=True, 
            text=True, 
            check=check
        )
        
        if result.stdout:
            print("STDOUT:", result.stdout)
        if result.stderr:
            print("STDERR:", result.stderr)
            
        return result
    except subprocess.CalledProcessError as e:
        print(f"Command failed with return code {e.returncode}")
        if e.stdout:
            print("STDOUT:", e.stdout)
        if e.stderr:
            print("STDERR:", e.stderr)
        raise

def main():
    """Main build process"""
    project_root = "/home/yun/Documents/refactor_backtrader/backtrader_cpp"
    build_dir = os.path.join(project_root, "build")
    
    print("=== Starting C++ project build process ===")
    
    # Check if we're in the right directory
    if not os.path.exists(os.path.join(project_root, "CMakeLists.txt")):
        print(f"Error: CMakeLists.txt not found in {project_root}")
        return 1
    
    # Step 1: Clean and create build directory
    print("\n1. Cleaning existing build files...")
    if os.path.exists(build_dir):
        shutil.rmtree(build_dir)
    os.makedirs(build_dir)
    
    # Step 2: Check dependencies
    print("\n2. Checking dependencies...")
    
    # Check cmake
    try:
        result = run_command("cmake --version")
        print("CMake found:", result.stdout.split('\n')[0])
    except:
        print("ERROR: cmake not found")
        return 1
    
    # Check compiler
    try:
        result = run_command("g++ --version")
        print("GCC found:", result.stdout.split('\n')[0])
    except:
        try:
            result = run_command("clang++ --version")
            print("Clang found:", result.stdout.split('\n')[0])
        except:
            print("ERROR: No C++ compiler found")
            return 1
    
    # Step 3: Run CMake configuration
    print("\n3. Running CMake configuration...")
    cmake_cmd = [
        "cmake", "..",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DBUILD_TESTS=ON",
        "-DBUILD_PYTHON_BINDINGS=OFF",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    ]
    
    try:
        run_command(" ".join(cmake_cmd), cwd=build_dir)
        print("CMake configuration successful")
    except Exception as e:
        print(f"CMake configuration failed: {e}")
        return 1
    
    # Step 4: Build core library
    print("\n4. Building core library...")
    try:
        run_command("make backtrader_core -j$(nproc)", cwd=build_dir)
        print("Core library build successful")
    except Exception as e:
        print(f"Core library build failed: {e}")
        return 1
    
    # Step 5: Build tests
    print("\n5. Building tests...")
    try:
        run_command("make simple_test", cwd=build_dir)
        print("Test build successful")
    except Exception as e:
        print(f"Test build failed: {e}")
        # Don't return error here, continue to show what we built
    
    # Step 6: List what was built
    print("\n6. Listing build artifacts...")
    for root, dirs, files in os.walk(build_dir):
        for file in files:
            if file.endswith(('.so', '.a', '.exe', '.out')) or 'test' in file.lower():
                print(f"Built: {os.path.join(root, file)}")
    
    # Step 7: Run tests if available
    print("\n7. Running tests...")
    test_executable = os.path.join(build_dir, "tests", "simple_test")
    if os.path.exists(test_executable):
        try:
            run_command(test_executable)
            print("Tests executed successfully")
        except Exception as e:
            print(f"Test execution failed: {e}")
    else:
        print("Test executable not found, skipping test execution")
    
    print("\n=== Build process completed ===")
    return 0

if __name__ == "__main__":
    sys.exit(main())