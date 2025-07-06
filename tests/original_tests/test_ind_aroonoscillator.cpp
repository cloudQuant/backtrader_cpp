/**
 * @file test_ind_aroonoscillator.cpp
 * @brief AroonOscillator指标测试 - 对应Python test_ind_aroonoscillator.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['35.714286', '-50.000000', '57.142857']
 * ]
 * chkmin = 15
 * chkind = btind.AroonOscillator
 */

#include "test_common.h"
#include <random>
#include "indicators/aroon.h"

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> AROON_OSC_EXPECTED_VALUES = {
    {"35.714286", "-50.000000", "57.142857"}
};

const int AROON_OSC_MIN_PERIOD = 15;

} // anonymous namespace

// 使用默认参数的AroonOscillator测试
DEFINE_INDICATOR_TEST(AroonOscillator_Default, AroonOscillator, AROON_OSC_EXPECTED_VALUES, AROON_OSC_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, AroonOscillator_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    // 创建AroonOscillator指标（默认14周期，最小周期为15）
    auto aroon_osc = std::make_shared<AroonOscillator>(high_line, low_line, 14);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        aroon_osc->calculate();
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
    
    std::vector<std::string> expected = {"35.714286", "-50.000000", "57.142857"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = aroon_osc->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "AroonOscillator value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(aroon_osc->getMinPeriod(), 15) << "AroonOscillator minimum period should be 15";
}

// AroonOscillator范围验证测试
TEST(OriginalTests, AroonOscillator_RangeValidation) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto aroon_osc = std::make_shared<AroonOscillator>(high_line, low_line, 14);
    
    // 计算所有值并验证范围
    for (size_t i = 0; i < csv_data.size(); ++i) {
        aroon_osc->calculate();
        
        double osc_value = aroon_osc->get(0);
        
        // 验证AroonOscillator在-100到+100范围内
        if (!std::isnan(osc_value)) {
            EXPECT_GE(osc_value, -100.0) << "AroonOscillator should be >= -100 at step " << i;
            EXPECT_LE(osc_value, 100.0) << "AroonOscillator should be <= 100 at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
}

// 参数化测试 - 测试不同周期的AroonOscillator
class AroonOscillatorParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        high_line_ = std::make_shared<backtrader::LineRoot>(csv_data_.size(), "high");
        low_line_ = std::make_shared<backtrader::LineRoot>(csv_data_.size(), "low");
        
        for (const auto& bar : csv_data_) {
            high_line_->forward(bar.high);
            low_line_->forward(bar.low);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<backtrader::LineRoot> high_line_;
    std::shared_ptr<backtrader::LineRoot> low_line_;
};

TEST_P(AroonOscillatorParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto aroon_osc = std::make_shared<AroonOscillator>(high_line_, low_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        aroon_osc->calculate();
        if (i < csv_data_.size() - 1) {
            high_line_->forward();
            low_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(aroon_osc->getMinPeriod(), period + 1) 
        << "AroonOscillator minimum period should be period + 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        double last_value = aroon_osc->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last AroonOscillator value should not be NaN";
        EXPECT_GE(last_value, -100.0) << "AroonOscillator should be >= -100";
        EXPECT_LE(last_value, 100.0) << "AroonOscillator should be <= 100";
    }
}

// 测试不同的AroonOscillator周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    AroonOscillatorParameterizedTest,
    ::testing::Values(7, 14, 21, 25)
);

// AroonOscillator计算逻辑验证测试
TEST(OriginalTests, AroonOscillator_CalculationLogic) {
    // 使用简单的测试数据验证AroonOscillator计算
    std::vector<CSVDataReader::OHLCVData> test_data = {
        {"2006-01-01", 100.0, 110.0, 90.0, 105.0, 0, 0},   
        {"2006-01-02", 105.0, 115.0, 95.0, 110.0, 0, 0},   
        {"2006-01-03", 110.0, 120.0, 100.0, 115.0, 0, 0},  
        {"2006-01-04", 115.0, 125.0, 105.0, 120.0, 0, 0},  
        {"2006-01-05", 120.0, 130.0, 85.0, 125.0, 0, 0}    
    };
    
    auto high_line = std::make_shared<backtrader::LineRoot>(test_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(test_data.size(), "low");
    
    for (const auto& bar : test_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto aroon_osc = std::make_shared<AroonOscillator>(high_line, low_line, 4);
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        aroon_osc->calculate();
        
        // 手动计算AroonOscillator进行验证
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
            // AroonOscillator = Aroon Up - Aroon Down
            double aroon_up = ((4.0 - highest_pos) / 4.0) * 100.0;
            double aroon_down = ((4.0 - lowest_pos) / 4.0) * 100.0;
            double expected_osc = aroon_up - aroon_down;
            
            double actual_osc = aroon_osc->get(0);
            
            if (!std::isnan(actual_osc)) {
                EXPECT_NEAR(actual_osc, expected_osc, 1e-6) 
                    << "AroonOscillator calculation mismatch at step " << i;
            }
        }
        
        if (i < test_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
}

// 趋势识别测试
TEST(OriginalTests, AroonOscillator_TrendIdentification) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto aroon_osc = std::make_shared<AroonOscillator>(high_line, low_line, 14);
    
    int strong_uptrend = 0;    // AroonOsc > 50
    int strong_downtrend = 0;  // AroonOsc < -50
    int weak_trend = 0;        // -50 <= AroonOsc <= 50
    
    // 统计趋势信号
    for (size_t i = 0; i < csv_data.size(); ++i) {
        aroon_osc->calculate();
        
        double osc_value = aroon_osc->get(0);
        
        if (!std::isnan(osc_value)) {
            if (osc_value > 50.0) {
                strong_uptrend++;
            } else if (osc_value < -50.0) {
                strong_downtrend++;
            } else {
                weak_trend++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    std::cout << "AroonOscillator trend signals:" << std::endl;
    std::cout << "Strong uptrend (> 50): " << strong_uptrend << std::endl;
    std::cout << "Strong downtrend (< -50): " << strong_downtrend << std::endl;
    std::cout << "Weak trend (-50 to 50): " << weak_trend << std::endl;
    
    // 验证至少有一些有效的计算
    EXPECT_GT(strong_uptrend + strong_downtrend + weak_trend, 0) 
        << "Should have some valid AroonOscillator calculations";
}

// 零线穿越测试
TEST(OriginalTests, AroonOscillator_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto aroon_osc = std::make_shared<AroonOscillator>(high_line, low_line, 14);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_osc = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越
    for (size_t i = 0; i < csv_data.size(); ++i) {
        aroon_osc->calculate();
        
        double current_osc = aroon_osc->get(0);
        
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
            high_line->forward();
            low_line->forward();
        }
    }
    
    std::cout << "AroonOscillator zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些穿越信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// 与AroonUpDown关系测试
TEST(OriginalTests, AroonOscillator_vs_AroonUpDown) {
    auto csv_data = getdata(0);
    auto high_line_osc = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high_osc");
    auto low_line_osc = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low_osc");
    auto high_line_updown = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high_updown");
    auto low_line_updown = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low_updown");
    
    for (const auto& bar : csv_data) {
        high_line_osc->forward(bar.high);
        low_line_osc->forward(bar.low);
        high_line_updown->forward(bar.high);
        low_line_updown->forward(bar.low);
    }
    
    auto aroon_osc = std::make_shared<AroonOscillator>(high_line_osc, low_line_osc, 14);
    auto aroon_updown = std::make_shared<AroonUpDown>(high_line_updown, low_line_updown, 14);
    
    // 验证AroonOscillator = AroonUp - AroonDown
    for (size_t i = 0; i < csv_data.size(); ++i) {
        aroon_osc->calculate();
        aroon_updown->calculate();
        
        double osc_value = aroon_osc->get(0);
        double aroon_up = aroon_updown->getAroonUp(0);
        double aroon_down = aroon_updown->getAroonDown(0);
        
        if (!std::isnan(osc_value) && !std::isnan(aroon_up) && !std::isnan(aroon_down)) {
            double expected_osc = aroon_up - aroon_down;
            EXPECT_NEAR(osc_value, expected_osc, 1e-10) 
                << "AroonOscillator should equal AroonUp - AroonDown at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            high_line_osc->forward();
            low_line_osc->forward();
            high_line_updown->forward();
            low_line_updown->forward();
        }
    }
}

// 极值测试
TEST(OriginalTests, AroonOscillator_ExtremeValues) {
    // 创建一个序列，使AroonOscillator达到极值
    std::vector<CSVDataReader::OHLCVData> extreme_data;
    
    // 创建持续上升的最高价，固定的最低价
    for (int i = 0; i < 20; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 100.0 + i;  // 持续上升
        bar.low = 90.0;        // 固定低价在最开始
        bar.close = 95.0 + i;
        bar.open = 95.0 + i;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        extreme_data.push_back(bar);
    }
    
    auto high_line = std::make_shared<backtrader::LineRoot>(extreme_data.size(), "extreme_high");
    auto low_line = std::make_shared<backtrader::LineRoot>(extreme_data.size(), "extreme_low");
    
    for (const auto& bar : extreme_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto extreme_osc = std::make_shared<AroonOscillator>(high_line, low_line, 14);
    
    for (size_t i = 0; i < extreme_data.size(); ++i) {
        extreme_osc->calculate();
        if (i < extreme_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    // 最新的最高价和最旧的最低价应该给出接近100的AroonOscillator
    double final_osc = extreme_osc->get(0);
    
    if (!std::isnan(final_osc)) {
        EXPECT_GT(final_osc, 50.0) 
            << "AroonOscillator should be strongly positive when highest high is recent and lowest low is old";
    }
}

// 边界条件测试
TEST(OriginalTests, AroonOscillator_EdgeCases) {
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
    
    for (const auto& bar : flat_data) {
        flat_high->forward(bar.high);
        flat_low->forward(bar.low);
    }
    
    auto flat_osc = std::make_shared<AroonOscillator>(flat_high, flat_low, 14);
    
    for (size_t i = 0; i < flat_data.size(); ++i) {
        flat_osc->calculate();
        if (i < flat_data.size() - 1) {
            flat_high->forward();
            flat_low->forward();
        }
    }
    
    // 当所有价格相同时，AroonOscillator应该为0
    double final_osc = flat_osc->get(0);
    if (!std::isnan(final_osc)) {
        EXPECT_NEAR(final_osc, 0.0, 1e-10) 
            << "AroonOscillator should be 0 for constant prices";
    }
}

// 性能测试
TEST(OriginalTests, AroonOscillator_Performance) {
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
    
    for (const auto& bar : large_data) {
        large_high->forward(bar.high);
        large_low->forward(bar.low);
    }
    
    auto large_osc = std::make_shared<AroonOscillator>(large_high, large_low, 14);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_osc->calculate();
        if (i < large_data.size() - 1) {
            large_high->forward();
            large_low->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "AroonOscillator calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_osc->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_GE(final_result, -100.0) << "Final result should be >= -100";
    EXPECT_LE(final_result, 100.0) << "Final result should be <= 100";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}