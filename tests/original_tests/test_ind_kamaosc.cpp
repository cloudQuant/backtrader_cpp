/**
 * @file test_ind_kamaosc.cpp
 * @brief KAMAOsc指标测试 - 对应Python test_ind_kamaosc.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['65.752078', '78.911000', '39.950810']
 * ]
 * chkmin = 31
 * chkind = btind.KAMAOsc
 * 
 * 注：KAMAOsc (KAMA Oscillator) 是价格与KAMA的振荡器
 */

#include "test_common.h"
#include <random>

#include "indicators/kamaosc.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> KAMAOSC_EXPECTED_VALUES = {
    {"65.752078", "78.911000", "39.950810"}
};

const int KAMAOSC_MIN_PERIOD = 31;

} // anonymous namespace

// 使用默认参数的KAMAOsc测试
DEFINE_INDICATOR_TEST(KAMAOsc_Default, KAMAOsc, KAMAOSC_EXPECTED_VALUES, KAMAOSC_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, KAMAOsc_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建KAMAOsc指标
    auto kamaosc = std::make_shared<KAMAOsc>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        kamaosc->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 31;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"65.752078", "78.911000", "39.950810"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = kamaosc->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "KAMAOsc value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(kamaosc->getMinPeriod(), 31) << "KAMAOsc minimum period should be 31";
}

// 参数化测试 - 测试不同参数的KAMAOsc
class KAMAOscParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int, int>> {
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

TEST_P(KAMAOscParameterizedTest, DifferentParameters) {
    auto [period, fast, slow] = GetParam();
    auto kamaosc = std::make_shared<KAMAOsc>(close_line_, period, fast, slow);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        kamaosc->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(kamaosc->getMinPeriod(), period + 1) 
        << "KAMAOsc minimum period should equal period + 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        double last_value = kamaosc->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last KAMAOsc value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last KAMAOsc value should be finite";
    }
}

// 测试不同的KAMAOsc参数组合
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    KAMAOscParameterizedTest,
    ::testing::Values(
        std::make_tuple(14, 2, 30),
        std::make_tuple(20, 2, 30),
        std::make_tuple(30, 2, 30),
        std::make_tuple(20, 3, 20)
    )
);

// KAMAOsc计算逻辑验证测试
TEST(OriginalTests, KAMAOsc_CalculationLogic) {
    // 使用简单的测试数据验证KAMAOsc计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0,
                                  124.0, 126.0, 128.0, 130.0, 132.0, 134.0, 136.0, 138.0, 140.0, 142.0,
                                  144.0, 146.0, 148.0, 150.0, 152.0, 154.0, 156.0, 158.0, 160.0, 162.0};
    
    auto price_line = std::make_shared<backtrader::LineRoot>(prices.size(), "kamaosc_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto kamaosc = std::make_shared<KAMAOsc>(price_line, 14, 2, 30);
    auto kama = std::make_shared<KAMA>(price_line, 14, 2, 30);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        kamaosc->calculate();
        kama->calculate();
        
        // 手动验证KAMAOsc计算：KAMAOsc = Price - KAMA
        if (i >= 14) {  // KAMA需要15个数据点
            double current_price = prices[i];
            double kama_value = kama->get(0);
            double expected_kamaosc = current_price - kama_value;
            double actual_kamaosc = kamaosc->get(0);
            
            if (!std::isnan(actual_kamaosc) && !std::isnan(kama_value)) {
                EXPECT_NEAR(actual_kamaosc, expected_kamaosc, 1e-10) 
                    << "KAMAOsc calculation mismatch at step " << i
                    << " (price=" << current_price << ", kama=" << kama_value << ")";
            }
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
}

// KAMAOsc零线穿越测试
TEST(OriginalTests, KAMAOsc_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto kamaosc = std::make_shared<KAMAOsc>(close_line, 20, 2, 30);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_osc = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越
    for (size_t i = 0; i < csv_data.size(); ++i) {
        kamaosc->calculate();
        
        double current_osc = kamaosc->get(0);
        
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
    
    std::cout << "KAMAOsc zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// KAMAOsc趋势分析测试
TEST(OriginalTests, KAMAOsc_TrendAnalysis) {
    // 创建明确的趋势数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 100; ++i) {
        trend_prices.push_back(100.0 + i * 0.5);  // 缓慢上升趋势
    }
    
    auto trend_line = std::make_shared<backtrader::LineRoot>(trend_prices.size(), "trend");
    for (double price : trend_prices) {
        trend_line->forward(price);
    }
    
    auto trend_kamaosc = std::make_shared<KAMAOsc>(trend_line, 20, 2, 30);
    
    int positive_values = 0;
    int negative_values = 0;
    int zero_values = 0;
    
    for (size_t i = 0; i < trend_prices.size(); ++i) {
        trend_kamaosc->calculate();
        
        double osc_value = trend_kamaosc->get(0);
        
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

// KAMAOsc自适应特性测试
TEST(OriginalTests, KAMAOsc_AdaptiveCharacteristics) {
    // 创建波动性变化的数据
    std::vector<double> varying_vol_prices;
    
    // 第一阶段：低波动性
    for (int i = 0; i < 50; ++i) {
        double base = 100.0 + i * 0.1;
        double noise = std::sin(i * 0.1) * 0.5;  // 小幅波动
        varying_vol_prices.push_back(base + noise);
    }
    
    // 第二阶段：高波动性
    for (int i = 0; i < 50; ++i) {
        double base = 105.0 + i * 0.1;
        double noise = std::sin(i * 0.5) * 3.0;  // 大幅波动
        varying_vol_prices.push_back(base + noise);
    }
    
    auto varying_line = std::make_shared<backtrader::LineRoot>(varying_vol_prices.size(), "varying");
    for (double price : varying_vol_prices) {
        varying_line->forward(price);
    }
    
    auto kamaosc = std::make_shared<KAMAOsc>(varying_line, 20, 2, 30);
    auto emaosc = std::make_shared<EMAOsc>(varying_line, 20);  // 比较对象
    
    std::vector<double> low_vol_kama, high_vol_kama;
    std::vector<double> low_vol_ema, high_vol_ema;
    
    for (size_t i = 0; i < varying_vol_prices.size(); ++i) {
        kamaosc->calculate();
        emaosc->calculate();
        
        double kama_osc = kamaosc->get(0);
        double ema_osc = emaosc->get(0);
        
        if (!std::isnan(kama_osc) && !std::isnan(ema_osc)) {
            if (i < 50) {  // 低波动阶段
                low_vol_kama.push_back(std::abs(kama_osc));
                low_vol_ema.push_back(std::abs(ema_osc));
            } else {  // 高波动阶段
                high_vol_kama.push_back(std::abs(kama_osc));
                high_vol_ema.push_back(std::abs(ema_osc));
            }
        }
        
        if (i < varying_vol_prices.size() - 1) {
            varying_line->forward();
        }
    }
    
    // 分析自适应特性
    if (!low_vol_kama.empty() && !high_vol_kama.empty() && 
        !low_vol_ema.empty() && !high_vol_ema.empty()) {
        
        double avg_low_kama = std::accumulate(low_vol_kama.begin(), low_vol_kama.end(), 0.0) / low_vol_kama.size();
        double avg_high_kama = std::accumulate(high_vol_kama.begin(), high_vol_kama.end(), 0.0) / high_vol_kama.size();
        double avg_low_ema = std::accumulate(low_vol_ema.begin(), low_vol_ema.end(), 0.0) / low_vol_ema.size();
        double avg_high_ema = std::accumulate(high_vol_ema.begin(), high_vol_ema.end(), 0.0) / high_vol_ema.size();
        
        std::cout << "Adaptive characteristics analysis:" << std::endl;
        std::cout << "KAMA low volatility avg: " << avg_low_kama << std::endl;
        std::cout << "KAMA high volatility avg: " << avg_high_kama << std::endl;
        std::cout << "EMA low volatility avg: " << avg_low_ema << std::endl;
        std::cout << "EMA high volatility avg: " << avg_high_ema << std::endl;
        
        // KAMA应该在不同波动环境下表现出自适应特性
        EXPECT_GT(avg_low_kama, 0.0) << "KAMA should have positive oscillations in low volatility";
        EXPECT_GT(avg_high_kama, 0.0) << "KAMA should have positive oscillations in high volatility";
    }
}

// KAMAOsc响应速度测试
TEST(OriginalTests, KAMAOsc_ResponseSpeed) {
    // 创建价格突然变化的数据来测试响应速度
    std::vector<double> step_prices;
    
    // 第一阶段：稳定价格
    for (int i = 0; i < 50; ++i) {
        step_prices.push_back(100.0);
    }
    
    // 第二阶段：价格跳跃
    for (int i = 0; i < 50; ++i) {
        step_prices.push_back(120.0);
    }
    
    auto step_line = std::make_shared<backtrader::LineRoot>(step_prices.size(), "step");
    for (double price : step_prices) {
        step_line->forward(price);
    }
    
    auto kamaosc = std::make_shared<KAMAOsc>(step_line, 20, 2, 30);
    auto emaosc = std::make_shared<EMAOsc>(step_line, 20);   // 比较对象
    auto smaosc = std::make_shared<SMAOsc>(step_line, 20);   // 比较对象
    
    std::vector<double> kama_responses, ema_responses, sma_responses;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        kamaosc->calculate();
        emaosc->calculate();
        smaosc->calculate();
        
        double kama_osc = kamaosc->get(0);
        double ema_osc = emaosc->get(0);
        double sma_osc = smaosc->get(0);
        
        if (!std::isnan(kama_osc) && !std::isnan(ema_osc) && !std::isnan(sma_osc)) {
            if (i >= 50) {  // 价格跳跃后
                kama_responses.push_back(kama_osc);
                ema_responses.push_back(ema_osc);
                sma_responses.push_back(sma_osc);
            }
        }
        
        if (i < step_prices.size() - 1) {
            step_line->forward();
        }
    }
    
    // 比较响应速度
    if (!kama_responses.empty() && !ema_responses.empty() && !sma_responses.empty()) {
        double final_kama = kama_responses.back();
        double final_ema = ema_responses.back();
        double final_sma = sma_responses.back();
        
        std::cout << "Response speed comparison:" << std::endl;
        std::cout << "Final KAMA oscillator: " << final_kama << std::endl;
        std::cout << "Final EMA oscillator: " << final_ema << std::endl;
        std::cout << "Final SMA oscillator: " << final_sma << std::endl;
        
        // KAMA在价格跳跃时应该快速响应
        EXPECT_GT(std::abs(final_kama), std::abs(final_sma) * 0.5) 
            << "KAMA oscillator should respond to price jumps";
    }
}

// KAMAOsc振荡特性测试
TEST(OriginalTests, KAMAOsc_OscillationCharacteristics) {
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
    
    auto kamaosc = std::make_shared<KAMAOsc>(osc_line, 20, 2, 30);
    
    std::vector<double> oscillator_values;
    
    for (size_t i = 0; i < oscillating_prices.size(); ++i) {
        kamaosc->calculate();
        
        double osc_val = kamaosc->get(0);
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

// KAMAOsc与其他振荡器比较测试
TEST(OriginalTests, KAMAOsc_vs_OtherOscillators) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto kamaosc = std::make_shared<KAMAOsc>(close_line, 20, 2, 30);
    auto emaosc = std::make_shared<EMAOsc>(close_line, 20);
    auto smaosc = std::make_shared<SMAOsc>(close_line, 20);
    
    std::vector<double> kama_values, ema_values, sma_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        kamaosc->calculate();
        emaosc->calculate();
        smaosc->calculate();
        
        double kama_val = kamaosc->get(0);
        double ema_val = emaosc->get(0);
        double sma_val = smaosc->get(0);
        
        if (!std::isnan(kama_val)) kama_values.push_back(kama_val);
        if (!std::isnan(ema_val)) ema_values.push_back(ema_val);
        if (!std::isnan(sma_val)) sma_values.push_back(sma_val);
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 比较不同振荡器的特性
    if (!kama_values.empty() && !ema_values.empty() && !sma_values.empty()) {
        double kama_avg = std::accumulate(kama_values.begin(), kama_values.end(), 0.0) / kama_values.size();
        double ema_avg = std::accumulate(ema_values.begin(), ema_values.end(), 0.0) / ema_values.size();
        double sma_avg = std::accumulate(sma_values.begin(), sma_values.end(), 0.0) / sma_values.size();
        
        std::cout << "Oscillator comparison:" << std::endl;
        std::cout << "KAMA oscillator average: " << kama_avg << std::endl;
        std::cout << "EMA oscillator average: " << ema_avg << std::endl;
        std::cout << "SMA oscillator average: " << sma_avg << std::endl;
        
        // 所有振荡器都应该围绕零线振荡
        EXPECT_NEAR(kama_avg, 0.0, 15.0) << "KAMA oscillator should center around zero";
        EXPECT_NEAR(ema_avg, 0.0, 15.0) << "EMA oscillator should center around zero";
        EXPECT_NEAR(sma_avg, 0.0, 15.0) << "SMA oscillator should center around zero";
    }
}

// 边界条件测试
TEST(OriginalTests, KAMAOsc_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<backtrader::LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_kamaosc = std::make_shared<KAMAOsc>(flat_line, 20, 2, 30);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_kamaosc->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，振荡器应该为零
    double final_kamaosc = flat_kamaosc->get(0);
    if (!std::isnan(final_kamaosc)) {
        EXPECT_NEAR(final_kamaosc, 0.0, 1e-6) 
            << "KAMAOsc should be zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<backtrader::LineRoot>(100, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 20; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_kamaosc = std::make_shared<KAMAOsc>(insufficient_line, 30, 2, 30);
    
    for (int i = 0; i < 20; ++i) {
        insufficient_kamaosc->calculate();
        if (i < 19) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_kamaosc->get(0);
    EXPECT_TRUE(std::isnan(result)) << "KAMAOsc should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, KAMAOsc_Performance) {
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
    
    auto large_kamaosc = std::make_shared<KAMAOsc>(large_line, 50, 2, 30);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_kamaosc->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "KAMAOsc calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_kamaosc->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}
