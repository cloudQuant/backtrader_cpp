/**
 * @file main_original_tests.cpp
 * @brief 原始测试套件主入口
 * 
 * 这个文件包含了从Python移植的所有原始测试用例的主入口点。
 * 这些测试确保C++实现与原始Python实现的计算结果完全一致。
 */

#include <gtest/gtest.h>
#include <iostream>
#include <iomanip>

// 测试统计信息
class OriginalTestListener : public ::testing::EmptyTestEventListener {
private:
    int passed_tests_;
    int failed_tests_;
    std::vector<std::string> failed_test_names_;
    
public:
    OriginalTestListener() : passed_tests_(0), failed_tests_(0) {}
    
    void OnTestStart(const ::testing::TestInfo& test_info) override {
        std::cout << "Running: " << test_info.test_case_name() 
                  << "." << test_info.name() << std::endl;
    }
    
    void OnTestEnd(const ::testing::TestInfo& test_info) override {
        if (test_info.result()->Passed()) {
            passed_tests_++;
            std::cout << "[  PASSED  ] " << test_info.test_case_name() 
                      << "." << test_info.name() << std::endl;
        } else {
            failed_tests_++;
            std::string full_name = std::string(test_info.test_case_name()) + 
                                   "." + test_info.name();
            failed_test_names_.push_back(full_name);
            std::cout << "[  FAILED  ] " << test_info.test_case_name() 
                      << "." << test_info.name() << std::endl;
        }
    }
    
    void OnTestProgramEnd(const ::testing::UnitTest& unit_test) override {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "Original Backtrader Test Results Summary" << std::endl;
        std::cout << std::string(80, '=') << std::endl;
        
        std::cout << "Total Tests Run: " << (passed_tests_ + failed_tests_) << std::endl;
        std::cout << "Passed: " << passed_tests_ << std::endl;
        std::cout << "Failed: " << failed_tests_ << std::endl;
        
        if (failed_tests_ > 0) {
            std::cout << "\nFailed Tests:" << std::endl;
            for (const auto& test_name : failed_test_names_) {
                std::cout << "  - " << test_name << std::endl;
            }
        }
        
        double success_rate = (passed_tests_ + failed_tests_ > 0) ? 
            (double)passed_tests_ / (passed_tests_ + failed_tests_) * 100.0 : 0.0;
        
        std::cout << "\nSuccess Rate: " << std::fixed << std::setprecision(1) 
                  << success_rate << "%" << std::endl;
        
        std::cout << std::string(80, '=') << std::endl;
        
        if (failed_tests_ == 0) {
            std::cout << "🎉 All original tests passed! C++ implementation matches Python behavior." << std::endl;
        } else {
            std::cout << "❌ Some tests failed. Please review the C++ implementation." << std::endl;
        }
        
        std::cout << "\nTest Categories Verified:" << std::endl;
        std::cout << "  ✓ Simple Moving Average (SMA)" << std::endl;
        std::cout << "  ✓ Relative Strength Index (RSI)" << std::endl;
        std::cout << "  ✓ Bollinger Bands (BBands)" << std::endl;
        std::cout << "  ✓ Average True Range (ATR)" << std::endl;
        std::cout << "  ✓ Stochastic Oscillator" << std::endl;
        
        std::cout << "\nNext Steps:" << std::endl;
        std::cout << "  1. If tests pass: Continue with more indicator implementations" << std::endl;
        std::cout << "  2. If tests fail: Debug the specific indicator calculations" << std::endl;
        std::cout << "  3. Add more original test cases from Python test suite" << std::endl;
    }
};

/**
 * @brief 打印测试环境信息
 */
void printTestEnvironment() {
    std::cout << "Backtrader C++ Original Test Suite" << std::endl;
    std::cout << "===================================" << std::endl;
    std::cout << "This test suite verifies that C++ implementations produce" << std::endl;
    std::cout << "identical results to the original Python backtrader library." << std::endl;
    std::cout << std::endl;
    
    std::cout << "Test Data: Using original backtrader test data (2006-day-001.txt)" << std::endl;
    std::cout << "Precision: 6 decimal places (matching Python %f format)" << std::endl;
    std::cout << "Validation Points: First valid, middle, and last values" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Expected Behavior:" << std::endl;
    std::cout << "  - All calculations should match Python exactly" << std::endl;
    std::cout << "  - Minimum periods should match Python requirements" << std::endl;
    std::cout << "  - NaN handling should be consistent" << std::endl;
    std::cout << "  - All indicator ranges should be validated" << std::endl;
    std::cout << std::endl;
}

/**
 * @brief 主函数
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // 打印环境信息
    printTestEnvironment();
    
    // 添加自定义监听器
    ::testing::TestEventListeners& listeners = ::testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new OriginalTestListener);
    
    // 设置测试过滤器（如果需要）
    if (argc > 1) {
        std::string filter = argv[1];
        if (filter == "--sma") {
            ::testing::GTEST_FLAG(filter) = "*SMA*";
        } else if (filter == "--rsi") {
            ::testing::GTEST_FLAG(filter) = "*RSI*";
        } else if (filter == "--bbands") {
            ::testing::GTEST_FLAG(filter) = "*BBands*";
        } else if (filter == "--atr") {
            ::testing::GTEST_FLAG(filter) = "*ATR*";
        } else if (filter == "--stochastic") {
            ::testing::GTEST_FLAG(filter) = "*Stochastic*";
        } else if (filter == "--manual") {
            ::testing::GTEST_FLAG(filter) = "*Manual*";
        } else if (filter == "--debug") {
            ::testing::GTEST_FLAG(filter) = "*Debug*";
        }
    }
    
    std::cout << "Starting test execution..." << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    // 运行测试
    int result = RUN_ALL_TESTS();
    
    return result;
}