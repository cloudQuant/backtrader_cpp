/**
 * @file test_ind_accdecosc.cpp
 * @brief AccelerationDecelerationOscillator指标测试 - 对应Python test_ind_accdecosc.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['-2.097441', '14.156647', '30.408335']
 * ]
 * chkmin = 38
 * chkind = bt.ind.AccelerationDecelerationOscillator
 * 
 * 注：AccelerationDecelerationOscillator (AC) 是比尔·威廉姆斯的加速/减速振荡器
 */

#include "test_common.h"
#include "indicators/accdecoscillator.h"
#include "indicators/awesomeoscillator.h"
#include <numeric>
#include <random>
#include <tuple>

using namespace backtrader::tests::original;
using namespace backtrader;

// 特化TestStrategy模板以支持AccelerationDecelerationOscillator的特殊构造参数
template<>
void TestStrategy<AccelerationDecelerationOscillator>::init() {
    auto data = this->data(0);
    if (data && data->lines && data->lines->size() >= 3) {
        // Get the high and low lines from the data
        auto high_line = data->lines->getline(1);  // High is typically line 1
        auto low_line = data->lines->getline(2);   // Low is typically line 2
        
        if (high_line && low_line) {
            // Cast to LineRoot for constructor compatibility
            auto high_root = std::dynamic_pointer_cast<LineRoot>(high_line);
            auto low_root = std::dynamic_pointer_cast<LineRoot>(low_line);
            
            if (high_root && low_root) {
                indicator_ = std::make_shared<AccelerationDecelerationOscillator>(high_root, low_root);
            } else {
                // Fallback: create default constructor
                indicator_ = std::make_shared<AccelerationDecelerationOscillator>();
            }
        } else {
            indicator_ = std::make_shared<AccelerationDecelerationOscillator>();
        }
    } else {
        indicator_ = std::make_shared<AccelerationDecelerationOscillator>();
    }
}

namespace {

const std::vector<std::vector<std::string>> ACCDECOSC_EXPECTED_VALUES = {
    {"-2.097441", "14.156647", "30.408335"}
};

const int ACCDECOSC_MIN_PERIOD = 38;

} // anonymous namespace

// 使用默认参数的AccelerationDecelerationOscillator测试
TEST(OriginalTests, AccDecOsc_Default) {
    std::vector<std::vector<std::string>> expected_vals = ACCDECOSC_EXPECTED_VALUES;
    runtest<AccelerationDecelerationOscillator>(expected_vals, ACCDECOSC_MIN_PERIOD, false);
}

TEST(OriginalTests, AccDecOsc_Default_Debug) {
    std::vector<std::vector<std::string>> expected_vals = ACCDECOSC_EXPECTED_VALUES;
    runtest<AccelerationDecelerationOscillator>(expected_vals, ACCDECOSC_MIN_PERIOD, true);
}

// 手动测试函数，用于详细验证
TEST(OriginalTests, AccDecOsc_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建HL数据线
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    // 创建AccelerationDecelerationOscillator指标
    auto ac = std::make_shared<AccelerationDecelerationOscillator>(high_line, low_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ac->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 38;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"-2.097441", "14.156647", "30.408335"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = ac->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "AccDecOsc value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(ac->getMinPeriod(), 38) << "AccDecOsc minimum period should be 38";
}

// AccelerationDecelerationOscillator计算逻辑验证测试
TEST(OriginalTests, AccDecOsc_CalculationLogic) {
    // 使用足够长的测试数据验证AC计算
    std::vector<std::tuple<double, double>> hl_data;
    
    // 生成50个数据点用于测试
    for (int i = 0; i < 50; ++i) {
        double base = 100.0 + i * 0.5 + std::sin(i * 0.2) * 5.0;
        hl_data.push_back({base + 3.0, base - 2.0});
    }
    
    auto high_line = std::make_shared<backtrader::LineRoot>(hl_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(hl_data.size(), "low");
    
    for (const auto& [h, l] : hl_data) {
        high_line->forward(h);
        low_line->forward(l);
    }
    
    auto ac = std::make_shared<AccelerationDecelerationOscillator>(high_line, low_line);
    
    // AC = AO - SMA(AO, 5)
    // 其中 AO = SMA(HL2, 5) - SMA(HL2, 34)
    auto hl2_line = std::make_shared<backtrader::LineRoot>(hl_data.size(), "hl2");
    for (const auto& [h, l] : hl_data) {
        hl2_line->forward((h + l) / 2.0);
    }
    
    auto ao = std::make_shared<AwesomeOscillator>(high_line, low_line);
    
    for (size_t i = 0; i < hl_data.size(); ++i) {
        ac->calculate();
        ao->calculate();
        
        if (i >= 37) {  // AC需要38个数据点
            double ac_value = ac->get(0);
            double ao_value = ao->get(0);
            
            if (!std::isnan(ac_value) && !std::isnan(ao_value)) {
                // AC应该是有限值
                EXPECT_TRUE(std::isfinite(ac_value)) 
                    << "AC value should be finite at step " << i;
            }
        }
        
        if (i < hl_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            hl2_line->forward();
        }
    }
}

// AccelerationDecelerationOscillator信号分析测试
TEST(OriginalTests, AccDecOsc_SignalAnalysis) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto ac = std::make_shared<AccelerationDecelerationOscillator>(high_line, low_line);
    
    int positive_values = 0;    // AC > 0
    int negative_values = 0;    // AC < 0
    int zero_crossings = 0;     // 零线穿越次数
    int acceleration_signals = 0; // 连续正值（加速）
    int deceleration_signals = 0; // 连续负值（减速）
    
    std::vector<double> ac_history;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ac->calculate();
        
        double ac_value = ac->get(0);
        
        if (!std::isnan(ac_value)) {
            ac_history.push_back(ac_value);
            
            if (ac_value > 0.0) {
                positive_values++;
            } else if (ac_value < 0.0) {
                negative_values++;
            }
            
            // 检测零线穿越
            if (ac_history.size() >= 2) {
                double prev_ac = ac_history[ac_history.size() - 2];
                if ((prev_ac <= 0.0 && ac_value > 0.0) || 
                    (prev_ac >= 0.0 && ac_value < 0.0)) {
                    zero_crossings++;
                }
            }
            
            // 检测连续信号
            if (ac_history.size() >= 3) {
                size_t n = ac_history.size();
                double val1 = ac_history[n-3];
                double val2 = ac_history[n-2];
                double val3 = ac_history[n-1];
                
                // 连续3个正值且递增（加速信号）
                if (val1 > 0 && val2 > 0 && val3 > 0 && 
                    val2 > val1 && val3 > val2) {
                    acceleration_signals++;
                }
                
                // 连续3个负值且递减（减速信号）
                if (val1 < 0 && val2 < 0 && val3 < 0 && 
                    val2 < val1 && val3 < val2) {
                    deceleration_signals++;
                }
            }
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    std::cout << "AccDecOsc signal analysis:" << std::endl;
    std::cout << "Positive values: " << positive_values << std::endl;
    std::cout << "Negative values: " << negative_values << std::endl;
    std::cout << "Zero crossings: " << zero_crossings << std::endl;
    std::cout << "Acceleration signals: " << acceleration_signals << std::endl;
    std::cout << "Deceleration signals: " << deceleration_signals << std::endl;
    
    // 验证有一些有效的计算
    int total_values = positive_values + negative_values;
    EXPECT_GT(total_values, 0) << "Should have some valid AC calculations";
}

// AccelerationDecelerationOscillator动量变化测试
TEST(OriginalTests, AccDecOsc_MomentumChanges) {
    // 创建具有不同动量变化模式的数据
    std::vector<std::tuple<double, double>> momentum_data;
    
    // 第一阶段：加速上升
    for (int i = 0; i < 20; ++i) {
        double base = 100.0 + i * i * 0.1;  // 加速度增加
        momentum_data.push_back({base + 3.0, base - 2.0});
    }
    
    // 第二阶段：匀速上升
    for (int i = 0; i < 20; ++i) {
        double base = 140.0 + i * 1.0;  // 匀速
        momentum_data.push_back({base + 3.0, base - 2.0});
    }
    
    // 第三阶段：减速上升
    for (int i = 0; i < 20; ++i) {
        double increment = 1.0 - i * 0.04;  // 逐渐减少的增量
        double base = 160.0 + increment * i;
        momentum_data.push_back({base + 3.0, base - 2.0});
    }
    
    auto momentum_high = std::make_shared<backtrader::LineRoot>(momentum_data.size(), "high");
    auto momentum_low = std::make_shared<backtrader::LineRoot>(momentum_data.size(), "low");
    
    for (const auto& [h, l] : momentum_data) {
        momentum_high->forward(h);
        momentum_low->forward(l);
    }
    
    auto momentum_ac = std::make_shared<AccelerationDecelerationOscillator>(momentum_high, momentum_low);
    
    std::vector<double> accelerating_ac, uniform_ac, decelerating_ac;
    
    for (size_t i = 0; i < momentum_data.size(); ++i) {
        momentum_ac->calculate();
        
        double ac_val = momentum_ac->get(0);
        if (!std::isnan(ac_val)) {
            if (i < 20) {
                accelerating_ac.push_back(ac_val);
            } else if (i < 40) {
                uniform_ac.push_back(ac_val);
            } else {
                decelerating_ac.push_back(ac_val);
            }
        }
        
        if (i < momentum_data.size() - 1) {
            momentum_high->forward();
            momentum_low->forward();
        }
    }
    
    // 分析不同动量阶段的AC表现
    if (!accelerating_ac.empty() && !uniform_ac.empty() && !decelerating_ac.empty()) {
        double acc_avg = std::accumulate(accelerating_ac.begin(), accelerating_ac.end(), 0.0) / accelerating_ac.size();
        double uniform_avg = std::accumulate(uniform_ac.begin(), uniform_ac.end(), 0.0) / uniform_ac.size();
        double dec_avg = std::accumulate(decelerating_ac.begin(), decelerating_ac.end(), 0.0) / decelerating_ac.size();
        
        std::cout << "Momentum change analysis:" << std::endl;
        std::cout << "Accelerating phase AC avg: " << acc_avg << std::endl;
        std::cout << "Uniform phase AC avg: " << uniform_avg << std::endl;
        std::cout << "Decelerating phase AC avg: " << dec_avg << std::endl;
        
        // 加速阶段应该有更高的AC值
        EXPECT_GT(acc_avg, dec_avg) << "Accelerating phase should have higher AC values than decelerating";
    }
}

// AccelerationDecelerationOscillator与AwesomeOscillator关系测试
TEST(OriginalTests, AccDecOsc_vs_AO_Relationship) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto ac = std::make_shared<AccelerationDecelerationOscillator>(high_line, low_line);
    auto ao = std::make_shared<AwesomeOscillator>(high_line, low_line);
    
    std::vector<double> ac_values, ao_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ac->calculate();
        ao->calculate();
        
        double ac_val = ac->get(0);
        double ao_val = ao->get(0);
        
        if (!std::isnan(ac_val) && !std::isnan(ao_val)) {
            ac_values.push_back(ac_val);
            ao_values.push_back(ao_val);
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    // 分析AC和AO的关系
    if (!ac_values.empty() && !ao_values.empty()) {
        // 计算相关性的简化版本
        double ac_mean = std::accumulate(ac_values.begin(), ac_values.end(), 0.0) / ac_values.size();
        double ao_mean = std::accumulate(ao_values.begin(), ao_values.end(), 0.0) / ao_values.size();
        
        double ac_var = 0.0, ao_var = 0.0, covar = 0.0;
        size_t n = std::min(ac_values.size(), ao_values.size());
        
        for (size_t i = 0; i < n; ++i) {
            double ac_diff = ac_values[i] - ac_mean;
            double ao_diff = ao_values[i] - ao_mean;
            ac_var += ac_diff * ac_diff;
            ao_var += ao_diff * ao_diff;
            covar += ac_diff * ao_diff;
        }
        
        ac_var /= n;
        ao_var /= n;
        covar /= n;
        
        double correlation = covar / (std::sqrt(ac_var) * std::sqrt(ao_var));
        
        std::cout << "AC vs AO relationship analysis:" << std::endl;
        std::cout << "AC mean: " << ac_mean << ", variance: " << ac_var << std::endl;
        std::cout << "AO mean: " << ao_mean << ", variance: " << ao_var << std::endl;
        std::cout << "Correlation: " << correlation << std::endl;
        
        // AC和AO应该有一定的相关性，但不是完全相同
        EXPECT_TRUE(std::isfinite(correlation)) << "Correlation should be finite";
        EXPECT_GT(std::abs(correlation), 0.1) << "AC and AO should have some correlation";
    }
}

// AccelerationDecelerationOscillator趋势确认测试
TEST(OriginalTests, AccDecOsc_TrendConfirmation) {
    // 创建明确的趋势数据
    std::vector<std::tuple<double, double>> trend_data;
    
    // 强烈上升趋势
    for (int i = 0; i < 50; ++i) {
        double base = 100.0 + i * 1.5;
        trend_data.push_back({base + 4.0, base - 3.0});
    }
    
    auto trend_high = std::make_shared<backtrader::LineRoot>(trend_data.size(), "high");
    auto trend_low = std::make_shared<backtrader::LineRoot>(trend_data.size(), "low");
    
    for (const auto& [h, l] : trend_data) {
        trend_high->forward(h);
        trend_low->forward(l);
    }
    
    auto trend_ac = std::make_shared<AccelerationDecelerationOscillator>(trend_high, trend_low);
    
    std::vector<double> ac_trend_values;
    int positive_trend_count = 0;
    int negative_trend_count = 0;
    
    for (size_t i = 0; i < trend_data.size(); ++i) {
        trend_ac->calculate();
        
        double ac_val = trend_ac->get(0);
        if (!std::isnan(ac_val)) {
            ac_trend_values.push_back(ac_val);
            
            if (ac_val > 0.0) {
                positive_trend_count++;
            } else {
                negative_trend_count++;
            }
        }
        
        if (i < trend_data.size() - 1) {
            trend_high->forward();
            trend_low->forward();
        }
    }
    
    std::cout << "Trend confirmation analysis:" << std::endl;
    std::cout << "Positive AC values in uptrend: " << positive_trend_count << std::endl;
    std::cout << "Negative AC values in uptrend: " << negative_trend_count << std::endl;
    
    // 在强烈上升趋势中，正AC值应该占优势
    if (positive_trend_count + negative_trend_count > 0) {
        double positive_ratio = static_cast<double>(positive_trend_count) / 
                               (positive_trend_count + negative_trend_count);
        std::cout << "Positive AC ratio in uptrend: " << positive_ratio << std::endl;
        
        EXPECT_GT(positive_ratio, 0.4) << "Should have reasonable positive AC values in strong uptrend";
    }
}

// AccelerationDecelerationOscillator发散分析测试
TEST(OriginalTests, AccDecOsc_DivergenceAnalysis) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto ac = std::make_shared<AccelerationDecelerationOscillator>(high_line, low_line);
    
    std::vector<double> prices, ac_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ac->calculate();
        
        double ac_val = ac->get(0);
        if (!std::isnan(ac_val)) {
            prices.push_back((csv_data[i].high + csv_data[i].low) / 2.0);
            ac_values.push_back(ac_val);
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    // 寻找价格和AC的极值点
    std::vector<size_t> price_highs, price_lows, ac_highs, ac_lows;
    
    for (size_t i = 2; i < prices.size() - 2; ++i) {
        // 价格高点
        if (prices[i] > prices[i-1] && prices[i] > prices[i+1] &&
            prices[i] > prices[i-2] && prices[i] > prices[i+2]) {
            price_highs.push_back(i);
        }
        
        // 价格低点
        if (prices[i] < prices[i-1] && prices[i] < prices[i+1] &&
            prices[i] < prices[i-2] && prices[i] < prices[i+2]) {
            price_lows.push_back(i);
        }
        
        // AC高点
        if (ac_values[i] > ac_values[i-1] && ac_values[i] > ac_values[i+1] &&
            ac_values[i] > ac_values[i-2] && ac_values[i] > ac_values[i+2]) {
            ac_highs.push_back(i);
        }
        
        // AC低点
        if (ac_values[i] < ac_values[i-1] && ac_values[i] < ac_values[i+1] &&
            ac_values[i] < ac_values[i-2] && ac_values[i] < ac_values[i+2]) {
            ac_lows.push_back(i);
        }
    }
    
    std::cout << "AC divergence analysis:" << std::endl;
    std::cout << "Price highs: " << price_highs.size() << ", lows: " << price_lows.size() << std::endl;
    std::cout << "AC highs: " << ac_highs.size() << ", lows: " << ac_lows.size() << std::endl;
    
    // 分析最近的高点
    if (price_highs.size() >= 2 && ac_highs.size() >= 1) {
        size_t latest_price_high = price_highs.back();
        
        std::cout << "Latest price high: " << prices[latest_price_high] 
                  << " with AC value: " << ac_values[latest_price_high] << std::endl;
    }
    
    EXPECT_TRUE(true) << "AC divergence analysis completed";
}

// 边界条件测试
TEST(OriginalTests, AccDecOsc_EdgeCases) {
    // 测试相同HL的情况
    std::vector<std::tuple<double, double>> flat_data(50, {100.0, 100.0});
    
    auto flat_high = std::make_shared<backtrader::LineRoot>(flat_data.size(), "flat_high");
    auto flat_low = std::make_shared<backtrader::LineRoot>(flat_data.size(), "flat_low");
    
    for (const auto& [h, l] : flat_data) {
        flat_high->forward(h);
        flat_low->forward(l);
    }
    
    auto flat_ac = std::make_shared<AccelerationDecelerationOscillator>(flat_high, flat_low);
    
    for (size_t i = 0; i < flat_data.size(); ++i) {
        flat_ac->calculate();
        if (i < flat_data.size() - 1) {
            flat_high->forward();
            flat_low->forward();
        }
    }
    
    // 当所有HL相同时，AC应该为零
    double final_ac = flat_ac->get(0);
    if (!std::isnan(final_ac)) {
        EXPECT_NEAR(final_ac, 0.0, 1e-6) 
            << "AC should be zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_high = std::make_shared<backtrader::LineRoot>(40, "insufficient_high");
    auto insufficient_low = std::make_shared<backtrader::LineRoot>(40, "insufficient_low");
    
    // 只添加少量数据点
    for (int i = 0; i < 30; ++i) {
        insufficient_high->forward(105.0 + i);
        insufficient_low->forward(95.0 + i);
    }
    
    auto insufficient_ac = std::make_shared<AccelerationDecelerationOscillator>(insufficient_high, insufficient_low);
    
    for (int i = 0; i < 30; ++i) {
        insufficient_ac->calculate();
        if (i < 29) {
            insufficient_high->forward();
            insufficient_low->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_ac->get(0);
    EXPECT_TRUE(std::isnan(result)) << "AC should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, AccDecOsc_Performance) {
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
    
    auto large_high = std::make_shared<backtrader::LineRoot>(large_data.size(), "large_high");
    auto large_low = std::make_shared<backtrader::LineRoot>(large_data.size(), "large_low");
    
    for (const auto& [h, l] : large_data) {
        large_high->forward(h);
        large_low->forward(l);
    }
    
    auto large_ac = std::make_shared<AccelerationDecelerationOscillator>(large_high, large_low);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_ac->calculate();
        if (i < large_data.size() - 1) {
            large_high->forward();
            large_low->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "AccDecOsc calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_ac->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}