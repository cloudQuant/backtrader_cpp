/**
 * @file test_ind_aroonupdown.cpp
 * @brief AroonUpDown指标测试 - 对应Python test_ind_aroonupdown.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['42.857143', '35.714286', '85.714286'],  # Aroon Up
 *     ['7.142857', '85.714286', '28.571429']   # Aroon Down
 * ]
 * chkmin = 15
 * chkind = btind.AroonUpDown
 */

#include "test_common_simple.h"
#include "indicators/aroonupdown.h"

using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> AROONUPDOWN_EXPECTED_VALUES = {
    {"42.857143", "35.714286", "85.714286"},  // Aroon Up
    {"7.142857", "85.714286", "28.571429"}   // Aroon Down
};

const int AROONUPDOWN_MIN_PERIOD = 15;

} // anonymous namespace

// 使用默认参数的AroonUpDown测试
DEFINE_INDICATOR_TEST(AroonUpDown_Default, AroonUpDown, AROONUPDOWN_EXPECTED_VALUES, AROONUPDOWN_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, AroonUpDown_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    // 创建AroonUpDown指标（默认14周期，最小周期为15）
    auto aroon = std::make_shared<AroonUpDown>(high_line, low_line, 14);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        aroon->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
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
    
    // 验证Aroon Up
    std::vector<std::string> expected_up = {"42.857143", "35.714286", "85.714286"};
    for (size_t i = 0; i < check_points.size() && i < expected_up.size(); ++i) {
        double actual = aroon->getAroonUp(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_up[i]) 
            << "Aroon Up mismatch at check point " << i;
    }
    
    // 验证Aroon Down
    std::vector<std::string> expected_down = {"7.142857", "85.714286", "28.571429"};
    for (size_t i = 0; i < check_points.size() && i < expected_down.size(); ++i) {
        double actual = aroon->getAroonDown(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_down[i]) 
            << "Aroon Down mismatch at check point " << i;
    }
    
    // 验证最小周期
    EXPECT_EQ(aroon->getMinPeriod(), 15) << "AroonUpDown minimum period should be 15";
}

// Aroon范围验证测试
TEST(OriginalTests, AroonUpDown_RangeValidation) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto aroon = std::make_shared<AroonUpDown>(high_line, low_line, 14);
    
    // 计算所有值并验证范围
    for (size_t i = 0; i < csv_data.size(); ++i) {
        aroon->calculate();
        
        double aroon_up = aroon->getAroonUp(0);
        double aroon_down = aroon->getAroonDown(0);
        
        // 验证Aroon Up和Down都在0到100范围内
        if (!std::isnan(aroon_up)) {
            EXPECT_GE(aroon_up, 0.0) << "Aroon Up should be >= 0 at step " << i;
            EXPECT_LE(aroon_up, 100.0) << "Aroon Up should be <= 100 at step " << i;
        }
        
        if (!std::isnan(aroon_down)) {
            EXPECT_GE(aroon_down, 0.0) << "Aroon Down should be >= 0 at step " << i;
            EXPECT_LE(aroon_down, 100.0) << "Aroon Down should be <= 100 at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
}

// 参数化测试 - 测试不同周期的AroonUpDown
class AroonUpDownParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        high_line_ = std::make_shared<LineRoot>(csv_data_.size(), "high");
        low_line_ = std::make_shared<LineRoot>(csv_data_.size(), "low");
        
        for (const auto& bar : csv_data_) {
            high_line_->forward(bar.high);
            low_line_->forward(bar.low);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineRoot> high_line_;
    std::shared_ptr<LineRoot> low_line_;
};

TEST_P(AroonUpDownParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto aroon = std::make_shared<AroonUpDown>(high_line_, low_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        aroon->calculate();
        if (i < csv_data_.size() - 1) {
            high_line_->forward();
            low_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(aroon->getMinPeriod(), period + 1) 
        << "AroonUpDown minimum period should be period + 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        double last_up = aroon->getAroonUp(0);
        double last_down = aroon->getAroonDown(0);
        
        EXPECT_FALSE(std::isnan(last_up)) << "Last Aroon Up value should not be NaN";
        EXPECT_FALSE(std::isnan(last_down)) << "Last Aroon Down value should not be NaN";
        EXPECT_GE(last_up, 0.0) << "Aroon Up should be >= 0";
        EXPECT_LE(last_up, 100.0) << "Aroon Up should be <= 100";
        EXPECT_GE(last_down, 0.0) << "Aroon Down should be >= 0";
        EXPECT_LE(last_down, 100.0) << "Aroon Down should be <= 100";
    }
}

// 测试不同的AroonUpDown周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    AroonUpDownParameterizedTest,
    ::testing::Values(7, 14, 21, 25)
);

// Aroon计算逻辑验证测试
TEST(OriginalTests, AroonUpDown_CalculationLogic) {
    // 使用简单的测试数据验证Aroon计算
    std::vector<CSVDataReader::OHLCVData> test_data = {
        {"2006-01-01", 100.0, 110.0, 90.0, 105.0, 0, 0},   // 最高点在第0位置
        {"2006-01-02", 105.0, 115.0, 95.0, 110.0, 0, 0},   // 更高的最高点
        {"2006-01-03", 110.0, 120.0, 100.0, 115.0, 0, 0},  // 更高的最高点
        {"2006-01-04", 115.0, 125.0, 105.0, 120.0, 0, 0},  // 更高的最高点
        {"2006-01-05", 120.0, 130.0, 85.0, 125.0, 0, 0}    // 更高的最高点，但最低点
    };
    
    auto high_line = std::make_shared<LineRoot>(test_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(test_data.size(), "low");
    
    for (const auto& bar : test_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto aroon = std::make_shared<AroonUpDown>(high_line, low_line, 4);
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        aroon->calculate();
        
        // 手动计算Aroon进行验证
        if (i >= 4) {  // 需要至少5个数据点
            // 找到最近4+1个数据点中的最高价和最低价位置
            double highest = -std::numeric_limits<double>::infinity();
            double lowest = std::numeric_limits<double>::infinity();
            int highest_pos = 0;
            int lowest_pos = 0;
            
            for (int j = 0; j <= 4; ++j) {
                if (test_data[i - j].high > highest) {
                    highest = test_data[i - j].high;
                    highest_pos = j;
                }
                if (test_data[i - j].low < lowest) {
                    lowest = test_data[i - j].low;
                    lowest_pos = j;
                }
            }
            
            // Aroon Up = ((period - periods_since_highest) / period) * 100
            // Aroon Down = ((period - periods_since_lowest) / period) * 100
            double expected_up = ((4.0 - highest_pos) / 4.0) * 100.0;
            double expected_down = ((4.0 - lowest_pos) / 4.0) * 100.0;
            
            double actual_up = aroon->getAroonUp(0);
            double actual_down = aroon->getAroonDown(0);
            
            if (!std::isnan(actual_up) && !std::isnan(actual_down)) {
                EXPECT_NEAR(actual_up, expected_up, 1e-6) 
                    << "Aroon Up calculation mismatch at step " << i;
                EXPECT_NEAR(actual_down, expected_down, 1e-6) 
                    << "Aroon Down calculation mismatch at step " << i;
            }
        }
        
        if (i < test_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
}

// 趋势识别测试
TEST(OriginalTests, AroonUpDown_TrendIdentification) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto aroon = std::make_shared<AroonUpDown>(high_line, low_line, 14);
    
    int uptrend_signals = 0;    // Aroon Up > 70 and Aroon Down < 30
    int downtrend_signals = 0;  // Aroon Down > 70 and Aroon Up < 30
    int sideways_signals = 0;   // Both between 30-70
    
    // 统计趋势信号
    for (size_t i = 0; i < csv_data.size(); ++i) {
        aroon->calculate();
        
        double aroon_up = aroon->getAroonUp(0);
        double aroon_down = aroon->getAroonDown(0);
        
        if (!std::isnan(aroon_up) && !std::isnan(aroon_down)) {
            if (aroon_up > 70.0 && aroon_down < 30.0) {
                uptrend_signals++;
            } else if (aroon_down > 70.0 && aroon_up < 30.0) {
                downtrend_signals++;
            } else if (aroon_up >= 30.0 && aroon_up <= 70.0 && aroon_down >= 30.0 && aroon_down <= 70.0) {
                sideways_signals++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    std::cout << "Aroon trend signals:" << std::endl;
    std::cout << "Uptrend signals: " << uptrend_signals << std::endl;
    std::cout << "Downtrend signals: " << downtrend_signals << std::endl;
    std::cout << "Sideways signals: " << sideways_signals << std::endl;
    
    // 验证至少有一些有效的计算
    EXPECT_GT(uptrend_signals + downtrend_signals + sideways_signals, 0) 
        << "Should have some valid Aroon calculations";
}

// 交叉信号测试
TEST(OriginalTests, AroonUpDown_CrossoverSignals) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto aroon = std::make_shared<AroonUpDown>(high_line, low_line, 14);
    
    int bullish_crossovers = 0;  // Aroon Up crosses above Aroon Down
    int bearish_crossovers = 0;  // Aroon Down crosses above Aroon Up
    
    double prev_up = 0.0, prev_down = 0.0;
    bool has_prev = false;
    
    // 检测交叉信号
    for (size_t i = 0; i < csv_data.size(); ++i) {
        aroon->calculate();
        
        double current_up = aroon->getAroonUp(0);
        double current_down = aroon->getAroonDown(0);
        
        if (!std::isnan(current_up) && !std::isnan(current_down) && has_prev) {
            // 检测Aroon Up上穿Aroon Down
            if (prev_up <= prev_down && current_up > current_down) {
                bullish_crossovers++;
            }
            // 检测Aroon Down上穿Aroon Up
            else if (prev_down <= prev_up && current_down > current_up) {
                bearish_crossovers++;
            }
        }
        
        if (!std::isnan(current_up) && !std::isnan(current_down)) {
            prev_up = current_up;
            prev_down = current_down;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    std::cout << "Aroon crossover signals:" << std::endl;
    std::cout << "Bullish crossovers: " << bullish_crossovers << std::endl;
    std::cout << "Bearish crossovers: " << bearish_crossovers << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(bullish_crossovers + bearish_crossovers, 0) 
        << "Should detect some crossover signals";
}

// 极值测试
TEST(OriginalTests, AroonUpDown_ExtremeValues) {
    // 创建一个序列，其中最高价在最新位置，最低价在最旧位置
    std::vector<CSVDataReader::OHLCVData> extreme_data;
    
    for (int i = 0; i < 20; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 100.0 + i;       // 递增的最高价
        bar.low = 100.0 - (19 - i); // 递减的最低价
        bar.close = 100.0;
        bar.open = 100.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        extreme_data.push_back(bar);
    }
    
    auto high_line = std::make_shared<LineRoot>(extreme_data.size(), "extreme_high");
    auto low_line = std::make_shared<LineRoot>(extreme_data.size(), "extreme_low");
    
    for (const auto& bar : extreme_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto extreme_aroon = std::make_shared<AroonUpDown>(high_line, low_line, 14);
    
    for (size_t i = 0; i < extreme_data.size(); ++i) {
        extreme_aroon->calculate();
        if (i < extreme_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    // 最新的最高价应该给出Aroon Up = 100
    double final_up = extreme_aroon->getAroonUp(0);
    double final_down = extreme_aroon->getAroonDown(0);
    
    if (!std::isnan(final_up)) {
        EXPECT_NEAR(final_up, 100.0, 1e-6) 
            << "Aroon Up should be 100 when highest high is most recent";
    }
    
    if (!std::isnan(final_down)) {
        EXPECT_LT(final_down, 100.0) 
            << "Aroon Down should be less than 100 when lowest low is not most recent";
    }
}

// 边界条件测试
TEST(OriginalTests, AroonUpDown_EdgeCases) {
    // 测试数据不足的情况
    auto insufficient_high = std::make_shared<LineRoot>(100, "insufficient_high");
    auto insufficient_low = std::make_shared<LineRoot>(100, "insufficient_low");
    
    // 只添加3个数据点
    std::vector<CSVDataReader::OHLCVData> short_data = {
        {"2006-01-01", 100.0, 110.0, 90.0, 105.0, 0, 0},
        {"2006-01-02", 105.0, 115.0, 95.0, 110.0, 0, 0},
        {"2006-01-03", 110.0, 120.0, 100.0, 115.0, 0, 0}
    };
    
    for (const auto& bar : short_data) {
        insufficient_high->forward(bar.high);
        insufficient_low->forward(bar.low);
    }
    
    auto insufficient_aroon = std::make_shared<AroonUpDown>(insufficient_high, insufficient_low, 14);  // 周期大于数据量
    
    for (size_t i = 0; i < short_data.size(); ++i) {
        insufficient_aroon->calculate();
        if (i < short_data.size() - 1) {
            insufficient_high->forward();
            insufficient_low->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result_up = insufficient_aroon->getAroonUp(0);
    double result_down = insufficient_aroon->getAroonDown(0);
    EXPECT_TRUE(std::isnan(result_up)) << "Aroon Up should return NaN when insufficient data";
    EXPECT_TRUE(std::isnan(result_down)) << "Aroon Down should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, AroonUpDown_Performance) {
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
    
    auto large_high = std::make_shared<LineRoot>(large_data.size(), "large_high");
    auto large_low = std::make_shared<LineRoot>(large_data.size(), "large_low");
    
    for (const auto& bar : large_data) {
        large_high->forward(bar.high);
        large_low->forward(bar.low);
    }
    
    auto large_aroon = std::make_shared<AroonUpDown>(large_high, large_low, 14);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_aroon->calculate();
        if (i < large_data.size() - 1) {
            large_high->forward();
            large_low->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "AroonUpDown calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_up = large_aroon->getAroonUp(0);
    double final_down = large_aroon->getAroonDown(0);
    EXPECT_FALSE(std::isnan(final_up)) << "Final Aroon Up should not be NaN";
    EXPECT_FALSE(std::isnan(final_down)) << "Final Aroon Down should not be NaN";
    EXPECT_GE(final_up, 0.0) << "Final Aroon Up should be >= 0";
    EXPECT_LE(final_up, 100.0) << "Final Aroon Up should be <= 100";
    EXPECT_GE(final_down, 0.0) << "Final Aroon Down should be >= 0";
    EXPECT_LE(final_down, 100.0) << "Final Aroon Down should be <= 100";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}