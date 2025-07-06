/**
 * @file test_ind_rsi.cpp
 * @brief RSI指标测试 - 对应Python test_ind_rsi.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['57.644284', '41.630968', '53.352553'],
 * ]
 * chkmin = 15
 * chkind = btind.RSI
 */

#include "test_common.h"

#include "indicators/rsi.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> RSI_EXPECTED_VALUES = {
    {"57.644284", "41.630968", "53.352553"}
};

const int RSI_MIN_PERIOD = 15;

} // anonymous namespace

// 使用默认参数的RSI测试
DEFINE_INDICATOR_TEST(RSI_Default, RSI, RSI_EXPECTED_VALUES, RSI_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, RSI_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建RSI指标（默认14周期，最小周期为15）
    auto rsi = std::make_shared<RSI>(close_line, 14);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        rsi->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 15;  // RSI的最小周期是period + 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"57.644284", "41.630968", "53.352553"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = rsi->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "RSI value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(rsi->getMinPeriod(), 15) << "RSI minimum period should be 15";
}

// RSI范围验证测试
TEST(OriginalTests, RSI_RangeValidation) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto rsi = std::make_shared<RSI>(close_line, 14);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        rsi->calculate();
        
        // 检查当前值是否在有效范围内
        double current_rsi = rsi->get(0);
        if (!std::isnan(current_rsi)) {
            EXPECT_GE(current_rsi, 0.0) << "RSI should be >= 0";
            EXPECT_LE(current_rsi, 100.0) << "RSI should be <= 100";
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
}

// 参数化测试 - 测试不同周期的RSI
class RSIParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line_ = std::make_shared<backtrader::LineRoot>(csv_data_.size(), "close");
        for (const auto& bar : csv_data_) {
            close_line_->forward(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<backtrader::LineRoot> close_line_;
};

TEST_P(RSIParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto rsi = std::make_shared<RSI>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        rsi->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期 (period + 1)
    EXPECT_EQ(rsi->getMinPeriod(), period + 1) 
        << "RSI minimum period should be period + 1";
    
    // 验证最后的值在有效范围内
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        double last_value = rsi->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last RSI value should not be NaN";
        EXPECT_GE(last_value, 0.0) << "RSI should be >= 0";
        EXPECT_LE(last_value, 100.0) << "RSI should be <= 100";
    }
}

// 测试不同的RSI周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    RSIParameterizedTest,
    ::testing::Values(7, 14, 21, 28)
);

// 超买超卖测试
TEST(OriginalTests, RSI_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto rsi = std::make_shared<RSI>(close_line, 14);
    
    bool found_overbought = false;
    bool found_oversold = false;
    
    // 计算所有值并检查是否有超买超卖情况
    for (size_t i = 0; i < csv_data.size(); ++i) {
        rsi->calculate();
        
        double current_rsi = rsi->get(0);
        if (!std::isnan(current_rsi)) {
            if (current_rsi > 70.0) {
                found_overbought = true;
            }
            if (current_rsi < 30.0) {
                found_oversold = true;
            }
            
            // 测试RSI的超买超卖状态函数
            double status = rsi->getOverboughtOversoldStatus();
            EXPECT_TRUE(status == -1.0 || status == 0.0 || status == 1.0)
                << "Overbought/Oversold status should be -1, 0, or 1";
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 注意：不强制要求找到超买超卖，因为这取决于具体的测试数据
    std::cout << "Found overbought: " << found_overbought 
              << ", Found oversold: " << found_oversold << std::endl;
}

// 边界条件测试
TEST(OriginalTests, RSI_EdgeCases) {
    // 测试相同价格序列
    auto close_line = std::make_shared<backtrader::LineRoot>(100, "constant");
    
    // 添加相同的价格
    for (int i = 0; i < 50; ++i) {
        close_line->forward(100.0);
    }
    
    auto rsi = std::make_shared<RSI>(close_line, 14);
    
    for (int i = 0; i < 50; ++i) {
        rsi->calculate();
        if (i < 49) {
            close_line->forward();
        }
    }
    
    // 当价格不变时，RSI应该是50.0或NaN
    double result = rsi->get(0);
    if (!std::isnan(result)) {
        EXPECT_NEAR(result, 50.0, 1e-6) 
            << "RSI should be 50 when prices are constant";
    }
}

// 计算验证测试 - 验证RSI计算逻辑
TEST(OriginalTests, RSI_CalculationLogic) {
    // 使用一个简单的上升价格序列
    std::vector<double> prices = {
        100.0, 101.0, 102.0, 103.0, 104.0, 105.0, 106.0, 107.0,
        108.0, 109.0, 110.0, 111.0, 112.0, 113.0, 114.0, 115.0
    };
    
    auto close_line = std::make_shared<backtrader::LineRoot>(prices.size(), "ascending");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto rsi = std::make_shared<RSI>(close_line, 14);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        rsi->calculate();
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
    
    // 对于持续上升的价格序列，RSI应该接近100
    double final_rsi = rsi->get(0);
    EXPECT_FALSE(std::isnan(final_rsi)) << "RSI should not be NaN";
    EXPECT_GT(final_rsi, 50.0) << "RSI should be > 50 for ascending prices";
    
    // 但不应该完全等于100（除非是极端情况）
    EXPECT_LT(final_rsi, 100.0) << "RSI should be < 100 for gradual price increase";
}
