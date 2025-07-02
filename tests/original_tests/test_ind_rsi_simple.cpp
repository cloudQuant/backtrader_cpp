/**
 * @file test_ind_rsi_simple.cpp
 * @brief RSI指标测试 - 简化版本
 */

#include "test_common_simple.h"
#include "indicators/RSI.h"

using namespace backtrader::tests::original;
using namespace backtrader;

namespace {

// 来自Python版本的期望值
const std::vector<std::vector<std::string>> RSI_EXPECTED_VALUES = {
    {"57.644284", "41.630968", "53.352553"}
};

const int RSI_MIN_PERIOD = 15; // RSI(14) + 1

} // anonymous namespace

// 使用默认参数的RSI测试
DEFINE_INDICATOR_TEST(RSI_Default, RSI, RSI_EXPECTED_VALUES, RSI_MIN_PERIOD)

// 手动测试函数，用于调试
TEST(OriginalTests, RSI_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    // 创建RSI指标（默认14周期）
    auto rsi = std::make_shared<RSI>(close_line, 14);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        close_line->forward(csv_data[i].close);
        rsi->calculate();
    }
    
    // 验证基本属性
    EXPECT_EQ(rsi->getMinPeriod(), 15) << "RSI minimum period should be 15";
    EXPECT_EQ(rsi->getPeriod(), 14) << "RSI period should be 14";
    
    // 验证最后的值不是NaN且在合理范围内
    double last_value = rsi->get(0);
    EXPECT_FALSE(std::isnan(last_value)) << "Last RSI value should not be NaN";
    EXPECT_GE(last_value, 0.0) << "RSI value should be >= 0";
    EXPECT_LE(last_value, 100.0) << "RSI value should be <= 100";
}

// 边界条件测试
TEST(OriginalTests, RSI_EdgeCases) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    // 只添加几个数据点
    for (size_t i = 0; i < std::min(size_t(10), csv_data.size()); ++i) {
        close_line->forward(csv_data[i].close);
    }
    
    // 测试数据不足的情况
    auto rsi = std::make_shared<RSI>(close_line, 14);
    
    for (int i = 0; i < 10; ++i) {
        rsi->calculate();
        if (i < 9) {
            close_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = rsi->get(0);
    EXPECT_TRUE(std::isnan(result)) << "RSI should return NaN when insufficient data";
}

// 测试不同参数
TEST(OriginalTests, RSI_DifferentPeriods) {
    auto csv_data = getdata(0);
    
    std::vector<int> periods = {7, 14, 21};
    
    for (int period : periods) {
        // 为每个测试重新创建数据线
        auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
        auto rsi = std::make_shared<RSI>(close_line, period);
        
        // 计算所有值
        for (size_t i = 0; i < csv_data.size(); ++i) {
            close_line->forward(csv_data[i].close);
            rsi->calculate();
        }
        
        EXPECT_EQ(rsi->getPeriod(), period) << "RSI period should match parameter";
        EXPECT_EQ(rsi->getMinPeriod(), period + 1) << "RSI minimum period should be period + 1";
        
        // 如果有足够的数据，最后的值应该不是NaN
        if (csv_data.size() >= static_cast<size_t>(period + 1)) {
            double last_value = rsi->get(0);
            EXPECT_FALSE(std::isnan(last_value)) << "Last RSI value should not be NaN for period " << period;
            EXPECT_GE(last_value, 0.0) << "RSI value should be >= 0";
            EXPECT_LE(last_value, 100.0) << "RSI value should be <= 100";
        }
    }
}