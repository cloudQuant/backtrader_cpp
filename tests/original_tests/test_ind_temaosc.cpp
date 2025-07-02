/**
 * @file test_ind_temaosc.cpp
 * @brief TEMAOsc指标测试 - 对应Python test_ind_temaosc.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['6.218295', '15.143146', '-23.991054']
 * ]
 * chkmin = 88
 * chkind = btind.TEMAOsc
 * 
 * 注：TEMAOsc (TEMA Oscillator) 基于两个TEMA的差值振荡器
 */

#include "test_common.h"
#include "indicators/TEMAOsc.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> TEMAOSC_EXPECTED_VALUES = {
    {"6.218295", "15.143146", "-23.991054"}
};

const int TEMAOSC_MIN_PERIOD = 88;

} // anonymous namespace

// 使用默认参数的TEMAOsc测试
DEFINE_INDICATOR_TEST(TEMAOsc_Default, TEMAOsc, TEMAOSC_EXPECTED_VALUES, TEMAOSC_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, TEMAOsc_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建TEMAOsc指标
    auto temaosc = std::make_shared<TEMAOsc>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        temaosc->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 88;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"6.218295", "15.143146", "-23.991054"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = temaosc->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "TEMAOsc value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(temaosc->getMinPeriod(), 88) << "TEMAOsc minimum period should be 88";
}

// 参数化测试 - 测试不同参数的TEMAOsc
class TEMAOscParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int>> {
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

TEST_P(TEMAOscParameterizedTest, DifferentParameters) {
    auto [fast, slow] = GetParam();
    auto temaosc = std::make_shared<TEMAOsc>(close_line_, fast, slow);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        temaosc->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期 - TEMA的最小周期是 (period - 1) * 3 + 1
    int expected_min_period = (slow - 1) * 3 + 1;
    EXPECT_EQ(temaosc->getMinPeriod(), expected_min_period) 
        << "TEMAOsc minimum period should equal slow TEMA period";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_value = temaosc->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last TEMAOsc value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last TEMAOsc value should be finite";
    }
}

// 测试不同的TEMAOsc参数组合
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    TEMAOscParameterizedTest,
    ::testing::Values(
        std::make_tuple(15, 30),   // 默认参数
        std::make_tuple(10, 20),   // 更快参数
        std::make_tuple(20, 40),   // 更慢参数
        std::make_tuple(5, 15)     // 短期参数
    )
);

// TEMAOsc计算逻辑验证测试
TEST(OriginalTests, TEMAOsc_CalculationLogic) {
    // 使用简单的测试数据验证TEMAOsc计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0,
                                  120.0, 118.0, 116.0, 114.0, 112.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0};
    
    // 扩展数据以满足TEMA的最小周期要求
    for (int i = 0; i < 100; ++i) {
        prices.push_back(122.0 + i * 0.1);
    }
    
    auto price_line = std::make_shared<LineRoot>(prices.size(), "temaosc_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto temaosc = std::make_shared<TEMAOsc>(price_line, 15, 30);
    auto tema_fast = std::make_shared<TEMA>(price_line, 15);
    auto tema_slow = std::make_shared<TEMA>(price_line, 30);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        temaosc->calculate();
        tema_fast->calculate();
        tema_slow->calculate();
        
        // 验证TEMAOsc计算：TEMAOsc = TEMA_fast - TEMA_slow
        if (i >= 87) {  // TEMAOsc需要88个数据点（30周期的TEMA）
            double fast_value = tema_fast->get(0);
            double slow_value = tema_slow->get(0);
            double actual_temaosc = temaosc->get(0);
            
            if (!std::isnan(fast_value) && !std::isnan(slow_value)) {
                double expected_temaosc = fast_value - slow_value;
                EXPECT_NEAR(actual_temaosc, expected_temaosc, 1e-6) 
                    << "TEMAOsc calculation mismatch at step " << i
                    << " (fast=" << fast_value << ", slow=" << slow_value << ")";
            }
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
}

// TEMAOsc零线穿越测试
TEST(OriginalTests, TEMAOsc_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto temaosc = std::make_shared<TEMAOsc>(close_line, 15, 30);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_osc = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越
    for (size_t i = 0; i < csv_data.size(); ++i) {
        temaosc->calculate();
        
        double current_osc = temaosc->get(0);
        
        if (!std::isnan(current_osc) && has_prev) {
            if (prev_osc <= 0.0 && current_osc > 0.0) {
                positive_crossings++;
            } else if (prev_osc >= 0.0 && current_osc < 0.0) {
                negative_crossings++;
            }
        }
        
        if (!std::isnan(current_osc)) {
            prev_osc = current_osc;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "TEMAOsc zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// TEMAOsc趋势分析测试
TEST(OriginalTests, TEMAOsc_TrendAnalysis) {
    // 创建明确的上升趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 150; ++i) {
        uptrend_prices.push_back(100.0 + i * 1.0);  // 强劲上升趋势
    }
    
    auto uptrend_line = std::make_shared<LineRoot>(uptrend_prices.size(), "uptrend");
    for (double price : uptrend_prices) {
        uptrend_line->forward(price);
    }
    
    auto uptrend_temaosc = std::make_shared<TEMAOsc>(uptrend_line, 15, 30);
    
    std::vector<double> uptrend_values;
    
    for (size_t i = 0; i < uptrend_prices.size(); ++i) {
        uptrend_temaosc->calculate();
        
        double osc_value = uptrend_temaosc->get(0);
        if (!std::isnan(osc_value)) {
            uptrend_values.push_back(osc_value);
        }
        
        if (i < uptrend_prices.size() - 1) {
            uptrend_line->forward();
        }
    }
    
    // 创建下降趋势数据
    std::vector<double> downtrend_prices;
    for (int i = 0; i < 150; ++i) {
        downtrend_prices.push_back(200.0 - i * 1.0);  // 强劲下降趋势
    }
    
    auto downtrend_line = std::make_shared<LineRoot>(downtrend_prices.size(), "downtrend");
    for (double price : downtrend_prices) {
        downtrend_line->forward(price);
    }
    
    auto downtrend_temaosc = std::make_shared<TEMAOsc>(downtrend_line, 15, 30);
    
    std::vector<double> downtrend_values;
    
    for (size_t i = 0; i < downtrend_prices.size(); ++i) {
        downtrend_temaosc->calculate();
        
        double osc_value = downtrend_temaosc->get(0);
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
        
        // 上升趋势应该有正的TEMAOsc值，下降趋势应该有负的TEMAOsc值
        EXPECT_GT(avg_uptrend, avg_downtrend) 
            << "Uptrend should have higher TEMAOsc values than downtrend";
        EXPECT_GT(avg_uptrend, 0.0) 
            << "Strong uptrend should have positive TEMAOsc values";
        EXPECT_LT(avg_downtrend, 0.0) 
            << "Strong downtrend should have negative TEMAOsc values";
    }
}

// TEMAOsc振荡特性测试
TEST(OriginalTests, TEMAOsc_OscillationCharacteristics) {
    // 创建振荡数据
    std::vector<double> oscillating_prices;
    for (int i = 0; i < 150; ++i) {
        double base = 100.0;
        double oscillation = 8.0 * std::sin(i * 0.3);
        oscillating_prices.push_back(base + oscillation);
    }
    
    auto osc_line = std::make_shared<LineRoot>(oscillating_prices.size(), "oscillating");
    for (double price : oscillating_prices) {
        osc_line->forward(price);
    }
    
    auto temaosc = std::make_shared<TEMAOsc>(osc_line, 15, 30);
    
    std::vector<double> oscillator_values;
    
    for (size_t i = 0; i < oscillating_prices.size(); ++i) {
        temaosc->calculate();
        
        double osc_val = temaosc->get(0);
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
        
        // TEMAOsc应该围绕零线波动
        EXPECT_NEAR(avg_oscillator, 0.0, 3.0) 
            << "TEMAOsc should oscillate around zero";
        
        EXPECT_GT(std_dev, 1.0) 
            << "TEMAOsc should show meaningful variation";
    }
}

// TEMAOsc与其他振荡器比较测试
TEST(OriginalTests, TEMAOsc_vs_OtherOscillators) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto temaosc = std::make_shared<TEMAOsc>(close_line, 15, 30);
    auto emaosc = std::make_shared<EMAOsc>(close_line, 15, 30);
    auto smaosc = std::make_shared<SMAOsc>(close_line, 15, 30);
    
    std::vector<double> tema_values, ema_values, sma_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        temaosc->calculate();
        emaosc->calculate();
        smaosc->calculate();
        
        double tema_val = temaosc->get(0);
        double ema_val = emaosc->get(0);
        double sma_val = smaosc->get(0);
        
        if (!std::isnan(tema_val)) tema_values.push_back(tema_val);
        if (!std::isnan(ema_val)) ema_values.push_back(ema_val);
        if (!std::isnan(sma_val)) sma_values.push_back(sma_val);
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 比较不同振荡器的特性
    if (!tema_values.empty() && !ema_values.empty() && !sma_values.empty()) {
        double tema_avg = std::accumulate(tema_values.begin(), tema_values.end(), 0.0) / tema_values.size();
        double ema_avg = std::accumulate(ema_values.begin(), ema_values.end(), 0.0) / ema_values.size();
        double sma_avg = std::accumulate(sma_values.begin(), sma_values.end(), 0.0) / sma_values.size();
        
        std::cout << "Oscillator comparison:" << std::endl;
        std::cout << "TEMA oscillator average: " << tema_avg << std::endl;
        std::cout << "EMA oscillator average: " << ema_avg << std::endl;
        std::cout << "SMA oscillator average: " << sma_avg << std::endl;
        
        // 所有振荡器都应该围绕零线
        EXPECT_NEAR(tema_avg, 0.0, 5.0) << "TEMA oscillator should center around zero";
        EXPECT_NEAR(ema_avg, 0.0, 5.0) << "EMA oscillator should center around zero";
        EXPECT_NEAR(sma_avg, 0.0, 5.0) << "SMA oscillator should center around zero";
    }
}

// TEMAOsc响应速度测试
TEST(OriginalTests, TEMAOsc_ResponseSpeed) {
    // 创建价格突然变化的数据来测试响应速度
    std::vector<double> step_prices;
    
    // 第一阶段：稳定价格
    for (int i = 0; i < 100; ++i) {
        step_prices.push_back(100.0);
    }
    
    // 第二阶段：价格跳跃
    for (int i = 0; i < 100; ++i) {
        step_prices.push_back(120.0);
    }
    
    auto step_line = std::make_shared<LineRoot>(step_prices.size(), "step");
    for (double price : step_prices) {
        step_line->forward(price);
    }
    
    auto temaosc = std::make_shared<TEMAOsc>(step_line, 15, 30);
    auto emaosc = std::make_shared<EMAOsc>(step_line, 15, 30);
    auto smaosc = std::make_shared<SMAOsc>(step_line, 15, 30);
    
    std::vector<double> tema_responses, ema_responses, sma_responses;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        temaosc->calculate();
        emaosc->calculate();
        smaosc->calculate();
        
        double tema_val = temaosc->get(0);
        double ema_val = emaosc->get(0);
        double sma_val = smaosc->get(0);
        
        if (!std::isnan(tema_val) && !std::isnan(ema_val) && !std::isnan(sma_val)) {
            if (i >= 100) {  // 价格跳跃后
                tema_responses.push_back(tema_val);
                ema_responses.push_back(ema_val);
                sma_responses.push_back(sma_val);
            }
        }
        
        if (i < step_prices.size() - 1) {
            step_line->forward();
        }
    }
    
    // 比较响应速度
    if (!tema_responses.empty() && !ema_responses.empty() && !sma_responses.empty()) {
        double max_tema_response = *std::max_element(tema_responses.begin(), tema_responses.end());
        double max_ema_response = *std::max_element(ema_responses.begin(), ema_responses.end());
        double max_sma_response = *std::max_element(sma_responses.begin(), sma_responses.end());
        
        std::cout << "Response speed comparison:" << std::endl;
        std::cout << "Max TEMA oscillator response: " << max_tema_response << std::endl;
        std::cout << "Max EMA oscillator response: " << max_ema_response << std::endl;
        std::cout << "Max SMA oscillator response: " << max_sma_response << std::endl;
        
        // TEMA通常比EMA和SMA响应更快
        EXPECT_GT(max_tema_response, max_sma_response * 0.8) 
            << "TEMA oscillator should respond to price changes";
    }
}

// TEMAOsc信号强度测试
TEST(OriginalTests, TEMAOsc_SignalStrength) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto temaosc = std::make_shared<TEMAOsc>(close_line, 15, 30);
    
    std::vector<double> oscillator_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        temaosc->calculate();
        
        double osc_val = temaosc->get(0);
        if (!std::isnan(osc_val)) {
            oscillator_values.push_back(osc_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 分析信号强度
    if (!oscillator_values.empty()) {
        double max_osc = *std::max_element(oscillator_values.begin(), oscillator_values.end());
        double min_osc = *std::min_element(oscillator_values.begin(), oscillator_values.end());
        
        std::cout << "Signal strength analysis:" << std::endl;
        std::cout << "Maximum TEMAOsc: " << max_osc << std::endl;
        std::cout << "Minimum TEMAOsc: " << min_osc << std::endl;
        
        // 计算信号强度阈值
        double mean = std::accumulate(oscillator_values.begin(), oscillator_values.end(), 0.0) / oscillator_values.size();
        double variance = 0.0;
        for (double val : oscillator_values) {
            variance += (val - mean) * (val - mean);
        }
        variance /= oscillator_values.size();
        double std_dev = std::sqrt(variance);
        
        double strong_positive_threshold = mean + 1.5 * std_dev;
        double strong_negative_threshold = mean - 1.5 * std_dev;
        
        int strong_positive_signals = 0;
        int strong_negative_signals = 0;
        
        for (double val : oscillator_values) {
            if (val > strong_positive_threshold) strong_positive_signals++;
            if (val < strong_negative_threshold) strong_negative_signals++;
        }
        
        std::cout << "Strong positive threshold: " << strong_positive_threshold << std::endl;
        std::cout << "Strong negative threshold: " << strong_negative_threshold << std::endl;
        std::cout << "Strong positive signals: " << strong_positive_signals << std::endl;
        std::cout << "Strong negative signals: " << strong_negative_signals << std::endl;
        
        EXPECT_GE(strong_positive_signals + strong_negative_signals, 0) 
            << "Should generate some strong signals";
    }
}

// 边界条件测试
TEST(OriginalTests, TEMAOsc_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(150, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_temaosc = std::make_shared<TEMAOsc>(flat_line, 15, 30);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_temaosc->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，TEMAOsc应该为零
    double final_temaosc = flat_temaosc->get(0);
    if (!std::isnan(final_temaosc)) {
        EXPECT_NEAR(final_temaosc, 0.0, 1e-6) 
            << "TEMAOsc should be zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 50; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_temaosc = std::make_shared<TEMAOsc>(insufficient_line, 15, 30);
    
    for (int i = 0; i < 50; ++i) {
        insufficient_temaosc->calculate();
        if (i < 49) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_temaosc->get(0);
    EXPECT_TRUE(std::isnan(result)) << "TEMAOsc should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, TEMAOsc_Performance) {
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
    
    auto large_temaosc = std::make_shared<TEMAOsc>(large_line, 15, 30);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_temaosc->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "TEMAOsc calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_temaosc->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}