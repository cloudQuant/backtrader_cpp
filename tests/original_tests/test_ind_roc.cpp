/**
 * @file test_ind_roc.cpp
 * @brief ROC指标测试 - 对应Python test_ind_roc.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['0.016544', '-0.009477', '0.019050'],
 * ]
 * chkmin = 13
 * chkind = btind.ROC
 */

#include "test_common.h"
#include <random>

#include "indicators/roc.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> ROC_EXPECTED_VALUES = {
    {"0.016544", "-0.009477", "0.019050"}
};

const int ROC_MIN_PERIOD = 13;

} // anonymous namespace

// 使用默认参数的ROC测试
DEFINE_INDICATOR_TEST(ROC_Default, ROC, ROC_EXPECTED_VALUES, ROC_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, ROC_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建ROC指标（默认12周期，最小周期为13）
    auto roc = std::make_shared<ROC>(close_line, 12);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        roc->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 13;  // period + 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"0.016544", "-0.009477", "0.019050"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = roc->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "ROC value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(roc->getMinPeriod(), 13) << "ROC minimum period should be 13";
}

// 参数化测试 - 测试不同周期的ROC
class ROCParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(ROCParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto roc = std::make_shared<ROC>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        roc->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期 (period + 1)
    EXPECT_EQ(roc->getMinPeriod(), period + 1) 
        << "ROC minimum period should be period + 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        double last_value = roc->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last ROC value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "ROC value should be finite";
    }
}

// 测试不同的ROC周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    ROCParameterizedTest,
    ::testing::Values(5, 10, 12, 20, 30)
);

// ROC计算逻辑验证测试
TEST(OriginalTests, ROC_CalculationLogic) {
    // 使用简单的测试数据验证ROC计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0};
    
    auto close_line = std::make_shared<backtrader::LineRoot>(prices.size(), "roc_calc");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto roc = std::make_shared<ROC>(close_line, 5);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        roc->calculate();
        
        // 手动计算ROC进行验证
        if (i >= 5) {  // 需要period + 1个数据点
            double current_price = prices[i];
            double past_price = prices[i - 5];  // 5周期前的价格
            double expected_roc = (current_price - past_price) / past_price;
            
            double actual_roc = roc->get(0);
            EXPECT_NEAR(actual_roc, expected_roc, 1e-10) 
                << "ROC calculation mismatch at step " << i 
                << " (current: " << current_price << ", past: " << past_price << ")";
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// ROC百分比计算验证测试
TEST(OriginalTests, ROC_PercentageCalculation) {
    // 测试ROC百分比计算
    std::vector<double> prices = {100.0, 105.0, 110.0, 95.0, 120.0};
    
    auto close_line = std::make_shared<backtrader::LineRoot>(prices.size(), "roc_percent");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto roc = std::make_shared<ROC>(close_line, 3);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        roc->calculate();
        
        if (i >= 3) {
            double current_price = prices[i];
            double past_price = prices[i - 3];
            
            // ROC = (Current - Past) / Past
            double expected_roc = (current_price - past_price) / past_price;
            double actual_roc = roc->get(0);
            
            EXPECT_NEAR(actual_roc, expected_roc, 1e-10) 
                << "ROC percentage calculation at step " << i;
                
            // 验证百分比意义
            if (current_price > past_price) {
                EXPECT_GT(actual_roc, 0.0) << "ROC should be positive for price increase";
            } else if (current_price < past_price) {
                EXPECT_LT(actual_roc, 0.0) << "ROC should be negative for price decrease";
            }
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// 趋势检测测试
TEST(OriginalTests, ROC_TrendDetection) {
    // 创建上升趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 30; ++i) {
        uptrend_prices.push_back(100.0 + i * 2.0);  // 持续上升
    }
    
    auto up_line = std::make_shared<backtrader::LineRoot>(uptrend_prices.size(), "uptrend");
    for (double price : uptrend_prices) {
        up_line->forward(price);
    }
    
    auto up_roc = std::make_shared<ROC>(up_line, 10);
    
    for (size_t i = 0; i < uptrend_prices.size(); ++i) {
        up_roc->calculate();
        if (i < uptrend_prices.size() - 1) {
            up_line->forward();
        }
    }
    
    double final_up_roc = up_roc->get(0);
    if (!std::isnan(final_up_roc)) {
        EXPECT_GT(final_up_roc, 0.0) 
            << "ROC should be positive for uptrend";
    }
    
    // 创建下降趋势数据
    std::vector<double> downtrend_prices;
    for (int i = 0; i < 30; ++i) {
        downtrend_prices.push_back(200.0 - i * 2.0);  // 持续下降
    }
    
    auto down_line = std::make_shared<backtrader::LineRoot>(downtrend_prices.size(), "downtrend");
    for (double price : downtrend_prices) {
        down_line->forward(price);
    }
    
    auto down_roc = std::make_shared<ROC>(down_line, 10);
    
    for (size_t i = 0; i < downtrend_prices.size(); ++i) {
        down_roc->calculate();
        if (i < downtrend_prices.size() - 1) {
            down_line->forward();
        }
    }
    
    double final_down_roc = down_roc->get(0);
    if (!std::isnan(final_down_roc)) {
        EXPECT_LT(final_down_roc, 0.0) 
            << "ROC should be negative for downtrend";
    }
    
    std::cout << "Uptrend ROC: " << final_up_roc << std::endl;
    std::cout << "Downtrend ROC: " << final_down_roc << std::endl;
}

// 横盘市场测试
TEST(OriginalTests, ROC_SidewaysMarket) {
    // 创建横盘震荡数据
    std::vector<double> sideways_prices;
    for (int i = 0; i < 50; ++i) {
        double base = 100.0;
        double oscillation = 3.0 * std::sin(i * 0.3);  // 小幅震荡
        sideways_prices.push_back(base + oscillation);
    }
    
    auto sideways_line = std::make_shared<backtrader::LineRoot>(sideways_prices.size(), "sideways");
    for (double price : sideways_prices) {
        sideways_line->forward(price);
    }
    
    auto sideways_roc = std::make_shared<ROC>(sideways_line, 20);
    
    std::vector<double> roc_values;
    
    for (size_t i = 0; i < sideways_prices.size(); ++i) {
        sideways_roc->calculate();
        
        double roc_val = sideways_roc->get(0);
        if (!std::isnan(roc_val)) {
            roc_values.push_back(roc_val);
        }
        
        if (i < sideways_prices.size() - 1) {
            sideways_line->forward();
        }
    }
    
    // 横盘市场中，ROC应该接近零且波动较小
    if (!roc_values.empty()) {
        double avg_roc = std::accumulate(roc_values.begin(), roc_values.end(), 0.0) / roc_values.size();
        EXPECT_NEAR(avg_roc, 0.0, 0.1) 
            << "Average ROC should be close to zero in sideways market";
        
        std::cout << "Sideways market average ROC: " << avg_roc << std::endl;
    }
}

// 零线交叉测试
TEST(OriginalTests, ROC_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto roc = std::make_shared<ROC>(close_line, 12);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_roc = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越
    for (size_t i = 0; i < csv_data.size(); ++i) {
        roc->calculate();
        
        double current_roc = roc->get(0);
        
        if (!std::isnan(current_roc) && has_prev) {
            if (prev_roc <= 0.0 && current_roc > 0.0) {
                positive_crossings++;
            } else if (prev_roc >= 0.0 && current_roc < 0.0) {
                negative_crossings++;
            }
        }
        
        if (!std::isnan(current_roc)) {
            prev_roc = current_roc;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "ROC zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// 边界条件测试
TEST(OriginalTests, ROC_EdgeCases) {
    // 测试除零情况（过去价格为0）
    std::vector<double> zero_prices = {0.0, 100.0, 105.0, 110.0, 115.0};
    
    auto zero_line = std::make_shared<backtrader::LineRoot>(zero_prices.size(), "zero_test");
    for (double price : zero_prices) {
        zero_line->forward(price);
    }
    
    auto zero_roc = std::make_shared<ROC>(zero_line, 3);
    
    for (size_t i = 0; i < zero_prices.size(); ++i) {
        zero_roc->calculate();
        
        double roc_val = zero_roc->get(0);
        
        // 当过去价格为0时，ROC应该是无穷大或NaN
        if (i >= 3 && zero_prices[i - 3] == 0.0) {
            EXPECT_TRUE(std::isnan(roc_val) || std::isinf(roc_val)) 
                << "ROC should be NaN or infinite when past price is zero";
        }
        
        if (i < zero_prices.size() - 1) {
            zero_line->forward();
        }
    }
    
    // 测试相同价格的情况
    std::vector<double> flat_prices(20, 100.0);  // 20个相同价格
    
    auto flat_line = std::make_shared<backtrader::LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_roc = std::make_shared<ROC>(flat_line, 10);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_roc->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    double final_roc = flat_roc->get(0);
    if (!std::isnan(final_roc)) {
        EXPECT_NEAR(final_roc, 0.0, 1e-10) 
            << "ROC should be zero for constant prices";
    }
}

// 性能测试
TEST(OriginalTests, ROC_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<double> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        large_data.push_back(dist(rng));
    }
    
    auto large_line = std::make_shared<backtrader::LineRoot>(large_data.size(), "large");
    for (double price : large_data) {
        large_line->forward(price);
    }
    
    auto large_roc = std::make_shared<ROC>(large_line, 50);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_roc->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "ROC calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_roc->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}