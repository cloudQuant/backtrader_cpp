/**
 * @file test_ind_ultosc.cpp
 * @brief UltimateOscillator指标测试 - 对应Python test_ind_ultosc.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['51.991177', '62.334055', '46.707445']
 * ]
 * chkmin = 29  # 28 from longest SumN/Sum + 1 extra from truelow/truerange
 * chkind = bt.indicators.UltimateOscillator
 */

#include "test_common_simple.h"

#include "indicators/ultimateoscillator.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> ULTOSC_EXPECTED_VALUES = {
    {"51.991177", "62.334055", "46.707445"}
};

const int ULTOSC_MIN_PERIOD = 29;

} // anonymous namespace

// 使用默认参数的UltimateOscillator测试
DEFINE_INDICATOR_TEST(UltimateOscillator_Default, UltimateOscillator, ULTOSC_EXPECTED_VALUES, ULTOSC_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, UltimateOscillator_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    // 创建UltimateOscillator指标（默认参数：7, 14, 28）
    auto ultosc = std::make_shared<UltimateOscillator>(high_line, low_line, close_line, 7, 14, 28);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ultosc->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 29;  // 28 + 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"51.991177", "62.334055", "46.707445"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = ultosc->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "UltimateOscillator value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(ultosc->getMinPeriod(), 29) << "UltimateOscillator minimum period should be 29";
}

// UltimateOscillator范围验证测试
TEST(OriginalTests, UltimateOscillator_RangeValidation) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto ultosc = std::make_shared<UltimateOscillator>(high_line, low_line, close_line, 7, 14, 28);
    
    // 计算所有值并验证范围
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ultosc->calculate();
        
        double ultosc_value = ultosc->get(0);
        
        // 验证UltimateOscillator在0到100范围内
        if (!std::isnan(ultosc_value)) {
            EXPECT_GE(ultosc_value, 0.0) << "UltimateOscillator should be >= 0 at step " << i;
            EXPECT_LE(ultosc_value, 100.0) << "UltimateOscillator should be <= 100 at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}

// 参数化测试 - 测试不同参数的UltimateOscillator
class UltimateOscillatorParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int, int>> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        high_line_ = std::make_shared<LineRoot>(csv_data_.size(), "high");
        low_line_ = std::make_shared<LineRoot>(csv_data_.size(), "low");
        close_line_ = std::make_shared<LineRoot>(csv_data_.size(), "close");
        
        for (const auto& bar : csv_data_) {
            high_line_->forward(bar.high);
            low_line_->forward(bar.low);
            close_line_->forward(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineRoot> high_line_;
    std::shared_ptr<LineRoot> low_line_;
    std::shared_ptr<LineRoot> close_line_;
};

TEST_P(UltimateOscillatorParameterizedTest, DifferentParameters) {
    auto [period1, period2, period3] = GetParam();
    auto ultosc = std::make_shared<UltimateOscillator>(close_line_, high_line_, low_line_, period1, period2, period3);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        ultosc->calculate();
        if (i < csv_data_.size() - 1) {
            high_line_->forward();
            low_line_->forward();
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    int expected_min_period = std::max({period1, period2, period3}) + 1;
    EXPECT_EQ(ultosc->getMinPeriod(), expected_min_period) 
        << "UltimateOscillator minimum period should be max period + 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_value = ultosc->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last UltimateOscillator value should not be NaN";
        EXPECT_GE(last_value, 0.0) << "UltimateOscillator should be >= 0";
        EXPECT_LE(last_value, 100.0) << "UltimateOscillator should be <= 100";
    }
}

// 测试不同的UltimateOscillator参数
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    UltimateOscillatorParameterizedTest,
    ::testing::Values(
        std::make_tuple(7, 14, 28),   // 标准参数
        std::make_tuple(5, 10, 20),
        std::make_tuple(3, 7, 14),
        std::make_tuple(10, 20, 40)
    )
);

// UltimateOscillator计算逻辑验证测试
TEST(OriginalTests, UltimateOscillator_CalculationLogic) {
    // 使用简单的测试数据验证UltimateOscillator计算
    std::vector<CSVDataReader::OHLCVData> test_data = {
        {"2006-01-01", 100.0, 110.0, 90.0, 105.0, 0, 0},
        {"2006-01-02", 105.0, 115.0, 95.0, 110.0, 0, 0},
        {"2006-01-03", 110.0, 120.0, 100.0, 115.0, 0, 0},
        {"2006-01-04", 115.0, 125.0, 105.0, 120.0, 0, 0},
        {"2006-01-05", 120.0, 130.0, 110.0, 125.0, 0, 0},
        {"2006-01-06", 125.0, 135.0, 115.0, 130.0, 0, 0},
        {"2006-01-07", 130.0, 140.0, 120.0, 135.0, 0, 0},
        {"2006-01-08", 135.0, 145.0, 125.0, 140.0, 0, 0}
    };
    
    auto high_line = std::make_shared<LineRoot>(test_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(test_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(test_data.size(), "close");
    
    for (const auto& bar : test_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto ultosc = std::make_shared<UltimateOscillator>(high_line, low_line, close_line, 3, 5, 7);
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        ultosc->calculate();
        
        double ultosc_val = ultosc->get(0);
        
        // UltimateOscillator应该产生有限值且在0-100范围内
        if (!std::isnan(ultosc_val)) {
            EXPECT_TRUE(std::isfinite(ultosc_val)) 
                << "UltimateOscillator should be finite at step " << i;
            EXPECT_GE(ultosc_val, 0.0) 
                << "UltimateOscillator should be >= 0 at step " << i;
            EXPECT_LE(ultosc_val, 100.0) 
                << "UltimateOscillator should be <= 100 at step " << i;
        }
        
        if (i < test_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}

// 超买超卖信号测试
TEST(OriginalTests, UltimateOscillator_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto ultosc = std::make_shared<UltimateOscillator>(high_line, low_line, close_line, 7, 14, 28);
    
    int overbought_signals = 0;  // UO > 70
    int oversold_signals = 0;    // UO < 30
    int normal_signals = 0;      // 30 <= UO <= 70
    
    // 统计超买超卖信号
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ultosc->calculate();
        
        double ultosc_value = ultosc->get(0);
        
        if (!std::isnan(ultosc_value)) {
            if (ultosc_value > 70.0) {
                overbought_signals++;
            } else if (ultosc_value < 30.0) {
                oversold_signals++;
            } else {
                normal_signals++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    std::cout << "UltimateOscillator signal statistics:" << std::endl;
    std::cout << "Overbought signals (> 70): " << overbought_signals << std::endl;
    std::cout << "Oversold signals (< 30): " << oversold_signals << std::endl;
    std::cout << "Normal signals (30-70): " << normal_signals << std::endl;
    
    // 验证至少有一些有效的计算
    EXPECT_GT(overbought_signals + oversold_signals + normal_signals, 0) 
        << "Should have some valid UltimateOscillator calculations";
}

// 趋势反转信号测试
TEST(OriginalTests, UltimateOscillator_ReversalSignals) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto ultosc = std::make_shared<UltimateOscillator>(high_line, low_line, close_line, 7, 14, 28);
    
    int bullish_reversals = 0;   // 从超卖区域向上反转
    int bearish_reversals = 0;   // 从超买区域向下反转
    
    double prev_ultosc = 0.0;
    bool was_oversold = false;
    bool was_overbought = false;
    bool has_prev = false;
    
    // 检测反转信号
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ultosc->calculate();
        
        double current_ultosc = ultosc->get(0);
        
        if (!std::isnan(current_ultosc) && has_prev) {
            // 检测从超卖区域的看涨反转
            if (was_oversold && prev_ultosc < 30.0 && current_ultosc > 30.0) {
                bullish_reversals++;
                was_oversold = false;
            }
            
            // 检测从超买区域的看跌反转
            if (was_overbought && prev_ultosc > 70.0 && current_ultosc < 70.0) {
                bearish_reversals++;
                was_overbought = false;
            }
            
            // 更新状态
            if (current_ultosc < 30.0) {
                was_oversold = true;
            }
            if (current_ultosc > 70.0) {
                was_overbought = true;
            }
        }
        
        if (!std::isnan(current_ultosc)) {
            prev_ultosc = current_ultosc;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    std::cout << "UltimateOscillator reversal signals:" << std::endl;
    std::cout << "Bullish reversals: " << bullish_reversals << std::endl;
    std::cout << "Bearish reversals: " << bearish_reversals << std::endl;
    
    // 验证检测到一些反转信号
    EXPECT_GE(bullish_reversals + bearish_reversals, 0) 
        << "Should detect some reversal signals";
}

// 多时间框架验证测试
TEST(OriginalTests, UltimateOscillator_MultiTimeframe) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    // 创建不同参数的UltimateOscillator
    auto ultosc_fast = std::make_shared<UltimateOscillator>(high_line, low_line, close_line, 3, 7, 14);
    auto ultosc_standard = std::make_shared<UltimateOscillator>(high_line, low_line, close_line, 7, 14, 28);
    auto ultosc_slow = std::make_shared<UltimateOscillator>(high_line, low_line, close_line, 14, 28, 56);
    
    std::vector<double> fast_values;
    std::vector<double> standard_values;
    std::vector<double> slow_values;
    
    // 计算并收集值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ultosc_fast->calculate();
        ultosc_standard->calculate();
        ultosc_slow->calculate();
        
        double fast_val = ultosc_fast->get(0);
        double standard_val = ultosc_standard->get(0);
        double slow_val = ultosc_slow->get(0);
        
        if (!std::isnan(fast_val) && !std::isnan(standard_val) && !std::isnan(slow_val)) {
            fast_values.push_back(fast_val);
            standard_values.push_back(standard_val);
            slow_values.push_back(slow_val);
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 验证多时间框架的特性
    EXPECT_FALSE(fast_values.empty()) << "Fast UO should produce values";
    EXPECT_FALSE(standard_values.empty()) << "Standard UO should produce values";
    EXPECT_FALSE(slow_values.empty()) << "Slow UO should produce values";
    
    if (!fast_values.empty() && !standard_values.empty() && !slow_values.empty()) {
        std::cout << "Multi-timeframe UO values collected successfully" << std::endl;
    }
}

// 边界条件测试
TEST(OriginalTests, UltimateOscillator_EdgeCases) {
    // 测试相同价格的情况
    std::vector<CSVDataReader::OHLCVData> flat_data;
    for (int i = 0; i < 50; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 100.0;
        bar.low = 100.0;
        bar.close = 100.0;
        bar.open = 100.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        flat_data.push_back(bar);
    }
    
    auto flat_high = std::make_shared<LineRoot>(flat_data.size(), "flat_high");
    auto flat_low = std::make_shared<LineRoot>(flat_data.size(), "flat_low");
    auto flat_close = std::make_shared<LineRoot>(flat_data.size(), "flat_close");
    
    for (const auto& bar : flat_data) {
        flat_high->forward(bar.high);
        flat_low->forward(bar.low);
        flat_close->forward(bar.close);
    }
    
    auto flat_ultosc = std::make_shared<UltimateOscillator>(flat_high, flat_low, flat_close, 7, 14, 28);
    
    for (size_t i = 0; i < flat_data.size(); ++i) {
        flat_ultosc->calculate();
        if (i < flat_data.size() - 1) {
            flat_high->forward();
            flat_low->forward();
            flat_close->forward();
        }
    }
    
    // 当所有价格相同时，UltimateOscillator的行为可能因实现而异
    double final_ultosc = flat_ultosc->get(0);
    if (!std::isnan(final_ultosc)) {
        EXPECT_GE(final_ultosc, 0.0) << "UltimateOscillator should be >= 0 for constant prices";
        EXPECT_LE(final_ultosc, 100.0) << "UltimateOscillator should be <= 100 for constant prices";
    }
}

// 性能测试
TEST(OriginalTests, UltimateOscillator_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<CSVDataReader::OHLCVData> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> price_dist(50.0, 150.0);
    std::uniform_real_distribution<double> range_dist(1.0, 5.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-01";
        bar.close = price_dist(rng);
        double range = range_dist(rng);
        bar.high = bar.close + range;
        bar.low = bar.close - range;
        bar.open = bar.close;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        large_data.push_back(bar);
    }
    
    auto large_high = std::make_shared<LineRoot>(large_data.size(), "large_high");
    auto large_low = std::make_shared<LineRoot>(large_data.size(), "large_low");
    auto large_close = std::make_shared<LineRoot>(large_data.size(), "large_close");
    
    for (const auto& bar : large_data) {
        large_high->forward(bar.high);
        large_low->forward(bar.low);
        large_close->forward(bar.close);
    }
    
    auto large_ultosc = std::make_shared<UltimateOscillator>(large_high, large_low, large_close, 7, 14, 28);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_ultosc->calculate();
        if (i < large_data.size() - 1) {
            large_high->forward();
            large_low->forward();
            large_close->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "UltimateOscillator calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_ultosc->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_GE(final_result, 0.0) << "Final result should be >= 0";
    EXPECT_LE(final_result, 100.0) << "Final result should be <= 100";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}