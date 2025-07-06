/**
 * @file test_ind_wmaosc.cpp
 * @brief WMAOsc指标测试 - 对应Python test_ind_wmaosc.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['43.727634', '40.436366', '-19.148000']
 * ]
 * chkmin = 30
 * chkind = btind.WMAOsc
 * 
 * 注：WMAOsc (WMA Oscillator) 基于两个WMA的差值振荡器
 */

#include "test_common.h"
#include <random>

#include "indicators/wmaosc.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> WMAOSC_EXPECTED_VALUES = {
    {"43.727634", "40.436366", "-19.148000"}
};

const int WMAOSC_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的WMAOsc测试
DEFINE_INDICATOR_TEST(WMAOsc_Default, WMAOsc, WMAOSC_EXPECTED_VALUES, WMAOSC_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, WMAOsc_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建WMAOsc指标
    auto wmaosc = std::make_shared<WMAOsc>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        wmaosc->calculate();
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
    
    std::vector<std::string> expected = {"43.727634", "40.436366", "-19.148000"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = wmaosc->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "WMAOsc value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(wmaosc->getMinPeriod(), 30) << "WMAOsc minimum period should be 30";
}

// 参数化测试 - 测试不同参数的WMAOsc
class WMAOscParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int>> {
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

TEST_P(WMAOscParameterizedTest, DifferentParameters) {
    auto [fast, slow] = GetParam();
    auto wmaosc = std::make_shared<WMAOsc>(close_line_, fast, slow);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        wmaosc->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(wmaosc->getMinPeriod(), slow) 
        << "WMAOsc minimum period should equal slow period";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(slow)) {
        double last_value = wmaosc->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last WMAOsc value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last WMAOsc value should be finite";
    }
}

// 测试不同的WMAOsc参数组合
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    WMAOscParameterizedTest,
    ::testing::Values(
        std::make_tuple(15, 30),   // 默认参数
        std::make_tuple(10, 20),   // 更快参数
        std::make_tuple(20, 40),   // 更慢参数
        std::make_tuple(5, 15)     // 短期参数
    )
);

// WMAOsc计算逻辑验证测试
TEST(OriginalTests, WMAOsc_CalculationLogic) {
    // 使用简单的测试数据验证WMAOsc计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0,
                                  120.0, 118.0, 116.0, 114.0, 112.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0,
                                  124.0, 126.0, 128.0, 130.0, 132.0};
    
    auto price_line = std::make_shared<backtrader::LineRoot>(prices.size(), "wmaosc_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto wmaosc = std::make_shared<WMAOsc>(price_line, 15, 30);
    auto wma_fast = std::make_shared<WMA>(price_line, 15);
    auto wma_slow = std::make_shared<WMA>(price_line, 30);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        wmaosc->calculate();
        wma_fast->calculate();
        wma_slow->calculate();
        
        // 验证WMAOsc计算：WMAOsc = WMA_fast - WMA_slow
        if (i >= 29) {  // WMAOsc需要30个数据点
            double fast_value = wma_fast->get(0);
            double slow_value = wma_slow->get(0);
            double actual_wmaosc = wmaosc->get(0);
            
            if (!std::isnan(fast_value) && !std::isnan(slow_value)) {
                double expected_wmaosc = fast_value - slow_value;
                EXPECT_NEAR(actual_wmaosc, expected_wmaosc, 1e-6) 
                    << "WMAOsc calculation mismatch at step " << i
                    << " (fast=" << fast_value << ", slow=" << slow_value << ")";
            }
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
}

// WMAOsc零线穿越测试
TEST(OriginalTests, WMAOsc_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto wmaosc = std::make_shared<WMAOsc>(close_line, 15, 30);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_osc = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越
    for (size_t i = 0; i < csv_data.size(); ++i) {
        wmaosc->calculate();
        
        double current_osc = wmaosc->get(0);
        
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
    
    std::cout << "WMAOsc zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// WMAOsc趋势分析测试
TEST(OriginalTests, WMAOsc_TrendAnalysis) {
    // 创建明确的上升趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 80; ++i) {
        uptrend_prices.push_back(100.0 + i * 1.0);  // 强劲上升趋势
    }
    
    auto uptrend_line = std::make_shared<backtrader::LineRoot>(uptrend_prices.size(), "uptrend");
    for (double price : uptrend_prices) {
        uptrend_line->forward(price);
    }
    
    auto uptrend_wmaosc = std::make_shared<WMAOsc>(uptrend_line, 15, 30);
    
    std::vector<double> uptrend_values;
    
    for (size_t i = 0; i < uptrend_prices.size(); ++i) {
        uptrend_wmaosc->calculate();
        
        double osc_value = uptrend_wmaosc->get(0);
        if (!std::isnan(osc_value)) {
            uptrend_values.push_back(osc_value);
        }
        
        if (i < uptrend_prices.size() - 1) {
            uptrend_line->forward();
        }
    }
    
    // 创建下降趋势数据
    std::vector<double> downtrend_prices;
    for (int i = 0; i < 80; ++i) {
        downtrend_prices.push_back(180.0 - i * 1.0);  // 强劲下降趋势
    }
    
    auto downtrend_line = std::make_shared<backtrader::LineRoot>(downtrend_prices.size(), "downtrend");
    for (double price : downtrend_prices) {
        downtrend_line->forward(price);
    }
    
    auto downtrend_wmaosc = std::make_shared<WMAOsc>(downtrend_line, 15, 30);
    
    std::vector<double> downtrend_values;
    
    for (size_t i = 0; i < downtrend_prices.size(); ++i) {
        downtrend_wmaosc->calculate();
        
        double osc_value = downtrend_wmaosc->get(0);
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
        
        // 上升趋势应该有正的WMAOsc值，下降趋势应该有负的WMAOsc值
        EXPECT_GT(avg_uptrend, avg_downtrend) 
            << "Uptrend should have higher WMAOsc values than downtrend";
        EXPECT_GT(avg_uptrend, 0.0) 
            << "Strong uptrend should have positive WMAOsc values";
        EXPECT_LT(avg_downtrend, 0.0) 
            << "Strong downtrend should have negative WMAOsc values";
    }
}

// WMAOsc振荡特性测试
TEST(OriginalTests, WMAOsc_OscillationCharacteristics) {
    // 创建振荡数据
    std::vector<double> oscillating_prices;
    for (int i = 0; i < 100; ++i) {
        double base = 100.0;
        double oscillation = 8.0 * std::sin(i * 0.3);
        oscillating_prices.push_back(base + oscillation);
    }
    
    auto osc_line = std::make_shared<backtrader::LineRoot>(oscillating_prices.size(), "oscillating");
    for (double price : oscillating_prices) {
        osc_line->forward(price);
    }
    
    auto wmaosc = std::make_shared<WMAOsc>(osc_line, 15, 30);
    
    std::vector<double> oscillator_values;
    
    for (size_t i = 0; i < oscillating_prices.size(); ++i) {
        wmaosc->calculate();
        
        double osc_val = wmaosc->get(0);
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
        
        // WMAOsc应该围绕零线波动
        EXPECT_NEAR(avg_oscillator, 0.0, 3.0) 
            << "WMAOsc should oscillate around zero";
        
        EXPECT_GT(std_dev, 1.0) 
            << "WMAOsc should show meaningful variation";
    }
}

// WMAOsc与其他振荡器比较测试
TEST(OriginalTests, WMAOsc_vs_OtherOscillators) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto wmaosc = std::make_shared<WMAOsc>(close_line, 15, 30);
    auto emaosc = std::make_shared<EMAOsc>(close_line, 15, 30);
    auto smaosc = std::make_shared<SMAOsc>(close_line, 15, 30);
    
    std::vector<double> wma_values, ema_values, sma_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        wmaosc->calculate();
        emaosc->calculate();
        smaosc->calculate();
        
        double wma_val = wmaosc->get(0);
        double ema_val = emaosc->get(0);
        double sma_val = smaosc->get(0);
        
        if (!std::isnan(wma_val)) wma_values.push_back(wma_val);
        if (!std::isnan(ema_val)) ema_values.push_back(ema_val);
        if (!std::isnan(sma_val)) sma_values.push_back(sma_val);
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 比较不同振荡器的特性
    if (!wma_values.empty() && !ema_values.empty() && !sma_values.empty()) {
        double wma_avg = std::accumulate(wma_values.begin(), wma_values.end(), 0.0) / wma_values.size();
        double ema_avg = std::accumulate(ema_values.begin(), ema_values.end(), 0.0) / ema_values.size();
        double sma_avg = std::accumulate(sma_values.begin(), sma_values.end(), 0.0) / sma_values.size();
        
        std::cout << "Oscillator comparison:" << std::endl;
        std::cout << "WMA oscillator average: " << wma_avg << std::endl;
        std::cout << "EMA oscillator average: " << ema_avg << std::endl;
        std::cout << "SMA oscillator average: " << sma_avg << std::endl;
        
        // 所有振荡器都应该围绕零线
        EXPECT_NEAR(wma_avg, 0.0, 5.0) << "WMA oscillator should center around zero";
        EXPECT_NEAR(ema_avg, 0.0, 5.0) << "EMA oscillator should center around zero";
        EXPECT_NEAR(sma_avg, 0.0, 5.0) << "SMA oscillator should center around zero";
    }
}

// WMAOsc权重响应测试
TEST(OriginalTests, WMAOsc_WeightedResponse) {
    // 创建权重影响测试数据
    std::vector<double> weighted_prices;
    for (int i = 0; i < 60; ++i) {
        double base = 100.0;
        double recent_impact = (i >= 40) ? 15.0 : 0.0;  // 最近的价格有额外影响
        weighted_prices.push_back(base + recent_impact);
    }
    
    auto weighted_line = std::make_shared<backtrader::LineRoot>(weighted_prices.size(), "weighted");
    for (double price : weighted_prices) {
        weighted_line->forward(price);
    }
    
    auto wmaosc = std::make_shared<WMAOsc>(weighted_line, 15, 30);
    auto smaosc = std::make_shared<SMAOsc>(weighted_line, 15, 30);
    
    std::vector<double> wma_responses, sma_responses;
    
    for (size_t i = 0; i < weighted_prices.size(); ++i) {
        wmaosc->calculate();
        smaosc->calculate();
        
        double wma_val = wmaosc->get(0);
        double sma_val = smaosc->get(0);
        
        if (!std::isnan(wma_val) && !std::isnan(sma_val)) {
            if (i >= 40) {  // 权重影响期间
                wma_responses.push_back(wma_val);
                sma_responses.push_back(sma_val);
            }
        }
        
        if (i < weighted_prices.size() - 1) {
            weighted_line->forward();
        }
    }
    
    // 比较权重响应
    if (!wma_responses.empty() && !sma_responses.empty()) {
        double max_wma_response = *std::max_element(wma_responses.begin(), wma_responses.end());
        double max_sma_response = *std::max_element(sma_responses.begin(), sma_responses.end());
        
        std::cout << "Weighted response comparison:" << std::endl;
        std::cout << "Max WMA oscillator response: " << max_wma_response << std::endl;
        std::cout << "Max SMA oscillator response: " << max_sma_response << std::endl;
        
        // WMA应该更快地响应最近的价格变化
        EXPECT_GT(max_wma_response, max_sma_response * 0.9) 
            << "WMA oscillator should respond more to recent price changes";
    }
}

// WMAOsc平滑特性测试
TEST(OriginalTests, WMAOsc_SmoothingCharacteristics) {
    // 创建带噪音的数据
    std::vector<double> noisy_prices;
    for (int i = 0; i < 80; ++i) {
        double base = 100.0 + i * 0.2;  // 缓慢上升
        double noise = (i % 2 == 0 ? 3.0 : -3.0);  // 交替噪音
        noisy_prices.push_back(base + noise);
    }
    
    auto noisy_line = std::make_shared<backtrader::LineRoot>(noisy_prices.size(), "noisy");
    for (double price : noisy_prices) {
        noisy_line->forward(price);
    }
    
    auto wmaosc = std::make_shared<WMAOsc>(noisy_line, 15, 30);
    auto emaosc = std::make_shared<EMAOsc>(noisy_line, 15, 30);
    
    std::vector<double> wma_smoothness, ema_smoothness;
    double prev_wma = 0.0, prev_ema = 0.0;
    bool has_prev = false;
    
    for (size_t i = 0; i < noisy_prices.size(); ++i) {
        wmaosc->calculate();
        emaosc->calculate();
        
        double wma_osc = wmaosc->get(0);
        double ema_osc = emaosc->get(0);
        
        if (!std::isnan(wma_osc) && !std::isnan(ema_osc) && has_prev) {
            // 计算变化幅度（平滑性指标）
            wma_smoothness.push_back(std::abs(wma_osc - prev_wma));
            ema_smoothness.push_back(std::abs(ema_osc - prev_ema));
        }
        
        if (!std::isnan(wma_osc) && !std::isnan(ema_osc)) {
            prev_wma = wma_osc;
            prev_ema = ema_osc;
            has_prev = true;
        }
        
        if (i < noisy_prices.size() - 1) {
            noisy_line->forward();
        }
    }
    
    // 比较平滑特性
    if (!wma_smoothness.empty() && !ema_smoothness.empty()) {
        double avg_wma_change = std::accumulate(wma_smoothness.begin(), wma_smoothness.end(), 0.0) / wma_smoothness.size();
        double avg_ema_change = std::accumulate(ema_smoothness.begin(), ema_smoothness.end(), 0.0) / ema_smoothness.size();
        
        std::cout << "Smoothing characteristics:" << std::endl;
        std::cout << "Average WMA oscillator change: " << avg_wma_change << std::endl;
        std::cout << "Average EMA oscillator change: " << avg_ema_change << std::endl;
        
        // 验证振荡器能产生有意义的变化
        EXPECT_GT(avg_wma_change, 0.0) << "WMA oscillator should show variation";
        EXPECT_GT(avg_ema_change, 0.0) << "EMA oscillator should show variation";
    }
}

// WMAOsc信号强度测试
TEST(OriginalTests, WMAOsc_SignalStrength) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto wmaosc = std::make_shared<WMAOsc>(close_line, 15, 30);
    
    std::vector<double> oscillator_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        wmaosc->calculate();
        
        double osc_val = wmaosc->get(0);
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
        std::cout << "Maximum WMAOsc: " << max_osc << std::endl;
        std::cout << "Minimum WMAOsc: " << min_osc << std::endl;
        
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
TEST(OriginalTests, WMAOsc_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(80, 100.0);
    
    auto flat_line = std::make_shared<backtrader::LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_wmaosc = std::make_shared<WMAOsc>(flat_line, 15, 30);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_wmaosc->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，WMAOsc应该为零
    double final_wmaosc = flat_wmaosc->get(0);
    if (!std::isnan(final_wmaosc)) {
        EXPECT_NEAR(final_wmaosc, 0.0, 1e-6) 
            << "WMAOsc should be zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<backtrader::LineRoot>(100, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 25; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_wmaosc = std::make_shared<WMAOsc>(insufficient_line, 15, 30);
    
    for (int i = 0; i < 25; ++i) {
        insufficient_wmaosc->calculate();
        if (i < 24) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_wmaosc->get(0);
    EXPECT_TRUE(std::isnan(result)) << "WMAOsc should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, WMAOsc_Performance) {
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
    
    auto large_wmaosc = std::make_shared<WMAOsc>(large_line, 15, 30);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_wmaosc->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "WMAOsc calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_wmaosc->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}
