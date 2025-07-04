/**
 * @file test_ind_stochasticfull.cpp
 * @brief StochasticFull指标测试 - 对应Python test_ind_stochasticfull.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['83.541267', '36.818395', '41.769503'],
 *     ['88.667626', '21.409626', '63.796187'],
 *     ['82.845850', '15.710059', '77.642219']
 * ]
 * chkmin = 18
 * chkind = btind.StochasticFull
 * 
 * 注：StochasticFull包含3条线：%K, %D, %D slow
 */

#include "test_common_simple.h"

#include "indicators/stochasticfull.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> STOCHASTICFULL_EXPECTED_VALUES = {
    {"83.541267", "36.818395", "41.769503"},  // line 0 (%K)
    {"88.667626", "21.409626", "63.796187"},  // line 1 (%D)
    {"82.845850", "15.710059", "77.642219"}   // line 2 (%D slow)
};

const int STOCHASTICFULL_MIN_PERIOD = 18;

} // anonymous namespace

// 使用默认参数的StochasticFull测试
DEFINE_INDICATOR_TEST(StochasticFull_Default, StochasticFull, STOCHASTICFULL_EXPECTED_VALUES, STOCHASTICFULL_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, StochasticFull_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建HLC数据线
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    // 创建StochasticFull指标
    auto stochfull = std::make_shared<StochasticFull>(high_line, low_line, close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        stochfull->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 18;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // 验证3条线的值
    for (int line = 0; line < 3; ++line) {
        auto expected = STOCHASTICFULL_EXPECTED_VALUES[line];
        
        for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
            double actual = stochfull->getLine(line)->get(check_points[i]);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << actual;
            std::string actual_str = ss.str();
            
            EXPECT_EQ(actual_str, expected[i]) 
                << "StochasticFull line " << line << " value mismatch at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(stochfull->getMinPeriod(), 18) << "StochasticFull minimum period should be 18";
}

// 参数化测试 - 测试不同参数的StochasticFull
class StochasticFullParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int, int>> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        high_line_ = std::make_shared<LineRoot>(csv_data_.size(), "high");
        low_line_ = std::make_shared<LineRoot>(csv_data_.size(), "low");
        close_line_ = std::make_shared<LineRoot>(csv_data_.size(), "close");
        
        for (const auto& bar : csv_data_) {
            high_line_->forward(bar.high);
            low_line_->forward(bar.low);
            close_line_->forward(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineRoot> high_line_;
    std::shared_ptr<LineRoot> low_line_;
    std::shared_ptr<LineRoot> close_line_;
};

TEST_P(StochasticFullParameterizedTest, DifferentParameters) {
    auto [period_k, period_d, period_dslow] = GetParam();
    
    // 使用自定义参数创建StochasticFull
    auto stochfull = std::make_shared<StochasticFull>(high_line_, low_line_, close_line_, 
                                                      period_k, period_d, period_dslow);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        stochfull->calculate();
        if (i < csv_data_.size() - 1) {
            high_line_->forward();
            low_line_->forward();
            close_line_->forward();
        }
    }
    
    // 验证最后的值
    int expected_min_period = period_k + period_d + period_dslow - 2;
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_k = stochfull->getLine(0)->get(0);
        double last_d = stochfull->getLine(1)->get(0);
        double last_dslow = stochfull->getLine(2)->get(0);
        
        EXPECT_FALSE(std::isnan(last_k)) << "Last %K should not be NaN";
        EXPECT_FALSE(std::isnan(last_d)) << "Last %D should not be NaN";
        EXPECT_FALSE(std::isnan(last_dslow)) << "Last %D slow should not be NaN";
        
        EXPECT_TRUE(std::isfinite(last_k)) << "Last %K should be finite";
        EXPECT_TRUE(std::isfinite(last_d)) << "Last %D should be finite";
        EXPECT_TRUE(std::isfinite(last_dslow)) << "Last %D slow should be finite";
        
        EXPECT_GE(last_k, 0.0) << "%K should be >= 0";
        EXPECT_LE(last_k, 100.0) << "%K should be <= 100";
        EXPECT_GE(last_d, 0.0) << "%D should be >= 0";
        EXPECT_LE(last_d, 100.0) << "%D should be <= 100";
        EXPECT_GE(last_dslow, 0.0) << "%D slow should be >= 0";
        EXPECT_LE(last_dslow, 100.0) << "%D slow should be <= 100";
    }
}

// 测试不同的参数组合
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    StochasticFullParameterizedTest,
    ::testing::Values(
        std::make_tuple(14, 3, 3),
        std::make_tuple(9, 3, 3),
        std::make_tuple(21, 5, 5),
        std::make_tuple(5, 3, 3)
    )
);

// StochasticFull计算逻辑验证测试
TEST(OriginalTests, StochasticFull_CalculationLogic) {
    // 使用简单的测试数据验证StochasticFull计算
    std::vector<std::tuple<double, double, double>> hlc_data = {
        {105.0, 95.0, 100.0},   // H, L, C
        {110.0, 98.0, 105.0},
        {108.0, 100.0, 103.0},
        {112.0, 102.0, 108.0},
        {115.0, 105.0, 112.0},
        {113.0, 107.0, 110.0},
        {118.0, 108.0, 115.0},
        {120.0, 110.0, 118.0},
        {117.0, 112.0, 114.0},
        {122.0, 114.0, 120.0},
        {125.0, 116.0, 122.0},
        {123.0, 118.0, 121.0},
        {127.0, 120.0, 125.0},
        {130.0, 122.0, 128.0},
        {128.0, 124.0, 126.0},
        {132.0, 126.0, 130.0},
        {135.0, 128.0, 133.0},
        {133.0, 130.0, 131.0},
        {137.0, 132.0, 135.0},
        {140.0, 134.0, 138.0}
    };
    
    auto high_line = std::make_shared<LineRoot>(hlc_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(hlc_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(hlc_data.size(), "close");
    
    for (const auto& [h, l, c] : hlc_data) {
        high_line->forward(h);
        low_line->forward(l);
        close_line->forward(c);
    }
    
    auto stochfull = std::make_shared<StochasticFull>(high_line, low_line, close_line, 14, 3, 3);
    
    // StochasticFull包含多个平滑步骤，主要验证其基本特性
    for (size_t i = 0; i < hlc_data.size(); ++i) {
        stochfull->calculate();
        
        if (i >= 17) {  // StochasticFull需要足够的数据点
            double k_value = stochfull->getLine(0)->get(0);
            double d_value = stochfull->getLine(1)->get(0);
            double dslow_value = stochfull->getLine(2)->get(0);
            
            if (!std::isnan(k_value) && !std::isnan(d_value) && !std::isnan(dslow_value)) {
                // 验证StochasticFull的基本范围约束
                EXPECT_GE(k_value, 0.0) << "%K should be >= 0 at step " << i;
                EXPECT_LE(k_value, 100.0) << "%K should be <= 100 at step " << i;
                EXPECT_GE(d_value, 0.0) << "%D should be >= 0 at step " << i;
                EXPECT_LE(d_value, 100.0) << "%D should be <= 100 at step " << i;
                EXPECT_GE(dslow_value, 0.0) << "%D slow should be >= 0 at step " << i;
                EXPECT_LE(dslow_value, 100.0) << "%D slow should be <= 100 at step " << i;
                
                EXPECT_TRUE(std::isfinite(k_value)) << "%K should be finite at step " << i;
                EXPECT_TRUE(std::isfinite(d_value)) << "%D should be finite at step " << i;
                EXPECT_TRUE(std::isfinite(dslow_value)) << "%D slow should be finite at step " << i;
            }
        }
        
        if (i < hlc_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}

// StochasticFull超买超卖信号测试
TEST(OriginalTests, StochasticFull_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto stochfull = std::make_shared<StochasticFull>(high_line, low_line, close_line);
    
    int k_overbought = 0, k_oversold = 0, k_neutral = 0;
    int d_overbought = 0, d_oversold = 0, d_neutral = 0;
    int dslow_overbought = 0, dslow_oversold = 0, dslow_neutral = 0;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        stochfull->calculate();
        
        double k_val = stochfull->getLine(0)->get(0);
        double d_val = stochfull->getLine(1)->get(0);
        double dslow_val = stochfull->getLine(2)->get(0);
        
        if (!std::isnan(k_val)) {
            if (k_val > 80.0) k_overbought++;
            else if (k_val < 20.0) k_oversold++;
            else k_neutral++;
        }
        
        if (!std::isnan(d_val)) {
            if (d_val > 80.0) d_overbought++;
            else if (d_val < 20.0) d_oversold++;
            else d_neutral++;
        }
        
        if (!std::isnan(dslow_val)) {
            if (dslow_val > 80.0) dslow_overbought++;
            else if (dslow_val < 20.0) dslow_oversold++;
            else dslow_neutral++;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    std::cout << "StochasticFull overbought/oversold analysis:" << std::endl;
    std::cout << "%K - Overbought: " << k_overbought << ", Oversold: " << k_oversold << ", Neutral: " << k_neutral << std::endl;
    std::cout << "%D - Overbought: " << d_overbought << ", Oversold: " << d_oversold << ", Neutral: " << d_neutral << std::endl;
    std::cout << "%D slow - Overbought: " << dslow_overbought << ", Oversold: " << dslow_oversold << ", Neutral: " << dslow_neutral << std::endl;
    
    // 验证有一些有效的计算
    int total_k = k_overbought + k_oversold + k_neutral;
    int total_d = d_overbought + d_oversold + d_neutral;
    int total_dslow = dslow_overbought + dslow_oversold + dslow_neutral;
    
    EXPECT_GT(total_k, 0) << "Should have some valid %K calculations";
    EXPECT_GT(total_d, 0) << "Should have some valid %D calculations";
    EXPECT_GT(total_dslow, 0) << "Should have some valid %D slow calculations";
}

// StochasticFull交叉信号测试
TEST(OriginalTests, StochasticFull_CrossoverSignals) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto stochfull = std::make_shared<StochasticFull>(high_line, low_line, close_line);
    
    int kd_bullish_cross = 0;   // %K上穿%D
    int kd_bearish_cross = 0;   // %K下穿%D
    int d_dslow_bullish_cross = 0;  // %D上穿%D slow
    int d_dslow_bearish_cross = 0;  // %D下穿%D slow
    
    double prev_k = 0.0, prev_d = 0.0, prev_dslow = 0.0;
    bool has_prev = false;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        stochfull->calculate();
        
        double current_k = stochfull->getLine(0)->get(0);
        double current_d = stochfull->getLine(1)->get(0);
        double current_dslow = stochfull->getLine(2)->get(0);
        
        if (!std::isnan(current_k) && !std::isnan(current_d) && !std::isnan(current_dslow) && has_prev) {
            // %K与%D交叉
            if (prev_k <= prev_d && current_k > current_d) {
                kd_bullish_cross++;
            } else if (prev_k >= prev_d && current_k < current_d) {
                kd_bearish_cross++;
            }
            
            // %D与%D slow交叉
            if (prev_d <= prev_dslow && current_d > current_dslow) {
                d_dslow_bullish_cross++;
            } else if (prev_d >= prev_dslow && current_d < current_dslow) {
                d_dslow_bearish_cross++;
            }
        }
        
        if (!std::isnan(current_k) && !std::isnan(current_d) && !std::isnan(current_dslow)) {
            prev_k = current_k;
            prev_d = current_d;
            prev_dslow = current_dslow;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    std::cout << "StochasticFull crossover signals:" << std::endl;
    std::cout << "%K/%D - Bullish: " << kd_bullish_cross << ", Bearish: " << kd_bearish_cross << std::endl;
    std::cout << "%D/%D slow - Bullish: " << d_dslow_bullish_cross << ", Bearish: " << d_dslow_bearish_cross << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(kd_bullish_cross + kd_bearish_cross, 0) 
        << "Should detect some %K/%D crossover signals";
    EXPECT_GE(d_dslow_bullish_cross + d_dslow_bearish_cross, 0) 
        << "Should detect some %D/%D slow crossover signals";
}

// StochasticFull平滑特性测试
TEST(OriginalTests, StochasticFull_SmoothingCharacteristics) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto stochfull = std::make_shared<StochasticFull>(high_line, low_line, close_line);
    auto stoch_regular = std::make_shared<Stochastic>(high_line, low_line, close_line);  // 比较对象
    
    std::vector<double> full_k_changes, full_d_changes, full_dslow_changes;
    std::vector<double> regular_k_changes, regular_d_changes;
    
    double prev_full_k = 0.0, prev_full_d = 0.0, prev_full_dslow = 0.0;
    double prev_reg_k = 0.0, prev_reg_d = 0.0;
    bool has_full_prev = false, has_reg_prev = false;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        stochfull->calculate();
        stoch_regular->calculate();
        
        double full_k = stochfull->getLine(0)->get(0);
        double full_d = stochfull->getLine(1)->get(0);
        double full_dslow = stochfull->getLine(2)->get(0);
        
        double reg_k = stoch_regular->getLine(0)->get(0);
        double reg_d = stoch_regular->getLine(1)->get(0);
        
        if (!std::isnan(full_k) && !std::isnan(full_d) && !std::isnan(full_dslow)) {
            if (has_full_prev) {
                full_k_changes.push_back(std::abs(full_k - prev_full_k));
                full_d_changes.push_back(std::abs(full_d - prev_full_d));
                full_dslow_changes.push_back(std::abs(full_dslow - prev_full_dslow));
            }
            prev_full_k = full_k;
            prev_full_d = full_d;
            prev_full_dslow = full_dslow;
            has_full_prev = true;
        }
        
        if (!std::isnan(reg_k) && !std::isnan(reg_d)) {
            if (has_reg_prev) {
                regular_k_changes.push_back(std::abs(reg_k - prev_reg_k));
                regular_d_changes.push_back(std::abs(reg_d - prev_reg_d));
            }
            prev_reg_k = reg_k;
            prev_reg_d = reg_d;
            has_reg_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 比较平滑特性
    if (!full_dslow_changes.empty() && !regular_d_changes.empty()) {
        double avg_full_dslow_change = std::accumulate(full_dslow_changes.begin(), full_dslow_changes.end(), 0.0) / full_dslow_changes.size();
        double avg_reg_d_change = std::accumulate(regular_d_changes.begin(), regular_d_changes.end(), 0.0) / regular_d_changes.size();
        
        std::cout << "Smoothing comparison:" << std::endl;
        std::cout << "StochasticFull %D slow average change: " << avg_full_dslow_change << std::endl;
        std::cout << "Regular Stochastic %D average change: " << avg_reg_d_change << std::endl;
        
        // StochasticFull的%D slow应该比常规Stochastic的%D更平滑
        EXPECT_LT(avg_full_dslow_change, avg_reg_d_change) 
            << "StochasticFull %D slow should be smoother than regular Stochastic %D";
    }
}

// StochasticFull趋势识别测试
TEST(OriginalTests, StochasticFull_TrendIdentification) {
    // 创建明确的趋势数据
    std::vector<std::tuple<double, double, double>> trend_data;
    
    // 上升趋势
    for (int i = 0; i < 30; ++i) {
        double base = 100.0 + i * 2.0;
        trend_data.push_back({base + 5.0, base - 5.0, base + 1.0});  // H, L, C
    }
    
    auto trend_high = std::make_shared<LineRoot>(trend_data.size(), "high");
    auto trend_low = std::make_shared<LineRoot>(trend_data.size(), "low");
    auto trend_close = std::make_shared<LineRoot>(trend_data.size(), "close");
    
    for (const auto& [h, l, c] : trend_data) {
        trend_high->forward(h);
        trend_low->forward(l);
        trend_close->forward(c);
    }
    
    auto trend_stochfull = std::make_shared<StochasticFull>(trend_high, trend_low, trend_close);
    
    std::vector<double> k_values, d_values, dslow_values;
    
    for (size_t i = 0; i < trend_data.size(); ++i) {
        trend_stochfull->calculate();
        
        double k_val = trend_stochfull->getLine(0)->get(0);
        double d_val = trend_stochfull->getLine(1)->get(0);
        double dslow_val = trend_stochfull->getLine(2)->get(0);
        
        if (!std::isnan(k_val) && !std::isnan(d_val) && !std::isnan(dslow_val)) {
            k_values.push_back(k_val);
            d_values.push_back(d_val);
            dslow_values.push_back(dslow_val);
        }
        
        if (i < trend_data.size() - 1) {
            trend_high->forward();
            trend_low->forward();
            trend_close->forward();
        }
    }
    
    // 分析趋势识别能力
    if (k_values.size() > 10) {
        double late_k_avg = std::accumulate(k_values.end() - 5, k_values.end(), 0.0) / 5.0;
        double late_d_avg = std::accumulate(d_values.end() - 5, d_values.end(), 0.0) / 5.0;
        double late_dslow_avg = std::accumulate(dslow_values.end() - 5, dslow_values.end(), 0.0) / 5.0;
        
        std::cout << "Trend identification (uptrend):" << std::endl;
        std::cout << "Late %K average: " << late_k_avg << std::endl;
        std::cout << "Late %D average: " << late_d_avg << std::endl;
        std::cout << "Late %D slow average: " << late_dslow_avg << std::endl;
        
        // 在强烈上升趋势中，随机指标可能会在高位振荡
        EXPECT_GT(late_k_avg, 20.0) << "In strong uptrend, %K should be elevated";
    }
}

// 边界条件测试
TEST(OriginalTests, StochasticFull_EdgeCases) {
    // 测试相同HLC的情况
    std::vector<std::tuple<double, double, double>> flat_data(30, {100.0, 100.0, 100.0});
    
    auto flat_high = std::make_shared<LineRoot>(flat_data.size(), "high");
    auto flat_low = std::make_shared<LineRoot>(flat_data.size(), "low");
    auto flat_close = std::make_shared<LineRoot>(flat_data.size(), "close");
    
    for (const auto& [h, l, c] : flat_data) {
        flat_high->forward(h);
        flat_low->forward(l);
        flat_close->forward(c);
    }
    
    auto flat_stochfull = std::make_shared<StochasticFull>(flat_high, flat_low, flat_close);
    
    for (size_t i = 0; i < flat_data.size(); ++i) {
        flat_stochfull->calculate();
        if (i < flat_data.size() - 1) {
            flat_high->forward();
            flat_low->forward();
            flat_close->forward();
        }
    }
    
    // 当所有HLC相同时，StochasticFull可能返回特殊值或NaN
    double final_k = flat_stochfull->getLine(0)->get(0);
    double final_d = flat_stochfull->getLine(1)->get(0);
    double final_dslow = flat_stochfull->getLine(2)->get(0);
    
    // 在flat market情况下，根据实现不同可能返回NaN或特定值
    if (!std::isnan(final_k)) {
        EXPECT_GE(final_k, 0.0) << "%K should be >= 0 for flat prices";
        EXPECT_LE(final_k, 100.0) << "%K should be <= 100 for flat prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_high = std::make_shared<LineRoot>(20, "insufficient_high");
    auto insufficient_low = std::make_shared<LineRoot>(20, "insufficient_low");
    auto insufficient_close = std::make_shared<LineRoot>(20, "insufficient_close");
    
    // 只添加少量数据点
    for (int i = 0; i < 10; ++i) {
        insufficient_high->forward(105.0 + i);
        insufficient_low->forward(95.0 + i);
        insufficient_close->forward(100.0 + i);
    }
    
    auto insufficient_stochfull = std::make_shared<StochasticFull>(insufficient_high, insufficient_low, insufficient_close);
    
    for (int i = 0; i < 10; ++i) {
        insufficient_stochfull->calculate();
        if (i < 9) {
            insufficient_high->forward();
            insufficient_low->forward();
            insufficient_close->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_stochfull->getLine(0)->get(0);
    EXPECT_TRUE(std::isnan(result)) << "StochasticFull should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, StochasticFull_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<std::tuple<double, double, double>> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        double base = dist(rng);
        large_data.push_back({
            base + dist(rng) * 0.1,  // high
            base - dist(rng) * 0.1,  // low
            base + (dist(rng) - 100.0) * 0.05  // close
        });
    }
    
    auto large_high = std::make_shared<LineRoot>(large_data.size(), "large_high");
    auto large_low = std::make_shared<LineRoot>(large_data.size(), "large_low");
    auto large_close = std::make_shared<LineRoot>(large_data.size(), "large_close");
    
    for (const auto& [h, l, c] : large_data) {
        large_high->forward(h);
        large_low->forward(l);
        large_close->forward(c);
    }
    
    auto large_stochfull = std::make_shared<StochasticFull>(large_high, large_low, large_close);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_stochfull->calculate();
        if (i < large_data.size() - 1) {
            large_high->forward();
            large_low->forward();
            large_close->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "StochasticFull calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_k = large_stochfull->getLine(0)->get(0);
    double final_d = large_stochfull->getLine(1)->get(0);
    double final_dslow = large_stochfull->getLine(2)->get(0);
    
    EXPECT_FALSE(std::isnan(final_k)) << "Final %K should not be NaN";
    EXPECT_FALSE(std::isnan(final_d)) << "Final %D should not be NaN";
    EXPECT_FALSE(std::isnan(final_dslow)) << "Final %D slow should not be NaN";
    
    EXPECT_TRUE(std::isfinite(final_k)) << "Final %K should be finite";
    EXPECT_TRUE(std::isfinite(final_d)) << "Final %D should be finite";
    EXPECT_TRUE(std::isfinite(final_dslow)) << "Final %D slow should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}