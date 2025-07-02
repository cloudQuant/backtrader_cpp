/**
 * @file main_original_tests_simple.cpp
 * @brief 原始测试的简化主入口
 */

#include <gtest/gtest.h>
#include <iostream>

int main(int argc, char** argv) {
    std::cout << "=== Backtrader C++ Original Tests (Simplified) ===" << std::endl;
    std::cout << "Running tests to verify compatibility with Python backtrader..." << std::endl;
    
    ::testing::InitGoogleTest(&argc, argv);
    
    // 设置测试环境
    ::testing::GTEST_FLAG(color) = "yes";
    ::testing::GTEST_FLAG(print_time) = true;
    
    int result = RUN_ALL_TESTS();
    
    if (result == 0) {
        std::cout << "\n=== All tests PASSED! ===" << std::endl;
        std::cout << "C++ implementation appears to be compatible with Python backtrader." << std::endl;
    } else {
        std::cout << "\n=== Some tests FAILED! ===" << std::endl;
        std::cout << "Further investigation needed to match Python behavior." << std::endl;
    }
    
    return result;
}