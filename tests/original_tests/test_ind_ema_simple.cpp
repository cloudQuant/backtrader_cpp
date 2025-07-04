/**
 * @file test_ind_ema_simple.cpp
 * @brief EMA指标测试 - 简化版本
 */

#include "test_common_simple.h"
#include "indicators/ema.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

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
    
    // 创建数据线系列
    auto close_line_series = std::make_shared<LineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    close_line_series->lines->add_alias("close", 0);
    
    // 逐步添加数据到线缓冲区
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    if (close_buffer) {
        for (size_t i = 0; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    // 创建EMA指标（30周期）
    auto ema = std::make_shared<EMA>(close_line_series, 30);
    
    // 计算所有值
    ema->calculate();
    
    // 验证基本属性
    EXPECT_EQ(ema->getMinPeriod(), 30) << "EMA minimum period should be 30";
    
    // 验证最后的值不是NaN
    double last_value = ema->get(0);
    EXPECT_FALSE(std::isnan(last_value)) << "Last EMA value should not be NaN";
}

// 边界条件测试
TEST(OriginalTests, EMA_EdgeCases) {
    auto csv_data = getdata(0);
    auto close_line_series = std::make_shared<LineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    close_line_series->lines->add_alias("close", 0);
    
    // 只添加少量数据点
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    if (close_buffer) {
        for (size_t i = 0; i < std::min(size_t(25), csv_data.size()); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    // 测试数据不足的情况
    auto ema = std::make_shared<EMA>(close_line_series, 30);
    ema->calculate();
    
    // EMA should start immediately since minperiod is 1
    double result = ema->get(0);
    EXPECT_FALSE(std::isnan(result)) << "EMA should not return NaN even with few data points";
}

// 测试不同参数
TEST(OriginalTests, EMA_DifferentPeriods) {
    auto csv_data = getdata(0);
    
    std::vector<int> periods = {5, 10, 20, 30, 50};
    
    for (int period : periods) {
        auto close_line_series = std::make_shared<LineSeries>();
        close_line_series->lines->add_line(std::make_shared<LineBuffer>());
        close_line_series->lines->add_alias("close", 0);
        
        auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
        if (close_buffer) {
            for (size_t i = 0; i < csv_data.size(); ++i) {
                close_buffer->append(csv_data[i].close);
            }
        }
        
        auto ema = std::make_shared<EMA>(close_line_series, period);
        ema->calculate();
        
        EXPECT_EQ(ema->getMinPeriod(), period) << "EMA minimum period should equal period";
        
        // 最后的值应该不是NaN
        double last_value = ema->get(0);
        EXPECT_FALSE(std::isnan(last_value)) 
            << "Last EMA value should not be NaN for period " << period;
    }
}