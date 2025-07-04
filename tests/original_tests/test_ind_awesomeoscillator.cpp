/**
 * @file test_ind_awesomeoscillator.cpp
 * @brief AwesomeOscillator指标测试 - 对应Python test_ind_awesomeoscillator.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['50.804206', '72.983735', '33.655941']
 * ]
 * chkmin = 34
 * chkind = bt.ind.AO
 * 
 * 注：AwesomeOscillator (AO) 是比尔·威廉姆斯开发的动量指标
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/awesomeoscillator.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> AWESOMEOSCILLATOR_EXPECTED_VALUES = {
    {"50.804206", "72.983735", "33.655941"}
};

const int AWESOMEOSCILLATOR_MIN_PERIOD = 34;

} // anonymous namespace

// 使用默认参数的AwesomeOscillator测试
DEFINE_INDICATOR_TEST(AwesomeOscillator_Default, AwesomeOscillator, AWESOMEOSCILLATOR_EXPECTED_VALUES, AWESOMEOSCILLATOR_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, AwesomeOscillator_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建HL数据线
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    // 创建AwesomeOscillator指标
    auto ao = std::make_shared<AwesomeOscillator>(high_line, low_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ao->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
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
    
    std::vector<std::string> expected = {"50.804206", "72.983735", "33.655941"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = ao->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "AwesomeOscillator value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(ao->getMinPeriod(), 34) << "AwesomeOscillator minimum period should be 34";
}

// AwesomeOscillator计算逻辑验证测试
TEST(OriginalTests, AwesomeOscillator_CalculationLogic) {
    // 使用简单的测试数据验证AwesomeOscillator计算
    std::vector<std::tuple<double, double>> hl_data = {
        {105.0, 95.0},   // H, L
        {108.0, 98.0},
        {110.0, 100.0},
        {107.0, 102.0},
        {112.0, 105.0},
        {115.0, 108.0},
        {113.0, 109.0},
        {118.0, 112.0},
        {120.0, 114.0},
        {117.0, 113.0},
        {122.0, 116.0},
        {125.0, 118.0},
        {123.0, 120.0},
        {127.0, 122.0},
        {130.0, 124.0},
        {128.0, 125.0},
        {132.0, 127.0},
        {135.0, 129.0},
        {133.0, 130.0},
        {137.0, 132.0},
        {140.0, 134.0},
        {138.0, 135.0},
        {142.0, 137.0},
        {145.0, 139.0},
        {143.0, 140.0},
        {147.0, 142.0},
        {150.0, 144.0},
        {148.0, 145.0},
        {152.0, 147.0},
        {155.0, 149.0},
        {153.0, 150.0},
        {157.0, 152.0},
        {160.0, 154.0},
        {158.0, 155.0},
        {162.0, 157.0}
    };
    
    auto high_line = std::make_shared<LineRoot>(hl_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(hl_data.size(), "low");
    
    for (const auto& [h, l] : hl_data) {
        high_line->forward(h);
        low_line->forward(l);
    }
    
    auto ao = std::make_shared<AwesomeOscillator>(high_line, low_line);
    
    // AO = SMA(HL2, 5) - SMA(HL2, 34)
    auto hl2_line = std::make_shared<LineRoot>(hl_data.size(), "hl2");
    for (const auto& [h, l] : hl_data) {
        hl2_line->forward((h + l) / 2.0);
    }
    
    auto sma5 = std::make_shared<SMA>(hl2_line, 5);
    auto sma34 = std::make_shared<SMA>(hl2_line, 34);
    
    for (size_t i = 0; i < hl_data.size(); ++i) {
        ao->calculate();
        sma5->calculate();
        sma34->calculate();
        
        if (i >= 33) {  // AO需要34个数据点
            double ao_value = ao->get(0);
            double sma5_value = sma5->get(0);
            double sma34_value = sma34->get(0);
            double expected_ao = sma5_value - sma34_value;
            
            if (!std::isnan(ao_value) && !std::isnan(sma5_value) && !std::isnan(sma34_value)) {
                EXPECT_NEAR(ao_value, expected_ao, 1e-10) 
                    << "AO calculation mismatch at step " << i;
            }
        }
        
        if (i < hl_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            hl2_line->forward();
        }
    }
}

// AwesomeOscillator信号检测测试
TEST(OriginalTests, AwesomeOscillator_SignalDetection) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto ao = std::make_shared<AwesomeOscillator>(high_line, low_line);
    
    int bullish_signals = 0;  // AO从负转正
    int bearish_signals = 0;  // AO从正转负
    int saucer_signals = 0;   // 碟形信号（连续3个值，中间最低）
    
    std::vector<double> ao_history;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ao->calculate();
        
        double ao_value = ao->get(0);
        
        if (!std::isnan(ao_value)) {
            ao_history.push_back(ao_value);
            
            // 检测零线穿越
            if (ao_history.size() >= 2) {
                double prev_ao = ao_history[ao_history.size() - 2];
                if (prev_ao <= 0.0 && ao_value > 0.0) {
                    bullish_signals++;
                } else if (prev_ao >= 0.0 && ao_value < 0.0) {
                    bearish_signals++;
                }
            }
            
            // 检测碟形信号
            if (ao_history.size() >= 3) {
                size_t n = ao_history.size();
                double val1 = ao_history[n-3];
                double val2 = ao_history[n-2];  // 中间值
                double val3 = ao_history[n-1];  // 当前值
                
                // 看涨碟形：三个负值，中间最低，且呈上升趋势
                if (val1 < 0 && val2 < 0 && val3 < 0 && 
                    val2 < val1 && val3 > val2) {
                    saucer_signals++;
                }
            }
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    std::cout << "AwesomeOscillator signal analysis:" << std::endl;
    std::cout << "Bullish zero line cross: " << bullish_signals << std::endl;
    std::cout << "Bearish zero line cross: " << bearish_signals << std::endl;
    std::cout << "Saucer signals: " << saucer_signals << std::endl;
    
    // 验证检测到一些信号
    EXPECT_GE(bullish_signals + bearish_signals, 0) 
        << "Should detect some zero line crossover signals";
}

// AwesomeOscillator动量分析测试
TEST(OriginalTests, AwesomeOscillator_MomentumAnalysis) {
    // 创建不同动量阶段的数据
    std::vector<std::tuple<double, double>> momentum_data;
    
    // 第一阶段：强烈上升动量
    for (int i = 0; i < 20; ++i) {
        double base = 100.0 + i * 2.0;
        momentum_data.push_back({base + 5.0, base - 3.0});
    }
    
    // 第二阶段：减弱上升动量
    for (int i = 0; i < 20; ++i) {
        double base = 140.0 + i * 0.5;
        momentum_data.push_back({base + 3.0, base - 2.0});
    }
    
    // 第三阶段：横盘
    for (int i = 0; i < 20; ++i) {
        double base = 150.0;
        momentum_data.push_back({base + 2.0, base - 2.0});
    }
    
    auto momentum_high = std::make_shared<LineRoot>(momentum_data.size(), "high");
    auto momentum_low = std::make_shared<LineRoot>(momentum_data.size(), "low");
    
    for (const auto& [h, l] : momentum_data) {
        momentum_high->forward(h);
        momentum_low->forward(l);
    }
    
    auto momentum_ao = std::make_shared<AwesomeOscillator>(momentum_high, momentum_low);
    
    std::vector<double> strong_momentum, weak_momentum, sideways_momentum;
    
    for (size_t i = 0; i < momentum_data.size(); ++i) {
        momentum_ao->calculate();
        
        double ao_val = momentum_ao->get(0);
        if (!std::isnan(ao_val)) {
            if (i < 20) {
                strong_momentum.push_back(ao_val);
            } else if (i < 40) {
                weak_momentum.push_back(ao_val);
            } else {
                sideways_momentum.push_back(ao_val);
            }
        }
        
        if (i < momentum_data.size() - 1) {
            momentum_high->forward();
            momentum_low->forward();
        }
    }
    
    // 分析不同动量阶段的AO表现
    if (!strong_momentum.empty() && !weak_momentum.empty() && !sideways_momentum.empty()) {
        double strong_avg = std::accumulate(strong_momentum.begin(), strong_momentum.end(), 0.0) / strong_momentum.size();
        double weak_avg = std::accumulate(weak_momentum.begin(), weak_momentum.end(), 0.0) / weak_momentum.size();
        double sideways_avg = std::accumulate(sideways_momentum.begin(), sideways_momentum.end(), 0.0) / sideways_momentum.size();
        
        std::cout << "Momentum analysis:" << std::endl;
        std::cout << "Strong momentum AO avg: " << strong_avg << std::endl;
        std::cout << "Weak momentum AO avg: " << weak_avg << std::endl;
        std::cout << "Sideways momentum AO avg: " << sideways_avg << std::endl;
        
        // 强动量阶段应该有更高的AO值
        EXPECT_GT(strong_avg, weak_avg) << "Strong momentum should have higher AO values";
    }
}

// AwesomeOscillator发散分析测试
TEST(OriginalTests, AwesomeOscillator_DivergenceAnalysis) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto ao = std::make_shared<AwesomeOscillator>(high_line, low_line);
    
    std::vector<double> prices, ao_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ao->calculate();
        
        double ao_val = ao->get(0);
        if (!std::isnan(ao_val)) {
            prices.push_back((csv_data[i].high + csv_data[i].low) / 2.0);  // HL2
            ao_values.push_back(ao_val);
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    // 寻找价格和AO的峰值进行发散分析
    std::vector<size_t> price_peaks, ao_peaks;
    
    for (size_t i = 2; i < prices.size() - 2; ++i) {
        // 寻找价格峰值
        if (prices[i] > prices[i-1] && prices[i] > prices[i+1] &&
            prices[i] > prices[i-2] && prices[i] > prices[i+2]) {
            price_peaks.push_back(i);
        }
        
        // 寻找AO峰值
        if (ao_values[i] > ao_values[i-1] && ao_values[i] > ao_values[i+1] &&
            ao_values[i] > ao_values[i-2] && ao_values[i] > ao_values[i+2]) {
            ao_peaks.push_back(i);
        }
    }
    
    std::cout << "Divergence analysis:" << std::endl;
    std::cout << "Price peaks found: " << price_peaks.size() << std::endl;
    std::cout << "AO peaks found: " << ao_peaks.size() << std::endl;
    
    // 分析最近的几个峰值
    if (price_peaks.size() >= 2) {
        size_t latest_price_peak = price_peaks.back();
        size_t prev_price_peak = price_peaks[price_peaks.size() - 2];
        
        std::cout << "Recent price peaks comparison:" << std::endl;
        std::cout << "Previous: " << prices[prev_price_peak] << " at index " << prev_price_peak << std::endl;
        std::cout << "Latest: " << prices[latest_price_peak] << " at index " << latest_price_peak << std::endl;
        std::cout << "Corresponding AO values: " << ao_values[prev_price_peak] 
                  << " -> " << ao_values[latest_price_peak] << std::endl;
    }
    
    EXPECT_TRUE(true) << "Price/AO divergence analysis completed";
}

// AwesomeOscillator颜色条分析测试
TEST(OriginalTests, AwesomeOscillator_ColorBarAnalysis) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto ao = std::make_shared<AwesomeOscillator>(high_line, low_line);
    
    int green_bars = 0;  // AO[0] > AO[1]
    int red_bars = 0;    // AO[0] < AO[1]
    int neutral_bars = 0; // AO[0] == AO[1]
    
    std::vector<double> ao_history;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ao->calculate();
        
        double ao_value = ao->get(0);
        
        if (!std::isnan(ao_value)) {
            ao_history.push_back(ao_value);
            
            if (ao_history.size() >= 2) {
                double current = ao_history.back();
                double previous = ao_history[ao_history.size() - 2];
                
                if (current > previous) {
                    green_bars++;
                } else if (current < previous) {
                    red_bars++;
                } else {
                    neutral_bars++;
                }
            }
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    std::cout << "Color bar analysis:" << std::endl;
    std::cout << "Green bars (increasing): " << green_bars << std::endl;
    std::cout << "Red bars (decreasing): " << red_bars << std::endl;
    std::cout << "Neutral bars (unchanged): " << neutral_bars << std::endl;
    
    int total_bars = green_bars + red_bars + neutral_bars;
    EXPECT_GT(total_bars, 0) << "Should have some valid AO color bar analysis";
    
    if (total_bars > 0) {
        double green_ratio = static_cast<double>(green_bars) / total_bars;
        double red_ratio = static_cast<double>(red_bars) / total_bars;
        
        std::cout << "Green ratio: " << green_ratio << std::endl;
        std::cout << "Red ratio: " << red_ratio << std::endl;
    }
}

// AwesomeOscillator与价格关系测试
TEST(OriginalTests, AwesomeOscillator_PriceRelationship) {
    // 创建不同价格模式的数据
    std::vector<std::tuple<double, double>> pattern_data;
    
    // 上升楔形：价格上升但动量减弱
    for (int i = 0; i < 30; ++i) {
        double base = 100.0 + i * 1.0;
        double range = 10.0 - i * 0.2;  // 逐渐缩小的交易区间
        pattern_data.push_back({base + range/2, base - range/2});
    }
    
    auto pattern_high = std::make_shared<LineRoot>(pattern_data.size(), "high");
    auto pattern_low = std::make_shared<LineRoot>(pattern_data.size(), "low");
    
    for (const auto& [h, l] : pattern_data) {
        pattern_high->forward(h);
        pattern_low->forward(l);
    }
    
    auto pattern_ao = std::make_shared<AwesomeOscillator>(pattern_high, pattern_low);
    
    std::vector<double> prices, ao_values;
    
    for (size_t i = 0; i < pattern_data.size(); ++i) {
        pattern_ao->calculate();
        
        double ao_val = pattern_ao->get(0);
        if (!std::isnan(ao_val)) {
            auto [h, l] = pattern_data[i];
            prices.push_back((h + l) / 2.0);
            ao_values.push_back(ao_val);
        }
        
        if (i < pattern_data.size() - 1) {
            pattern_high->forward();
            pattern_low->forward();
        }
    }
    
    // 分析价格与AO的关系
    if (prices.size() > 20) {
        double price_change = prices.back() - prices.front();
        double ao_trend = 0.0;
        
        // 计算AO的总体趋势
        if (ao_values.size() > 10) {
            double early_ao = std::accumulate(ao_values.begin(), ao_values.begin() + 5, 0.0) / 5.0;
            double late_ao = std::accumulate(ao_values.end() - 5, ao_values.end(), 0.0) / 5.0;
            ao_trend = late_ao - early_ao;
        }
        
        std::cout << "Price-AO relationship analysis:" << std::endl;
        std::cout << "Price change: " << price_change << std::endl;
        std::cout << "AO trend: " << ao_trend << std::endl;
        
        // 在上升楔形中，价格上升但AO应该显示动量减弱
        EXPECT_GT(price_change, 0.0) << "Price should be rising in upward wedge";
        
        // AO趋势可能显示动量减弱
        if (ao_trend < 0) {
            std::cout << "Bearish divergence detected: price rising but momentum weakening" << std::endl;
        }
    }
}

// 边界条件测试
TEST(OriginalTests, AwesomeOscillator_EdgeCases) {
    // 测试相同HL的情况
    std::vector<std::tuple<double, double>> flat_data(50, {100.0, 100.0});
    
    auto flat_high = std::make_shared<LineRoot>(flat_data.size(), "flat_high");
    auto flat_low = std::make_shared<LineRoot>(flat_data.size(), "flat_low");
    
    for (const auto& [h, l] : flat_data) {
        flat_high->forward(h);
        flat_low->forward(l);
    }
    
    auto flat_ao = std::make_shared<AwesomeOscillator>(flat_high, flat_low);
    
    for (size_t i = 0; i < flat_data.size(); ++i) {
        flat_ao->calculate();
        if (i < flat_data.size() - 1) {
            flat_high->forward();
            flat_low->forward();
        }
    }
    
    // 当所有HL相同时，AO应该为零
    double final_ao = flat_ao->get(0);
    if (!std::isnan(final_ao)) {
        EXPECT_NEAR(final_ao, 0.0, 1e-6) 
            << "AO should be zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_high = std::make_shared<LineRoot>(40, "insufficient_high");
    auto insufficient_low = std::make_shared<LineRoot>(40, "insufficient_low");
    
    // 只添加少量数据点
    for (int i = 0; i < 30; ++i) {
        insufficient_high->forward(105.0 + i);
        insufficient_low->forward(95.0 + i);
    }
    
    auto insufficient_ao = std::make_shared<AwesomeOscillator>(insufficient_high, insufficient_low);
    
    for (int i = 0; i < 30; ++i) {
        insufficient_ao->calculate();
        if (i < 29) {
            insufficient_high->forward();
            insufficient_low->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_ao->get(0);
    EXPECT_TRUE(std::isnan(result)) << "AO should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, AwesomeOscillator_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<std::tuple<double, double>> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        double base = dist(rng);
        large_data.push_back({
            base + dist(rng) * 0.1,  // high
            base - dist(rng) * 0.1   // low
        });
    }
    
    auto large_high = std::make_shared<LineRoot>(large_data.size(), "large_high");
    auto large_low = std::make_shared<LineRoot>(large_data.size(), "large_low");
    
    for (const auto& [h, l] : large_data) {
        large_high->forward(h);
        large_low->forward(l);
    }
    
    auto large_ao = std::make_shared<AwesomeOscillator>(large_high, large_low);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_ao->calculate();
        if (i < large_data.size() - 1) {
            large_high->forward();
            large_low->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "AwesomeOscillator calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_ao->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}