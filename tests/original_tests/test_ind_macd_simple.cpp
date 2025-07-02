/**
 * @file test_ind_macd_simple.cpp
 * @brief MACD指标测试 - 简化版本
 */

#include "test_common_simple.h"
#include "indicators/MACD.h"

using namespace backtrader::tests::original;
using namespace backtrader;

namespace {

// 来自Python版本的期望值 - MACD histogram
const std::vector<std::vector<std::string>> MACD_EXPECTED_VALUES = {
    {"3.843516", "5.999669", "4.618090"}
};

const int MACD_MIN_PERIOD = 34; // MACD histogram的最小周期

} // anonymous namespace

// MACD histogram测试 - 使用自定义测试而非宏
TEST(OriginalTests, MACD_Default) {
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        close_line->forward(csv_data[i].close);
        macd->calculate();
    }
    
    // 验证最小周期
    EXPECT_EQ(macd->getMinPeriod(), MACD_MIN_PERIOD) << "MACD minimum period should be " << MACD_MIN_PERIOD;
    
    // 验证histogram值(第2条线，索引2)
    int data_length = static_cast<int>(csv_data.size());
    std::vector<int> check_points = {
        0,
        -(data_length - MACD_MIN_PERIOD),
        static_cast<int>(std::floor(static_cast<double>(-(data_length - MACD_MIN_PERIOD)) / 2.0))
    };
    
    const auto& expected_vals = MACD_EXPECTED_VALUES[0];
    for (size_t i = 0; i < check_points.size() && i < expected_vals.size(); ++i) {
        double actual_val = macd->getHistogram(check_points[i]);  // 测试histogram线
        
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual_val;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_vals[i]) 
            << "MACD histogram mismatch at point " << i 
            << " (actual: " << actual_str << ", expected: " << expected_vals[i] << ")";
    }
}

TEST(OriginalTests, MACD_Default_Debug) {
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        close_line->forward(csv_data[i].close);
        macd->calculate();
    }
    
    int data_length = static_cast<int>(csv_data.size());
    std::vector<int> check_points = {
        0,
        -(data_length - MACD_MIN_PERIOD),
        static_cast<int>(std::floor(static_cast<double>(-(data_length - MACD_MIN_PERIOD)) / 2.0))
    };
    
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "Data length: " << data_length << std::endl;
    std::cout << "Minimum period: " << MACD_MIN_PERIOD << std::endl;
    std::cout << "Check points: ";
    for (int cp : check_points) {
        std::cout << cp << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Actual histogram values:" << std::endl;
    for (size_t i = 0; i < check_points.size(); ++i) {
        double val = macd->getHistogram(check_points[i]);
        std::cout << "  [" << i << "] = " << std::fixed << std::setprecision(6) << val << std::endl;
    }
    
    std::cout << "Expected histogram values:" << std::endl;
    const auto& expected_vals = MACD_EXPECTED_VALUES[0];
    for (size_t i = 0; i < expected_vals.size(); ++i) {
        std::cout << "  [" << i << "] = " << expected_vals[i] << std::endl;
    }
}

// 手动测试函数，用于调试
TEST(OriginalTests, MACD_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    // 创建MACD指标（默认12,26,9）
    auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
    
    // 按照backtrader的方式进行计算：先添加数据，再计算
    for (size_t i = 0; i < csv_data.size(); ++i) {
        close_line->forward(csv_data[i].close);
        macd->calculate();
    }
    
    // 验证基本属性
    EXPECT_EQ(macd->getMinPeriod(), 34) << "MACD minimum period should be 34";
    
    // 验证最后的值不是NaN
    double last_value = macd->get(0);
    EXPECT_FALSE(std::isnan(last_value)) << "Last MACD value should not be NaN";
}

// 边界条件测试
TEST(OriginalTests, MACD_EdgeCases) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    // 测试数据不足的情况
    auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
    
    // 只添加少量数据点
    for (size_t i = 0; i < std::min(size_t(20), csv_data.size()); ++i) {
        close_line->forward(csv_data[i].close);
        macd->calculate();
    }
    
    // 数据不足时应该返回NaN
    double result = macd->get(0);
    EXPECT_TRUE(std::isnan(result)) << "MACD should return NaN when insufficient data";
}