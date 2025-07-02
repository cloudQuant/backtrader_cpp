/**
 * @file test_ind_bbands_simple.cpp
 * @brief Bollinger Bands指标测试 - 简化版本
 */

#include "test_common_simple.h"
#include "indicators/BollingerBands.h"

using namespace backtrader::tests::original;
using namespace backtrader;

namespace {

// 来自Python版本的期望值 - 中线 (SMA)
const std::vector<std::vector<std::string>> BBANDS_EXPECTED_VALUES = {
    {"4065.884000", "3621.185000", "3582.895500"}
};

const int BBANDS_MIN_PERIOD = 20; // 默认周期

} // anonymous namespace

// 使用默认参数的BollingerBands测试
DEFINE_INDICATOR_TEST(BollingerBands_Default, BollingerBands, BBANDS_EXPECTED_VALUES, BBANDS_MIN_PERIOD)

// 手动测试函数，用于调试
TEST(OriginalTests, BollingerBands_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    // 创建Bollinger Bands指标（默认20周期，2.0倍标准差）
    auto bbands = std::make_shared<BollingerBands>(close_line, 20, 2.0);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        close_line->forward(csv_data[i].close);
        bbands->calculate();
    }
    
    // 验证基本属性
    EXPECT_EQ(bbands->getMinPeriod(), 20) << "BollingerBands minimum period should be 20";
    
    // 验证最后的值不是NaN
    double last_value = bbands->get(0);
    EXPECT_FALSE(std::isnan(last_value)) << "Last BollingerBands value should not be NaN";
}

// 边界条件测试
TEST(OriginalTests, BollingerBands_EdgeCases) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    // 测试数据不足的情况
    auto bbands = std::make_shared<BollingerBands>(close_line, 20, 2.0);
    
    // 只添加少量数据点
    for (size_t i = 0; i < std::min(size_t(15), csv_data.size()); ++i) {
        close_line->forward(csv_data[i].close);
        bbands->calculate();
    }
    
    // 数据不足时应该返回NaN
    double result = bbands->get(0);
    EXPECT_TRUE(std::isnan(result)) << "BollingerBands should return NaN when insufficient data";
}

// 测试不同参数
TEST(OriginalTests, BollingerBands_DifferentParams) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    std::vector<std::pair<int, double>> params = {{10, 1.5}, {20, 2.0}, {30, 2.5}};
    
    for (const auto& [period, dev_factor] : params) {
        // 重新创建数据线为每个测试
        close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
        auto bbands = std::make_shared<BollingerBands>(close_line, period, dev_factor);
        
        // 计算所有值
        for (size_t i = 0; i < csv_data.size(); ++i) {
            close_line->forward(csv_data[i].close);
            bbands->calculate();
        }
        
        EXPECT_EQ(bbands->getMinPeriod(), period) << "BollingerBands minimum period should match parameter";
        
        // 如果有足够的数据，最后的值应该不是NaN
        if (csv_data.size() >= static_cast<size_t>(period)) {
            double last_value = bbands->get(0);
            EXPECT_FALSE(std::isnan(last_value)) 
                << "Last BollingerBands value should not be NaN for period " << period;
        }
        
    }
}