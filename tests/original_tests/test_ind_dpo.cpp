/**
 * @file test_ind_dpo.cpp
 * @brief DPO指标测试 - 对应Python test_ind_dpo.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['83.271000', '105.625000', '1.187000']
 * ]
 * chkmin = 29
 * chkind = btind.DPO
 */

#include "test_common.h"
#include "lineseries.h"
#include <random>
#include <cmath>

#include "indicators/dpo.h"
#include "indicators/sma.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> DPO_EXPECTED_VALUES = {
    {"83.271000", "105.625000", "1.187000"}
};

const int DPO_MIN_PERIOD = 29;

} // anonymous namespace

// 使用默认参数的DPO测试
// DPO can use DEFINE_INDICATOR_TEST as it has a DataSeries constructor
DEFINE_INDICATOR_TEST(DPO_Default, DPO, DPO_EXPECTED_VALUES, DPO_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, DPO_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线 - LineSeries + LineBuffer pattern
    auto close_linewrapper = std::make_shared<LineSeries>();
    close_linewrapper->lines->add_line(std::make_shared<LineBuffer>());
    close_linewrapper->lines->add_alias("close", 0);
    auto close_line = std::dynamic_pointer_cast<LineBuffer>(close_linewrapper->lines->getline(0));
    
    // Initialize the line buffer with data
    if (close_line) {
        // Reset to remove the initial NaN value
        close_line->reset();
        for (const auto& bar : csv_data) {
            close_line->append(bar.close);
        }
        // Set the index to the last element to make it current
        close_line->set_idx(close_line->size() - 1);
        
    }
    
    // 创建DPO指标（默认20周期）
    auto dpo = std::make_shared<DPO>(close_linewrapper, 20);
    
    // Calculate DPO value for all data points at once
    dpo->calculate();
    
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 29;  // period=20: 20 + (20/2) - 1 = 29
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    // The Python test uses negative indices where l is data length (not DPO size)
    int l = data_length;  // Use original data length like Python, not DPO size
    int second_idx = -l + min_period;  // -256 + 29 = -227
    // Python floor division for negative numbers
    int third_idx = static_cast<int>(std::floor(static_cast<double>(second_idx) / 2.0));  // -227 // 2 = -114
    std::vector<int> check_points = {
        0,                           // Most recent value  
        second_idx,                  // -227
        third_idx                    // -114
    };
    
    std::vector<std::string> expected = {"83.271000", "105.625000", "1.187000"};
    
    
    // Use negative indices as Python does
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = dpo->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "DPO value mismatch at check point " << i 
            << " (index=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(dpo->getMinPeriod(), 29) << "DPO minimum period should be 29";
}

// 参数化测试 - 测试不同周期的DPO
class DPOParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line_wrapper = std::make_shared<LineSeries>();
        close_line_wrapper->lines->add_line(std::make_shared<LineBuffer>());
        close_line_wrapper->lines->add_alias("data", 0);
        auto close_line = std::dynamic_pointer_cast<LineBuffer>(close_line_wrapper->lines->getline(0));
        
        if (close_line) {
            close_line->set(0, csv_data_[0].close);
            for (size_t i = 1; i < csv_data_.size(); ++i) {
                close_line->append(csv_data_[i].close);
            }
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<backtrader::LineBuffer> close_line;
    std::shared_ptr<LineSeries> close_line_wrapper;
};

TEST_P(DPOParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto dpo = std::make_shared<DPO>(close_line_wrapper, period);
    
    // 计算所有值;
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    dpo->calculate();
    
    // 验证最小周期
    int expected_min_period = period + (period / 2) - 1;
    EXPECT_EQ(dpo->getMinPeriod(), expected_min_period) 
        << "DPO minimum period calculation for period " << period;
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_value = dpo->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last DPO value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last DPO value should be finite";
    }
}

// 测试不同的DPO周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    DPOParameterizedTest,
    ::testing::Values(10, 14, 20, 30)
);

// DPO去趋势效果测试
TEST(OriginalTests, DPO_DetrendingEffect) {
    // 创建有明显趋势的数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 100; ++i) {
        // 线性上升趋势加上周期性波动
        double trend = i * 2.0;
        double cycle = 10.0 * std::sin(i * 0.2);
        trend_prices.push_back(100.0 + trend + cycle);
    }
    auto trend_line = std::make_shared<LineSeries>();
    trend_line->lines->add_line(std::make_shared<LineBuffer>());
    trend_line->lines->add_alias("trend", 0);
    auto trend_buffer = std::dynamic_pointer_cast<LineBuffer>(trend_line->lines->getline(0));
    if (trend_buffer) {
        trend_buffer->set(0, trend_prices[0]);
        for (size_t i = 1; i < trend_prices.size(); ++i) {
            trend_buffer->append(trend_prices[i]);
        }
    }
    
    auto dpo = std::make_shared<DPO>(trend_line, 20);
    
    // 修复性能：单次计算替代O(n²)循环
    dpo->calculate();
    
    std::vector<double> dpo_values;
    std::vector<double> price_values;
    
    // 收集计算结果
    for (size_t i = 0; i < dpo->size(); ++i) {
        double dpo_val = dpo->get(-static_cast<int>(i));
        if (!std::isnan(dpo_val)) {
            dpo_values.push_back(dpo_val);
            if (i < trend_prices.size()) {
                price_values.push_back(trend_prices[trend_prices.size() - 1 - i]);
            }
        }
    }
    
    // 比较原始价格和DPO的趋势
    if (dpo_values.size() > 20 && price_values.size() > 20) {
        // 计算价格的趋势（最后10个减去前10个的平均值）
        double early_price = std::accumulate(price_values.begin(), price_values.begin() + 10, 0.0) / 10.0;
        double late_price = std::accumulate(price_values.end() - 10, price_values.end(), 0.0) / 10.0;
        double price_trend = late_price - early_price;
        
        // 计算DPO的趋势
        double early_dpo = std::accumulate(dpo_values.begin(), dpo_values.begin() + 10, 0.0) / 10.0;
        double late_dpo = std::accumulate(dpo_values.end() - 10, dpo_values.end(), 0.0) / 10.0;
        double dpo_trend = late_dpo - early_dpo;
        
        std::cout << "Detrending effect:" << std::endl;
        std::cout << "Price trend: " << price_trend << std::endl;
        std::cout << "DPO trend: " << dpo_trend << std::endl;
        
        // DPO应该去除趋势，使其趋势变化远小于原始价格
        EXPECT_LT(std::abs(dpo_trend), std::abs(price_trend) * 0.5) 
            << "DPO should remove trend from price data";
    }
}

// DPO周期识别测试
TEST(OriginalTests, DPO_CycleIdentification) {
    // 创建纯周期性数据（无趋势）
    std::vector<double> cycle_prices;
    for (int i = 0; i < 200; ++i) {
        double cycle = 15.0 * std::sin(i * 2.0 * M_PI / 40.0);  // 40个点为一个周期
        cycle_prices.push_back(100.0 + cycle);
    }
    auto cycle_line = std::make_shared<LineSeries>();
    cycle_line->lines->add_line(std::make_shared<LineBuffer>());
    cycle_line->lines->add_alias("cycle", 0);
    auto cycle_buffer = std::dynamic_pointer_cast<LineBuffer>(cycle_line->lines->getline(0));
    if (cycle_buffer) {
        cycle_buffer->set(0, cycle_prices[0]);
        for (size_t i = 1; i < cycle_prices.size(); ++i) {
            cycle_buffer->append(cycle_prices[i]);
        }
    }
    
    auto dpo = std::make_shared<DPO>(cycle_line, 20);
    
    // 修复性能：单次计算替代O(n²)循环
    dpo->calculate();
    
    std::vector<double> dpo_values;
    for (size_t i = 0; i < dpo->size(); ++i) {
        double dpo_val = dpo->get(-static_cast<int>(i));
        if (!std::isnan(dpo_val)) {
            dpo_values.push_back(dpo_val);
        }
    }
    
    // 分析DPO的周期特性
    if (dpo_values.size() > 80) {
        // 寻找峰值和谷值
        int peaks = 0;
        int troughs = 0;
    for (size_t i = 1; i < dpo_values.size() - 1; ++i) {
            if (dpo_values[i] > dpo_values[i-1] && dpo_values[i] > dpo_values[i+1]) {
                peaks++;
            }
            if (dpo_values[i] < dpo_values[i-1] && dpo_values[i] < dpo_values[i+1]) {
                troughs++;
            }
        }
        
        std::cout << "Cycle identification:" << std::endl;
        std::cout << "DPO peaks: " << peaks << std::endl;
        std::cout << "DPO troughs: " << troughs << std::endl;
        
        // 应该检测到一些周期性特征
        EXPECT_GT(peaks + troughs, 0) << "Should detect some cyclical patterns";
    }
}

// DPO振荡特性测试
TEST(OriginalTests, DPO_OscillationCharacteristics) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("close", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    if (close_buffer) {
        close_buffer->set(0, csv_data[0].close);
        for (size_t i = 1; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    auto dpo = std::make_shared<DPO>(close_line, 14);
    
    std::vector<double> dpo_values;
    double sum_positive = 0.0;
    double sum_negative = 0.0;
    int positive_count = 0;
    int negative_count = 0;
    
    // 修复性能：单次计算替代O(n²)循环
    dpo->calculate();
    
    // 分析DPO的振荡特性
    for (size_t i = 0; i < dpo->size(); ++i) {
        double dpo_val = dpo->get(-static_cast<int>(i));
        if (!std::isnan(dpo_val)) {
            dpo_values.push_back(dpo_val);
            
            if (dpo_val > 0) {
                sum_positive += dpo_val;
                positive_count++;
            } else if (dpo_val < 0) {
                sum_negative += dpo_val;
                negative_count++;
            }
        }
    }
    
    std::cout << "DPO oscillation characteristics:" << std::endl;
    std::cout << "Positive values: " << positive_count << std::endl;
    std::cout << "Negative values: " << negative_count << std::endl;
    
    if (positive_count > 0) {
        double avg_positive = sum_positive / positive_count;
        std::cout << "Average positive DPO: " << avg_positive << std::endl;
    }
    
    if (negative_count > 0) {
        double avg_negative = sum_negative / negative_count;
        std::cout << "Average negative DPO: " << avg_negative << std::endl;
    }
    
    // 验证DPO围绕零线振荡
    if (!dpo_values.empty()) {
        double avg_dpo = std::accumulate(dpo_values.begin(), dpo_values.end(), 0.0) / dpo_values.size();
        std::cout << "Average DPO: " << avg_dpo << std::endl;
        
        // DPO作为振荡器，长期平均值应该接近零
        EXPECT_NEAR(avg_dpo, 0.0, 50.0) 
            << "DPO should oscillate around zero";
    }
}

// DPO与SMA关系测试
TEST(OriginalTests, DPO_SMARelationship) {
    // 使用简单递增数据验证DPO计算
    std::vector<double> simple_prices;
    for (int i = 1; i <= 50; ++i) {
        simple_prices.push_back(static_cast<double>(i * 10));
    }
    
    auto simple_line = std::make_shared<LineSeries>();
    simple_line->lines->add_line(std::make_shared<LineBuffer>());
    simple_line->lines->add_alias("simple", 0);
    auto simple_buffer = std::dynamic_pointer_cast<LineBuffer>(simple_line->lines->getline(0));

    if (simple_buffer) {
        simple_buffer->set(0, simple_prices[0]);
        for (size_t i = 1; i < simple_prices.size(); ++i) {
            simple_buffer->append(simple_prices[i]);
        }
    }
    
    auto dpo = std::make_shared<DPO>(simple_line, 10);
    auto sma = std::make_shared<SMA>(simple_line, 10);
    // 修复性能：单次计算替代O(n²)循环
    dpo->calculate();
    sma->calculate();
    
    // 手动验证DPO计算：DPO = Price - SMA(period/2+1期前)
    if (simple_prices.size() >= 16) {  // 确保有足够数据
        double current_price = simple_prices.back();
        // Python formula: ma(-period // 2 + 1)
        // For period=10: -10//2 + 1 = -5 + 1 = -4
        double displaced_sma = sma->get(-4);  // 4期前的SMA
        double expected_dpo = current_price - displaced_sma;
        double actual_dpo = dpo->get(0);
        
        if (!std::isnan(actual_dpo) && !std::isnan(displaced_sma)) {
            std::cout << "DPO relationship verification:" << std::endl;
            std::cout << "Current price: " << current_price << std::endl;
            std::cout << "Displaced SMA: " << displaced_sma << std::endl;
            std::cout << "Expected DPO: " << expected_dpo << std::endl;
            std::cout << "Actual DPO: " << actual_dpo << std::endl;
            
            EXPECT_NEAR(actual_dpo, expected_dpo, 1e-6) 
                << "DPO should equal price minus displaced SMA";
        }
    }
}

// DPO超买超卖信号测试
TEST(OriginalTests, DPO_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("close", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    if (close_buffer) {
        close_buffer->set(0, csv_data[0].close);
        for (size_t i = 1; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    auto dpo = std::make_shared<DPO>(close_line, 20);
    
    std::vector<double> dpo_values;
    // 修复性能：单次计算替代O(n²)循环
    dpo->calculate();
    
    // 收集DPO值
    for (size_t i = 0; i < dpo->size(); ++i) {
        double dpo_val = dpo->get(-static_cast<int>(i));
        if (!std::isnan(dpo_val)) {
            dpo_values.push_back(dpo_val);
        }
    }
    
    // 计算DPO的统计特性
    if (!dpo_values.empty()) {
        double mean = std::accumulate(dpo_values.begin(), dpo_values.end(), 0.0) / dpo_values.size();
        
        double variance = 0.0;
    
    for (double val : dpo_values) {
            variance += (val - mean) * (val - mean);
        }
        variance /= dpo_values.size();
        double std_dev = std::sqrt(variance);
        
        // 定义超买超卖水平（例如：±1标准差）
        double overbought_level = mean + std_dev;
        double oversold_level = mean - std_dev;
        
        int overbought_signals = 0;
        int oversold_signals = 0;
    
    for (double val : dpo_values) {
            if (val > overbought_level) {
                overbought_signals++;
            } else if (val < oversold_level) {
                oversold_signals++;
            }
        }
        
        std::cout << "DPO overbought/oversold analysis:" << std::endl;
        std::cout << "Mean: " << mean << ", Std Dev: " << std_dev << std::endl;
        std::cout << "Overbought level: " << overbought_level << std::endl;
        std::cout << "Oversold level: " << oversold_level << std::endl;
        std::cout << "Overbought signals: " << overbought_signals << std::endl;
        std::cout << "Oversold signals: " << oversold_signals << std::endl;
        
        // 验证有一些信号产生
        EXPECT_GT(overbought_signals + oversold_signals, 0) 
            << "Should generate some overbought/oversold signals";
    }
}

// 边界条件测试
TEST(OriginalTests, DPO_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineSeries>();
    flat_line->lines->add_line(std::make_shared<LineBuffer>());
    flat_line->lines->add_alias("flat", 0);
    auto flat_buffer = std::dynamic_pointer_cast<LineBuffer>(flat_line->lines->getline(0));

    if (flat_buffer) {
        for (size_t i = 0; i < flat_prices.size(); ++i) {
            if (i == 0) {
                flat_buffer->set(0, flat_prices[i]);
            } else {
                flat_buffer->append(flat_prices[i]);
            }
        }
    }
    
    auto flat_dpo = std::make_shared<DPO>(flat_line, 20);
    // 修复性能：单次计算替代O(n²)循环
    flat_dpo->calculate();
    
    // 当所有价格相同时，DPO应该为零
    double final_dpo = flat_dpo->get(0);
    if (!std::isnan(final_dpo)) {
        EXPECT_NEAR(final_dpo, 0.0, 1e-6) 
            << "DPO should be zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineSeries>();
    insufficient_line->lines->add_line(std::make_shared<LineBuffer>());
    insufficient_line->lines->add_alias("insufficient", 0);
    auto insufficient_buffer = std::dynamic_pointer_cast<LineBuffer>(insufficient_line->lines->getline(0));

    if (insufficient_buffer) {
        insufficient_buffer->set(0, 100.0);
        for (int i = 1; i < 20; ++i) {
            insufficient_buffer->append(100.0 + i);
        }
    }
    
    auto insufficient_dpo = std::make_shared<DPO>(insufficient_line, 14);
    // 修复性能：单次计算替代O(n²)循环
    insufficient_dpo->calculate();
    
    // 数据不足时应该返回NaN
    double result = insufficient_dpo->get(0);
    EXPECT_TRUE(std::isnan(result)) << "DPO should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, DPO_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<double> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    for (size_t i = 0; i < data_size; ++i) {
        large_data.push_back(dist(rng));
    }
    
    auto large_line = std::make_shared<LineSeries>();
    large_line->lines->add_line(std::make_shared<LineBuffer>());
    large_line->lines->add_alias("large", 0);
    auto large_buffer = std::dynamic_pointer_cast<LineBuffer>(large_line->lines->getline(0));

    if (large_buffer) {
        large_buffer->set(0, large_data[0]);
        for (size_t i = 1; i < large_data.size(); ++i) {
            large_buffer->append(large_data[i]);
        }
    }
    
    auto large_dpo = std::make_shared<DPO>(large_line, 20);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    // 修复性能：单次计算替代O(n²)循环 - 大数据集测试
    large_dpo->calculate();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "DPO calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_dpo->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}
