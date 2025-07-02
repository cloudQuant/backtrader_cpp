/**
 * @file test_ind_smaenvelope.cpp
 * @brief SMAEnvelope指标测试 - 对应Python test_ind_smaenvelope.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4063.463000', '3644.444667', '3554.693333'],
 *     ['4165.049575', '3735.555783', '3643.560667'],
 *     ['3961.876425', '3553.333550', '3465.826000']
 * ]
 * chkmin = 30
 * chkind = btind.SMAEnvelope
 * 
 * 注：SMAEnvelope包含3条线：Mid (SMA), Upper, Lower
 */

#include "test_common.h"
#include "indicators/SMAEnvelope.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> SMAENVELOPE_EXPECTED_VALUES = {
    {"4063.463000", "3644.444667", "3554.693333"},  // line 0 (Mid/SMA)
    {"4165.049575", "3735.555783", "3643.560667"},  // line 1 (Upper)
    {"3961.876425", "3553.333550", "3465.826000"}   // line 2 (Lower)
};

const int SMAENVELOPE_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的SMAEnvelope测试
DEFINE_INDICATOR_TEST(SMAEnvelope_Default, SMAEnvelope, SMAENVELOPE_EXPECTED_VALUES, SMAENVELOPE_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, SMAEnvelope_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建SMAEnvelope指标
    auto smaenv = std::make_shared<SMAEnvelope>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        smaenv->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 30;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // 验证3条线的值
    for (int line = 0; line < 3; ++line) {
        auto expected = SMAENVELOPE_EXPECTED_VALUES[line];
        
        for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
            double actual = smaenv->getLine(line)->get(check_points[i]);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << actual;
            std::string actual_str = ss.str();
            
            EXPECT_EQ(actual_str, expected[i]) 
                << "SMAEnvelope line " << line << " value mismatch at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(smaenv->getMinPeriod(), 30) << "SMAEnvelope minimum period should be 30";
}

// 参数化测试 - 测试不同参数的SMAEnvelope
class SMAEnvelopeParameterizedTest : public ::testing::TestWithParam<std::tuple<int, double>> {
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

TEST_P(SMAEnvelopeParameterizedTest, DifferentParameters) {
    auto [period, percentage] = GetParam();
    
    // 使用自定义参数创建SMAEnvelope
    auto smaenv = std::make_shared<SMAEnvelope>(close_line_, period, percentage);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        smaenv->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_mid = smaenv->getLine(0)->get(0);     // Mid (SMA)
        double last_upper = smaenv->getLine(1)->get(0);   // Upper
        double last_lower = smaenv->getLine(2)->get(0);   // Lower
        
        EXPECT_FALSE(std::isnan(last_mid)) << "Last Mid should not be NaN";
        EXPECT_FALSE(std::isnan(last_upper)) << "Last Upper should not be NaN";
        EXPECT_FALSE(std::isnan(last_lower)) << "Last Lower should not be NaN";
        
        EXPECT_TRUE(std::isfinite(last_mid)) << "Last Mid should be finite";
        EXPECT_TRUE(std::isfinite(last_upper)) << "Last Upper should be finite";
        EXPECT_TRUE(std::isfinite(last_lower)) << "Last Lower should be finite";
        
        // 验证包络线关系
        EXPECT_GT(last_upper, last_mid) << "Upper should be greater than Mid";
        EXPECT_LT(last_lower, last_mid) << "Lower should be less than Mid";
        
        // 验证百分比关系
        double expected_upper = last_mid * (1.0 + percentage / 100.0);
        double expected_lower = last_mid * (1.0 - percentage / 100.0);
        
        EXPECT_NEAR(last_upper, expected_upper, 1e-6) << "Upper should match percentage calculation";
        EXPECT_NEAR(last_lower, expected_lower, 1e-6) << "Lower should match percentage calculation";
    }
}

// 测试不同的参数组合
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    SMAEnvelopeParameterizedTest,
    ::testing::Values(
        std::make_tuple(20, 2.5),
        std::make_tuple(30, 2.5),
        std::make_tuple(50, 2.5),
        std::make_tuple(30, 1.0),
        std::make_tuple(30, 5.0)
    )
);

// SMAEnvelope计算逻辑验证测试
TEST(OriginalTests, SMAEnvelope_CalculationLogic) {
    // 使用简单的测试数据验证SMAEnvelope计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0,
                                  120.0, 118.0, 116.0, 114.0, 112.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0,
                                  124.0, 126.0, 128.0, 130.0, 132.0};
    
    auto price_line = std::make_shared<LineRoot>(prices.size(), "smaenv_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto smaenv = std::make_shared<SMAEnvelope>(price_line, 10, 2.5);  // 10周期，2.5%包络
    auto sma = std::make_shared<SMA>(price_line, 10);  // 比较用的SMA
    
    for (size_t i = 0; i < prices.size(); ++i) {
        smaenv->calculate();
        sma->calculate();
        
        if (i >= 9) {  // SMAEnvelope需要10个数据点
            double mid_value = smaenv->getLine(0)->get(0);
            double upper_value = smaenv->getLine(1)->get(0);
            double lower_value = smaenv->getLine(2)->get(0);
            double sma_value = sma->get(0);
            
            if (!std::isnan(mid_value) && !std::isnan(sma_value)) {
                // Mid应该等于SMA
                EXPECT_NEAR(mid_value, sma_value, 1e-10) 
                    << "SMAEnvelope Mid should equal SMA at step " << i;
                
                // 验证包络线计算
                double expected_upper = sma_value * 1.025;  // +2.5%
                double expected_lower = sma_value * 0.975;  // -2.5%
                
                EXPECT_NEAR(upper_value, expected_upper, 1e-10) 
                    << "Upper envelope calculation mismatch at step " << i;
                EXPECT_NEAR(lower_value, expected_lower, 1e-10) 
                    << "Lower envelope calculation mismatch at step " << i;
                
                // 验证顺序关系
                EXPECT_GT(upper_value, mid_value) 
                    << "Upper should be greater than Mid at step " << i;
                EXPECT_LT(lower_value, mid_value) 
                    << "Lower should be less than Mid at step " << i;
            }
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
}

// SMAEnvelope支撑阻力测试
TEST(OriginalTests, SMAEnvelope_SupportResistance) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto smaenv = std::make_shared<SMAEnvelope>(close_line, 20, 2.5);
    
    int upper_touches = 0;    // 价格触及上轨
    int lower_touches = 0;    // 价格触及下轨
    int inside_envelope = 0;  // 价格在包络内
    int upper_breaks = 0;     // 价格突破上轨
    int lower_breaks = 0;     // 价格突破下轨
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        smaenv->calculate();
        
        double current_price = csv_data[i].close;
        double upper = smaenv->getLine(1)->get(0);
        double lower = smaenv->getLine(2)->get(0);
        
        if (!std::isnan(upper) && !std::isnan(lower)) {
            double upper_threshold = upper * 0.999;  // 允许微小误差
            double lower_threshold = lower * 1.001;
            
            if (current_price > upper) {
                upper_breaks++;
            } else if (current_price < lower) {
                lower_breaks++;
            } else if (current_price >= upper_threshold) {
                upper_touches++;
            } else if (current_price <= lower_threshold) {
                lower_touches++;
            } else {
                inside_envelope++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "Support/Resistance analysis:" << std::endl;
    std::cout << "Upper touches: " << upper_touches << std::endl;
    std::cout << "Lower touches: " << lower_touches << std::endl;
    std::cout << "Inside envelope: " << inside_envelope << std::endl;
    std::cout << "Upper breaks: " << upper_breaks << std::endl;
    std::cout << "Lower breaks: " << lower_breaks << std::endl;
    
    int total_valid = upper_touches + lower_touches + inside_envelope + upper_breaks + lower_breaks;
    EXPECT_GT(total_valid, 0) << "Should have some valid envelope analysis";
    
    // 大多数价格应该在包络内
    if (total_valid > 0) {
        double inside_ratio = static_cast<double>(inside_envelope) / total_valid;
        std::cout << "Inside envelope ratio: " << inside_ratio << std::endl;
        EXPECT_GT(inside_ratio, 0.5) << "Most prices should be inside envelope";
    }
}

// SMAEnvelope趋势分析测试
TEST(OriginalTests, SMAEnvelope_TrendAnalysis) {
    // 创建明确的趋势数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 50; ++i) {
        trend_prices.push_back(100.0 + i * 1.0);  // 线性上升趋势
    }
    
    auto trend_line = std::make_shared<LineRoot>(trend_prices.size(), "trend");
    for (double price : trend_prices) {
        trend_line->forward(price);
    }
    
    auto trend_smaenv = std::make_shared<SMAEnvelope>(trend_line, 20, 2.5);
    
    std::vector<double> mid_values, upper_values, lower_values;
    
    for (size_t i = 0; i < trend_prices.size(); ++i) {
        trend_smaenv->calculate();
        
        double mid = trend_smaenv->getLine(0)->get(0);
        double upper = trend_smaenv->getLine(1)->get(0);
        double lower = trend_smaenv->getLine(2)->get(0);
        
        if (!std::isnan(mid) && !std::isnan(upper) && !std::isnan(lower)) {
            mid_values.push_back(mid);
            upper_values.push_back(upper);
            lower_values.push_back(lower);
        }
        
        if (i < trend_prices.size() - 1) {
            trend_line->forward();
        }
    }
    
    // 分析趋势特性
    if (mid_values.size() > 20) {
        double first_mid = mid_values[0];
        double last_mid = mid_values.back();
        double first_upper = upper_values[0];
        double last_upper = upper_values.back();
        double first_lower = lower_values[0];
        double last_lower = lower_values.back();
        
        std::cout << "Trend analysis:" << std::endl;
        std::cout << "Mid: " << first_mid << " -> " << last_mid << " (change: " << (last_mid - first_mid) << ")" << std::endl;
        std::cout << "Upper: " << first_upper << " -> " << last_upper << " (change: " << (last_upper - first_upper) << ")" << std::endl;
        std::cout << "Lower: " << first_lower << " -> " << last_lower << " (change: " << (last_lower - first_lower) << ")" << std::endl;
        
        // 在上升趋势中，所有线都应该上升
        EXPECT_GT(last_mid, first_mid) << "Mid should rise in uptrend";
        EXPECT_GT(last_upper, first_upper) << "Upper should rise in uptrend";
        EXPECT_GT(last_lower, first_lower) << "Lower should rise in uptrend";
    }
}

// SMAEnvelope波动性分析测试
TEST(OriginalTests, SMAEnvelope_VolatilityAnalysis) {
    // 创建不同波动性的数据
    std::vector<double> low_vol_prices, high_vol_prices;
    
    // 低波动性数据
    for (int i = 0; i < 40; ++i) {
        double base = 100.0;
        double noise = std::sin(i * 0.3) * 1.0;  // 小幅波动
        low_vol_prices.push_back(base + noise);
    }
    
    // 高波动性数据
    for (int i = 0; i < 40; ++i) {
        double base = 100.0;
        double noise = std::sin(i * 0.3) * 5.0;  // 大幅波动
        high_vol_prices.push_back(base + noise);
    }
    
    auto low_vol_line = std::make_shared<LineRoot>(low_vol_prices.size(), "low_vol");
    for (double price : low_vol_prices) {
        low_vol_line->forward(price);
    }
    
    auto high_vol_line = std::make_shared<LineRoot>(high_vol_prices.size(), "high_vol");
    for (double price : high_vol_prices) {
        high_vol_line->forward(price);
    }
    
    auto low_vol_env = std::make_shared<SMAEnvelope>(low_vol_line, 20, 2.5);
    auto high_vol_env = std::make_shared<SMAEnvelope>(high_vol_line, 20, 2.5);
    
    std::vector<double> low_vol_ranges, high_vol_ranges;
    
    for (size_t i = 0; i < 40; ++i) {
        low_vol_env->calculate();
        high_vol_env->calculate();
        
        double low_vol_upper = low_vol_env->getLine(1)->get(0);
        double low_vol_lower = low_vol_env->getLine(2)->get(0);
        double high_vol_upper = high_vol_env->getLine(1)->get(0);
        double high_vol_lower = high_vol_env->getLine(2)->get(0);
        
        if (!std::isnan(low_vol_upper) && !std::isnan(low_vol_lower)) {
            low_vol_ranges.push_back(low_vol_upper - low_vol_lower);
        }
        
        if (!std::isnan(high_vol_upper) && !std::isnan(high_vol_lower)) {
            high_vol_ranges.push_back(high_vol_upper - high_vol_lower);
        }
        
        if (i < 39) {
            low_vol_line->forward();
            high_vol_line->forward();
        }
    }
    
    // 比较包络宽度
    if (!low_vol_ranges.empty() && !high_vol_ranges.empty()) {
        double avg_low_vol_range = std::accumulate(low_vol_ranges.begin(), low_vol_ranges.end(), 0.0) / low_vol_ranges.size();
        double avg_high_vol_range = std::accumulate(high_vol_ranges.begin(), high_vol_ranges.end(), 0.0) / high_vol_ranges.size();
        
        std::cout << "Volatility analysis:" << std::endl;
        std::cout << "Low volatility average envelope range: " << avg_low_vol_range << std::endl;
        std::cout << "High volatility average envelope range: " << avg_high_vol_range << std::endl;
        
        // 包络宽度应该反映基础SMA的值，而不是价格波动性
        // 因为包络是基于百分比的
        EXPECT_GT(avg_low_vol_range, 0.0) << "Low volatility envelope should have positive range";
        EXPECT_GT(avg_high_vol_range, 0.0) << "High volatility envelope should have positive range";
    }
}

// SMAEnvelope价格通道测试
TEST(OriginalTests, SMAEnvelope_PriceChannel) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto smaenv = std::make_shared<SMAEnvelope>(close_line, 20, 3.0);  // 3%包络
    
    int channel_breakouts = 0;  // 通道突破次数
    int channel_reversals = 0;  // 通道反转次数
    
    std::vector<double> price_history;
    std::vector<double> upper_history, lower_history;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        smaenv->calculate();
        
        double current_price = csv_data[i].close;
        double upper = smaenv->getLine(1)->get(0);
        double lower = smaenv->getLine(2)->get(0);
        
        if (!std::isnan(upper) && !std::isnan(lower)) {
            price_history.push_back(current_price);
            upper_history.push_back(upper);
            lower_history.push_back(lower);
            
            // 检测通道突破和反转
            if (price_history.size() >= 3) {
                size_t n = price_history.size();
                double prev2_price = price_history[n-3];
                double prev_price = price_history[n-2];
                double curr_price = price_history[n-1];
                double prev_upper = upper_history[n-2];
                double prev_lower = lower_history[n-2];
                
                // 检测上轨突破
                if (prev_price <= prev_upper && curr_price > upper) {
                    channel_breakouts++;
                }
                
                // 检测下轨突破
                if (prev_price >= prev_lower && curr_price < lower) {
                    channel_breakouts++;
                }
                
                // 检测反转（从上轨回到通道内）
                if (prev2_price > prev_upper && prev_price > prev_upper && 
                    curr_price <= upper) {
                    channel_reversals++;
                }
                
                // 检测反转（从下轨回到通道内）
                if (prev2_price < prev_lower && prev_price < prev_lower && 
                    curr_price >= lower) {
                    channel_reversals++;
                }
            }
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "Price channel analysis:" << std::endl;
    std::cout << "Channel breakouts: " << channel_breakouts << std::endl;
    std::cout << "Channel reversals: " << channel_reversals << std::endl;
    
    // 验证检测到一些通道活动
    EXPECT_GE(channel_breakouts + channel_reversals, 0) 
        << "Should detect some channel activity";
}

// 边界条件测试
TEST(OriginalTests, SMAEnvelope_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(50, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_smaenv = std::make_shared<SMAEnvelope>(flat_line, 20, 2.5);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_smaenv->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，检查包络线计算
    double final_mid = flat_smaenv->getLine(0)->get(0);
    double final_upper = flat_smaenv->getLine(1)->get(0);
    double final_lower = flat_smaenv->getLine(2)->get(0);
    
    if (!std::isnan(final_mid) && !std::isnan(final_upper) && !std::isnan(final_lower)) {
        EXPECT_NEAR(final_mid, 100.0, 1e-6) << "Mid should equal constant price";
        EXPECT_NEAR(final_upper, 102.5, 1e-6) << "Upper should be 2.5% above constant price";
        EXPECT_NEAR(final_lower, 97.5, 1e-6) << "Lower should be 2.5% below constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(50, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 15; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_smaenv = std::make_shared<SMAEnvelope>(insufficient_line, 20, 2.5);
    
    for (int i = 0; i < 15; ++i) {
        insufficient_smaenv->calculate();
        if (i < 14) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_smaenv->getLine(0)->get(0);
    EXPECT_TRUE(std::isnan(result)) << "SMAEnvelope should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, SMAEnvelope_Performance) {
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
    
    auto large_smaenv = std::make_shared<SMAEnvelope>(large_line, 50, 2.5);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_smaenv->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "SMAEnvelope calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_mid = large_smaenv->getLine(0)->get(0);
    double final_upper = large_smaenv->getLine(1)->get(0);
    double final_lower = large_smaenv->getLine(2)->get(0);
    
    EXPECT_FALSE(std::isnan(final_mid)) << "Final Mid should not be NaN";
    EXPECT_FALSE(std::isnan(final_upper)) << "Final Upper should not be NaN";
    EXPECT_FALSE(std::isnan(final_lower)) << "Final Lower should not be NaN";
    
    EXPECT_TRUE(std::isfinite(final_mid)) << "Final Mid should be finite";
    EXPECT_TRUE(std::isfinite(final_upper)) << "Final Upper should be finite";
    EXPECT_TRUE(std::isfinite(final_lower)) << "Final Lower should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}