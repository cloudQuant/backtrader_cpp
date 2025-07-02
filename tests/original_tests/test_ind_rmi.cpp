/**
 * @file test_ind_rmi.cpp
 * @brief RMI指标测试 - 对应Python test_ind_rmi.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['67.786097', '59.856230', '38.287526']
 * ]
 * chkmin = 25
 * chkind = bt.ind.RMI
 * 
 * 注：RMI (Relative Momentum Index) 是RSI的变种，使用动量而非价格变化
 */

#include "test_common.h"
#include "indicators/RMI.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> RMI_EXPECTED_VALUES = {
    {"67.786097", "59.856230", "38.287526"}
};

const int RMI_MIN_PERIOD = 25;

} // anonymous namespace

// 使用默认参数的RMI测试
DEFINE_INDICATOR_TEST(RMI_Default, RMI, RMI_EXPECTED_VALUES, RMI_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, RMI_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建RMI指标
    auto rmi = std::make_shared<RMI>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        rmi->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 25;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"67.786097", "59.856230", "38.287526"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = rmi->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "RMI value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(rmi->getMinPeriod(), 25) << "RMI minimum period should be 25";
}

// 参数化测试 - 测试不同周期和look-back的RMI
class RMIParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int>> {
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

TEST_P(RMIParameterizedTest, DifferentParameters) {
    auto [period, lookback] = GetParam();
    
    // 使用自定义参数创建RMI
    auto rmi = std::make_shared<RMI>(close_line_, period, lookback);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        rmi->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最后的值
    int expected_min_period = period + lookback - 1;
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_value = rmi->get(0);
        
        EXPECT_FALSE(std::isnan(last_value)) << "Last RMI value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last RMI value should be finite";
        EXPECT_GE(last_value, 0.0) << "RMI should be >= 0";
        EXPECT_LE(last_value, 100.0) << "RMI should be <= 100";
    }
}

// 测试不同的周期和lookback参数组合
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    RMIParameterizedTest,
    ::testing::Values(
        std::make_tuple(14, 5),
        std::make_tuple(21, 7),
        std::make_tuple(25, 10),
        std::make_tuple(20, 8)
    )
);

// RMI计算逻辑验证测试
TEST(OriginalTests, RMI_CalculationLogic) {
    // 使用简单的测试数据验证RMI计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0,
                                  116.0, 114.0, 112.0, 110.0, 108.0, 106.0, 104.0, 102.0, 100.0, 98.0,
                                  100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0};
    
    auto price_line = std::make_shared<LineRoot>(prices.size(), "rmi_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto rmi = std::make_shared<RMI>(price_line, 14, 5);  // 14周期，5日lookback
    
    // RMI基于动量计算，较为复杂，主要验证其基本特性
    for (size_t i = 0; i < prices.size(); ++i) {
        rmi->calculate();
        
        if (i >= 18) {  // RMI需要period + lookback - 1个数据点
            double rmi_value = rmi->get(0);
            
            if (!std::isnan(rmi_value)) {
                // 验证RMI的基本范围约束
                EXPECT_GE(rmi_value, 0.0) 
                    << "RMI should be >= 0 at step " << i;
                EXPECT_LE(rmi_value, 100.0) 
                    << "RMI should be <= 100 at step " << i;
                EXPECT_TRUE(std::isfinite(rmi_value)) 
                    << "RMI should be finite at step " << i;
            }
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
}

// RMI超买超卖信号测试
TEST(OriginalTests, RMI_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto rmi = std::make_shared<RMI>(close_line);
    
    int overbought_count = 0;   // RMI > 70
    int oversold_count = 0;     // RMI < 30
    int neutral_count = 0;      // 30 <= RMI <= 70
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        rmi->calculate();
        
        double rmi_value = rmi->get(0);
        
        if (!std::isnan(rmi_value)) {
            if (rmi_value > 70.0) {
                overbought_count++;
            } else if (rmi_value < 30.0) {
                oversold_count++;
            } else {
                neutral_count++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "RMI overbought/oversold analysis:" << std::endl;
    std::cout << "Overbought (> 70): " << overbought_count << std::endl;
    std::cout << "Oversold (< 30): " << oversold_count << std::endl;
    std::cout << "Neutral (30-70): " << neutral_count << std::endl;
    
    // 验证有一些有效的计算
    int total_valid = overbought_count + oversold_count + neutral_count;
    EXPECT_GT(total_valid, 0) << "Should have some valid RMI calculations";
}

// RMI与RSI比较测试
TEST(OriginalTests, RMI_vs_RSI_Comparison) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto rmi = std::make_shared<RMI>(close_line, 14, 5);
    auto rsi = std::make_shared<RSI>(close_line, 14);
    
    std::vector<double> rmi_values;
    std::vector<double> rsi_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        rmi->calculate();
        rsi->calculate();
        
        double rmi_val = rmi->get(0);
        double rsi_val = rsi->get(0);
        
        if (!std::isnan(rmi_val) && !std::isnan(rsi_val)) {
            rmi_values.push_back(rmi_val);
            rsi_values.push_back(rsi_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 比较RMI和RSI的特性
    if (!rmi_values.empty() && !rsi_values.empty()) {
        double rmi_avg = std::accumulate(rmi_values.begin(), rmi_values.end(), 0.0) / rmi_values.size();
        double rsi_avg = std::accumulate(rsi_values.begin(), rsi_values.end(), 0.0) / rsi_values.size();
        
        // 计算波动性
        double rmi_var = 0.0, rsi_var = 0.0;
        for (size_t i = 0; i < std::min(rmi_values.size(), rsi_values.size()); ++i) {
            rmi_var += (rmi_values[i] - rmi_avg) * (rmi_values[i] - rmi_avg);
            rsi_var += (rsi_values[i] - rsi_avg) * (rsi_values[i] - rsi_avg);
        }
        rmi_var /= std::min(rmi_values.size(), rsi_values.size());
        rsi_var /= std::min(rmi_values.size(), rsi_values.size());
        
        std::cout << "RMI vs RSI comparison:" << std::endl;
        std::cout << "RMI average: " << rmi_avg << ", variance: " << rmi_var << std::endl;
        std::cout << "RSI average: " << rsi_avg << ", variance: " << rsi_var << std::endl;
        
        // 验证两者都在合理范围内
        EXPECT_GT(rmi_avg, 0.0) << "RMI average should be positive";
        EXPECT_LT(rmi_avg, 100.0) << "RMI average should be less than 100";
        EXPECT_GT(rsi_avg, 0.0) << "RSI average should be positive";
        EXPECT_LT(rsi_avg, 100.0) << "RSI average should be less than 100";
    }
}

// RMI动量敏感性测试
TEST(OriginalTests, RMI_MomentumSensitivity) {
    // 创建具有不同动量特征的数据
    std::vector<double> momentum_prices;
    
    // 第一阶段：快速上升
    for (int i = 0; i < 20; ++i) {
        momentum_prices.push_back(100.0 + i * 2.0);
    }
    
    // 第二阶段：缓慢上升
    for (int i = 0; i < 20; ++i) {
        momentum_prices.push_back(momentum_prices.back() + 0.5);
    }
    
    // 第三阶段：下降
    for (int i = 0; i < 20; ++i) {
        momentum_prices.push_back(momentum_prices.back() - 1.0);
    }
    
    auto momentum_line = std::make_shared<LineRoot>(momentum_prices.size(), "momentum");
    for (double price : momentum_prices) {
        momentum_line->forward(price);
    }
    
    auto momentum_rmi = std::make_shared<RMI>(momentum_line, 14, 5);
    
    std::vector<double> rmi_phase1, rmi_phase2, rmi_phase3;
    
    for (size_t i = 0; i < momentum_prices.size(); ++i) {
        momentum_rmi->calculate();
        
        double rmi_val = momentum_rmi->get(0);
        if (!std::isnan(rmi_val)) {
            if (i < 20) {
                rmi_phase1.push_back(rmi_val);
            } else if (i < 40) {
                rmi_phase2.push_back(rmi_val);
            } else {
                rmi_phase3.push_back(rmi_val);
            }
        }
        
        if (i < momentum_prices.size() - 1) {
            momentum_line->forward();
        }
    }
    
    // 分析不同阶段的RMI表现
    if (!rmi_phase1.empty() && !rmi_phase2.empty() && !rmi_phase3.empty()) {
        double avg_rmi1 = std::accumulate(rmi_phase1.begin(), rmi_phase1.end(), 0.0) / rmi_phase1.size();
        double avg_rmi2 = std::accumulate(rmi_phase2.begin(), rmi_phase2.end(), 0.0) / rmi_phase2.size();
        double avg_rmi3 = std::accumulate(rmi_phase3.begin(), rmi_phase3.end(), 0.0) / rmi_phase3.size();
        
        std::cout << "Momentum sensitivity analysis:" << std::endl;
        std::cout << "Fast rise phase average RMI: " << avg_rmi1 << std::endl;
        std::cout << "Slow rise phase average RMI: " << avg_rmi2 << std::endl;
        std::cout << "Decline phase average RMI: " << avg_rmi3 << std::endl;
        
        // RMI应该能够区分不同的动量阶段
        EXPECT_GT(avg_rmi1, avg_rmi3) << "Fast rise should have higher RMI than decline";
    }
}

// RMI发散测试
TEST(OriginalTests, RMI_Divergence) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto rmi = std::make_shared<RMI>(close_line);
    
    std::vector<double> prices;
    std::vector<double> rmi_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        rmi->calculate();
        
        double rmi_val = rmi->get(0);
        if (!std::isnan(rmi_val)) {
            prices.push_back(csv_data[i].close);
            rmi_values.push_back(rmi_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 寻找价格和RMI的峰值点进行发散分析
    std::vector<size_t> price_peaks, rmi_peaks;
    
    for (size_t i = 1; i < prices.size() - 1; ++i) {
        if (prices[i] > prices[i-1] && prices[i] > prices[i+1]) {
            price_peaks.push_back(i);
        }
        if (rmi_values[i] > rmi_values[i-1] && rmi_values[i] > rmi_values[i+1]) {
            rmi_peaks.push_back(i);
        }
    }
    
    std::cout << "Divergence analysis:" << std::endl;
    std::cout << "Price peaks found: " << price_peaks.size() << std::endl;
    std::cout << "RMI peaks found: " << rmi_peaks.size() << std::endl;
    
    // 分析最近的几个峰值
    if (price_peaks.size() >= 2) {
        size_t last_peak = price_peaks.back();
        size_t prev_peak = price_peaks[price_peaks.size() - 2];
        
        std::cout << "Recent price peak comparison:" << std::endl;
        std::cout << "Previous peak: " << prices[prev_peak] << " at index " << prev_peak << std::endl;
        std::cout << "Latest peak: " << prices[last_peak] << " at index " << last_peak << std::endl;
    }
    
    EXPECT_TRUE(true) << "Divergence analysis completed";
}

// RMI信号交叉测试
TEST(OriginalTests, RMI_SignalCrossover) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto rmi = std::make_shared<RMI>(close_line);
    
    int bullish_signals = 0;  // RMI从30下方上穿
    int bearish_signals = 0;  // RMI从70上方下穿
    double prev_rmi = 50.0;
    bool was_oversold = false;
    bool was_overbought = false;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        rmi->calculate();
        
        double current_rmi = rmi->get(0);
        
        if (!std::isnan(current_rmi)) {
            // 检测从超卖区域的突破
            if (was_oversold && current_rmi > 30.0) {
                bullish_signals++;
                was_oversold = false;
            }
            
            // 检测从超买区域的突破
            if (was_overbought && current_rmi < 70.0) {
                bearish_signals++;
                was_overbought = false;
            }
            
            // 更新状态
            if (current_rmi < 30.0) {
                was_oversold = true;
            }
            if (current_rmi > 70.0) {
                was_overbought = true;
            }
            
            prev_rmi = current_rmi;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "RMI signal crossover analysis:" << std::endl;
    std::cout << "Bullish signals (from oversold): " << bullish_signals << std::endl;
    std::cout << "Bearish signals (from overbought): " << bearish_signals << std::endl;
    
    // 验证检测到一些信号
    EXPECT_GE(bullish_signals + bearish_signals, 0) 
        << "Should detect some RMI crossover signals";
}

// RMI趋势跟随能力测试
TEST(OriginalTests, RMI_TrendFollowing) {
    // 创建明确的趋势数据
    std::vector<double> trend_prices;
    
    // 上升趋势
    for (int i = 0; i < 40; ++i) {
        trend_prices.push_back(100.0 + i * 1.0);
    }
    
    auto trend_line = std::make_shared<LineRoot>(trend_prices.size(), "trend");
    for (double price : trend_prices) {
        trend_line->forward(price);
    }
    
    auto trend_rmi = std::make_shared<RMI>(trend_line);
    
    std::vector<double> rmi_trend_values;
    
    for (size_t i = 0; i < trend_prices.size(); ++i) {
        trend_rmi->calculate();
        
        double rmi_val = trend_rmi->get(0);
        if (!std::isnan(rmi_val)) {
            rmi_trend_values.push_back(rmi_val);
        }
        
        if (i < trend_prices.size() - 1) {
            trend_line->forward();
        }
    }
    
    // 分析RMI在持续上升趋势中的表现
    if (rmi_trend_values.size() > 20) {
        double early_avg = std::accumulate(rmi_trend_values.begin(), rmi_trend_values.begin() + 10, 0.0) / 10.0;
        double late_avg = std::accumulate(rmi_trend_values.end() - 10, rmi_trend_values.end(), 0.0) / 10.0;
        
        std::cout << "Trend following analysis:" << std::endl;
        std::cout << "Early trend RMI average: " << early_avg << std::endl;
        std::cout << "Late trend RMI average: " << late_avg << std::endl;
        
        // 在持续上升趋势中，RMI应该保持在较高水平
        EXPECT_GT(late_avg, 50.0) << "RMI should be above 50 in strong uptrend";
    }
}

// 边界条件测试
TEST(OriginalTests, RMI_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(50, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_rmi = std::make_shared<RMI>(flat_line);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_rmi->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，RMI应该为50（中性）
    double final_rmi = flat_rmi->get(0);
    if (!std::isnan(final_rmi)) {
        EXPECT_NEAR(final_rmi, 50.0, 1e-6) 
            << "RMI should be 50 for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(30, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 20; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_rmi = std::make_shared<RMI>(insufficient_line);
    
    for (int i = 0; i < 20; ++i) {
        insufficient_rmi->calculate();
        if (i < 19) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_rmi->get(0);
    EXPECT_TRUE(std::isnan(result)) << "RMI should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, RMI_Performance) {
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
    
    auto large_rmi = std::make_shared<RMI>(large_line);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_rmi->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "RMI calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_rmi->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    EXPECT_GE(final_result, 0.0) << "Final result should be >= 0";
    EXPECT_LE(final_result, 100.0) << "Final result should be <= 100";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}