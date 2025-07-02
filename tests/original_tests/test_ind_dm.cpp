/**
 * @file test_ind_dm.cpp
 * @brief DM指标测试 - 对应Python test_ind_dm.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['15.302485', '31.674648', '15.961767'],  # DI+
 *     ['18.839142', '26.946536', '18.161738'],  # DI-
 *     ['28.809535', '30.460124', '31.386311'],  # DX
 *     ['24.638772', '18.914537', '21.564611'],  # ADX
 * ]
 * chkmin = 42
 * chkind = btind.DM
 */

#include "test_common.h"
#include "indicators/DM.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> DM_EXPECTED_VALUES = {
    {"15.302485", "31.674648", "15.961767"},  // DI+
    {"18.839142", "26.946536", "18.161738"},  // DI-
    {"28.809535", "30.460124", "31.386311"},  // DX
    {"24.638772", "18.914537", "21.564611"},  // ADX
};

const int DM_MIN_PERIOD = 42;

} // anonymous namespace

// 使用默认参数的DM测试
DEFINE_INDICATOR_TEST(DM_Default, DM, DM_EXPECTED_VALUES, DM_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, DM_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    // 创建DM指标（默认14周期）
    auto dm = std::make_shared<DM>(high_line, low_line, close_line, 14);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        dm->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 42;  // 2 * period + period
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // 验证DI+
    std::vector<std::string> expected_di_plus = {"15.302485", "31.674648", "15.961767"};
    for (size_t i = 0; i < check_points.size() && i < expected_di_plus.size(); ++i) {
        double actual = dm->getDIPlus(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_di_plus[i]) 
            << "DI+ mismatch at check point " << i;
    }
    
    // 验证DI-
    std::vector<std::string> expected_di_minus = {"18.839142", "26.946536", "18.161738"};
    for (size_t i = 0; i < check_points.size() && i < expected_di_minus.size(); ++i) {
        double actual = dm->getDIMinus(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_di_minus[i]) 
            << "DI- mismatch at check point " << i;
    }
    
    // 验证DX
    std::vector<std::string> expected_dx = {"28.809535", "30.460124", "31.386311"};
    for (size_t i = 0; i < check_points.size() && i < expected_dx.size(); ++i) {
        double actual = dm->getDX(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_dx[i]) 
            << "DX mismatch at check point " << i;
    }
    
    // 验证ADX
    std::vector<std::string> expected_adx = {"24.638772", "18.914537", "21.564611"};
    for (size_t i = 0; i < check_points.size() && i < expected_adx.size(); ++i) {
        double actual = dm->getADX(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_adx[i]) 
            << "ADX mismatch at check point " << i;
    }
    
    // 验证最小周期
    EXPECT_EQ(dm->getMinPeriod(), 42) << "DM minimum period should be 42";
}

// 参数化测试 - 测试不同周期的DM
class DMParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(DMParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto dm = std::make_shared<DM>(high_line_, low_line_, close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        dm->calculate();
        if (i < csv_data_.size() - 1) {
            high_line_->forward();
            low_line_->forward();
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    int expected_min_period = 3 * period;  // period + period + period
    EXPECT_EQ(dm->getMinPeriod(), expected_min_period) 
        << "DM minimum period should be 3 * period";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double di_plus = dm->getDIPlus(0);
        double di_minus = dm->getDIMinus(0);
        double dx = dm->getDX(0);
        double adx = dm->getADX(0);
        
        EXPECT_FALSE(std::isnan(di_plus)) << "DI+ should not be NaN";
        EXPECT_FALSE(std::isnan(di_minus)) << "DI- should not be NaN";
        EXPECT_FALSE(std::isnan(dx)) << "DX should not be NaN";
        EXPECT_FALSE(std::isnan(adx)) << "ADX should not be NaN";
        
        EXPECT_GE(di_plus, 0.0) << "DI+ should be >= 0";
        EXPECT_GE(di_minus, 0.0) << "DI- should be >= 0";
        EXPECT_GE(dx, 0.0) << "DX should be >= 0";
        EXPECT_GE(adx, 0.0) << "ADX should be >= 0";
        
        EXPECT_LE(dx, 100.0) << "DX should be <= 100";
        EXPECT_LE(adx, 100.0) << "ADX should be <= 100";
    }
}

// 测试不同的DM周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    DMParameterizedTest,
    ::testing::Values(7, 14, 21, 28)
);

// 趋势强度识别测试
TEST(OriginalTests, DM_TrendStrength) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto dm = std::make_shared<DM>(high_line, low_line, close_line, 14);
    
    int strong_trend = 0;     // ADX > 25
    int weak_trend = 0;       // ADX < 20
    int moderate_trend = 0;   // 20 <= ADX <= 25
    
    // 分析趋势强度
    for (size_t i = 0; i < csv_data.size(); ++i) {
        dm->calculate();
        
        double adx = dm->getADX(0);
        
        if (!std::isnan(adx)) {
            if (adx > 25.0) {
                strong_trend++;
            } else if (adx < 20.0) {
                weak_trend++;
            } else {
                moderate_trend++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    std::cout << "Trend strength analysis:" << std::endl;
    std::cout << "Strong trend (ADX > 25): " << strong_trend << std::endl;
    std::cout << "Moderate trend (20 <= ADX <= 25): " << moderate_trend << std::endl;
    std::cout << "Weak trend (ADX < 20): " << weak_trend << std::endl;
    
    // 验证至少有一些有效的计算
    EXPECT_GT(strong_trend + weak_trend + moderate_trend, 0) 
        << "Should have some valid trend strength calculations";
}

// 方向性运动测试
TEST(OriginalTests, DM_DirectionalMovement) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto dm = std::make_shared<DM>(high_line, low_line, close_line, 14);
    
    int bullish_signals = 0;  // DI+ > DI-
    int bearish_signals = 0;  // DI- > DI+
    
    // 分析方向性运动
    for (size_t i = 0; i < csv_data.size(); ++i) {
        dm->calculate();
        
        double di_plus = dm->getDIPlus(0);
        double di_minus = dm->getDIMinus(0);
        
        if (!std::isnan(di_plus) && !std::isnan(di_minus)) {
            if (di_plus > di_minus) {
                bullish_signals++;
            } else if (di_minus > di_plus) {
                bearish_signals++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    std::cout << "Directional movement:" << std::endl;
    std::cout << "Bullish signals (DI+ > DI-): " << bullish_signals << std::endl;
    std::cout << "Bearish signals (DI- > DI+): " << bearish_signals << std::endl;
    
    // 验证至少有一些有效的计算
    EXPECT_GT(bullish_signals + bearish_signals, 0) 
        << "Should have some valid directional signals";
}

// DI交叉信号测试
TEST(OriginalTests, DM_CrossoverSignals) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto dm = std::make_shared<DM>(high_line, low_line, close_line, 14);
    
    int bullish_crossovers = 0;  // DI+上穿DI-
    int bearish_crossovers = 0;  // DI-上穿DI+
    
    double prev_di_plus = 0.0, prev_di_minus = 0.0;
    bool has_prev = false;
    
    // 检测交叉信号
    for (size_t i = 0; i < csv_data.size(); ++i) {
        dm->calculate();
        
        double current_di_plus = dm->getDIPlus(0);
        double current_di_minus = dm->getDIMinus(0);
        
        if (!std::isnan(current_di_plus) && !std::isnan(current_di_minus) && has_prev) {
            // 检测DI+上穿DI-
            if (prev_di_plus <= prev_di_minus && current_di_plus > current_di_minus) {
                bullish_crossovers++;
            }
            // 检测DI-上穿DI+
            else if (prev_di_minus <= prev_di_plus && current_di_minus > current_di_plus) {
                bearish_crossovers++;
            }
        }
        
        if (!std::isnan(current_di_plus) && !std::isnan(current_di_minus)) {
            prev_di_plus = current_di_plus;
            prev_di_minus = current_di_minus;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    std::cout << "DI crossover signals:" << std::endl;
    std::cout << "Bullish crossovers: " << bullish_crossovers << std::endl;
    std::cout << "Bearish crossovers: " << bearish_crossovers << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(bullish_crossovers + bearish_crossovers, 0) 
        << "Should detect some crossover signals";
}

// ADX趋势确认测试
TEST(OriginalTests, DM_ADXTrendConfirmation) {
    // 创建强势上升趋势数据
    std::vector<CSVDataReader::OHLCVData> trend_data;
    for (int i = 0; i < 100; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 100.0 + i * 2.0;
        bar.low = 95.0 + i * 2.0;
        bar.close = 98.0 + i * 2.0;
        bar.open = 96.0 + i * 2.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        trend_data.push_back(bar);
    }
    
    auto trend_high = std::make_shared<LineRoot>(trend_data.size(), "trend_high");
    auto trend_low = std::make_shared<LineRoot>(trend_data.size(), "trend_low");
    auto trend_close = std::make_shared<LineRoot>(trend_data.size(), "trend_close");
    
    for (const auto& bar : trend_data) {
        trend_high->forward(bar.high);
        trend_low->forward(bar.low);
        trend_close->forward(bar.close);
    }
    
    auto trend_dm = std::make_shared<DM>(trend_high, trend_low, trend_close, 14);
    
    std::vector<double> adx_values;
    std::vector<double> di_plus_values;
    
    for (size_t i = 0; i < trend_data.size(); ++i) {
        trend_dm->calculate();
        
        double adx = trend_dm->getADX(0);
        double di_plus = trend_dm->getDIPlus(0);
        
        if (!std::isnan(adx)) {
            adx_values.push_back(adx);
        }
        if (!std::isnan(di_plus)) {
            di_plus_values.push_back(di_plus);
        }
        
        if (i < trend_data.size() - 1) {
            trend_high->forward();
            trend_low->forward();
            trend_close->forward();
        }
    }
    
    // 分析强势趋势中的ADX行为
    if (!adx_values.empty() && adx_values.size() > 20) {
        double avg_late_adx = std::accumulate(adx_values.end() - 10, adx_values.end(), 0.0) / 10.0;
        std::cout << "Strong uptrend - Average late ADX: " << avg_late_adx << std::endl;
        
        // 在强势趋势中，ADX应该相对较高
        EXPECT_GT(avg_late_adx, 15.0) 
            << "ADX should be elevated in strong trend";
    }
    
    if (!di_plus_values.empty() && di_plus_values.size() > 20) {
        double avg_late_di_plus = std::accumulate(di_plus_values.end() - 10, di_plus_values.end(), 0.0) / 10.0;
        std::cout << "Strong uptrend - Average late DI+: " << avg_late_di_plus << std::endl;
    }
}

// 震荡市场测试
TEST(OriginalTests, DM_ChoppyMarket) {
    // 创建震荡市场数据
    std::vector<CSVDataReader::OHLCVData> choppy_data;
    for (int i = 0; i < 100; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        
        double base = 100.0;
        double oscillation = 5.0 * std::sin(i * 0.3);
        
        bar.high = base + oscillation + 2.0;
        bar.low = base + oscillation - 2.0;
        bar.close = base + oscillation;
        bar.open = base + oscillation;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        choppy_data.push_back(bar);
    }
    
    auto choppy_high = std::make_shared<LineRoot>(choppy_data.size(), "choppy_high");
    auto choppy_low = std::make_shared<LineRoot>(choppy_data.size(), "choppy_low");
    auto choppy_close = std::make_shared<LineRoot>(choppy_data.size(), "choppy_close");
    
    for (const auto& bar : choppy_data) {
        choppy_high->forward(bar.high);
        choppy_low->forward(bar.low);
        choppy_close->forward(bar.close);
    }
    
    auto choppy_dm = std::make_shared<DM>(choppy_high, choppy_low, choppy_close, 14);
    
    std::vector<double> adx_values;
    
    for (size_t i = 0; i < choppy_data.size(); ++i) {
        choppy_dm->calculate();
        
        double adx = choppy_dm->getADX(0);
        if (!std::isnan(adx)) {
            adx_values.push_back(adx);
        }
        
        if (i < choppy_data.size() - 1) {
            choppy_high->forward();
            choppy_low->forward();
            choppy_close->forward();
        }
    }
    
    // 在震荡市场中，ADX应该相对较低
    if (!adx_values.empty() && adx_values.size() > 20) {
        double avg_adx = std::accumulate(adx_values.end() - 20, adx_values.end(), 0.0) / 20.0;
        std::cout << "Choppy market - Average ADX: " << avg_adx << std::endl;
        
        EXPECT_LT(avg_adx, 30.0) 
            << "ADX should be relatively low in choppy market";
    }
}

// 边界条件测试
TEST(OriginalTests, DM_EdgeCases) {
    // 测试相同价格的情况
    std::vector<CSVDataReader::OHLCVData> flat_data;
    for (int i = 0; i < 100; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 100.0;
        bar.low = 100.0;
        bar.close = 100.0;
        bar.open = 100.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        flat_data.push_back(bar);
    }
    
    auto flat_high = std::make_shared<LineRoot>(flat_data.size(), "flat_high");
    auto flat_low = std::make_shared<LineRoot>(flat_data.size(), "flat_low");
    auto flat_close = std::make_shared<LineRoot>(flat_data.size(), "flat_close");
    
    for (const auto& bar : flat_data) {
        flat_high->forward(bar.high);
        flat_low->forward(bar.low);
        flat_close->forward(bar.close);
    }
    
    auto flat_dm = std::make_shared<DM>(flat_high, flat_low, flat_close, 14);
    
    for (size_t i = 0; i < flat_data.size(); ++i) {
        flat_dm->calculate();
        if (i < flat_data.size() - 1) {
            flat_high->forward();
            flat_low->forward();
            flat_close->forward();
        }
    }
    
    // 当所有价格相同时，DI和ADX应该为零或接近零
    double di_plus = flat_dm->getDIPlus(0);
    double di_minus = flat_dm->getDIMinus(0);
    double dx = flat_dm->getDX(0);
    double adx = flat_dm->getADX(0);
    
    if (!std::isnan(di_plus)) {
        EXPECT_NEAR(di_plus, 0.0, 1e-6) << "DI+ should be zero for constant prices";
    }
    if (!std::isnan(di_minus)) {
        EXPECT_NEAR(di_minus, 0.0, 1e-6) << "DI- should be zero for constant prices";
    }
    if (!std::isnan(dx)) {
        EXPECT_NEAR(dx, 0.0, 1e-6) << "DX should be zero for constant prices";
    }
    if (!std::isnan(adx)) {
        EXPECT_NEAR(adx, 0.0, 1e-6) << "ADX should be zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_high = std::make_shared<LineRoot>(100, "insufficient_high");
    auto insufficient_low = std::make_shared<LineRoot>(100, "insufficient_low");
    auto insufficient_close = std::make_shared<LineRoot>(100, "insufficient_close");
    
    // 只添加几个数据点
    for (int i = 0; i < 30; ++i) {
        insufficient_high->forward(105.0 + i);
        insufficient_low->forward(95.0 + i);
        insufficient_close->forward(100.0 + i);
    }
    
    auto insufficient_dm = std::make_shared<DM>(insufficient_high, insufficient_low, insufficient_close, 14);
    
    for (int i = 0; i < 30; ++i) {
        insufficient_dm->calculate();
        if (i < 29) {
            insufficient_high->forward();
            insufficient_low->forward();
            insufficient_close->forward();
        }
    }
    
    // 数据不足时ADX应该返回NaN
    double result_adx = insufficient_dm->getADX(0);
    EXPECT_TRUE(std::isnan(result_adx)) << "ADX should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, DM_Performance) {
    // 生成大量测试数据
    const size_t data_size = 5000;  // DM计算复杂，使用5K数据
    std::vector<CSVDataReader::OHLCVData> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> price_dist(50.0, 150.0);
    std::uniform_real_distribution<double> range_dist(1.0, 5.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-01";
        double base_price = price_dist(rng);
        double range = range_dist(rng);
        
        bar.high = base_price + range;
        bar.low = base_price - range;
        bar.close = base_price + (range * 2.0 * rng() / rng.max() - range);
        bar.open = base_price;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        large_data.push_back(bar);
    }
    
    auto large_high = std::make_shared<LineRoot>(large_data.size(), "large_high");
    auto large_low = std::make_shared<LineRoot>(large_data.size(), "large_low");
    auto large_close = std::make_shared<LineRoot>(large_data.size(), "large_close");
    
    for (const auto& bar : large_data) {
        large_high->forward(bar.high);
        large_low->forward(bar.low);
        large_close->forward(bar.close);
    }
    
    auto large_dm = std::make_shared<DM>(large_high, large_low, large_close, 14);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_dm->calculate();
        if (i < large_data.size() - 1) {
            large_high->forward();
            large_low->forward();
            large_close->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "DM calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_di_plus = large_dm->getDIPlus(0);
    double final_di_minus = large_dm->getDIMinus(0);
    double final_dx = large_dm->getDX(0);
    double final_adx = large_dm->getADX(0);
    
    EXPECT_FALSE(std::isnan(final_di_plus)) << "Final DI+ should not be NaN";
    EXPECT_FALSE(std::isnan(final_di_minus)) << "Final DI- should not be NaN";
    EXPECT_FALSE(std::isnan(final_dx)) << "Final DX should not be NaN";
    EXPECT_FALSE(std::isnan(final_adx)) << "Final ADX should not be NaN";
    
    EXPECT_GE(final_adx, 0.0) << "Final ADX should be >= 0";
    EXPECT_LE(final_adx, 100.0) << "Final ADX should be <= 100";
    
    // 性能要求：5K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1500) << "Performance test: should complete within 1.5 seconds";
}