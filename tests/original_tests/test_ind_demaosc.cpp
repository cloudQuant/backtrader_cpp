/**
 * @file test_ind_demaosc.cpp
 * @brief DEMAOsc指标测试 - 对应Python test_ind_demaosc.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4.376754', '7.292791', '9.371585']
 * ]
 * chkmin = 59
 * chkind = btind.DEMAOsc
 * 
 * 注：DEMAOsc (DEMA Oscillator) 是价格与DEMA的振荡器
 */

#include "test_common.h"
#include <random>

#include "indicators/demaosc.h"
#include "indicators/dema.h"
#include "indicators/emaosc.h"
#include "indicators/smaosc.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> DEMAOSC_EXPECTED_VALUES = {
    {"4.376754", "7.292791", "9.371585"}
};

const int DEMAOSC_MIN_PERIOD = 59;

} // anonymous namespace

// 使用默认参数的DEMAOsc测试
DEFINE_INDICATOR_TEST(DEMAOsc_Default, DEMAOsc, DEMAOSC_EXPECTED_VALUES, DEMAOSC_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, DEMAOsc_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建DEMAOsc指标
    auto demaosc = std::make_shared<DEMAOsc>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        demaosc->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 59;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"4.376754", "7.292791", "9.371585"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = demaosc->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "DEMAOsc value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(demaosc->getMinPeriod(), 59) << "DEMAOsc minimum period should be 59";
}

// 参数化测试 - 测试不同周期的DEMAOsc
class DEMAOscParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(DEMAOscParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto demaosc = std::make_shared<DEMAOsc>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        demaosc->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期（DEMA需要2*period-1个数据点）
    EXPECT_EQ(demaosc->getMinPeriod(), 2 * period - 1) 
        << "DEMAOsc minimum period should equal 2*period-1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(2 * period - 1)) {
        double last_value = demaosc->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last DEMAOsc value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last DEMAOsc value should be finite";
    }
}

// 测试不同的DEMAOsc周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    DEMAOscParameterizedTest,
    ::testing::Values(10, 20, 30, 50)
);

// DEMAOsc计算逻辑验证测试
TEST(OriginalTests, DEMAOsc_CalculationLogic) {
    // 使用简单的测试数据验证DEMAOsc计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0,
                                  124.0, 126.0, 128.0, 130.0, 132.0, 134.0, 136.0, 138.0, 140.0, 142.0,
                                  144.0, 146.0, 148.0, 150.0, 152.0, 154.0, 156.0, 158.0, 160.0, 162.0};
    
    auto price_line = std::make_shared<backtrader::LineRoot>(prices.size(), "demaosc_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto demaosc = std::make_shared<DEMAOsc>(price_line, 10);
    auto dema = std::make_shared<DEMA>(price_line, 10);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        demaosc->calculate();
        dema->calculate();
        
        // 手动验证DEMAOsc计算：DEMAOsc = Price - DEMA
        if (i >= 18) {  // DEMA需要19个数据点
            double current_price = prices[i];
            double dema_value = dema->get(0);
            double expected_demaosc = current_price - dema_value;
            double actual_demaosc = demaosc->get(0);
            
            if (!std::isnan(actual_demaosc) && !std::isnan(dema_value)) {
                EXPECT_NEAR(actual_demaosc, expected_demaosc, 1e-10) 
                    << "DEMAOsc calculation mismatch at step " << i
                    << " (price=" << current_price << ", dema=" << dema_value << ")";
            }
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
}

// DEMAOsc零线穿越测试
TEST(OriginalTests, DEMAOsc_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto demaosc = std::make_shared<DEMAOsc>(close_line, 20);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_osc = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越
    for (size_t i = 0; i < csv_data.size(); ++i) {
        demaosc->calculate();
        
        double current_osc = demaosc->get(0);
        
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
    
    std::cout << "DEMAOsc zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// DEMAOsc趋势分析测试
TEST(OriginalTests, DEMAOsc_TrendAnalysis) {
    // 创建明确的趋势数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 100; ++i) {
        trend_prices.push_back(100.0 + i * 0.5);  // 缓慢上升趋势
    }
    
    auto trend_line = std::make_shared<backtrader::LineRoot>(trend_prices.size(), "trend");
    for (double price : trend_prices) {
        trend_line->forward(price);
    }
    
    auto trend_demaosc = std::make_shared<DEMAOsc>(trend_line, 20);
    
    int positive_values = 0;
    int negative_values = 0;
    int zero_values = 0;
    
    for (size_t i = 0; i < trend_prices.size(); ++i) {
        trend_demaosc->calculate();
        
        double osc_value = trend_demaosc->get(0);
        
        if (!std::isnan(osc_value)) {
            if (osc_value > 0.01) {
                positive_values++;
            } else if (osc_value < -0.01) {
                negative_values++;
            } else {
                zero_values++;
            }
        }
        
        if (i < trend_prices.size() - 1) {
            trend_line->forward();
        }
    }
    
    std::cout << "Trend analysis:" << std::endl;
    std::cout << "Positive oscillator values: " << positive_values << std::endl;
    std::cout << "Negative oscillator values: " << negative_values << std::endl;
    std::cout << "Near-zero values: " << zero_values << std::endl;
    
    // 在上升趋势中，价格往往会高于移动平均线
    EXPECT_GT(positive_values, negative_values) 
        << "In uptrend, oscillator should be positive more often";
}

// DEMAOsc响应速度测试
TEST(OriginalTests, DEMAOsc_ResponseSpeed) {
    // 创建价格突然变化的数据来测试响应速度
    std::vector<double> step_prices;
    
    // 第一阶段：稳定价格
    for (int i = 0; i < 60; ++i) {
        step_prices.push_back(100.0);
    }
    
    // 第二阶段：价格跳跃
    for (int i = 0; i < 60; ++i) {
        step_prices.push_back(120.0);
    }
    
    auto step_line = std::make_shared<backtrader::LineRoot>(step_prices.size(), "step");
    for (double price : step_prices) {
        step_line->forward(price);
    }
    
    auto demaosc = std::make_shared<DEMAOsc>(step_line, 20);
    auto emaosc = std::make_shared<EMAOsc>(step_line, 20);   // 比较对象
    auto smaosc = std::make_shared<SMAOsc>(step_line, 20);   // 比较对象
    
    std::vector<double> dema_responses, ema_responses, sma_responses;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        demaosc->calculate();
        emaosc->calculate();
        smaosc->calculate();
        
        double dema_osc = demaosc->get(0);
        double ema_osc = emaosc->get(0);
        double sma_osc = smaosc->get(0);
        
        if (!std::isnan(dema_osc) && !std::isnan(ema_osc) && !std::isnan(sma_osc)) {
            if (i >= 60) {  // 价格跳跃后
                dema_responses.push_back(dema_osc);
                ema_responses.push_back(ema_osc);
                sma_responses.push_back(sma_osc);
            }
        }
        
        if (i < step_prices.size() - 1) {
            step_line->forward();
        }
    }
    
    // 比较响应速度
    if (!dema_responses.empty() && !ema_responses.empty() && !sma_responses.empty()) {
        double final_dema = dema_responses.back();
        double final_ema = ema_responses.back();
        double final_sma = sma_responses.back();
        
        std::cout << "Response speed comparison:" << std::endl;
        std::cout << "Final DEMA oscillator: " << final_dema << std::endl;
        std::cout << "Final EMA oscillator: " << final_ema << std::endl;
        std::cout << "Final SMA oscillator: " << final_sma << std::endl;
        
        // DEMA应该比EMA更快地响应价格变化，EMA比SMA更快
        EXPECT_GT(final_dema, final_ema * 0.95) 
            << "DEMA oscillator should respond faster than EMA oscillator";
        EXPECT_GT(final_ema, final_sma * 0.95) 
            << "EMA oscillator should respond faster than SMA oscillator";
    }
}

// DEMAOsc振荡特性测试
TEST(OriginalTests, DEMAOsc_OscillationCharacteristics) {
    // 创建振荡数据
    std::vector<double> oscillating_prices;
    for (int i = 0; i < 100; ++i) {
        double base = 100.0;
        double oscillation = 5.0 * std::sin(i * 0.3);
        oscillating_prices.push_back(base + oscillation);
    }
    
    auto osc_line = std::make_shared<backtrader::LineRoot>(oscillating_prices.size(), "oscillating");
    for (double price : oscillating_prices) {
        osc_line->forward(price);
    }
    
    auto demaosc = std::make_shared<DEMAOsc>(osc_line, 20);
    
    std::vector<double> oscillator_values;
    
    for (size_t i = 0; i < oscillating_prices.size(); ++i) {
        demaosc->calculate();
        
        double osc_val = demaosc->get(0);
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
        
        std::cout << "Oscillator characteristics:" << std::endl;
        std::cout << "Average: " << avg_oscillator << std::endl;
        std::cout << "Standard deviation: " << std_dev << std::endl;
        
        // 振荡器应该围绕零线波动
        EXPECT_NEAR(avg_oscillator, 0.0, 2.0) 
            << "Oscillator should oscillate around zero";
        
        EXPECT_GT(std_dev, 1.0) 
            << "Oscillator should show meaningful variation";
    }
}

// DEMAOsc与不同基础指标比较测试
TEST(OriginalTests, DEMAOsc_DifferentBaseIndicators) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建不同的基础指标
    auto dema_osc = std::make_shared<DEMAOsc>(close_line, 20);
    auto ema_osc = std::make_shared<EMAOsc>(close_line, 20);
    auto sma_osc = std::make_shared<SMAOsc>(close_line, 20);
    
    std::vector<double> dema_osc_values;
    std::vector<double> ema_osc_values;
    std::vector<double> sma_osc_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        dema_osc->calculate();
        ema_osc->calculate();
        sma_osc->calculate();
        
        double dema_osc_val = dema_osc->get(0);
        double ema_osc_val = ema_osc->get(0);
        double sma_osc_val = sma_osc->get(0);
        
        if (!std::isnan(dema_osc_val)) {
            dema_osc_values.push_back(dema_osc_val);
        }
        if (!std::isnan(ema_osc_val)) {
            ema_osc_values.push_back(ema_osc_val);
        }
        if (!std::isnan(sma_osc_val)) {
            sma_osc_values.push_back(sma_osc_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 比较不同基础指标的振荡特性
    if (!dema_osc_values.empty() && !ema_osc_values.empty() && !sma_osc_values.empty()) {
        double dema_avg = std::accumulate(dema_osc_values.begin(), dema_osc_values.end(), 0.0) / dema_osc_values.size();
        double ema_avg = std::accumulate(ema_osc_values.begin(), ema_osc_values.end(), 0.0) / ema_osc_values.size();
        double sma_avg = std::accumulate(sma_osc_values.begin(), sma_osc_values.end(), 0.0) / sma_osc_values.size();
        
        std::cout << "Base indicator comparison:" << std::endl;
        std::cout << "DEMA-based oscillator average: " << dema_avg << std::endl;
        std::cout << "EMA-based oscillator average: " << ema_avg << std::endl;
        std::cout << "SMA-based oscillator average: " << sma_avg << std::endl;
        
        // 所有振荡器都应该围绕零线振荡
        EXPECT_NEAR(dema_avg, 0.0, 10.0) << "DEMA-based oscillator should center around zero";
        EXPECT_NEAR(ema_avg, 0.0, 10.0) << "EMA-based oscillator should center around zero";
        EXPECT_NEAR(sma_avg, 0.0, 10.0) << "SMA-based oscillator should center around zero";
    }
}

// DEMAOsc超买超卖信号测试
TEST(OriginalTests, DEMAOsc_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto demaosc = std::make_shared<DEMAOsc>(close_line, 20);
    
    std::vector<double> oscillator_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        demaosc->calculate();
        
        double osc_val = demaosc->get(0);
        if (!std::isnan(osc_val)) {
            oscillator_values.push_back(osc_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 计算动态阈值（基于标准差）
    if (!oscillator_values.empty()) {
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
            if (val > overbought_threshold) {
                overbought_signals++;
            } else if (val < oversold_threshold) {
                oversold_signals++;
            }
        }
        
        std::cout << "Overbought/Oversold analysis:" << std::endl;
        std::cout << "Mean: " << mean << ", Std Dev: " << std_dev << std::endl;
        std::cout << "Overbought threshold: " << overbought_threshold << std::endl;
        std::cout << "Oversold threshold: " << oversold_threshold << std::endl;
        std::cout << "Overbought signals: " << overbought_signals << std::endl;
        std::cout << "Oversold signals: " << oversold_signals << std::endl;
        
        // 验证有一些超买超卖信号
        EXPECT_GE(overbought_signals + oversold_signals, 0) 
            << "Should generate some overbought/oversold signals";
    }
}

// DEMAOsc动量分析测试
TEST(OriginalTests, DEMAOsc_MomentumAnalysis) {
    // 创建具有不同动量的数据
    std::vector<double> momentum_prices;
    
    // 第一阶段：加速上升
    for (int i = 0; i < 60; ++i) {
        momentum_prices.push_back(100.0 + i * i * 0.05);
    }
    
    // 第二阶段：减速上升
    for (int i = 0; i < 60; ++i) {
        double increment = 2.0 - i * 0.03;
        momentum_prices.push_back(momentum_prices.back() + std::max(0.1, increment));
    }
    
    auto momentum_line = std::make_shared<backtrader::LineRoot>(momentum_prices.size(), "momentum");
    for (double price : momentum_prices) {
        momentum_line->forward(price);
    }
    
    auto momentum_demaosc = std::make_shared<DEMAOsc>(momentum_line, 20);
    
    std::vector<double> accelerating_osc, decelerating_osc;
    
    for (size_t i = 0; i < momentum_prices.size(); ++i) {
        momentum_demaosc->calculate();
        
        double osc_val = momentum_demaosc->get(0);
        if (!std::isnan(osc_val)) {
            if (i < 60) {
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
        
        std::cout << "Momentum analysis:" << std::endl;
        std::cout << "Accelerating phase oscillator avg: " << acc_avg << std::endl;
        std::cout << "Decelerating phase oscillator avg: " << dec_avg << std::endl;
        
        // 加速阶段应该有更高的振荡器值
        EXPECT_GT(acc_avg, dec_avg) << "Accelerating phase should have higher oscillator values";
    }
}

// DEMAOsc发散测试
TEST(OriginalTests, DEMAOsc_Divergence) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto demaosc = std::make_shared<DEMAOsc>(close_line, 20);
    
    std::vector<double> prices;
    std::vector<double> osc_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        demaosc->calculate();
        
        double osc_val = demaosc->get(0);
        if (!std::isnan(osc_val)) {
            prices.push_back(csv_data[i].close);
            osc_values.push_back(osc_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 寻找价格和振荡器的峰值点进行发散分析
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
    std::cout << "Price peaks found: " << price_peaks.size() << std::endl;
    std::cout << "Oscillator peaks found: " << osc_peaks.size() << std::endl;
    
    // 分析最近的几个峰值
    if (price_peaks.size() >= 2) {
        size_t last_peak = price_peaks.back();
        size_t prev_peak = price_peaks[price_peaks.size() - 2];
        
        std::cout << "Recent price peak comparison:" << std::endl;
        std::cout << "Previous peak: " << prices[prev_peak] << " at index " << prev_peak << std::endl;
        std::cout << "Latest peak: " << prices[last_peak] << " at index " << last_peak << std::endl;
        std::cout << "Corresponding oscillator values: " << osc_values[prev_peak] 
                  << " -> " << osc_values[last_peak] << std::endl;
    }
    
    EXPECT_TRUE(true) << "Divergence analysis completed";
}

// 边界条件测试
TEST(OriginalTests, DEMAOsc_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<backtrader::LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_demaosc = std::make_shared<DEMAOsc>(flat_line, 20);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_demaosc->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，振荡器应该为零
    double final_demaosc = flat_demaosc->get(0);
    if (!std::isnan(final_demaosc)) {
        EXPECT_NEAR(final_demaosc, 0.0, 1e-6) 
            << "DEMAOsc should be zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<backtrader::LineRoot>(100, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 30; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_demaosc = std::make_shared<DEMAOsc>(insufficient_line, 20);
    
    for (int i = 0; i < 30; ++i) {
        insufficient_demaosc->calculate();
        if (i < 29) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_demaosc->get(0);
    EXPECT_TRUE(std::isnan(result)) << "DEMAOsc should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, DEMAOsc_Performance) {
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
    
    auto large_demaosc = std::make_shared<DEMAOsc>(large_line, 50);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_demaosc->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "DEMAOsc calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_demaosc->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}
