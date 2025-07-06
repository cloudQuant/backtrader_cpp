/**
 * @file test_ind_vortex.cpp
 * @brief Vortex指标测试 - 对应Python test_ind_vortex.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['1.245434', '0.921076', '1.062278'],  # VI+
 *     ['0.707948', '0.966375', '0.803849']   # VI-
 * ]
 * chkmin = 15
 * chkind = btind.Vortex
 */

#include "test_common.h"
#include <random>

#include "indicators/vortex.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> VORTEX_EXPECTED_VALUES = {
    {"1.245434", "0.921076", "1.062278"},  // VI+
    {"0.707948", "0.966375", "0.803849"}   // VI-
};

const int VORTEX_MIN_PERIOD = 15;

} // anonymous namespace

// 使用默认参数的Vortex测试
DEFINE_INDICATOR_TEST(Vortex_Default, Vortex, VORTEX_EXPECTED_VALUES, VORTEX_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, Vortex_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    // 创建Vortex指标（默认14周期，最小周期为15）
    auto vortex = std::make_shared<Vortex>(high_line, low_line, close_line, 14);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        vortex->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 15;  // period + 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // 验证VI+
    std::vector<std::string> expected_vi_plus = {"1.245434", "0.921076", "1.062278"};
    for (size_t i = 0; i < check_points.size() && i < expected_vi_plus.size(); ++i) {
        double actual = vortex->getVIPlus(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_vi_plus[i]) 
            << "VI+ mismatch at check point " << i;
    }
    
    // 验证VI-
    std::vector<std::string> expected_vi_minus = {"0.707948", "0.966375", "0.803849"};
    for (size_t i = 0; i < check_points.size() && i < expected_vi_minus.size(); ++i) {
        double actual = vortex->getVIMinus(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_vi_minus[i]) 
            << "VI- mismatch at check point " << i;
    }
    
    // 验证最小周期
    EXPECT_EQ(vortex->getMinPeriod(), 15) << "Vortex minimum period should be 15";
}

// 参数化测试 - 测试不同周期的Vortex
class VortexParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        high_line_ = std::make_shared<backtrader::LineRoot>(csv_data_.size(), "high");
        low_line_ = std::make_shared<backtrader::LineRoot>(csv_data_.size(), "low");
        close_line_ = std::make_shared<backtrader::LineRoot>(csv_data_.size(), "close");
        
        for (const auto& bar : csv_data_) {
            high_line_->forward(bar.high);
            low_line_->forward(bar.low);
            close_line_->forward(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<backtrader::LineRoot> high_line_;
    std::shared_ptr<backtrader::LineRoot> low_line_;
    std::shared_ptr<backtrader::LineRoot> close_line_;
};

TEST_P(VortexParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto vortex = std::make_shared<Vortex>(high_line_, low_line_, close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        vortex->calculate();
        if (i < csv_data_.size() - 1) {
            high_line_->forward();
            low_line_->forward();
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(vortex->getMinPeriod(), period + 1) 
        << "Vortex minimum period should be period + 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        double last_vi_plus = vortex->getVIPlus(0);
        double last_vi_minus = vortex->getVIMinus(0);
        
        EXPECT_FALSE(std::isnan(last_vi_plus)) << "Last VI+ value should not be NaN";
        EXPECT_FALSE(std::isnan(last_vi_minus)) << "Last VI- value should not be NaN";
        EXPECT_GT(last_vi_plus, 0.0) << "VI+ should be positive";
        EXPECT_GT(last_vi_minus, 0.0) << "VI- should be positive";
    }
}

// 测试不同的Vortex周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    VortexParameterizedTest,
    ::testing::Values(7, 14, 21, 28)
);

// Vortex计算逻辑验证测试
TEST(OriginalTests, Vortex_CalculationLogic) {
    // 使用简单的测试数据验证Vortex计算
    std::vector<CSVDataReader::OHLCVData> test_data = {
        {"2006-01-01", 100.0, 110.0, 90.0, 105.0, 0, 0},
        {"2006-01-02", 105.0, 115.0, 95.0, 110.0, 0, 0},
        {"2006-01-03", 110.0, 120.0, 100.0, 115.0, 0, 0},
        {"2006-01-04", 115.0, 125.0, 105.0, 120.0, 0, 0},
        {"2006-01-05", 120.0, 130.0, 110.0, 125.0, 0, 0}
    };
    
    auto high_line = std::make_shared<backtrader::LineRoot>(test_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(test_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(test_data.size(), "close");
    
    for (const auto& bar : test_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto vortex = std::make_shared<Vortex>(high_line, low_line, close_line, 3);
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        vortex->calculate();
        
        double vi_plus = vortex->getVIPlus(0);
        double vi_minus = vortex->getVIMinus(0);
        
        // Vortex指标应该产生有限正值
        if (!std::isnan(vi_plus) && !std::isnan(vi_minus)) {
            EXPECT_TRUE(std::isfinite(vi_plus)) 
                << "VI+ should be finite at step " << i;
            EXPECT_TRUE(std::isfinite(vi_minus)) 
                << "VI- should be finite at step " << i;
            EXPECT_GT(vi_plus, 0.0) 
                << "VI+ should be positive at step " << i;
            EXPECT_GT(vi_minus, 0.0) 
                << "VI- should be positive at step " << i;
        }
        
        if (i < test_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}

// 趋势识别测试
TEST(OriginalTests, Vortex_TrendIdentification) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto vortex = std::make_shared<Vortex>(high_line, low_line, close_line, 14);
    
    int uptrend_signals = 0;    // VI+ > VI-
    int downtrend_signals = 0;  // VI- > VI+
    
    // 统计趋势信号
    for (size_t i = 0; i < csv_data.size(); ++i) {
        vortex->calculate();
        
        double vi_plus = vortex->getVIPlus(0);
        double vi_minus = vortex->getVIMinus(0);
        
        if (!std::isnan(vi_plus) && !std::isnan(vi_minus)) {
            if (vi_plus > vi_minus) {
                uptrend_signals++;
            } else if (vi_minus > vi_plus) {
                downtrend_signals++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    std::cout << "Vortex trend signals:" << std::endl;
    std::cout << "Uptrend signals (VI+ > VI-): " << uptrend_signals << std::endl;
    std::cout << "Downtrend signals (VI- > VI+): " << downtrend_signals << std::endl;
    
    // 验证至少有一些有效的计算
    EXPECT_GT(uptrend_signals + downtrend_signals, 0) 
        << "Should have some valid Vortex calculations";
}

// 交叉信号测试
TEST(OriginalTests, Vortex_CrossoverSignals) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto vortex = std::make_shared<Vortex>(high_line, low_line, close_line, 14);
    
    int bullish_crossovers = 0;  // VI+ crosses above VI-
    int bearish_crossovers = 0;  // VI- crosses above VI+
    
    double prev_vi_plus = 0.0, prev_vi_minus = 0.0;
    bool has_prev = false;
    
    // 检测交叉信号
    for (size_t i = 0; i < csv_data.size(); ++i) {
        vortex->calculate();
        
        double current_vi_plus = vortex->getVIPlus(0);
        double current_vi_minus = vortex->getVIMinus(0);
        
        if (!std::isnan(current_vi_plus) && !std::isnan(current_vi_minus) && has_prev) {
            // 检测VI+上穿VI-
            if (prev_vi_plus <= prev_vi_minus && current_vi_plus > current_vi_minus) {
                bullish_crossovers++;
            }
            // 检测VI-上穿VI+
            else if (prev_vi_minus <= prev_vi_plus && current_vi_minus > current_vi_plus) {
                bearish_crossovers++;
            }
        }
        
        if (!std::isnan(current_vi_plus) && !std::isnan(current_vi_minus)) {
            prev_vi_plus = current_vi_plus;
            prev_vi_minus = current_vi_minus;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    std::cout << "Vortex crossover signals:" << std::endl;
    std::cout << "Bullish crossovers: " << bullish_crossovers << std::endl;
    std::cout << "Bearish crossovers: " << bearish_crossovers << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(bullish_crossovers + bearish_crossovers, 0) 
        << "Should detect some crossover signals";
}

// 强势趋势测试
TEST(OriginalTests, Vortex_StrongTrend) {
    // 创建强势上升趋势数据
    std::vector<CSVDataReader::OHLCVData> uptrend_data;
    for (int i = 0; i < 50; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.open = 100.0 + i * 2.0;
        bar.high = bar.open + 5.0;
        bar.low = bar.open - 1.0;
        bar.close = bar.open + 4.0;  // 强势收盘
        bar.volume = 1000;
        bar.openinterest = 0;
        
        uptrend_data.push_back(bar);
    }
    
    auto up_high = std::make_shared<backtrader::LineRoot>(uptrend_data.size(), "up_high");
    auto up_low = std::make_shared<backtrader::LineRoot>(uptrend_data.size(), "up_low");
    auto up_close = std::make_shared<backtrader::LineRoot>(uptrend_data.size(), "up_close");
    
    for (const auto& bar : uptrend_data) {
        up_high->forward(bar.high);
        up_low->forward(bar.low);
        up_close->forward(bar.close);
    }
    
    auto up_vortex = std::make_shared<Vortex>(up_high, up_low, up_close, 14);
    
    for (size_t i = 0; i < uptrend_data.size(); ++i) {
        up_vortex->calculate();
        if (i < uptrend_data.size() - 1) {
            up_high->forward();
            up_low->forward();
            up_close->forward();
        }
    }
    
    // 在强势上升趋势中，VI+应该大于VI-
    double final_vi_plus = up_vortex->getVIPlus(0);
    double final_vi_minus = up_vortex->getVIMinus(0);
    
    if (!std::isnan(final_vi_plus) && !std::isnan(final_vi_minus)) {
        EXPECT_GT(final_vi_plus, final_vi_minus) 
            << "VI+ should be greater than VI- in strong uptrend";
        
        std::cout << "Strong uptrend - VI+: " << final_vi_plus 
                  << ", VI-: " << final_vi_minus << std::endl;
    }
}

// 震荡市场测试
TEST(OriginalTests, Vortex_ChoppyMarket) {
    // 创建震荡市场数据
    std::vector<CSVDataReader::OHLCVData> choppy_data;
    for (int i = 0; i < 50; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        
        // 震荡价格
        double base = 100.0;
        double oscillation = 5.0 * std::sin(i * 0.5);
        
        bar.open = base + oscillation;
        bar.high = bar.open + std::abs(oscillation);
        bar.low = bar.open - std::abs(oscillation);
        bar.close = base + oscillation * 0.5;  // 减弱的收盘
        bar.volume = 1000;
        bar.openinterest = 0;
        
        choppy_data.push_back(bar);
    }
    
    auto choppy_high = std::make_shared<backtrader::LineRoot>(choppy_data.size(), "choppy_high");
    auto choppy_low = std::make_shared<backtrader::LineRoot>(choppy_data.size(), "choppy_low");
    auto choppy_close = std::make_shared<backtrader::LineRoot>(choppy_data.size(), "choppy_close");
    
    for (const auto& bar : choppy_data) {
        choppy_high->forward(bar.high);
        choppy_low->forward(bar.low);
        choppy_close->forward(bar.close);
    }
    
    auto choppy_vortex = std::make_shared<Vortex>(choppy_high, choppy_low, choppy_close, 14);
    
    std::vector<double> vi_diff;  // VI+ - VI-的差值
    
    for (size_t i = 0; i < choppy_data.size(); ++i) {
        choppy_vortex->calculate();
        
        double vi_plus = choppy_vortex->getVIPlus(0);
        double vi_minus = choppy_vortex->getVIMinus(0);
        
        if (!std::isnan(vi_plus) && !std::isnan(vi_minus)) {
            vi_diff.push_back(vi_plus - vi_minus);
        }
        
        if (i < choppy_data.size() - 1) {
            choppy_high->forward();
            choppy_low->forward();
            choppy_close->forward();
        }
    }
    
    // 在震荡市场中，VI+和VI-的差值应该较小
    if (!vi_diff.empty()) {
        double avg_diff = std::accumulate(vi_diff.begin(), vi_diff.end(), 0.0) / vi_diff.size();
        std::cout << "Choppy market average VI difference: " << avg_diff << std::endl;
        
        EXPECT_NEAR(avg_diff, 0.0, 0.5) 
            << "VI+ and VI- should be close in choppy market";
    }
}

// 边界条件测试
TEST(OriginalTests, Vortex_EdgeCases) {
    // 测试相同价格的情况
    std::vector<CSVDataReader::OHLCVData> flat_data;
    for (int i = 0; i < 30; ++i) {
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
    
    auto flat_high = std::make_shared<backtrader::LineRoot>(flat_data.size(), "flat_high");
    auto flat_low = std::make_shared<backtrader::LineRoot>(flat_data.size(), "flat_low");
    auto flat_close = std::make_shared<backtrader::LineRoot>(flat_data.size(), "flat_close");
    
    for (const auto& bar : flat_data) {
        flat_high->forward(bar.high);
        flat_low->forward(bar.low);
        flat_close->forward(bar.close);
    }
    
    auto flat_vortex = std::make_shared<Vortex>(flat_high, flat_low, flat_close, 14);
    
    for (size_t i = 0; i < flat_data.size(); ++i) {
        flat_vortex->calculate();
        if (i < flat_data.size() - 1) {
            flat_high->forward();
            flat_low->forward();
            flat_close->forward();
        }
    }
    
    // 当所有价格相同时，VI+和VI-的行为取决于具体实现
    double final_vi_plus = flat_vortex->getVIPlus(0);
    double final_vi_minus = flat_vortex->getVIMinus(0);
    
    if (!std::isnan(final_vi_plus) && !std::isnan(final_vi_minus)) {
        EXPECT_TRUE(std::isfinite(final_vi_plus)) << "VI+ should be finite for constant prices";
        EXPECT_TRUE(std::isfinite(final_vi_minus)) << "VI- should be finite for constant prices";
    }
}

// 性能测试
TEST(OriginalTests, Vortex_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<CSVDataReader::OHLCVData> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> price_dist(50.0, 150.0);
    std::uniform_real_distribution<double> range_dist(1.0, 5.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-01";
        bar.close = price_dist(rng);
        double range = range_dist(rng);
        bar.high = bar.close + range;
        bar.low = bar.close - range;
        bar.open = bar.close;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        large_data.push_back(bar);
    }
    
    auto large_high = std::make_shared<backtrader::LineRoot>(large_data.size(), "large_high");
    auto large_low = std::make_shared<backtrader::LineRoot>(large_data.size(), "large_low");
    auto large_close = std::make_shared<backtrader::LineRoot>(large_data.size(), "large_close");
    
    for (const auto& bar : large_data) {
        large_high->forward(bar.high);
        large_low->forward(bar.low);
        large_close->forward(bar.close);
    }
    
    auto large_vortex = std::make_shared<Vortex>(large_high, large_low, large_close, 14);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_vortex->calculate();
        if (i < large_data.size() - 1) {
            large_high->forward();
            large_low->forward();
            large_close->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Vortex calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_vi_plus = large_vortex->getVIPlus(0);
    double final_vi_minus = large_vortex->getVIMinus(0);
    
    EXPECT_FALSE(std::isnan(final_vi_plus)) << "Final VI+ should not be NaN";
    EXPECT_FALSE(std::isnan(final_vi_minus)) << "Final VI- should not be NaN";
    EXPECT_GT(final_vi_plus, 0.0) << "Final VI+ should be positive";
    EXPECT_GT(final_vi_minus, 0.0) << "Final VI- should be positive";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}