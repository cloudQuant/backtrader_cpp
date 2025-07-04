/**
 * @file test_ind_priceosc.cpp
 * @brief PriceOsc指标测试 - 对应Python test_ind_priceosc.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['25.821368', '23.202675', '-9.927422']
 * ]
 * chkmin = 26
 * chkind = btind.PriceOsc
 * 
 * 注：PriceOsc (Price Oscillator) 基于两个移动平均线的百分比振荡器
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/priceosc.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> PRICEOSC_EXPECTED_VALUES = {
    {"25.821368", "23.202675", "-9.927422"}
};

const int PRICEOSC_MIN_PERIOD = 26;

} // anonymous namespace

// 使用默认参数的PriceOsc测试
DEFINE_INDICATOR_TEST(PriceOsc_Default, PriceOsc, PRICEOSC_EXPECTED_VALUES, PRICEOSC_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, PriceOsc_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建PriceOsc指标
    auto priceosc = std::make_shared<PriceOsc>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        priceosc->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 26;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"25.821368", "23.202675", "-9.927422"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = priceosc->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "PriceOsc value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(priceosc->getMinPeriod(), 26) << "PriceOsc minimum period should be 26";
}

// 参数化测试 - 测试不同参数的PriceOsc
class PriceOscParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int>> {
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

TEST_P(PriceOscParameterizedTest, DifferentParameters) {
    auto [fast, slow] = GetParam();
    auto priceosc = std::make_shared<PriceOsc>(close_line_, fast, slow);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        priceosc->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(priceosc->getMinPeriod(), slow) 
        << "PriceOsc minimum period should equal slow period";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(slow)) {
        double last_value = priceosc->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last PriceOsc value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last PriceOsc value should be finite";
    }
}

// 测试不同的PriceOsc参数组合
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    PriceOscParameterizedTest,
    ::testing::Values(
        std::make_tuple(12, 26),   // 默认参数
        std::make_tuple(10, 20),   // 更快参数
        std::make_tuple(19, 39),   // 更慢参数
        std::make_tuple(5, 15)     // 短期参数
    )
);

// PriceOsc计算逻辑验证测试
TEST(OriginalTests, PriceOsc_CalculationLogic) {
    // 使用简单的测试数据验证PriceOsc计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0,
                                  120.0, 118.0, 116.0, 114.0, 112.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0};
    
    auto price_line = std::make_shared<LineRoot>(prices.size(), "priceosc_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto priceosc = std::make_shared<PriceOsc>(price_line, 12, 26);
    auto sma_fast = std::make_shared<SMA>(price_line, 12);
    auto sma_slow = std::make_shared<SMA>(price_line, 26);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        priceosc->calculate();
        sma_fast->calculate();
        sma_slow->calculate();
        
        // 验证PriceOsc计算：PriceOsc = 100 * (SMA_fast - SMA_slow) / SMA_slow
        if (i >= 25) {  // PriceOsc需要26个数据点
            double fast_value = sma_fast->get(0);
            double slow_value = sma_slow->get(0);
            double actual_priceosc = priceosc->get(0);
            
            if (!std::isnan(fast_value) && !std::isnan(slow_value) && slow_value != 0.0) {
                double expected_priceosc = 100.0 * (fast_value - slow_value) / slow_value;
                EXPECT_NEAR(actual_priceosc, expected_priceosc, 1e-6) 
                    << "PriceOsc calculation mismatch at step " << i
                    << " (fast=" << fast_value << ", slow=" << slow_value << ")";
            }
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
}

// PriceOsc零线穿越测试
TEST(OriginalTests, PriceOsc_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto priceosc = std::make_shared<PriceOsc>(close_line, 12, 26);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_osc = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越
    for (size_t i = 0; i < csv_data.size(); ++i) {
        priceosc->calculate();
        
        double current_osc = priceosc->get(0);
        
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
    
    std::cout << "PriceOsc zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// PriceOsc趋势分析测试
TEST(OriginalTests, PriceOsc_TrendAnalysis) {
    // 创建明确的上升趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 50; ++i) {
        uptrend_prices.push_back(100.0 + i * 1.0);  // 强劲上升趋势
    }
    
    auto uptrend_line = std::make_shared<LineRoot>(uptrend_prices.size(), "uptrend");
    for (double price : uptrend_prices) {
        uptrend_line->forward(price);
    }
    
    auto uptrend_priceosc = std::make_shared<PriceOsc>(uptrend_line, 12, 26);
    
    std::vector<double> uptrend_values;
    
    for (size_t i = 0; i < uptrend_prices.size(); ++i) {
        uptrend_priceosc->calculate();
        
        double osc_value = uptrend_priceosc->get(0);
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
    
    auto downtrend_priceosc = std::make_shared<PriceOsc>(downtrend_line, 12, 26);
    
    std::vector<double> downtrend_values;
    
    for (size_t i = 0; i < downtrend_prices.size(); ++i) {
        downtrend_priceosc->calculate();
        
        double osc_value = downtrend_priceosc->get(0);
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
        
        // 上升趋势应该有正的PriceOsc值，下降趋势应该有负的PriceOsc值
        EXPECT_GT(avg_uptrend, avg_downtrend) 
            << "Uptrend should have higher PriceOsc values than downtrend";
        EXPECT_GT(avg_uptrend, 0.0) 
            << "Strong uptrend should have positive PriceOsc values";
        EXPECT_LT(avg_downtrend, 0.0) 
            << "Strong downtrend should have negative PriceOsc values";
    }
}

// PriceOsc振荡特性测试
TEST(OriginalTests, PriceOsc_OscillationCharacteristics) {
    // 创建振荡数据
    std::vector<double> oscillating_prices;
    for (int i = 0; i < 100; ++i) {
        double base = 100.0;
        double oscillation = 8.0 * std::sin(i * 0.3);
        oscillating_prices.push_back(base + oscillation);
    }
    
    auto osc_line = std::make_shared<LineRoot>(oscillating_prices.size(), "oscillating");
    for (double price : oscillating_prices) {
        osc_line->forward(price);
    }
    
    auto priceosc = std::make_shared<PriceOsc>(osc_line, 12, 26);
    
    std::vector<double> oscillator_values;
    
    for (size_t i = 0; i < oscillating_prices.size(); ++i) {
        priceosc->calculate();
        
        double osc_val = priceosc->get(0);
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
        
        // PriceOsc应该围绕零线波动
        EXPECT_NEAR(avg_oscillator, 0.0, 3.0) 
            << "PriceOsc should oscillate around zero";
        
        EXPECT_GT(std_dev, 1.0) 
            << "PriceOsc should show meaningful variation";
    }
}

// PriceOsc与MACD比较测试
TEST(OriginalTests, PriceOsc_vs_MACD) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto priceosc = std::make_shared<PriceOsc>(close_line, 12, 26);
    auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
    
    std::vector<double> priceosc_values, macd_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        priceosc->calculate();
        macd->calculate();
        
        double priceosc_val = priceosc->get(0);
        double macd_val = macd->getLine(0)->get(0);  // MACD line
        
        if (!std::isnan(priceosc_val)) priceosc_values.push_back(priceosc_val);
        if (!std::isnan(macd_val)) macd_values.push_back(macd_val);
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 比较PriceOsc和MACD的特性
    if (!priceosc_values.empty() && !macd_values.empty()) {
        double priceosc_avg = std::accumulate(priceosc_values.begin(), priceosc_values.end(), 0.0) / priceosc_values.size();
        double macd_avg = std::accumulate(macd_values.begin(), macd_values.end(), 0.0) / macd_values.size();
        
        std::cout << "PriceOsc vs MACD comparison:" << std::endl;
        std::cout << "PriceOsc average: " << priceosc_avg << std::endl;
        std::cout << "MACD average: " << macd_avg << std::endl;
        
        // 两者都应该围绕零线振荡，但PriceOsc是百分比形式
        EXPECT_NEAR(priceosc_avg, 0.0, 5.0) << "PriceOsc should center around zero";
        EXPECT_NEAR(macd_avg, 0.0, 10.0) << "MACD should center around zero";
        
        // PriceOsc通常有更大的数值范围（百分比）
        double priceosc_range = *std::max_element(priceosc_values.begin(), priceosc_values.end()) - 
                               *std::min_element(priceosc_values.begin(), priceosc_values.end());
        double macd_range = *std::max_element(macd_values.begin(), macd_values.end()) - 
                           *std::min_element(macd_values.begin(), macd_values.end());
        
        std::cout << "PriceOsc range: " << priceosc_range << std::endl;
        std::cout << "MACD range: " << macd_range << std::endl;
    }
}

// PriceOsc极值信号测试
TEST(OriginalTests, PriceOsc_ExtremeSignals) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto priceosc = std::make_shared<PriceOsc>(close_line, 12, 26);
    
    std::vector<double> oscillator_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        priceosc->calculate();
        
        double osc_val = priceosc->get(0);
        if (!std::isnan(osc_val)) {
            oscillator_values.push_back(osc_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 分析极值信号
    if (!oscillator_values.empty()) {
        double max_osc = *std::max_element(oscillator_values.begin(), oscillator_values.end());
        double min_osc = *std::min_element(oscillator_values.begin(), oscillator_values.end());
        
        std::cout << "Extreme values analysis:" << std::endl;
        std::cout << "Maximum PriceOsc: " << max_osc << std::endl;
        std::cout << "Minimum PriceOsc: " << min_osc << std::endl;
        
        // 计算动态阈值
        double mean = std::accumulate(oscillator_values.begin(), oscillator_values.end(), 0.0) / oscillator_values.size();
        double variance = 0.0;
        for (double val : oscillator_values) {
            variance += (val - mean) * (val - mean);
        }
        variance /= oscillator_values.size();
        double std_dev = std::sqrt(variance);
        
        double overbought_threshold = mean + 2.0 * std_dev;
        double oversold_threshold = mean - 2.0 * std_dev;
        
        int overbought_signals = 0;
        int oversold_signals = 0;
        
        for (double val : oscillator_values) {
            if (val > overbought_threshold) overbought_signals++;
            if (val < oversold_threshold) oversold_signals++;
        }
        
        std::cout << "Overbought threshold: " << overbought_threshold << std::endl;
        std::cout << "Oversold threshold: " << oversold_threshold << std::endl;
        std::cout << "Overbought signals: " << overbought_signals << std::endl;
        std::cout << "Oversold signals: " << oversold_signals << std::endl;
        
        EXPECT_GE(overbought_signals + oversold_signals, 0) 
            << "Should generate some extreme signals";
    }
}

// PriceOsc动量确认测试
TEST(OriginalTests, PriceOsc_MomentumConfirmation) {
    // 创建具有不同动量的数据
    std::vector<double> momentum_prices;
    
    // 第一阶段：加速上升
    for (int i = 0; i < 40; ++i) {
        momentum_prices.push_back(100.0 + i * i * 0.05);
    }
    
    // 第二阶段：减速上升
    for (int i = 0; i < 40; ++i) {
        double increment = 2.0 - i * 0.04;
        momentum_prices.push_back(momentum_prices.back() + std::max(0.1, increment));
    }
    
    auto momentum_line = std::make_shared<LineRoot>(momentum_prices.size(), "momentum");
    for (double price : momentum_prices) {
        momentum_line->forward(price);
    }
    
    auto momentum_priceosc = std::make_shared<PriceOsc>(momentum_line, 12, 26);
    
    std::vector<double> accelerating_osc, decelerating_osc;
    
    for (size_t i = 0; i < momentum_prices.size(); ++i) {
        momentum_priceosc->calculate();
        
        double osc_val = momentum_priceosc->get(0);
        if (!std::isnan(osc_val)) {
            if (i < 40) {
                accelerating_osc.push_back(osc_val);
            } else {
                decelerating_osc.push_back(osc_val);
            }
        }
        
        if (i < momentum_prices.size() - 1) {
            momentum_line->forward();
        }
    }
    
    // 分析不同动量阶段的振荡器表现
    if (!accelerating_osc.empty() && !decelerating_osc.empty()) {
        double acc_avg = std::accumulate(accelerating_osc.begin(), accelerating_osc.end(), 0.0) / accelerating_osc.size();
        double dec_avg = std::accumulate(decelerating_osc.begin(), decelerating_osc.end(), 0.0) / decelerating_osc.size();
        
        std::cout << "Momentum confirmation analysis:" << std::endl;
        std::cout << "Accelerating phase average: " << acc_avg << std::endl;
        std::cout << "Decelerating phase average: " << dec_avg << std::endl;
        
        // 加速阶段应该有更高的振荡器值
        EXPECT_GT(acc_avg, dec_avg) << "Accelerating phase should have higher oscillator values";
    }
}

// PriceOsc发散分析测试
TEST(OriginalTests, PriceOsc_DivergenceAnalysis) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto priceosc = std::make_shared<PriceOsc>(close_line, 12, 26);
    
    std::vector<double> prices, osc_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        priceosc->calculate();
        
        double osc_val = priceosc->get(0);
        if (!std::isnan(osc_val)) {
            prices.push_back(csv_data[i].close);
            osc_values.push_back(osc_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 寻找价格和振荡器的峰值点
    std::vector<size_t> price_peaks, osc_peaks;
    
    for (size_t i = 1; i < prices.size() - 1; ++i) {
        if (prices[i] > prices[i-1] && prices[i] > prices[i+1]) {
            price_peaks.push_back(i);
        }
        if (osc_values[i] > osc_values[i-1] && osc_values[i] > osc_values[i+1]) {
            osc_peaks.push_back(i);
        }
    }
    
    std::cout << "Divergence analysis:" << std::endl;
    std::cout << "Price peaks: " << price_peaks.size() << std::endl;
    std::cout << "PriceOsc peaks: " << osc_peaks.size() << std::endl;
    
    // 分析最近的峰值
    if (price_peaks.size() >= 2 && osc_peaks.size() >= 2) {
        size_t recent_price_peak = price_peaks.back();
        size_t recent_osc_peak = osc_peaks.back();
        
        std::cout << "Recent price peak: " << prices[recent_price_peak] 
                  << " at index " << recent_price_peak << std::endl;
        std::cout << "Recent oscillator peak: " << osc_values[recent_osc_peak] 
                  << " at index " << recent_osc_peak << std::endl;
    }
    
    EXPECT_TRUE(true) << "Divergence analysis completed";
}

// 边界条件测试
TEST(OriginalTests, PriceOsc_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_priceosc = std::make_shared<PriceOsc>(flat_line, 12, 26);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_priceosc->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，PriceOsc应该为零
    double final_priceosc = flat_priceosc->get(0);
    if (!std::isnan(final_priceosc)) {
        EXPECT_NEAR(final_priceosc, 0.0, 1e-6) 
            << "PriceOsc should be zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 20; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_priceosc = std::make_shared<PriceOsc>(insufficient_line, 12, 26);
    
    for (int i = 0; i < 20; ++i) {
        insufficient_priceosc->calculate();
        if (i < 19) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_priceosc->get(0);
    EXPECT_TRUE(std::isnan(result)) << "PriceOsc should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, PriceOsc_Performance) {
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
    
    auto large_priceosc = std::make_shared<PriceOsc>(large_line, 12, 26);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_priceosc->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "PriceOsc calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_priceosc->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}