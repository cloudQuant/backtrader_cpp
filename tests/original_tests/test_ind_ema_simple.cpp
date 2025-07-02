/**
 * @file test_ind_ema_simple.cpp
 * @brief EMA指标测试 - 简化版本
 */

#include "test_common_simple.h"
#include "indicators/EMA.h"

using namespace backtrader::tests::original;
using namespace backtrader;

namespace {

// 来自Python版本的期望值
const std::vector<std::vector<std::string>> EMA_EXPECTED_VALUES = {
    {"4070.115719", "3644.444667", "3581.728712"}
};

const int EMA_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的EMA测试
DEFINE_INDICATOR_TEST(EMA_Default, EMA, EMA_EXPECTED_VALUES, EMA_MIN_PERIOD)

// 手动测试函数，用于调试
TEST(OriginalTests, EMA_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建EMA指标（30周期）
    auto ema = std::make_shared<EMA>(close_line, 30);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ema->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证基本属性
    EXPECT_EQ(ema->getMinPeriod(), 30) << "EMA minimum period should be 30";
    EXPECT_EQ(ema->getPeriod(), 30) << "EMA period should be 30";
    
    // 验证最后的值不是NaN
    double last_value = ema->get(0);
    EXPECT_FALSE(std::isnan(last_value)) << "Last EMA value should not be NaN";
}

// 边界条件测试
TEST(OriginalTests, EMA_EdgeCases) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    // 只添加少量数据点
    for (size_t i = 0; i < std::min(size_t(25), csv_data.size()); ++i) {
        close_line->forward(csv_data[i].close);
    }
    
    // 测试数据不足的情况
    auto ema = std::make_shared<EMA>(close_line, 30);
    
    for (int i = 0; i < 25; ++i) {
        ema->calculate();
        if (i < 24) {
            close_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = ema->get(0);
    EXPECT_TRUE(std::isnan(result)) << "EMA should return NaN when insufficient data";
}

// 测试不同参数
TEST(OriginalTests, EMA_DifferentPeriods) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    std::vector<int> periods = {5, 10, 20, 30, 50};
    
    for (int period : periods) {
        auto ema = std::make_shared<EMA>(close_line, period);
        
        // 计算所有值
        for (size_t i = 0; i < csv_data.size(); ++i) {
            ema->calculate();
            if (i < csv_data.size() - 1) {
                close_line->forward();
            }
        }
        
        EXPECT_EQ(ema->getPeriod(), period) << "EMA period should match parameter";
        EXPECT_EQ(ema->getMinPeriod(), period) << "EMA minimum period should equal period";
        
        // 如果有足够的数据，最后的值应该不是NaN
        if (csv_data.size() >= static_cast<size_t>(period)) {
            double last_value = ema->get(0);
            EXPECT_FALSE(std::isnan(last_value)) 
                << "Last EMA value should not be NaN for period " << period;
        }
        
        // 重置数据线位置
        close_line->home();
    }
}