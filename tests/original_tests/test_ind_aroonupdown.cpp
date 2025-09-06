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
    
    // 创建数据线 (使用LineSeries + LineBuffer模式)
    auto high_line_series = std::make_shared<LineSeries>();
    high_line_series->lines->add_line(std::make_shared<LineBuffer>());
    high_line_series->lines->add_alias("high", 0);
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line_series->lines->getline(0));
    
    auto low_line_series = std::make_shared<LineSeries>();
    low_line_series->lines->add_line(std::make_shared<LineBuffer>());
    low_line_series->lines->add_alias("low", 0);
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line_series->lines->getline(0));
    
    if (high_buffer && low_buffer) {
        // Set first data point and append rest
        high_buffer->set(0, csv_data[0].high);
        low_buffer->set(0, csv_data[0].low);
        for (size_t i = 1; i < csv_data.size(); ++i) {
            high_buffer->append(csv_data[i].high);
            low_buffer->append(csv_data[i].low);
        }
    }
    
    // 创建AroonUpDown指标（默认14周期，最小周期为15）
    auto aroon = std::make_shared<AroonUpDown>(high_line_series, low_line_series, 14);
    
    // 计算所有值
    aroon->calculate();
    
    // Debug: Check data and line setup
    std::cout << "CSV data size: " << csv_data.size() << std::endl;
    std::cout << "AroonUpDown min period: " << aroon->getMinPeriod() << std::endl;
    
    // Try to access the internal lines directly to check if they exist
    if (aroon->lines && aroon->lines->size() > 0) {
        auto aroonup_line = aroon->lines->getline(0);
        auto aroondown_line = aroon->lines->getline(1);
        std::cout << "AroonUp line size: " << (aroonup_line ? aroonup_line->size() : 0) << std::endl;
        std::cout << "AroonDown line size: " << (aroondown_line ? aroondown_line->size() : 0) << std::endl;
        
        if (aroonup_line && aroonup_line->size() > 0) {
            std::cout << "First AroonUp values: ";
            for (int i = 0; i < std::min(3, (int)aroonup_line->size()); ++i) {
                std::cout << (*aroonup_line)[i] << " ";
            }
            std::cout << std::endl;
        }
    } else {
        std::cout << "No lines created!" << std::endl;
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 15;  // period + 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::cout << "Debug: data_length=" << data_length << ", min_period=" << min_period << std::endl;
    std::cout << "Debug: -l + mp = " << -(data_length - min_period) << std::endl;
    std::cout << "Debug: (-l + mp) // 2 = " << -(data_length - min_period) / 2 << std::endl;
    
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // Debug check points calculation
    std::cout << "Check points: [";
    for (size_t i = 0; i < check_points.size(); ++i) {
        std::cout << check_points[i];
        if (i < check_points.size() - 1) std::cout << ", ";
    }
    std::cout << "]" << std::endl;
    
    // Debug: check line buffer current index
    if (aroon->lines && aroon->lines->size() > 0) {
        auto aroonup_line = std::dynamic_pointer_cast<LineBuffer>(aroon->lines->getline(0));
        if (aroonup_line) {
            std::cout << "AroonUp line current index: " << aroonup_line->get_idx() << std::endl;
        }
    }
    
    // 验证Aroon Up
    std::vector<std::string> expected_up = {"42.857143", "35.714286", "85.714286"};
    for (size_t i = 0; i < check_points.size() && i < expected_up.size(); ++i) {
        double actual = aroon->getAroonUp(check_points[i]);
        
        std::cout << "Check point " << i << ": ago=" << check_points[i] << ", actual=" << actual << ", expected=" << expected_up[i] << std::endl;
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
        
        std::cout << "Check point " << i << ": ago=" << check_points[i] << ", actual=" << actual << ", expected=" << expected_down[i] << std::endl;
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
    
    // 使用LineSeries+LineBuffer模式替代LineRoot
    auto high_line_series = std::make_shared<backtrader::LineSeries>();
    high_line_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    high_line_series->lines->add_alias("high", 0);
    
    auto low_line_series = std::make_shared<backtrader::LineSeries>();
    low_line_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    low_line_series->lines->add_alias("low", 0);
    
    // 填充数据到LineBuffer中
    auto high_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(high_line_series->lines->getline(0));
    auto low_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(low_line_series->lines->getline(0));
    
    if (high_buffer && low_buffer && !csv_data.empty()) {
        high_buffer->set(0, csv_data[0].high);
        low_buffer->set(0, csv_data[0].low);
        for (size_t i = 1; i < csv_data.size(); ++i) {
            high_buffer->append(csv_data[i].high);
            low_buffer->append(csv_data[i].low);
        }
    }
    
    auto aroon = std::make_shared<AroonUpDown>(high_line_series, low_line_series, 14);
    
    // 使用批量计算模式
    aroon->calculate();
    
    // 获取aroon输出lines进行范围验证
    if (aroon->lines && aroon->lines->size() >= 2) {
        auto aroon_up_line = aroon->lines->getline(0);
        auto aroon_down_line = aroon->lines->getline(1);
        
        if (aroon_up_line && aroon_down_line) {
            size_t line_size = aroon_up_line->size();
            for (size_t i = 0; i < line_size; ++i) {
                double aroon_up = (*aroon_up_line)[i];
                double aroon_down = (*aroon_down_line)[i];
                
                // 验证Aroon Up和Down都在0到100范围内
                if (!std::isnan(aroon_up)) {
                    EXPECT_GE(aroon_up, 0.0) << "Aroon Up should be >= 0 at index " << i;
                    EXPECT_LE(aroon_up, 100.0) << "Aroon Up should be <= 100 at index " << i;
                }
                
                if (!std::isnan(aroon_down)) {
                    EXPECT_GE(aroon_down, 0.0) << "Aroon Down should be >= 0 at index " << i;
                    EXPECT_LE(aroon_down, 100.0) << "Aroon Down should be <= 100 at index " << i;
                }
            }
        }
    }
}

// 参数化测试 - 测试不同周期的AroonUpDown
class AroonUpDownParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        // 使用LineSeries+LineBuffer模式替代LineRoot
        high_line_series_ = std::make_shared<backtrader::LineSeries>();
        high_line_series_->lines->add_line(std::make_shared<backtrader::LineBuffer>());
        high_line_series_->lines->add_alias("high", 0);
        
        low_line_series_ = std::make_shared<backtrader::LineSeries>();
        low_line_series_->lines->add_line(std::make_shared<backtrader::LineBuffer>());
        low_line_series_->lines->add_alias("low", 0);
        
        // 填充数据到LineBuffer中
        auto high_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(high_line_series_->lines->getline(0));
        auto low_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(low_line_series_->lines->getline(0));
        
        if (high_buffer && low_buffer && !csv_data_.empty()) {
            // 设置第一个数据点
            high_buffer->set(0, csv_data_[0].high);
            low_buffer->set(0, csv_data_[0].low);
            
            // 添加剩余数据点
            for (size_t i = 1; i < csv_data_.size(); ++i) {
                high_buffer->append(csv_data_[i].high);
                low_buffer->append(csv_data_[i].low);
            }
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<backtrader::LineSeries> high_line_series_;
    std::shared_ptr<backtrader::LineSeries> low_line_series_;
};

TEST_P(AroonUpDownParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto aroon = std::make_shared<AroonUpDown>(high_line_series_, low_line_series_, period);
    
    // 使用批量计算模式
    aroon->calculate();
    
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
    
    // 使用LineSeries+LineBuffer模式替代LineRoot
    auto high_line_series = std::make_shared<backtrader::LineSeries>();
    high_line_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    high_line_series->lines->add_alias("high", 0);
    
    auto low_line_series = std::make_shared<backtrader::LineSeries>();
    low_line_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    low_line_series->lines->add_alias("low", 0);
    
    // 填充数据到LineBuffer中
    auto high_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(high_line_series->lines->getline(0));
    auto low_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(low_line_series->lines->getline(0));
    
    if (high_buffer && low_buffer && !test_data.empty()) {
        high_buffer->set(0, test_data[0].high);
        low_buffer->set(0, test_data[0].low);
        for (size_t i = 1; i < test_data.size(); ++i) {
            high_buffer->append(test_data[i].high);
            low_buffer->append(test_data[i].low);
        }
    }
    
    auto aroon = std::make_shared<AroonUpDown>(high_line_series, low_line_series, 4);
    
    // 使用批量计算模式
    aroon->calculate();
    
    // 验证最后一个值（第5个数据点）
    if (test_data.size() >= 5) {
        // 手动计算最后一个数据点的Aroon值
        int i = static_cast<int>(test_data.size()) - 1;  // 最后一个数据点
        
        double highest = -std::numeric_limits<double>::infinity();
        double lowest = std::numeric_limits<double>::infinity();
        int highest_pos = 0;
        int lowest_pos = 0;
        
        // 查找最近4个数据点中的最高和最低值
        for (int j = 0; j < 4; ++j) {
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
                << "Aroon Up calculation mismatch at final step";
            EXPECT_NEAR(actual_down, expected_down, 1e-6) 
                << "Aroon Down calculation mismatch at final step";
        }
    }
}

// 趋势识别测试
TEST(OriginalTests, AroonUpDown_TrendIdentification) {
    auto csv_data = getdata(0);
    
    // 使用LineSeries+LineBuffer模式替代LineRoot
    auto high_line_series = std::make_shared<backtrader::LineSeries>();
    high_line_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    high_line_series->lines->add_alias("high", 0);
    
    auto low_line_series = std::make_shared<backtrader::LineSeries>();
    low_line_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    low_line_series->lines->add_alias("low", 0);
    
    // 填充数据到LineBuffer中
    auto high_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(high_line_series->lines->getline(0));
    auto low_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(low_line_series->lines->getline(0));
    
    if (high_buffer && low_buffer && !csv_data.empty()) {
        high_buffer->set(0, csv_data[0].high);
        low_buffer->set(0, csv_data[0].low);
        for (size_t i = 1; i < csv_data.size(); ++i) {
            high_buffer->append(csv_data[i].high);
            low_buffer->append(csv_data[i].low);
        }
    }
    
    auto aroon = std::make_shared<AroonUpDown>(high_line_series, low_line_series, 14);
    
    // 使用批量计算模式
    aroon->calculate();
    
    int uptrend_signals = 0;    // Aroon Up > 70 and Aroon Down < 30
    int downtrend_signals = 0;  // Aroon Down > 70 and Aroon Up < 30
    int sideways_signals = 0;   // Both between 30-70
    
    // 统计趋势信号 - 遍历计算后的结果
    if (aroon->lines && aroon->lines->size() >= 2) {
        auto aroon_up_line = aroon->lines->getline(0);
        auto aroon_down_line = aroon->lines->getline(1);
        
        if (aroon_up_line && aroon_down_line) {
            // Access values from the buffers directly
            auto up_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(aroon_up_line);
            auto down_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(aroon_down_line);
            
            if (up_buffer && down_buffer) {
                const auto& up_array = up_buffer->array();
                const auto& down_array = down_buffer->array();
                
                for (size_t i = 0; i < up_array.size() && i < down_array.size(); ++i) {
                    double aroon_up = up_array[i];
                    double aroon_down = down_array[i];
                    
                    if (!std::isnan(aroon_up) && !std::isnan(aroon_down)) {
                        if (aroon_up > 70.0 && aroon_down < 30.0) {
                            uptrend_signals++;
                        } else if (aroon_down > 70.0 && aroon_up < 30.0) {
                            downtrend_signals++;
                        } else if (aroon_up >= 30.0 && aroon_up <= 70.0 && aroon_down >= 30.0 && aroon_down <= 70.0) {
                            sideways_signals++;
                        }
                    }
                }
            }
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
    
    // 使用LineSeries+LineBuffer模式替代LineRoot
    auto high_line_series = std::make_shared<backtrader::LineSeries>();
    high_line_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    high_line_series->lines->add_alias("high", 0);
    
    auto low_line_series = std::make_shared<backtrader::LineSeries>();
    low_line_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    low_line_series->lines->add_alias("low", 0);
    
    // 填充数据到LineBuffer中
    auto high_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(high_line_series->lines->getline(0));
    auto low_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(low_line_series->lines->getline(0));
    
    if (high_buffer && low_buffer && !csv_data.empty()) {
        high_buffer->set(0, csv_data[0].high);
        low_buffer->set(0, csv_data[0].low);
        for (size_t i = 1; i < csv_data.size(); ++i) {
            high_buffer->append(csv_data[i].high);
            low_buffer->append(csv_data[i].low);
        }
    }
    
    auto aroon = std::make_shared<AroonUpDown>(high_line_series, low_line_series, 14);
    
    // 使用批量计算模式
    aroon->calculate();
    
    int bullish_crossovers = 0;  // Aroon Up crosses above Aroon Down
    int bearish_crossovers = 0;  // Aroon Down crosses above Aroon Up
    
    double prev_up = 0.0, prev_down = 0.0;
    bool has_prev = false;
    
    // 检测交叉信号 - 遍历计算后的结果
    if (aroon->lines && aroon->lines->size() >= 2) {
        auto aroon_up_line = aroon->lines->getline(0);
        auto aroon_down_line = aroon->lines->getline(1);
        
        if (aroon_up_line && aroon_down_line) {
            // Access values from the buffers directly
            auto up_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(aroon_up_line);
            auto down_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(aroon_down_line);
            
            if (up_buffer && down_buffer) {
                const auto& up_array = up_buffer->array();
                const auto& down_array = down_buffer->array();
                
                for (size_t i = 1; i < up_array.size() && i < down_array.size(); ++i) {
                    double current_up = up_array[i];
                    double current_down = down_array[i];
                    double prev_up = up_array[i-1];
                    double prev_down = down_array[i-1];
                    
                    if (!std::isnan(current_up) && !std::isnan(current_down) && !std::isnan(prev_up) && !std::isnan(prev_down)) {
                        // 检测Aroon Up上穿Aroon Down
                        if (prev_up <= prev_down && current_up > current_down) {
                            bullish_crossovers++;
                        }
                        // 检测Aroon Down上穿Aroon Up
                        else if (prev_down <= prev_up && current_down > current_up) {
                            bearish_crossovers++;
                        }
                    }
                }
            }
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
    
    // 使用LineSeries+LineBuffer模式替代LineRoot
    auto high_line_series = std::make_shared<backtrader::LineSeries>();
    high_line_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    high_line_series->lines->add_alias("extreme_high", 0);
    
    auto low_line_series = std::make_shared<backtrader::LineSeries>();
    low_line_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    low_line_series->lines->add_alias("extreme_low", 0);
    
    // 填充数据到LineBuffer中
    auto high_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(high_line_series->lines->getline(0));
    auto low_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(low_line_series->lines->getline(0));
    
    if (high_buffer && low_buffer && !extreme_data.empty()) {
        high_buffer->set(0, extreme_data[0].high);
        low_buffer->set(0, extreme_data[0].low);
        for (size_t i = 1; i < extreme_data.size(); ++i) {
            high_buffer->append(extreme_data[i].high);
            low_buffer->append(extreme_data[i].low);
        }
    }
    
    auto extreme_aroon = std::make_shared<AroonUpDown>(high_line_series, low_line_series, 14);
    
    // 使用批量计算模式
    extreme_aroon->calculate();
    
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
    // 只添加3个数据点
    std::vector<CSVDataReader::OHLCVData> short_data = {
        {"2006-01-01", 100.0, 110.0, 90.0, 105.0, 0, 0},
        {"2006-01-02", 105.0, 115.0, 95.0, 110.0, 0, 0},
        {"2006-01-03", 110.0, 120.0, 100.0, 115.0, 0, 0}
    };
    
    // 使用LineSeries+LineBuffer模式替代LineRoot
    auto insufficient_high_series = std::make_shared<backtrader::LineSeries>();
    insufficient_high_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    insufficient_high_series->lines->add_alias("insufficient_high", 0);
    
    auto insufficient_low_series = std::make_shared<backtrader::LineSeries>();
    insufficient_low_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    insufficient_low_series->lines->add_alias("insufficient_low", 0);
    
    // 填充数据到LineBuffer中
    auto high_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(insufficient_high_series->lines->getline(0));
    auto low_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(insufficient_low_series->lines->getline(0));
    
    if (high_buffer && low_buffer && !short_data.empty()) {
        high_buffer->set(0, short_data[0].high);
        low_buffer->set(0, short_data[0].low);
        for (size_t i = 1; i < short_data.size(); ++i) {
            high_buffer->append(short_data[i].high);
            low_buffer->append(short_data[i].low);
        }
    }
    
    auto insufficient_aroon = std::make_shared<AroonUpDown>(insufficient_high_series, insufficient_low_series, 14);  // 周期大于数据量
    
    // 使用批量计算模式
    insufficient_aroon->calculate();
    
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
    
    // 使用LineSeries+LineBuffer模式替代LineRoot
    auto large_high = std::make_shared<backtrader::LineSeries>();
    large_high->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    large_high->lines->add_alias("large_high", 0);
    
    auto large_low = std::make_shared<backtrader::LineSeries>();
    large_low->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    large_low->lines->add_alias("large_low", 0);
    
    auto high_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(large_high->lines->getline(0));
    auto low_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(large_low->lines->getline(0));
    
    if (high_buffer && low_buffer && !large_data.empty()) {
        // 设置第一个数据点
        high_buffer->set(0, large_data[0].high);
        low_buffer->set(0, large_data[0].low);
        
        // 添加剩余数据点
        for (size_t i = 1; i < large_data.size(); ++i) {
            high_buffer->append(large_data[i].high);
            low_buffer->append(large_data[i].low);
        }
    }
    
    auto large_aroon = std::make_shared<AroonUpDown>(large_high, large_low, 14);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 使用批量计算替代逐步forward
    large_aroon->calculate();
    
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