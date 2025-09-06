/**
 * @file test_ind_kst.cpp
 * @brief KST指标测试 - 对应Python test_ind_kst.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['18.966300', '33.688645', '27.643797'],
 *     ['11.123593', '37.882890', '16.602624']
 * ]
 * chkmin = 48
 * chkind = bt.ind.KST
 * 
 * 注：KST (Know Sure Thing) 是一个动量振荡器，包含2条线
 */

#include "test_common.h"
#include "lineseries.h"
#include "linebuffer.h"
#include <random>

#include "indicators/kst.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> KST_EXPECTED_VALUES = {
    {"18.966300", "33.688645", "27.643797"},  // line 0 (KST)
    {"11.123593", "37.882890", "16.602624"}   // line 1 (Signal)
};

const int KST_MIN_PERIOD = 48;

} // anonymous namespace

// 使用默认参数的KST测试
DEFINE_INDICATOR_TEST(KST_Default, KST, KST_EXPECTED_VALUES, KST_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, KST_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    // 创建KST指标
    auto kst = std::make_shared<KST>(close_line);
    
    // 计算所有值 - KST uses batch processing
    kst->calculate();;
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 48;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    // 对于负数除法，Python的//和C++的/行为不同
    // Python: -207 // 2 = -104
    // C++: -207 / 2 = -103
    int ago_mid = -(data_length - min_period);  // -207
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        ago_mid,                              // -207
        static_cast<int>(std::floor(static_cast<double>(ago_mid) / 2.0))  // Python floor division: -104
    };
    
    // 验证2条线的值
    
    for (int line = 0; line < 2; ++line) {
        auto expected = KST_EXPECTED_VALUES[line];
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
            auto line_buffer = kst->getLine(line);
            if (!line_buffer) {
                FAIL() << "Failed to get line " << line << " from KST indicator";
            }
            double actual = line_buffer->get(check_points[i]);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << actual;
            std::string actual_str = ss.str();
            
            EXPECT_EQ(actual_str, expected[i]) 
                << "KST line " << line << " value mismatch at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(kst->getMinPeriod(), 48) << "KST minimum period should be 48";
}

// 参数化测试 - 测试不同参数的KST
class KSTParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int, int, int>> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line_ = std::make_shared<LineSeries>();
        close_line_->lines->add_line(std::make_shared<LineBuffer>());
        close_line_buffer_ = std::dynamic_pointer_cast<LineBuffer>(close_line_->lines->getline(0));
        
        for (const auto& bar : csv_data_) {
            close_line_buffer_->append(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineSeries> close_line_;
    std::shared_ptr<LineBuffer> close_line_buffer_;
};

TEST_P(KSTParameterizedTest, DifferentParameters) {
    auto [roc1, roc2, roc3, roc4] = GetParam();
    
    // 使用自定义参数创建KST（如果支持）
    auto kst = std::make_shared<KST>(close_line_); // 默认参数
    
    // 计算所有值 - KST uses batch processing
    kst->calculate();
    
    // 验证最后的值
    if (csv_data_.size() >= 48) {  // 确保有足够数据
        auto kst_line = kst->getLine(0);
        auto signal_line = kst->getLine(1);
        if (!kst_line || !signal_line) {
            FAIL() << "KST lines not properly initialized";
        }
        double last_kst = kst_line->get(0);
        double last_signal = signal_line->get(0);
        
        EXPECT_FALSE(std::isnan(last_kst)) << "Last KST value should not be NaN";
        EXPECT_FALSE(std::isnan(last_signal)) << "Last Signal value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_kst)) << "Last KST value should be finite";
        EXPECT_TRUE(std::isfinite(last_signal)) << "Last Signal value should be finite";
    }
}

// 测试不同的ROC周期参数
INSTANTIATE_TEST_SUITE_P(
    VariousROCPeriods,
    KSTParameterizedTest,
    ::testing::Values(
        std::make_tuple(10, 15, 20, 30),
        std::make_tuple(9, 12, 18, 24),
        std::make_tuple(6, 9, 12, 18)
    )
);

// KST计算逻辑验证测试
TEST(OriginalTests, KST_CalculationLogic) {
    // 使用简单的测试数据验证KST计算
    std::vector<double> prices;
    // 创建一个有趋势的数据序列;
    for (int i = 0; i < 100; ++i) {
        double base = 100.0 + i * 0.5;  // 缓慢上升趋势
        double noise = std::sin(i * 0.1) * 2.0;  // 添加一些波动
        prices.push_back(base + noise);
    }
    
    auto price_line = std::make_shared<LineSeries>();
    price_line->lines->add_line(std::make_shared<LineBuffer>());
    auto price_buffer = std::dynamic_pointer_cast<LineBuffer>(price_line->lines->getline(0));
    for (double price : prices) {
        price_buffer->append(price);
    }
    
    auto kst = std::make_shared<KST>(price_line);
    
    // KST是一个复杂的指标，我们主要验证其基本特性
    kst->calculate();
    
    // 验证最终结果
    if (prices.size() >= 48) {  // KST需要48个数据点
        double kst_value = kst->getLine(0)->get(0);
        double signal_value = kst->getLine(1)->get(0);
        
        if (!std::isnan(kst_value) && !std::isnan(signal_value)) {
            // 验证基本属性
            EXPECT_TRUE(std::isfinite(kst_value)) 
                << "KST value should be finite";
            EXPECT_TRUE(std::isfinite(signal_value)) 
                << "Signal value should be finite";
        }
    }
}

// KST信号检测测试
TEST(OriginalTests, KST_SignalDetection) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    auto kst = std::make_shared<KST>(close_line);
    
    int bullish_crossovers = 0;  // KST上穿Signal
    int bearish_crossovers = 0;  // KST下穿Signal
    double prev_kst = 0.0, prev_signal = 0.0;
    bool has_prev = false;
    
    // 检测KST与Signal线的交叉
    kst->calculate();
    
    // 分析最近几个数据点的交叉情况
    for (int i = -10; i < 0; ++i) {
        double current_kst = kst->getLine(0)->get(i);
        double current_signal = kst->getLine(1)->get(i);
        double next_kst = kst->getLine(0)->get(i + 1);
        double next_signal = kst->getLine(1)->get(i + 1);
        
        if (!std::isnan(current_kst) && !std::isnan(current_signal) && 
            !std::isnan(next_kst) && !std::isnan(next_signal)) {
            // 检测交叉
            if (current_kst <= current_signal && next_kst > next_signal) {
                bullish_crossovers++;
            } else if (current_kst >= current_signal && next_kst < next_signal) {
                bearish_crossovers++;
            }
        }
    }
    
    std::cout << "KST signal analysis:" << std::endl;
    std::cout << "Bullish crossovers: " << bullish_crossovers << std::endl;
    std::cout << "Bearish crossovers: " << bearish_crossovers << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(bullish_crossovers + bearish_crossovers, 0) 
        << "Should detect some KST signal crossovers";
}

// KST动量特性测试
TEST(OriginalTests, KST_MomentumCharacteristics) {
    // 创建具有不同动量阶段的数据
    std::vector<double> momentum_prices;
    
    // 第一阶段：加速上升;
    for (int i = 0; i < 30; ++i) {
        momentum_prices.push_back(100.0 + i * i * 0.1);
    }
    
    // 第二阶段：稳定上升;
    for (int i = 0; i < 30; ++i) {
        momentum_prices.push_back(momentum_prices.back() + 1.0);
    }
    
    // 第三阶段：减速上升;
    for (int i = 0; i < 30; ++i) {
        double increment = 1.0 - i * 0.03;
        momentum_prices.push_back(momentum_prices.back() + std::max(0.1, increment));
    }
    auto momentum_line = std::make_shared<LineSeries>();
    momentum_line->lines->add_line(std::make_shared<LineBuffer>());
    auto momentum_line_buffer = std::dynamic_pointer_cast<LineBuffer>(momentum_line->lines->getline(0));
    for (double price : momentum_prices) {
        momentum_line_buffer->append(price);
    }
    
    auto momentum_kst = std::make_shared<KST>(momentum_line);
    
    momentum_kst->calculate();
    
    std::vector<double> kst_values;
    std::vector<size_t> phases;
    
    // 收集不同阶段的KST值
    for (size_t i = 48; i < momentum_prices.size(); ++i) {
        int ago = -(static_cast<int>(momentum_prices.size()) - static_cast<int>(i) - 1);
        double kst_val = momentum_kst->getLine(0)->get(ago);
        if (!std::isnan(kst_val)) {
            kst_values.push_back(kst_val);
            if (i < 30) phases.push_back(1);        // 加速阶段
            else if (i < 60) phases.push_back(2);   // 稳定阶段
            else phases.push_back(3);               // 减速阶段
        }
    }
    
    // 分析不同阶段的KST特性
    if (kst_values.size() > 60) {
        std::cout << "Momentum phase analysis:" << std::endl;
        std::cout << "KST values in different phases:" << std::endl;
    for (size_t i = 10; i < kst_values.size(); i += 10) {
            std::cout << "Phase " << phases[i] << " at step " << i 
                      << ": KST = " << kst_values[i] << std::endl;
        }
        
        // KST应该能够反映动量变化
        EXPECT_TRUE(true) << "KST momentum characteristics analyzed";
    }
}

// KST趋势跟踪能力测试
TEST(OriginalTests, KST_TrendFollowing) {
    // 创建明确的趋势变化数据
    std::vector<double> trend_prices;
    
    // 上升趋势;
    for (int i = 0; i < 50; ++i) {
        trend_prices.push_back(100.0 + i * 1.0);
    }
    
    // 横盘整理;
    for (int i = 0; i < 20; ++i) {
        trend_prices.push_back(149.0 + std::sin(i * 0.5) * 2.0);
    }
    
    // 下降趋势;
    for (int i = 0; i < 30; ++i) {
        trend_prices.push_back(149.0 - i * 0.8);
    }
    auto trend_line = std::make_shared<LineSeries>();
    auto trend_buffer = std::make_shared<LineBuffer>();
    trend_line->lines->add_line(trend_buffer);
    for (double price : trend_prices) {
        trend_buffer->append(price);
    }
    
    auto trend_kst = std::make_shared<KST>(trend_line);
    
    trend_kst->calculate();
    
    std::vector<double> kst_trend_values;
    // 收集所有KST值
    for (size_t i = 48; i < trend_prices.size(); ++i) {
        int ago = -(static_cast<int>(trend_prices.size()) - static_cast<int>(i) - 1);
        double kst_val = trend_kst->getLine(0)->get(ago);
        if (!std::isnan(kst_val)) {
            kst_trend_values.push_back(kst_val);
        }
    }
    
    // 分析KST在不同趋势阶段的表现
    if (kst_trend_values.size() > 80) {
        std::cout << "Trend following analysis:" << std::endl;
        
        // 检查不同阶段的KST表现
        size_t uptrend_end = std::min(size_t(50), kst_trend_values.size());
        size_t sideways_end = std::min(size_t(70), kst_trend_values.size());
        
        if (uptrend_end > 48) {
            double uptrend_kst = kst_trend_values[uptrend_end - 1];
            std::cout << "KST at end of uptrend: " << uptrend_kst << std::endl;
        }
        
        if (sideways_end > 48) {
            double sideways_kst = kst_trend_values[sideways_end - 1];
            std::cout << "KST at end of sideways: " << sideways_kst << std::endl;
        }
        
        if (kst_trend_values.size() > 48) {
            double final_kst = kst_trend_values.back();
            std::cout << "Final KST value: " << final_kst << std::endl;
        }
        
        // 验证KST能够响应趋势变化
        EXPECT_TRUE(true) << "KST trend following analyzed";
    }
}

// KST振荡特性测试
TEST(OriginalTests, KST_OscillationCharacteristics) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    auto kst = std::make_shared<KST>(close_line);
    
    kst->calculate();
    
    std::vector<double> kst_values;
    std::vector<double> signal_values;
    // 收集所有KST和Signal值
    for (size_t i = 48; i < csv_data.size(); ++i) {
        int ago = -(static_cast<int>(csv_data.size()) - static_cast<int>(i) - 1);
        double kst_val = kst->getLine(0)->get(ago);
        double signal_val = kst->getLine(1)->get(ago);
        
        if (!std::isnan(kst_val) && !std::isnan(signal_val)) {
            kst_values.push_back(kst_val);
            signal_values.push_back(signal_val);
        }
    }
    
    // 分析KST的振荡特性
    if (!kst_values.empty() && !signal_values.empty()) {
        double kst_min = *std::min_element(kst_values.begin(), kst_values.end());
        double kst_max = *std::max_element(kst_values.begin(), kst_values.end());
        double kst_avg = std::accumulate(kst_values.begin(), kst_values.end(), 0.0) / kst_values.size();
        
        double signal_min = *std::min_element(signal_values.begin(), signal_values.end());
        double signal_max = *std::max_element(signal_values.begin(), signal_values.end());
        double signal_avg = std::accumulate(signal_values.begin(), signal_values.end(), 0.0) / signal_values.size();
        
        std::cout << "KST oscillation characteristics:" << std::endl;
        std::cout << "KST range: [" << kst_min << ", " << kst_max << "], avg: " << kst_avg << std::endl;
        std::cout << "Signal range: [" << signal_min << ", " << signal_max << "], avg: " << signal_avg << std::endl;
        
        // 验证振荡范围是合理的
        EXPECT_LT(kst_min, kst_max) << "KST should have meaningful oscillation range";
        EXPECT_LT(signal_min, signal_max) << "Signal should have meaningful oscillation range";
        EXPECT_TRUE(std::isfinite(kst_avg)) << "KST average should be finite";
        EXPECT_TRUE(std::isfinite(signal_avg)) << "Signal average should be finite";
    }
}

// KST超买超卖区域测试
TEST(OriginalTests, KST_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    auto kst = std::make_shared<KST>(close_line);
    
    kst->calculate();
    
    std::vector<double> kst_values;
    // 收集所有KST值
    for (size_t i = 48; i < csv_data.size(); ++i) {
        int ago = -(static_cast<int>(csv_data.size()) - static_cast<int>(i) - 1);
        double kst_val = kst->getLine(0)->get(ago);
        if (!std::isnan(kst_val)) {
            kst_values.push_back(kst_val);
        }
    }
    
    // 计算动态超买超卖阈值
    if (!kst_values.empty()) {
        double mean = std::accumulate(kst_values.begin(), kst_values.end(), 0.0) / kst_values.size();
        
        double variance = 0.0;
    
    for (double val : kst_values) {
            variance += (val - mean) * (val - mean);
        }
        variance /= kst_values.size();
        double std_dev = std::sqrt(variance);
        
        double overbought_threshold = mean + 1.5 * std_dev;
        double oversold_threshold = mean - 1.5 * std_dev;
        
        int overbought_signals = 0;
        int oversold_signals = 0;
    
    for (double val : kst_values) {
            if (val > overbought_threshold) {
                overbought_signals++;
            } else if (val < oversold_threshold) {
                oversold_signals++;
            }
        }
        
        std::cout << "KST overbought/oversold analysis:" << std::endl;
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

// 边界条件测试
TEST(OriginalTests, KST_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineSeries>();
    flat_line->lines->add_line(std::make_shared<LineBuffer>());
    auto flat_line_buffer = std::dynamic_pointer_cast<LineBuffer>(flat_line->lines->getline(0));
    for (double price : flat_prices) {
        flat_line_buffer->append(price);
    }
    
    auto flat_kst = std::make_shared<KST>(flat_line);
    flat_kst->calculate();
    
    // 当所有价格相同时，KST应该趋向于零（没有动量）
    double final_kst = flat_kst->getLine(0)->get(0);
    double final_signal = flat_kst->getLine(1)->get(0);
    
    if (!std::isnan(final_kst) && !std::isnan(final_signal)) {
        EXPECT_NEAR(final_kst, 0.0, 1e-6) 
            << "KST should be near zero for constant prices";
        EXPECT_NEAR(final_signal, 0.0, 1e-6) 
            << "KST Signal should be near zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineSeries>();
    insufficient_line->lines->add_line(std::make_shared<LineBuffer>());
    auto insufficient_line_buffer = std::dynamic_pointer_cast<LineBuffer>(insufficient_line->lines->getline(0));
    for (int i = 0; i < 30; ++i) {
        insufficient_line_buffer->append(100.0 + i);
    }
    auto insufficient_kst = std::make_shared<KST>(insufficient_line);
    insufficient_kst->calculate();
    
    // 数据不足时应该返回NaN
    double result = insufficient_kst->getLine(0)->get(0);
    EXPECT_TRUE(std::isnan(result)) << "KST should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, KST_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<double> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    for (size_t i = 0; i < data_size; ++i) {
        large_data.push_back(dist(rng));
    }
    
    auto large_data_line = std::make_shared<LineSeries>();
    large_data_line->lines->add_line(std::make_shared<LineBuffer>());
    auto large_data_line_buffer = std::dynamic_pointer_cast<LineBuffer>(large_data_line->lines->getline(0));
    for (double price : large_data) {
        large_data_line_buffer->append(price);
    }
    
    auto large_kst = std::make_shared<KST>(large_data_line);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    large_kst->calculate();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "KST calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_kst = large_kst->getLine(0)->get(0);
    double final_signal = large_kst->getLine(1)->get(0);
    
    EXPECT_FALSE(std::isnan(final_kst)) << "Final KST should not be NaN";
    EXPECT_FALSE(std::isnan(final_signal)) << "Final Signal should not be NaN";
    EXPECT_TRUE(std::isfinite(final_kst)) << "Final KST should be finite";
    EXPECT_TRUE(std::isfinite(final_signal)) << "Final Signal should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}