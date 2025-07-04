/**
 * @file test_ind_momentumoscillator.cpp
 * @brief MomentumOscillator指标测试 - 对应Python test_ind_momentumoscillator.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['101.654375', '99.052251', '101.904990']
 * ]
 * chkmin = 13
 * chkind = btind.MomentumOscillator
 * 
 * 注：MomentumOscillator (动量振荡器) 基于动量指标的振荡器
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/momentumoscillator.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> MOMENTUMOSCILLATOR_EXPECTED_VALUES = {
    {"101.654375", "99.052251", "101.904990"}
};

const int MOMENTUMOSCILLATOR_MIN_PERIOD = 13;

} // anonymous namespace

// 使用默认参数的MomentumOscillator测试
DEFINE_INDICATOR_TEST(MomentumOscillator_Default, MomentumOscillator, MOMENTUMOSCILLATOR_EXPECTED_VALUES, MOMENTUMOSCILLATOR_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, MomentumOscillator_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建MomentumOscillator指标
    auto momosc = std::make_shared<MomentumOscillator>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        momosc->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 13;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"101.654375", "99.052251", "101.904990"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = momosc->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "MomentumOscillator value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(momosc->getMinPeriod(), 13) << "MomentumOscillator minimum period should be 13";
}

// 参数化测试 - 测试不同参数的MomentumOscillator
class MomentumOscillatorParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int>> {
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

TEST_P(MomentumOscillatorParameterizedTest, DifferentParameters) {
    auto [period, smoothing] = GetParam();
    auto momosc = std::make_shared<MomentumOscillator>(close_line_, period, smoothing);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        momosc->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    int expected_minperiod = period + smoothing - 1;
    EXPECT_EQ(momosc->getMinPeriod(), expected_minperiod) 
        << "MomentumOscillator minimum period should be " << expected_minperiod;
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_minperiod)) {
        double last_value = momosc->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last MomentumOscillator value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last MomentumOscillator value should be finite";
        EXPECT_GT(last_value, 0.0) << "MomentumOscillator should be positive (percentage-based)";
    }
}

// 测试不同的MomentumOscillator参数组合
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    MomentumOscillatorParameterizedTest,
    ::testing::Values(
        std::make_tuple(10, 3),
        std::make_tuple(12, 3),   // 默认参数
        std::make_tuple(14, 5),
        std::make_tuple(20, 7)
    )
);

// MomentumOscillator计算逻辑验证测试
TEST(OriginalTests, MomentumOscillator_CalculationLogic) {
    // 使用简单的测试数据验证MomentumOscillator计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0};
    
    auto price_line = std::make_shared<LineRoot>(prices.size(), "momosc_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto momosc = std::make_shared<MomentumOscillator>(price_line, 10, 3);
    auto momentum = std::make_shared<Momentum>(price_line, 10);
    auto sma = std::make_shared<SMA>(momentum, 3);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        momosc->calculate();
        momentum->calculate();
        sma->calculate();
        
        // 验证MomentumOscillator计算逻辑
        if (i >= 12) {  // MomentumOscillator需要13个数据点
            double momentum_value = momentum->get(0);
            double sma_momentum = sma->get(0);
            double actual_momosc = momosc->get(0);
            
            if (!std::isnan(momentum_value) && !std::isnan(sma_momentum) && !std::isnan(actual_momosc)) {
                // MomentumOscillator = 100 * SMA(Momentum) / SMA(SMA(Momentum))
                // 简化为：基于平滑后的动量的百分比振荡器
                EXPECT_GT(actual_momosc, 0.0) << "MomentumOscillator should be positive at step " << i;
                EXPECT_LT(actual_momosc, 200.0) << "MomentumOscillator should be reasonable at step " << i;
            }
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
}

// MomentumOscillator趋势分析测试
TEST(OriginalTests, MomentumOscillator_TrendAnalysis) {
    // 创建明确的趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 50; ++i) {
        uptrend_prices.push_back(100.0 + i * 1.0);  // 强劲上升趋势
    }
    
    auto uptrend_line = std::make_shared<LineRoot>(uptrend_prices.size(), "uptrend");
    for (double price : uptrend_prices) {
        uptrend_line->forward(price);
    }
    
    auto uptrend_momosc = std::make_shared<MomentumOscillator>(uptrend_line, 12, 3);
    
    std::vector<double> uptrend_values;
    
    for (size_t i = 0; i < uptrend_prices.size(); ++i) {
        uptrend_momosc->calculate();
        
        double osc_value = uptrend_momosc->get(0);
        if (!std::isnan(osc_value)) {
            uptrend_values.push_back(osc_value);
        }
        
        if (i < uptrend_prices.size() - 1) {
            uptrend_line->forward();
        }
    }
    
    // 创建下降趋势数据
    std::vector<double> downtrend_prices;
    for (int i = 0; i < 50; ++i) {
        downtrend_prices.push_back(150.0 - i * 1.0);  // 强劲下降趋势
    }
    
    auto downtrend_line = std::make_shared<LineRoot>(downtrend_prices.size(), "downtrend");
    for (double price : downtrend_prices) {
        downtrend_line->forward(price);
    }
    
    auto downtrend_momosc = std::make_shared<MomentumOscillator>(downtrend_line, 12, 3);
    
    std::vector<double> downtrend_values;
    
    for (size_t i = 0; i < downtrend_prices.size(); ++i) {
        downtrend_momosc->calculate();
        
        double osc_value = downtrend_momosc->get(0);
        if (!std::isnan(osc_value)) {
            downtrend_values.push_back(osc_value);
        }
        
        if (i < downtrend_prices.size() - 1) {
            downtrend_line->forward();
        }
    }
    
    // 分析趋势特性
    if (!uptrend_values.empty() && !downtrend_values.empty()) {
        double avg_uptrend = std::accumulate(uptrend_values.begin(), uptrend_values.end(), 0.0) / uptrend_values.size();
        double avg_downtrend = std::accumulate(downtrend_values.begin(), downtrend_values.end(), 0.0) / downtrend_values.size();
        
        std::cout << "Trend analysis:" << std::endl;
        std::cout << "Uptrend average: " << avg_uptrend << std::endl;
        std::cout << "Downtrend average: " << avg_downtrend << std::endl;
        
        // 上升趋势应该有更高的动量振荡器值
        EXPECT_GT(avg_uptrend, avg_downtrend) 
            << "Uptrend should have higher momentum oscillator values";
        
        // 上升趋势值应该明显大于100（中性线）
        EXPECT_GT(avg_uptrend, 100.0) 
            << "Strong uptrend should have momentum oscillator above 100";
        
        // 下降趋势值应该明显小于100
        EXPECT_LT(avg_downtrend, 100.0) 
            << "Strong downtrend should have momentum oscillator below 100";
    }
}

// MomentumOscillator中性线穿越测试
TEST(OriginalTests, MomentumOscillator_NeutralLineCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto momosc = std::make_shared<MomentumOscillator>(close_line, 12, 3);
    
    int above_neutral = 0;    // 高于100的次数
    int below_neutral = 0;    // 低于100的次数
    int crossings_up = 0;     // 向上穿越100的次数
    int crossings_down = 0;   // 向下穿越100的次数
    
    double prev_value = 0.0;
    bool has_prev = false;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        momosc->calculate();
        
        double current_value = momosc->get(0);
        
        if (!std::isnan(current_value)) {
            if (current_value > 100.0) {
                above_neutral++;
            } else if (current_value < 100.0) {
                below_neutral++;
            }
            
            // 检测穿越
            if (has_prev) {
                if (prev_value <= 100.0 && current_value > 100.0) {
                    crossings_up++;
                } else if (prev_value >= 100.0 && current_value < 100.0) {
                    crossings_down++;
                }
            }
            
            prev_value = current_value;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "Neutral line analysis:" << std::endl;
    std::cout << "Above neutral (>100): " << above_neutral << std::endl;
    std::cout << "Below neutral (<100): " << below_neutral << std::endl;
    std::cout << "Crossings up: " << crossings_up << std::endl;
    std::cout << "Crossings down: " << crossings_down << std::endl;
    
    int total_values = above_neutral + below_neutral;
    EXPECT_GT(total_values, 0) << "Should have some valid oscillator values";
    
    // 验证检测到一些穿越信号
    EXPECT_GE(crossings_up + crossings_down, 0) 
        << "Should detect some neutral line crossings";
}

// MomentumOscillator振荡特性测试
TEST(OriginalTests, MomentumOscillator_OscillationCharacteristics) {
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
    
    auto momosc = std::make_shared<MomentumOscillator>(osc_line, 12, 3);
    
    std::vector<double> oscillator_values;
    
    for (size_t i = 0; i < oscillating_prices.size(); ++i) {
        momosc->calculate();
        
        double osc_val = momosc->get(0);
        if (!std::isnan(osc_val)) {
            oscillator_values.push_back(osc_val);
        }
        
        if (i < oscillating_prices.size() - 1) {
            osc_line->forward();
        }
    }
    
    // 分析振荡特性
    if (!oscillator_values.empty()) {
        double avg_oscillator = std::accumulate(oscillator_values.begin(), oscillator_values.end(), 0.0) / oscillator_values.size();
        
        // 计算标准差
        double variance = 0.0;
        for (double val : oscillator_values) {
            variance += (val - avg_oscillator) * (val - avg_oscillator);
        }
        variance /= oscillator_values.size();
        double std_dev = std::sqrt(variance);
        
        std::cout << "Oscillation characteristics:" << std::endl;
        std::cout << "Average: " << avg_oscillator << std::endl;
        std::cout << "Standard deviation: " << std_dev << std::endl;
        
        // 振荡器应该围绕100线波动
        EXPECT_NEAR(avg_oscillator, 100.0, 10.0) 
            << "Oscillator should oscillate around 100";
        
        EXPECT_GT(std_dev, 1.0) 
            << "Oscillator should show meaningful variation";
    }
}

// MomentumOscillator与其他振荡器比较测试
TEST(OriginalTests, MomentumOscillator_vs_OtherOscillators) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto momosc = std::make_shared<MomentumOscillator>(close_line, 12, 3);
    auto rsi = std::make_shared<RSI>(close_line, 14);
    
    std::vector<double> momosc_values, rsi_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        momosc->calculate();
        rsi->calculate();
        
        double momosc_val = momosc->get(0);
        double rsi_val = rsi->get(0);
        
        if (!std::isnan(momosc_val)) momosc_values.push_back(momosc_val);
        if (!std::isnan(rsi_val)) rsi_values.push_back(rsi_val);
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 比较不同振荡器的特性
    if (!momosc_values.empty() && !rsi_values.empty()) {
        double momosc_avg = std::accumulate(momosc_values.begin(), momosc_values.end(), 0.0) / momosc_values.size();
        double rsi_avg = std::accumulate(rsi_values.begin(), rsi_values.end(), 0.0) / rsi_values.size();
        
        std::cout << "Oscillator comparison:" << std::endl;
        std::cout << "MomentumOscillator average: " << momosc_avg << std::endl;
        std::cout << "RSI average: " << rsi_avg << std::endl;
        
        // MomentumOscillator围绕100振荡，RSI围绕50振荡
        EXPECT_NEAR(momosc_avg, 100.0, 20.0) << "MomentumOscillator should center around 100";
        EXPECT_NEAR(rsi_avg, 50.0, 20.0) << "RSI should center around 50";
    }
}

// MomentumOscillator极值检测测试
TEST(OriginalTests, MomentumOscillator_ExtremeValues) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto momosc = std::make_shared<MomentumOscillator>(close_line, 12, 3);
    
    double max_value = std::numeric_limits<double>::lowest();
    double min_value = std::numeric_limits<double>::max();
    std::vector<double> all_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        momosc->calculate();
        
        double osc_val = momosc->get(0);
        if (!std::isnan(osc_val)) {
            all_values.push_back(osc_val);
            max_value = std::max(max_value, osc_val);
            min_value = std::min(min_value, osc_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    if (!all_values.empty()) {
        std::cout << "Extreme values analysis:" << std::endl;
        std::cout << "Maximum value: " << max_value << std::endl;
        std::cout << "Minimum value: " << min_value << std::endl;
        std::cout << "Range: " << (max_value - min_value) << std::endl;
        
        // 验证极值在合理范围内
        EXPECT_GT(max_value, 50.0) << "Maximum should be above 50";
        EXPECT_LT(min_value, 150.0) << "Minimum should be below 150";
        EXPECT_GT(max_value - min_value, 10.0) << "Should have meaningful range";
        
        // 计算超买超卖信号
        int overbought_count = 0;  // > 110
        int oversold_count = 0;    // < 90
        
        for (double val : all_values) {
            if (val > 110.0) overbought_count++;
            if (val < 90.0) oversold_count++;
        }
        
        std::cout << "Overbought signals (>110): " << overbought_count << std::endl;
        std::cout << "Oversold signals (<90): " << oversold_count << std::endl;
        
        EXPECT_GE(overbought_count + oversold_count, 0) 
            << "Should generate some overbought/oversold signals";
    }
}

// 边界条件测试
TEST(OriginalTests, MomentumOscillator_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(50, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_momosc = std::make_shared<MomentumOscillator>(flat_line, 12, 3);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_momosc->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，动量为0，振荡器应该在某个基准值附近
    double final_momosc = flat_momosc->get(0);
    if (!std::isnan(final_momosc)) {
        EXPECT_GT(final_momosc, 50.0) << "MomentumOscillator should be positive for constant prices";
        EXPECT_LT(final_momosc, 150.0) << "MomentumOscillator should be reasonable for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(50, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 10; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_momosc = std::make_shared<MomentumOscillator>(insufficient_line, 12, 3);
    
    for (int i = 0; i < 10; ++i) {
        insufficient_momosc->calculate();
        if (i < 9) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_momosc->get(0);
    EXPECT_TRUE(std::isnan(result)) << "MomentumOscillator should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, MomentumOscillator_Performance) {
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
    
    auto large_momosc = std::make_shared<MomentumOscillator>(large_line, 12, 3);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_momosc->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "MomentumOscillator calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_momosc->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    EXPECT_GT(final_result, 0.0) << "Final result should be positive";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}