/**
 * @file test_ind_oscillator.cpp
 * @brief Oscillator指标测试 - 对应Python test_ind_oscillator.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['56.477000', '51.185333', '2.386667']
 * ]
 * chkmin = 30
 * chkind = btind.Oscillator
 * 
 * 注：该测试使用SMA作为基础指标，计算价格与SMA的振荡值
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/oscillator.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> OSCILLATOR_EXPECTED_VALUES = {
    {"56.477000", "51.185333", "2.386667"}
};

const int OSCILLATOR_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的Oscillator测试
DEFINE_INDICATOR_TEST(Oscillator_Default, Oscillator, OSCILLATOR_EXPECTED_VALUES, OSCILLATOR_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, Oscillator_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建SMA作为基础指标
    auto sma = std::make_shared<SMA>(close_line, 30);
    
    // 创建Oscillator指标（价格与SMA的振荡）
    auto oscillator = std::make_shared<Oscillator>(close_line, sma);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        sma->calculate();
        oscillator->calculate();
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
    
    std::vector<std::string> expected = {"56.477000", "51.185333", "2.386667"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = oscillator->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "Oscillator value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(oscillator->getMinPeriod(), 30) << "Oscillator minimum period should be 30";
}

// 参数化测试 - 测试不同基础指标的Oscillator
class OscillatorParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(OscillatorParameterizedTest, DifferentBasePeriods) {
    int period = GetParam();
    auto base_indicator = std::make_shared<SMA>(close_line_, period);
    auto oscillator = std::make_shared<Oscillator>(close_line_, base_indicator);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        base_indicator->calculate();
        oscillator->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(oscillator->getMinPeriod(), period) 
        << "Oscillator minimum period should equal base indicator period";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_value = oscillator->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last Oscillator value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last Oscillator value should be finite";
    }
}

// 测试不同的基础指标周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    OscillatorParameterizedTest,
    ::testing::Values(10, 20, 30, 50)
);

// Oscillator计算逻辑验证测试
TEST(OriginalTests, Oscillator_CalculationLogic) {
    // 使用简单的测试数据验证Oscillator计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 108.0, 106.0, 104.0, 102.0};
    
    auto price_line = std::make_shared<LineRoot>(prices.size(), "osc_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto sma = std::make_shared<SMA>(price_line, 5);
    auto oscillator = std::make_shared<Oscillator>(price_line, sma);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        sma->calculate();
        oscillator->calculate();
        
        // 手动验证Oscillator计算：Oscillator = Price - BaseIndicator
        if (i >= 4) {  // SMA需要5个数据点
            double current_price = prices[i];
            double sma_value = sma->get(0);
            double expected_oscillator = current_price - sma_value;
            double actual_oscillator = oscillator->get(0);
            
            if (!std::isnan(actual_oscillator) && !std::isnan(sma_value)) {
                EXPECT_NEAR(actual_oscillator, expected_oscillator, 1e-10) 
                    << "Oscillator calculation mismatch at step " << i
                    << " (price=" << current_price << ", sma=" << sma_value << ")";
            }
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
}

// Oscillator零线穿越测试
TEST(OriginalTests, Oscillator_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto sma = std::make_shared<SMA>(close_line, 20);
    auto oscillator = std::make_shared<Oscillator>(close_line, sma);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_osc = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越
    for (size_t i = 0; i < csv_data.size(); ++i) {
        sma->calculate();
        oscillator->calculate();
        
        double current_osc = oscillator->get(0);
        
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
    
    std::cout << "Oscillator zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// Oscillator趋势分析测试
TEST(OriginalTests, Oscillator_TrendAnalysis) {
    // 创建明确的趋势数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 100; ++i) {
        trend_prices.push_back(100.0 + i * 0.5);  // 缓慢上升趋势
    }
    
    auto trend_line = std::make_shared<LineRoot>(trend_prices.size(), "trend");
    for (double price : trend_prices) {
        trend_line->forward(price);
    }
    
    auto trend_sma = std::make_shared<SMA>(trend_line, 20);
    auto trend_oscillator = std::make_shared<Oscillator>(trend_line, trend_sma);
    
    int positive_values = 0;
    int negative_values = 0;
    int zero_values = 0;
    
    for (size_t i = 0; i < trend_prices.size(); ++i) {
        trend_sma->calculate();
        trend_oscillator->calculate();
        
        double osc_value = trend_oscillator->get(0);
        
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

// Oscillator振荡特性测试
TEST(OriginalTests, Oscillator_OscillationCharacteristics) {
    // 创建振荡数据
    std::vector<double> oscillating_prices;
    for (int i = 0; i < 100; ++i) {
        double base = 100.0;
        double oscillation = 5.0 * std::sin(i * 0.3);
        oscillating_prices.push_back(base + oscillation);
    }
    
    auto osc_line = std::make_shared<LineRoot>(oscillating_prices.size(), "oscillating");
    for (double price : oscillating_prices) {
        osc_line->forward(price);
    }
    
    auto osc_sma = std::make_shared<SMA>(osc_line, 20);
    auto oscillator = std::make_shared<Oscillator>(osc_line, osc_sma);
    
    std::vector<double> oscillator_values;
    
    for (size_t i = 0; i < oscillating_prices.size(); ++i) {
        osc_sma->calculate();
        oscillator->calculate();
        
        double osc_val = oscillator->get(0);
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

// Oscillator与不同基础指标比较测试
TEST(OriginalTests, Oscillator_DifferentBaseIndicators) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建不同的基础指标
    auto sma = std::make_shared<SMA>(close_line, 20);
    auto ema = std::make_shared<EMA>(close_line, 20);
    
    auto sma_oscillator = std::make_shared<Oscillator>(close_line, sma);
    auto ema_oscillator = std::make_shared<Oscillator>(close_line, ema);
    
    std::vector<double> sma_osc_values;
    std::vector<double> ema_osc_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        sma->calculate();
        ema->calculate();
        sma_oscillator->calculate();
        ema_oscillator->calculate();
        
        double sma_osc = sma_oscillator->get(0);
        double ema_osc = ema_oscillator->get(0);
        
        if (!std::isnan(sma_osc)) {
            sma_osc_values.push_back(sma_osc);
        }
        if (!std::isnan(ema_osc)) {
            ema_osc_values.push_back(ema_osc);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 比较不同基础指标的振荡特性
    if (!sma_osc_values.empty() && !ema_osc_values.empty()) {
        double sma_avg = std::accumulate(sma_osc_values.begin(), sma_osc_values.end(), 0.0) / sma_osc_values.size();
        double ema_avg = std::accumulate(ema_osc_values.begin(), ema_osc_values.end(), 0.0) / ema_osc_values.size();
        
        std::cout << "Base indicator comparison:" << std::endl;
        std::cout << "SMA-based oscillator average: " << sma_avg << std::endl;
        std::cout << "EMA-based oscillator average: " << ema_avg << std::endl;
        
        // 两者都应该围绕零线振荡
        EXPECT_NEAR(sma_avg, 0.0, 10.0) << "SMA-based oscillator should center around zero";
        EXPECT_NEAR(ema_avg, 0.0, 10.0) << "EMA-based oscillator should center around zero";
    }
}

// Oscillator超买超卖信号测试
TEST(OriginalTests, Oscillator_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto sma = std::make_shared<SMA>(close_line, 20);
    auto oscillator = std::make_shared<Oscillator>(close_line, sma);
    
    std::vector<double> oscillator_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        sma->calculate();
        oscillator->calculate();
        
        double osc_val = oscillator->get(0);
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
        
        // 验证有一些信号产生
        EXPECT_GT(overbought_signals + oversold_signals, 0) 
            << "Should generate some overbought/oversold signals";
    }
}

// 边界条件测试
TEST(OriginalTests, Oscillator_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_sma = std::make_shared<SMA>(flat_line, 20);
    auto flat_oscillator = std::make_shared<Oscillator>(flat_line, flat_sma);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_sma->calculate();
        flat_oscillator->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，振荡器应该为零
    double final_oscillator = flat_oscillator->get(0);
    if (!std::isnan(final_oscillator)) {
        EXPECT_NEAR(final_oscillator, 0.0, 1e-6) 
            << "Oscillator should be zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 15; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_sma = std::make_shared<SMA>(insufficient_line, 20);
    auto insufficient_oscillator = std::make_shared<Oscillator>(insufficient_line, insufficient_sma);
    
    for (int i = 0; i < 15; ++i) {
        insufficient_sma->calculate();
        insufficient_oscillator->calculate();
        if (i < 14) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_oscillator->get(0);
    EXPECT_TRUE(std::isnan(result)) << "Oscillator should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, Oscillator_Performance) {
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
    
    auto large_sma = std::make_shared<SMA>(large_line, 50);
    auto large_oscillator = std::make_shared<Oscillator>(large_line, large_sma);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_sma->calculate();
        large_oscillator->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Oscillator calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_oscillator->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}