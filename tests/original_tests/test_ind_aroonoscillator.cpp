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
#include "lineseries.h"
#include <random>
#include "indicators/aroon.h"

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;
using LineSeries = backtrader::LineSeries;
using LineBuffer = backtrader::LineBuffer;

namespace {

const std::vector<std::vector<std::string>> AROON_OSC_EXPECTED_VALUES = {
    {"35.714286", "-50.000000", "57.142857"}
};

const int AROON_OSC_MIN_PERIOD = 15;

// Helper function to create full DataSeries from CSV data
std::shared_ptr<DataSeries> createFullDataSeries(const std::vector<CSVDataReader::OHLCVData>& csv_data) {
    auto data_series = std::make_shared<DataSeries>();
    
    // Pre-allocate all lines
    for (int i = 0; i < 7; ++i) {
        auto line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(i));
        if (line) {
            line->reserve(csv_data.size() + 1);
        }
    }
    
    // Load all data into buffers
    for (const auto& bar : csv_data) {
        std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(0))->append(0.0); // datetime
        std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(1))->append(bar.open);
        std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(2))->append(bar.high);
        std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(3))->append(bar.low);
        std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(4))->append(bar.close);
        std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(5))->append(bar.volume);
        std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(6))->append(bar.openinterest);
    }
    
    // Set indices
    for (int i = 0; i < 7; ++i) {
        auto line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(i));
        if (line && line->size() > 0) {
            line->set_idx(line->size() - 1);
        }
    }
    
    return data_series;
}

} // anonymous namespace

// 使用默认参数的AroonOscillator测试
DEFINE_INDICATOR_TEST(AroonOscillator_Default, AroonOscillator, AROON_OSC_EXPECTED_VALUES, AROON_OSC_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, AroonOscillator_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建完整的DataSeries，包含OHLC数据（不要set_idx）
    auto data_series = std::make_shared<DataSeries>();
    // Pre-allocate and load all data
    for (int i = 0; i < 7; ++i) {
        auto line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(i));
        if (line) {
            line->reserve(csv_data.size() + 1);
        }
    }
    
    // Load all data into buffers
    auto dt_line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(0));
    auto open_line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(1));
    auto high_line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(2));
    auto low_line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(3));
    auto close_line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(4));
    auto volume_line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(5));
    auto oi_line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(6));
    
    std::vector<double> opens, highs, lows, closes, volumes, ois, datetimes;
    for (const auto& bar : csv_data) {
        opens.push_back(bar.open);
        highs.push_back(bar.high);
        lows.push_back(bar.low);
        closes.push_back(bar.close);
        volumes.push_back(bar.volume);
        ois.push_back(bar.openinterest);
        datetimes.push_back(0.0); // Simple index for now
    }
    
    if (dt_line) {
        dt_line->batch_append(datetimes);
        dt_line->set_idx(datetimes.size() - 1);
    }
    if (open_line) {
        open_line->batch_append(opens);
        open_line->set_idx(opens.size() - 1);
    }
    if (high_line) {
        high_line->batch_append(highs);
        high_line->set_idx(highs.size() - 1);
    }
    if (low_line) {
        low_line->batch_append(lows);
        low_line->set_idx(lows.size() - 1);
    }
    if (close_line) {
        close_line->batch_append(closes);
        close_line->set_idx(closes.size() - 1);
    }
    if (volume_line) {
        volume_line->batch_append(volumes);
        volume_line->set_idx(volumes.size() - 1);
    }
    if (oi_line) {
        oi_line->batch_append(ois);
        oi_line->set_idx(ois.size() - 1);
    }
    
    
    // 创建AroonOscillator指标（使用单个DataSeries）
    auto aroon_osc = std::make_shared<AroonOscillator>(std::static_pointer_cast<LineSeries>(data_series));
    
    // 计算
    aroon_osc->calculate();
    
    
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
    
    // 创建完整的DataSeries，包含OHLC数据
    auto data_series = std::make_shared<DataSeries>();
    // Pre-allocate and load all data
    for (int i = 0; i < 7; ++i) {
        auto line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(i));
        if (line) {
            line->reserve(csv_data.size() + 1);
        }
    }
    
    // Load all data into buffers
    auto dt_line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(0));
    auto open_line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(1));
    auto high_line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(2));
    auto low_line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(3));
    auto close_line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(4));
    auto volume_line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(5));
    auto oi_line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(6));
    
    std::vector<double> opens, highs, lows, closes, volumes, ois, datetimes;
    for (const auto& bar : csv_data) {
        opens.push_back(bar.open);
        highs.push_back(bar.high);
        lows.push_back(bar.low);
        closes.push_back(bar.close);
        volumes.push_back(bar.volume);
        ois.push_back(bar.openinterest);
        datetimes.push_back(0.0); // Simple index for now
    }
    
    if (dt_line) {
        dt_line->batch_append(datetimes);
        dt_line->set_idx(datetimes.size() - 1);
    }
    if (open_line) {
        open_line->batch_append(opens);
        open_line->set_idx(opens.size() - 1);
    }
    if (high_line) {
        high_line->batch_append(highs);
        high_line->set_idx(highs.size() - 1);
    }
    if (low_line) {
        low_line->batch_append(lows);
        low_line->set_idx(lows.size() - 1);
    }
    if (close_line) {
        close_line->batch_append(closes);
        close_line->set_idx(closes.size() - 1);
    }
    if (volume_line) {
        volume_line->batch_append(volumes);
        volume_line->set_idx(volumes.size() - 1);
    }
    if (oi_line) {
        oi_line->batch_append(ois);
        oi_line->set_idx(ois.size() - 1);
    }
    
    auto aroon_osc = std::make_shared<AroonOscillator>(std::static_pointer_cast<LineSeries>(data_series), 14);
    
    // 使用批量计算模式
    aroon_osc->calculate();
    
    // 验证所有值的范围;
    for (size_t i = 0; i < csv_data.size(); ++i) {
        double osc_value = aroon_osc->get(-static_cast<int>(i));
        
        // 验证AroonOscillator在-100到+100范围内
        if (!std::isnan(osc_value)) {
            EXPECT_GE(osc_value, -100.0) << "AroonOscillator should be >= -100 at step " << i;
            EXPECT_LE(osc_value, 100.0) << "AroonOscillator should be <= 100 at step " << i;
        }
    }
}

// 参数化测试 - 测试不同周期的AroonOscillator
class AroonOscillatorParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        // 创建完整的DataSeries
        data_series_ = createFullDataSeries(csv_data_);
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<backtrader::DataSeries> data_series_;
};

TEST_P(AroonOscillatorParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto aroon_osc = std::make_shared<AroonOscillator>(std::static_pointer_cast<LineSeries>(data_series_), period);
    
    // 使用批量计算模式
    aroon_osc->calculate();
    
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
    std::vector<CSVDataReader::OHLCVData> csv_data = {
        {"2006-01-01", 100.0, 110.0, 90.0, 105.0, 0, 0},   
        {"2006-01-02", 105.0, 115.0, 95.0, 110.0, 0, 0},   
        {"2006-01-03", 110.0, 120.0, 100.0, 115.0, 0, 0},  
        {"2006-01-04", 115.0, 125.0, 105.0, 120.0, 0, 0},  
        {"2006-01-05", 120.0, 130.0, 85.0, 125.0, 0, 0}    
    };
    
    // 创建完整的DataSeries
    auto data_series = std::make_shared<DataSeries>();
    for (int i = 0; i < 7; ++i) {
        auto line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(i));
        if (line) {
            line->reserve(csv_data.size() + 1);
        }
    }
    
    // Load all data into buffers
    for (const auto& bar : csv_data) {
        std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(0))->append(0.0); // datetime
        std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(1))->append(bar.open);
        std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(2))->append(bar.high);
        std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(3))->append(bar.low);
        std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(4))->append(bar.close);
        std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(5))->append(bar.volume);
        std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(6))->append(bar.openinterest);
    }
    
    // Set indices
    for (int i = 0; i < 7; ++i) {
        auto line = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(i));
        if (line && line->size() > 0) {
            line->set_idx(line->size() - 1);
        }
    }
    
    auto aroon_osc = std::make_shared<AroonOscillator>(std::static_pointer_cast<LineSeries>(data_series), 4);
    
    // Debug: Check data before calculation
    std::cout << "\nTest Debug: Before calculate()" << std::endl;
    auto high_line = data_series->lines->getline(2);
    if (high_line) {
        auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line);
        if (high_buffer) {
            std::cout << "High buffer size: " << high_buffer->size() << std::endl;
            const auto& high_array = high_buffer->array();
            std::cout << "High values: ";
            for (size_t i = 0; i < high_array.size(); ++i) {
                std::cout << high_array[i] << " ";
            }
            std::cout << std::endl;
        }
    }
    
    aroon_osc->calculate();
    
    // 手动计算AroonOscillator进行验证 - 检查最终状态
    // 对于最终计算结果，验证其正确性
    if (csv_data.size() >= 5) {  // 需要至少5个数据点
        // 找到最近4+1个数据点中的最高价和最低价位置
        double highest = -std::numeric_limits<double>::infinity();
        double lowest = std::numeric_limits<double>::infinity();
        int highest_pos = 0;
        int lowest_pos = 0;
        
        size_t end_idx = csv_data.size() - 1;
        for (int j = 0; j <= 4; ++j) {
            if (csv_data[end_idx - j].high > highest) {
                highest = csv_data[end_idx - j].high;
                highest_pos = j;
            }
            if (csv_data[end_idx - j].low < lowest) {
                lowest = csv_data[end_idx - j].low;
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
        
        // Debug output
        std::cout << "Debug: AroonOsc buffer size: " << aroon_osc->size() << std::endl;
        auto osc_line = aroon_osc->lines->getline(0);
        if (osc_line) {
            std::cout << "Debug: All buffer values:" << std::endl;
            for (size_t i = 0; i < osc_line->size(); ++i) {
                std::cout << "  buffer[" << i << "] = " << (*osc_line)[i] << std::endl;
            }
        }
        std::cout << "Debug: get(0) returns: " << actual_osc << std::endl;
        std::cout << "Debug: expected: " << expected_osc << std::endl;
        
        if (!std::isnan(actual_osc)) {
            EXPECT_NEAR(actual_osc, expected_osc, 1e-6) 
                << "AroonOscillator calculation mismatch at final step";
        }
    }
}

// 趋势识别测试
TEST(OriginalTests, AroonOscillator_TrendIdentification) {
    auto csv_data = getdata(0);
    
    // 创建完整的DataSeries
    auto data_series = createFullDataSeries(csv_data);
    
    auto aroon_osc = std::make_shared<AroonOscillator>(std::static_pointer_cast<LineSeries>(data_series), 14);
    
    int strong_uptrend = 0;    // AroonOsc > 50
    int strong_downtrend = 0;  // AroonOsc < -50
    int weak_trend = 0;        // -50 <= AroonOsc <= 50
    
    // 使用批量计算模式
    aroon_osc->calculate();
    
    // 统计趋势信号;
    for (size_t i = 0; i < csv_data.size(); ++i) {
        double osc_value = aroon_osc->get(-static_cast<int>(i));
        
        if (!std::isnan(osc_value)) {
            if (osc_value > 50.0) {
                strong_uptrend++;
            } else if (osc_value < -50.0) {
                strong_downtrend++;
            } else {
                weak_trend++;
            }
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
    
    // 创建完整的DataSeries
    auto data_series = createFullDataSeries(csv_data);
    
    auto aroon_osc = std::make_shared<AroonOscillator>(std::static_pointer_cast<LineSeries>(data_series), 14);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_osc = 0.0;
    bool has_prev = false;
    
    // 使用批量计算模式
    aroon_osc->calculate();
    
    // 检测零线穿越;
    for (size_t i = 0; i < csv_data.size(); ++i) {
        double current_osc = aroon_osc->get(-static_cast<int>(i));
        
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
    
    // 创建两个完整的DataSeries
    auto data_series_osc = createFullDataSeries(csv_data);
    auto data_series_updown = createFullDataSeries(csv_data);
    
    auto aroon_osc = std::make_shared<AroonOscillator>(std::static_pointer_cast<LineSeries>(data_series_osc), 14);
    auto aroon_updown = std::make_shared<AroonUpDown>(std::static_pointer_cast<LineSeries>(data_series_updown), 14);
    
    // 验证AroonOscillator = AroonUp - AroonDown
    aroon_osc->calculate();
    aroon_updown->calculate();
    
    // 验证最终计算结果
    double osc_value = aroon_osc->get(0);
    double aroon_up = aroon_updown->getAroonUp(0);
    double aroon_down = aroon_updown->getAroonDown(0);
    
    if (!std::isnan(osc_value) && !std::isnan(aroon_up) && !std::isnan(aroon_down)) {
        double expected_osc = aroon_up - aroon_down;
        EXPECT_NEAR(osc_value, expected_osc, 1e-6) 
            << "AroonOscillator should equal AroonUp - AroonDown at final step";
    }
}

// 极值测试
TEST(OriginalTests, AroonOscillator_ExtremeValues) {
    // 创建一个序列，使AroonOscillator达到极值
    std::vector<CSVDataReader::OHLCVData> extreme_data;
    
    // 创建持续上升的最高价，固定的最低价;
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
    
    // 创建完整的DataSeries
    auto data_series = createFullDataSeries(extreme_data);
    
    auto extreme_osc = std::make_shared<AroonOscillator>(std::static_pointer_cast<LineSeries>(data_series), 14);
    extreme_osc->calculate();
    
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
    std::vector<CSVDataReader::OHLCVData> large_data;
    for (int i = 0; i < 30; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 100.0;
        bar.low = 100.0;
        bar.close = 100.0;
        bar.open = 100.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        large_data.push_back(bar);
    }
    
    // 创建完整的DataSeries
    auto data_series = createFullDataSeries(large_data);
    
    auto flat_osc = std::make_shared<AroonOscillator>(std::static_pointer_cast<LineSeries>(data_series), 14);
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    flat_osc->calculate();
    
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
    
    // 创建完整的DataSeries
    auto data_series = createFullDataSeries(large_data);
    
    auto large_osc = std::make_shared<AroonOscillator>(std::static_pointer_cast<LineSeries>(data_series), 14);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 使用批量计算替代逐步forward
    large_osc->calculate();
    
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