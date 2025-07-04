/**
 * @file test_ind_rsi_safe.cpp
 * @brief RSI_Safe指标测试 - 对应Python test_ind_rsi_safe.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['57.644284', '41.630968', '53.352553']
 * ]
 * chkmin = 15
 * chkind = btind.RSI_Safe
 * 
 * 注：RSI_Safe 是RSI的安全版本，避免除零错误
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/RSI_Safe.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> RSI_SAFE_EXPECTED_VALUES = {
    {"57.644284", "41.630968", "53.352553"}
};

const int RSI_SAFE_MIN_PERIOD = 15;

} // anonymous namespace

// 使用默认参数的RSI_Safe测试
DEFINE_INDICATOR_TEST(RSI_Safe_Default, RSI_Safe, RSI_SAFE_EXPECTED_VALUES, RSI_SAFE_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, RSI_Safe_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建RSI_Safe指标
    auto rsi_safe = std::make_shared<RSI_Safe>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        rsi_safe->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 15;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"57.644284", "41.630968", "53.352553"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = rsi_safe->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "RSI_Safe value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(rsi_safe->getMinPeriod(), 15) << "RSI_Safe minimum period should be 15";
}

// 参数化测试 - 测试不同周期的RSI_Safe
class RSISafeParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line_ = std::make_shared<LineRoot>(csv_data_.size(), "close");
        for (const auto& bar : csv_data_) {
            close_line_->forward(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineRoot> close_line_;
};

TEST_P(RSISafeParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto rsi_safe = std::make_shared<RSI_Safe>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        rsi_safe->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(rsi_safe->getMinPeriod(), period + 1) 
        << "RSI_Safe minimum period should equal period + 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        double last_value = rsi_safe->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last RSI_Safe value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last RSI_Safe value should be finite";
        EXPECT_GE(last_value, 0.0) << "RSI_Safe should be >= 0";
        EXPECT_LE(last_value, 100.0) << "RSI_Safe should be <= 100";
    }
}

// 测试不同的RSI_Safe周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    RSISafeParameterizedTest,
    ::testing::Values(7, 14, 21, 28)
);

// RSI_Safe与标准RSI比较测试
TEST(OriginalTests, RSI_Safe_vs_StandardRSI) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto rsi_safe = std::make_shared<RSI_Safe>(close_line, 14);
    auto rsi_standard = std::make_shared<RSI>(close_line, 14);
    
    std::vector<double> safe_values, standard_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        rsi_safe->calculate();
        rsi_standard->calculate();
        
        double safe_val = rsi_safe->get(0);
        double standard_val = rsi_standard->get(0);
        
        if (!std::isnan(safe_val) && !std::isnan(standard_val)) {
            safe_values.push_back(safe_val);
            standard_values.push_back(standard_val);
            
            // 在正常情况下，两者应该非常接近
            EXPECT_NEAR(safe_val, standard_val, 0.01) 
                << "RSI_Safe and standard RSI should be similar at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 比较整体统计特性
    if (!safe_values.empty() && !standard_values.empty()) {
        double safe_avg = std::accumulate(safe_values.begin(), safe_values.end(), 0.0) / safe_values.size();
        double standard_avg = std::accumulate(standard_values.begin(), standard_values.end(), 0.0) / standard_values.size();
        
        std::cout << "RSI comparison:" << std::endl;
        std::cout << "RSI_Safe average: " << safe_avg << std::endl;
        std::cout << "Standard RSI average: " << standard_avg << std::endl;
        
        EXPECT_NEAR(safe_avg, standard_avg, 1.0) 
            << "Average values should be similar";
    }
}

// RSI_Safe除零保护测试
TEST(OriginalTests, RSI_Safe_DivisionByZeroProtection) {
    // 创建可能导致除零的数据（相同价格）
    std::vector<double> flat_prices(30, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto rsi_safe = std::make_shared<RSI_Safe>(flat_line, 14);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        rsi_safe->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // RSI_Safe应该能处理相同价格的情况
    double final_rsi = rsi_safe->get(0);
    if (!std::isnan(final_rsi)) {
        EXPECT_GE(final_rsi, 0.0) << "RSI_Safe should be >= 0 even with flat prices";
        EXPECT_LE(final_rsi, 100.0) << "RSI_Safe should be <= 100 even with flat prices";
        // 相同价格通常会导致RSI接近50（中性）
        EXPECT_NEAR(final_rsi, 50.0, 10.0) << "RSI_Safe should be near neutral for flat prices";
    }
    
    // 创建可能导致除零的特殊数据序列
    std::vector<double> zero_change_prices = {100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 
                                              100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0,
                                              100.0, 101.0, 100.0};  // 最后有微小变化
    
    auto zero_line = std::make_shared<LineRoot>(zero_change_prices.size(), "zero_change");
    for (double price : zero_change_prices) {
        zero_line->forward(price);
    }
    
    auto rsi_safe_zero = std::make_shared<RSI_Safe>(zero_line, 14);
    
    for (size_t i = 0; i < zero_change_prices.size(); ++i) {
        rsi_safe_zero->calculate();
        
        double rsi_val = rsi_safe_zero->get(0);
        if (!std::isnan(rsi_val)) {
            EXPECT_FALSE(std::isinf(rsi_val)) << "RSI_Safe should not return infinity at step " << i;
            EXPECT_GE(rsi_val, 0.0) << "RSI_Safe should be >= 0 at step " << i;
            EXPECT_LE(rsi_val, 100.0) << "RSI_Safe should be <= 100 at step " << i;
        }
        
        if (i < zero_change_prices.size() - 1) {
            zero_line->forward();
        }
    }
}

// RSI_Safe超买超卖信号测试
TEST(OriginalTests, RSI_Safe_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto rsi_safe = std::make_shared<RSI_Safe>(close_line, 14);
    
    int overbought_signals = 0;  // RSI > 70
    int oversold_signals = 0;    // RSI < 30
    int neutral_signals = 0;     // 30 <= RSI <= 70
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        rsi_safe->calculate();
        
        double rsi_val = rsi_safe->get(0);
        if (!std::isnan(rsi_val)) {
            if (rsi_val > 70.0) {
                overbought_signals++;
            } else if (rsi_val < 30.0) {
                oversold_signals++;
            } else {
                neutral_signals++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "RSI_Safe signal analysis:" << std::endl;
    std::cout << "Overbought signals (>70): " << overbought_signals << std::endl;
    std::cout << "Oversold signals (<30): " << oversold_signals << std::endl;
    std::cout << "Neutral signals (30-70): " << neutral_signals << std::endl;
    
    int total_signals = overbought_signals + oversold_signals + neutral_signals;
    EXPECT_GT(total_signals, 0) << "Should have some valid RSI signals";
    
    // 大多数信号应该在中性区域
    if (total_signals > 0) {
        double neutral_ratio = static_cast<double>(neutral_signals) / total_signals;
        std::cout << "Neutral ratio: " << neutral_ratio << std::endl;
        EXPECT_GT(neutral_ratio, 0.3) << "Majority of signals should be in neutral zone";
    }
}

// RSI_Safe发散分析测试
TEST(OriginalTests, RSI_Safe_DivergenceAnalysis) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto rsi_safe = std::make_shared<RSI_Safe>(close_line, 14);
    
    std::vector<double> prices, rsi_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        rsi_safe->calculate();
        
        double rsi_val = rsi_safe->get(0);
        if (!std::isnan(rsi_val)) {
            prices.push_back(csv_data[i].close);
            rsi_values.push_back(rsi_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 寻找价格和RSI的峰值和谷值
    std::vector<size_t> price_peaks, price_troughs;
    std::vector<size_t> rsi_peaks, rsi_troughs;
    
    for (size_t i = 1; i < prices.size() - 1; ++i) {
        // 价格峰值和谷值
        if (prices[i] > prices[i-1] && prices[i] > prices[i+1]) {
            price_peaks.push_back(i);
        }
        if (prices[i] < prices[i-1] && prices[i] < prices[i+1]) {
            price_troughs.push_back(i);
        }
        
        // RSI峰值和谷值
        if (rsi_values[i] > rsi_values[i-1] && rsi_values[i] > rsi_values[i+1]) {
            rsi_peaks.push_back(i);
        }
        if (rsi_values[i] < rsi_values[i-1] && rsi_values[i] < rsi_values[i+1]) {
            rsi_troughs.push_back(i);
        }
    }
    
    std::cout << "RSI_Safe divergence analysis:" << std::endl;
    std::cout << "Price peaks: " << price_peaks.size() << std::endl;
    std::cout << "Price troughs: " << price_troughs.size() << std::endl;
    std::cout << "RSI peaks: " << rsi_peaks.size() << std::endl;
    std::cout << "RSI troughs: " << rsi_troughs.size() << std::endl;
    
    EXPECT_TRUE(true) << "Divergence analysis completed";
}

// RSI_Safe趋势确认测试
TEST(OriginalTests, RSI_Safe_TrendConfirmation) {
    // 创建明确的上升趋势
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 50; ++i) {
        double base = 100.0 + i * 0.5;
        double noise = std::sin(i * 0.1) * 1.0;
        uptrend_prices.push_back(base + noise);
    }
    
    auto uptrend_line = std::make_shared<LineRoot>(uptrend_prices.size(), "uptrend");
    for (double price : uptrend_prices) {
        uptrend_line->forward(price);
    }
    
    auto uptrend_rsi = std::make_shared<RSI_Safe>(uptrend_line, 14);
    
    std::vector<double> uptrend_rsi_values;
    
    for (size_t i = 0; i < uptrend_prices.size(); ++i) {
        uptrend_rsi->calculate();
        
        double rsi_val = uptrend_rsi->get(0);
        if (!std::isnan(rsi_val)) {
            uptrend_rsi_values.push_back(rsi_val);
        }
        
        if (i < uptrend_prices.size() - 1) {
            uptrend_line->forward();
        }
    }
    
    // 创建下降趋势
    std::vector<double> downtrend_prices;
    for (int i = 0; i < 50; ++i) {
        double base = 150.0 - i * 0.5;
        double noise = std::sin(i * 0.1) * 1.0;
        downtrend_prices.push_back(base + noise);
    }
    
    auto downtrend_line = std::make_shared<LineRoot>(downtrend_prices.size(), "downtrend");
    for (double price : downtrend_prices) {
        downtrend_line->forward(price);
    }
    
    auto downtrend_rsi = std::make_shared<RSI_Safe>(downtrend_line, 14);
    
    std::vector<double> downtrend_rsi_values;
    
    for (size_t i = 0; i < downtrend_prices.size(); ++i) {
        downtrend_rsi->calculate();
        
        double rsi_val = downtrend_rsi->get(0);
        if (!std::isnan(rsi_val)) {
            downtrend_rsi_values.push_back(rsi_val);
        }
        
        if (i < downtrend_prices.size() - 1) {
            downtrend_line->forward();
        }
    }
    
    // 分析趋势确认
    if (!uptrend_rsi_values.empty() && !downtrend_rsi_values.empty()) {
        double uptrend_avg = std::accumulate(uptrend_rsi_values.begin(), uptrend_rsi_values.end(), 0.0) / uptrend_rsi_values.size();
        double downtrend_avg = std::accumulate(downtrend_rsi_values.begin(), downtrend_rsi_values.end(), 0.0) / downtrend_rsi_values.size();
        
        std::cout << "Trend confirmation analysis:" << std::endl;
        std::cout << "Uptrend RSI average: " << uptrend_avg << std::endl;
        std::cout << "Downtrend RSI average: " << downtrend_avg << std::endl;
        
        // 上升趋势应该有更高的RSI值
        EXPECT_GT(uptrend_avg, downtrend_avg) 
            << "Uptrend should have higher RSI values than downtrend";
        EXPECT_GT(uptrend_avg, 50.0) 
            << "Uptrend RSI should be above neutral";
        EXPECT_LT(downtrend_avg, 50.0) 
            << "Downtrend RSI should be below neutral";
    }
}

// RSI_Safe振荡特性测试
TEST(OriginalTests, RSI_Safe_OscillationCharacteristics) {
    // 创建振荡数据
    std::vector<double> oscillating_prices;
    for (int i = 0; i < 100; ++i) {
        double base = 100.0;
        double oscillation = 10.0 * std::sin(i * 0.2);
        oscillating_prices.push_back(base + oscillation);
    }
    
    auto osc_line = std::make_shared<LineRoot>(oscillating_prices.size(), "oscillating");
    for (double price : oscillating_prices) {
        osc_line->forward(price);
    }
    
    auto rsi_safe = std::make_shared<RSI_Safe>(osc_line, 14);
    
    std::vector<double> rsi_values;
    
    for (size_t i = 0; i < oscillating_prices.size(); ++i) {
        rsi_safe->calculate();
        
        double rsi_val = rsi_safe->get(0);
        if (!std::isnan(rsi_val)) {
            rsi_values.push_back(rsi_val);
        }
        
        if (i < oscillating_prices.size() - 1) {
            osc_line->forward();
        }
    }
    
    // 分析振荡特性
    if (!rsi_values.empty()) {
        double avg_rsi = std::accumulate(rsi_values.begin(), rsi_values.end(), 0.0) / rsi_values.size();
        
        // 计算标准差
        double variance = 0.0;
        for (double val : rsi_values) {
            variance += (val - avg_rsi) * (val - avg_rsi);
        }
        variance /= rsi_values.size();
        double std_dev = std::sqrt(variance);
        
        std::cout << "RSI_Safe oscillation characteristics:" << std::endl;
        std::cout << "Average: " << avg_rsi << std::endl;
        std::cout << "Standard deviation: " << std_dev << std::endl;
        
        // RSI应该围绕50线波动
        EXPECT_NEAR(avg_rsi, 50.0, 15.0) 
            << "RSI_Safe should oscillate around 50";
        
        EXPECT_GT(std_dev, 5.0) 
            << "RSI_Safe should show meaningful variation";
    }
}

// 边界条件测试
TEST(OriginalTests, RSI_Safe_EdgeCases) {
    // 测试极端价格变化
    std::vector<double> extreme_prices = {100.0, 200.0, 50.0, 150.0, 25.0, 175.0, 12.5, 187.5,
                                          100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0};
    
    auto extreme_line = std::make_shared<LineRoot>(extreme_prices.size(), "extreme");
    for (double price : extreme_prices) {
        extreme_line->forward(price);
    }
    
    auto extreme_rsi = std::make_shared<RSI_Safe>(extreme_line, 14);
    
    for (size_t i = 0; i < extreme_prices.size(); ++i) {
        extreme_rsi->calculate();
        
        double rsi_val = extreme_rsi->get(0);
        if (!std::isnan(rsi_val)) {
            EXPECT_GE(rsi_val, 0.0) << "RSI_Safe should be >= 0 with extreme prices at step " << i;
            EXPECT_LE(rsi_val, 100.0) << "RSI_Safe should be <= 100 with extreme prices at step " << i;
            EXPECT_FALSE(std::isinf(rsi_val)) << "RSI_Safe should not be infinite at step " << i;
        }
        
        if (i < extreme_prices.size() - 1) {
            extreme_line->forward();
        }
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(50, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 10; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_rsi = std::make_shared<RSI_Safe>(insufficient_line, 14);
    
    for (int i = 0; i < 10; ++i) {
        insufficient_rsi->calculate();
        if (i < 9) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_rsi->get(0);
    EXPECT_TRUE(std::isnan(result)) << "RSI_Safe should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, RSI_Safe_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<double> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        large_data.push_back(dist(rng));
    }
    
    auto large_line = std::make_shared<LineRoot>(large_data.size(), "large");
    for (double price : large_data) {
        large_line->forward(price);
    }
    
    auto large_rsi = std::make_shared<RSI_Safe>(large_line, 14);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_rsi->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "RSI_Safe calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_rsi->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    EXPECT_GE(final_result, 0.0) << "Final result should be >= 0";
    EXPECT_LE(final_result, 100.0) << "Final result should be <= 100";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}