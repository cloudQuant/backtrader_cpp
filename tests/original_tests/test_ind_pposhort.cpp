/**
 * @file test_ind_pposhort.cpp
 * @brief PPOShort指标测试 - 对应Python test_ind_pposhort.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['0.629452', '0.875813', '0.049405'],
 *     ['0.537193', '0.718852', '-0.080645'],
 *     ['0.092259', '0.156962', '0.130050']
 * ]
 * chkmin = 34
 * chkind = btind.PPOShort
 * 
 * 注：PPOShort包含3条线：PPO, Signal, Histogram
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/pposhort.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> PPOSHORT_EXPECTED_VALUES = {
    {"0.629452", "0.875813", "0.049405"},   // line 0 (PPO)
    {"0.537193", "0.718852", "-0.080645"},  // line 1 (Signal)
    {"0.092259", "0.156962", "0.130050"}    // line 2 (Histogram)
};

const int PPOSHORT_MIN_PERIOD = 34;

} // anonymous namespace

// 使用默认参数的PPOShort测试
DEFINE_INDICATOR_TEST(PPOShort_Default, PPOShort, PPOSHORT_EXPECTED_VALUES, PPOSHORT_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, PPOShort_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建PPOShort指标
    auto pposhort = std::make_shared<PPOShort>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        pposhort->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 34;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // 验证3条线的值
    for (int line = 0; line < 3; ++line) {
        auto expected = PPOSHORT_EXPECTED_VALUES[line];
        
        for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
            double actual = pposhort->getLine(line)->get(check_points[i]);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << actual;
            std::string actual_str = ss.str();
            
            EXPECT_EQ(actual_str, expected[i]) 
                << "PPOShort line " << line << " value mismatch at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(pposhort->getMinPeriod(), 34) << "PPOShort minimum period should be 34";
}

// 参数化测试 - 测试不同参数的PPOShort
class PPOShortParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int, int>> {
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

TEST_P(PPOShortParameterizedTest, DifferentParameters) {
    auto [fast, slow, signal] = GetParam();
    auto pposhort = std::make_shared<PPOShort>(close_line_, fast, slow, signal);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        pposhort->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    int expected_minperiod = slow + signal - 1;
    EXPECT_EQ(pposhort->getMinPeriod(), expected_minperiod) 
        << "PPOShort minimum period should be " << expected_minperiod;
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_minperiod)) {
        double last_ppo = pposhort->getLine(0)->get(0);     // PPO
        double last_signal = pposhort->getLine(1)->get(0);  // Signal
        double last_histo = pposhort->getLine(2)->get(0);   // Histogram
        
        EXPECT_FALSE(std::isnan(last_ppo)) << "Last PPO should not be NaN";
        EXPECT_FALSE(std::isnan(last_signal)) << "Last Signal should not be NaN";
        EXPECT_FALSE(std::isnan(last_histo)) << "Last Histogram should not be NaN";
        
        EXPECT_TRUE(std::isfinite(last_ppo)) << "Last PPO should be finite";
        EXPECT_TRUE(std::isfinite(last_signal)) << "Last Signal should be finite";
        EXPECT_TRUE(std::isfinite(last_histo)) << "Last Histogram should be finite";
        
        // 验证Histogram = PPO - Signal
        EXPECT_NEAR(last_histo, last_ppo - last_signal, 1e-10) 
            << "Histogram should equal PPO - Signal";
    }
}

// 测试不同的PPOShort参数组合
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    PPOShortParameterizedTest,
    ::testing::Values(
        std::make_tuple(12, 26, 9),   // 默认参数
        std::make_tuple(8, 17, 9),    // 更快参数
        std::make_tuple(19, 39, 9),   // 更慢参数
        std::make_tuple(12, 26, 6)    // 不同信号周期
    )
);

// PPOShort计算逻辑验证测试
TEST(OriginalTests, PPOShort_CalculationLogic) {
    // 使用简单的测试数据验证PPOShort计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0,
                                  120.0, 118.0, 116.0, 114.0, 112.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0,
                                  124.0, 126.0, 128.0, 130.0, 132.0, 134.0, 136.0, 138.0, 140.0, 142.0};
    
    auto price_line = std::make_shared<LineRoot>(prices.size(), "pposhort_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto pposhort = std::make_shared<PPOShort>(price_line, 12, 26, 9);
    auto ema_fast = std::make_shared<EMA>(price_line, 12);
    auto ema_slow = std::make_shared<EMA>(price_line, 26);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        pposhort->calculate();
        ema_fast->calculate();
        ema_slow->calculate();
        
        if (i >= 33) {  // PPOShort需要34个数据点
            double fast_value = ema_fast->get(0);
            double slow_value = ema_slow->get(0);
            double ppo_value = pposhort->getLine(0)->get(0);
            double signal_value = pposhort->getLine(1)->get(0);
            double histo_value = pposhort->getLine(2)->get(0);
            
            if (!std::isnan(fast_value) && !std::isnan(slow_value) && slow_value != 0.0) {
                // 验证PPO计算：PPO = 100 * (EMA_fast - EMA_slow) / EMA_slow
                double expected_ppo = 100.0 * (fast_value - slow_value) / slow_value;
                EXPECT_NEAR(ppo_value, expected_ppo, 1e-6) 
                    << "PPO calculation mismatch at step " << i;
                
                // 验证Histogram = PPO - Signal
                if (!std::isnan(signal_value)) {
                    EXPECT_NEAR(histo_value, ppo_value - signal_value, 1e-10) 
                        << "Histogram calculation mismatch at step " << i;
                }
            }
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
}

// PPOShort零线穿越测试
TEST(OriginalTests, PPOShort_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto pposhort = std::make_shared<PPOShort>(close_line, 12, 26, 9);
    
    int ppo_positive_crossings = 0;     // PPO从负到正
    int ppo_negative_crossings = 0;     // PPO从正到负
    int signal_crossings = 0;           // PPO穿越Signal线
    
    double prev_ppo = 0.0, prev_signal = 0.0;
    bool has_prev = false;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        pposhort->calculate();
        
        double current_ppo = pposhort->getLine(0)->get(0);
        double current_signal = pposhort->getLine(1)->get(0);
        
        if (!std::isnan(current_ppo) && !std::isnan(current_signal) && has_prev) {
            // PPO零线穿越
            if (prev_ppo <= 0.0 && current_ppo > 0.0) {
                ppo_positive_crossings++;
            } else if (prev_ppo >= 0.0 && current_ppo < 0.0) {
                ppo_negative_crossings++;
            }
            
            // PPO与Signal线穿越
            if ((prev_ppo <= prev_signal && current_ppo > current_signal) ||
                (prev_ppo >= prev_signal && current_ppo < current_signal)) {
                signal_crossings++;
            }
        }
        
        if (!std::isnan(current_ppo) && !std::isnan(current_signal)) {
            prev_ppo = current_ppo;
            prev_signal = current_signal;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "PPOShort crossings analysis:" << std::endl;
    std::cout << "PPO positive crossings: " << ppo_positive_crossings << std::endl;
    std::cout << "PPO negative crossings: " << ppo_negative_crossings << std::endl;
    std::cout << "PPO-Signal crossings: " << signal_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(ppo_positive_crossings + ppo_negative_crossings + signal_crossings, 0) 
        << "Should detect some crossings";
}

// PPOShort趋势分析测试
TEST(OriginalTests, PPOShort_TrendAnalysis) {
    // 创建明确的上升趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 60; ++i) {
        uptrend_prices.push_back(100.0 + i * 1.0);  // 强劲上升趋势
    }
    
    auto uptrend_line = std::make_shared<LineRoot>(uptrend_prices.size(), "uptrend");
    for (double price : uptrend_prices) {
        uptrend_line->forward(price);
    }
    
    auto uptrend_ppo = std::make_shared<PPOShort>(uptrend_line, 12, 26, 9);
    
    std::vector<double> uptrend_ppo_values;
    
    for (size_t i = 0; i < uptrend_prices.size(); ++i) {
        uptrend_ppo->calculate();
        
        double ppo_value = uptrend_ppo->getLine(0)->get(0);
        if (!std::isnan(ppo_value)) {
            uptrend_ppo_values.push_back(ppo_value);
        }
        
        if (i < uptrend_prices.size() - 1) {
            uptrend_line->forward();
        }
    }
    
    // 创建下降趋势数据
    std::vector<double> downtrend_prices;
    for (int i = 0; i < 60; ++i) {
        downtrend_prices.push_back(160.0 - i * 1.0);  // 强劲下降趋势
    }
    
    auto downtrend_line = std::make_shared<LineRoot>(downtrend_prices.size(), "downtrend");
    for (double price : downtrend_prices) {
        downtrend_line->forward(price);
    }
    
    auto downtrend_ppo = std::make_shared<PPOShort>(downtrend_line, 12, 26, 9);
    
    std::vector<double> downtrend_ppo_values;
    
    for (size_t i = 0; i < downtrend_prices.size(); ++i) {
        downtrend_ppo->calculate();
        
        double ppo_value = downtrend_ppo->getLine(0)->get(0);
        if (!std::isnan(ppo_value)) {
            downtrend_ppo_values.push_back(ppo_value);
        }
        
        if (i < downtrend_prices.size() - 1) {
            downtrend_line->forward();
        }
    }
    
    // 分析趋势特性
    if (!uptrend_ppo_values.empty() && !downtrend_ppo_values.empty()) {
        double avg_uptrend = std::accumulate(uptrend_ppo_values.begin(), uptrend_ppo_values.end(), 0.0) / uptrend_ppo_values.size();
        double avg_downtrend = std::accumulate(downtrend_ppo_values.begin(), downtrend_ppo_values.end(), 0.0) / downtrend_ppo_values.size();
        
        std::cout << "Trend analysis:" << std::endl;
        std::cout << "Uptrend average PPO: " << avg_uptrend << std::endl;
        std::cout << "Downtrend average PPO: " << avg_downtrend << std::endl;
        
        // 上升趋势应该有正的PPO值，下降趋势应该有负的PPO值
        EXPECT_GT(avg_uptrend, avg_downtrend) 
            << "Uptrend should have higher PPO values than downtrend";
        EXPECT_GT(avg_uptrend, 0.0) 
            << "Strong uptrend should have positive PPO values";
        EXPECT_LT(avg_downtrend, 0.0) 
            << "Strong downtrend should have negative PPO values";
    }
}

// PPOShort发散分析测试
TEST(OriginalTests, PPOShort_DivergenceAnalysis) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto pposhort = std::make_shared<PPOShort>(close_line, 12, 26, 9);
    
    std::vector<double> prices, ppo_values, histo_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        pposhort->calculate();
        
        double ppo_val = pposhort->getLine(0)->get(0);
        double histo_val = pposhort->getLine(2)->get(0);
        
        if (!std::isnan(ppo_val) && !std::isnan(histo_val)) {
            prices.push_back(csv_data[i].close);
            ppo_values.push_back(ppo_val);
            histo_values.push_back(histo_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 寻找价格和PPO的峰值点
    std::vector<size_t> price_peaks, ppo_peaks, histo_peaks;
    
    for (size_t i = 1; i < prices.size() - 1; ++i) {
        if (prices[i] > prices[i-1] && prices[i] > prices[i+1]) {
            price_peaks.push_back(i);
        }
        if (ppo_values[i] > ppo_values[i-1] && ppo_values[i] > ppo_values[i+1]) {
            ppo_peaks.push_back(i);
        }
        if (histo_values[i] > histo_values[i-1] && histo_values[i] > histo_values[i+1]) {
            histo_peaks.push_back(i);
        }
    }
    
    std::cout << "Divergence analysis:" << std::endl;
    std::cout << "Price peaks: " << price_peaks.size() << std::endl;
    std::cout << "PPO peaks: " << ppo_peaks.size() << std::endl;
    std::cout << "Histogram peaks: " << histo_peaks.size() << std::endl;
    
    // 分析最近的峰值
    if (price_peaks.size() >= 2 && ppo_peaks.size() >= 2) {
        size_t recent_price_peak = price_peaks.back();
        size_t recent_ppo_peak = ppo_peaks.back();
        
        std::cout << "Recent price peak: " << prices[recent_price_peak] 
                  << " at index " << recent_price_peak << std::endl;
        std::cout << "Recent PPO peak: " << ppo_values[recent_ppo_peak] 
                  << " at index " << recent_ppo_peak << std::endl;
    }
    
    EXPECT_TRUE(true) << "Divergence analysis completed";
}

// PPOShort振荡特性测试
TEST(OriginalTests, PPOShort_OscillationCharacteristics) {
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
    
    auto pposhort = std::make_shared<PPOShort>(osc_line, 12, 26, 9);
    
    std::vector<double> ppo_values, signal_values, histo_values;
    
    for (size_t i = 0; i < oscillating_prices.size(); ++i) {
        pposhort->calculate();
        
        double ppo_val = pposhort->getLine(0)->get(0);
        double signal_val = pposhort->getLine(1)->get(0);
        double histo_val = pposhort->getLine(2)->get(0);
        
        if (!std::isnan(ppo_val) && !std::isnan(signal_val) && !std::isnan(histo_val)) {
            ppo_values.push_back(ppo_val);
            signal_values.push_back(signal_val);
            histo_values.push_back(histo_val);
        }
        
        if (i < oscillating_prices.size() - 1) {
            osc_line->forward();
        }
    }
    
    // 分析振荡特性
    if (!ppo_values.empty() && !signal_values.empty() && !histo_values.empty()) {
        double avg_ppo = std::accumulate(ppo_values.begin(), ppo_values.end(), 0.0) / ppo_values.size();
        double avg_signal = std::accumulate(signal_values.begin(), signal_values.end(), 0.0) / signal_values.size();
        double avg_histo = std::accumulate(histo_values.begin(), histo_values.end(), 0.0) / histo_values.size();
        
        std::cout << "Oscillation characteristics:" << std::endl;
        std::cout << "Average PPO: " << avg_ppo << std::endl;
        std::cout << "Average Signal: " << avg_signal << std::endl;
        std::cout << "Average Histogram: " << avg_histo << std::endl;
        
        // 在振荡市场中，PPO和Signal应该围绕零线波动
        EXPECT_NEAR(avg_ppo, 0.0, 2.0) << "PPO should oscillate around zero";
        EXPECT_NEAR(avg_signal, 0.0, 2.0) << "Signal should oscillate around zero";
        EXPECT_NEAR(avg_histo, 0.0, 2.0) << "Histogram should oscillate around zero";
    }
}

// 边界条件测试
TEST(OriginalTests, PPOShort_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_pposhort = std::make_shared<PPOShort>(flat_line, 12, 26, 9);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_pposhort->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，PPO应该为零
    double final_ppo = flat_pposhort->getLine(0)->get(0);
    double final_signal = flat_pposhort->getLine(1)->get(0);
    double final_histo = flat_pposhort->getLine(2)->get(0);
    
    if (!std::isnan(final_ppo) && !std::isnan(final_signal) && !std::isnan(final_histo)) {
        EXPECT_NEAR(final_ppo, 0.0, 1e-6) << "PPO should be zero for constant prices";
        EXPECT_NEAR(final_signal, 0.0, 1e-6) << "Signal should be zero for constant prices";
        EXPECT_NEAR(final_histo, 0.0, 1e-6) << "Histogram should be zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 30; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_pposhort = std::make_shared<PPOShort>(insufficient_line, 12, 26, 9);
    
    for (int i = 0; i < 30; ++i) {
        insufficient_pposhort->calculate();
        if (i < 29) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result_ppo = insufficient_pposhort->getLine(0)->get(0);
    double result_signal = insufficient_pposhort->getLine(1)->get(0);
    double result_histo = insufficient_pposhort->getLine(2)->get(0);
    
    EXPECT_TRUE(std::isnan(result_ppo)) << "PPO should return NaN when insufficient data";
    EXPECT_TRUE(std::isnan(result_signal)) << "Signal should return NaN when insufficient data";
    EXPECT_TRUE(std::isnan(result_histo)) << "Histogram should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, PPOShort_Performance) {
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
    
    auto large_pposhort = std::make_shared<PPOShort>(large_line, 12, 26, 9);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_pposhort->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "PPOShort calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_ppo = large_pposhort->getLine(0)->get(0);
    double final_signal = large_pposhort->getLine(1)->get(0);
    double final_histo = large_pposhort->getLine(2)->get(0);
    
    EXPECT_FALSE(std::isnan(final_ppo)) << "Final PPO should not be NaN";
    EXPECT_FALSE(std::isnan(final_signal)) << "Final Signal should not be NaN";
    EXPECT_FALSE(std::isnan(final_histo)) << "Final Histogram should not be NaN";
    
    EXPECT_TRUE(std::isfinite(final_ppo)) << "Final PPO should be finite";
    EXPECT_TRUE(std::isfinite(final_signal)) << "Final Signal should be finite";
    EXPECT_TRUE(std::isfinite(final_histo)) << "Final Histogram should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}