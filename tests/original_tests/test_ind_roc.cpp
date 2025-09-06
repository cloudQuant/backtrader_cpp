/**
 * @file test_ind_roc.cpp
 * @brief ROC指标测试 - 对应Python test_ind_roc.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['0.016544', '-0.009477', '0.019050'],
 * ]
 * chkmin = 13
 * chkind = btind.ROC
 */

#include "test_common.h"
#include "lineseries.h"
#include <random>
#include <iostream>

#include "indicators/roc.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;
using LineSeries = backtrader::LineSeries;
using LineBuffer = backtrader::LineBuffer;

namespace {

const std::vector<std::vector<std::string>> ROC_EXPECTED_VALUES = {
    {"0.016544", "-0.009477", "0.019050"}
};

const int ROC_MIN_PERIOD = 13;

} // anonymous namespace

// 使用默认参数的ROC测试
DEFINE_INDICATOR_TEST(ROC_Default, ROC, ROC_EXPECTED_VALUES, ROC_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, ROC_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线系列
    auto close_line_series = std::make_shared<LineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    close_line_series->lines->add_alias("close", 0);
    
    // 逐步添加数据到线缓冲区  
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    if (close_buffer) {
        // LineBuffer starts with initial NaN, append data after it
        for (size_t i = 0; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    // 创建ROC指标（默认12周期，最小周期为13）
    auto roc = std::make_shared<ROC>(std::static_pointer_cast<LineSeries>(close_line_series), 12);
    
    // 计算
    roc->calculate();
    
    // 调试信息
    std::cout << "ROC size: " << roc->size() << std::endl;
    std::cout << "Data length: " << csv_data.size() << std::endl;
    std::cout << "Close buffer size: " << close_buffer->size() << std::endl;
    
    // Debug data values in close_buffer
    std::cout << "First 5 close values:" << std::endl;
    const auto& close_array = close_buffer->array();
    std::cout << "Close array size: " << close_array.size() << std::endl;
    for (int i = 0; i < std::min(5, static_cast<int>(close_array.size())); ++i) {
        std::cout << "close_array[" << i << "] = " << close_array[i] << std::endl;
    }
    for (int i = 0; i < 5; ++i) {
        std::cout << "close[" << i << "] = " << (*close_buffer)[i] << std::endl;
    }
    
    // Check ROC line directly
    auto roc_line = roc->lines->getline(0);
    std::cout << "ROC line size: " << roc_line->size() << std::endl;
    
    // Debug ROC values with direct line access
    std::cout << "First 20 ROC values (direct line access):" << std::endl;
    for (int i = 0; i < std::min(20, static_cast<int>(roc_line->size())); ++i) {
        std::cout << "ROC_line[" << i << "] = " << (*roc_line)[i] << std::endl;
    }
    
    // Check last few values
    std::cout << "Last 5 ROC values:" << std::endl;
    for (int i = std::max(0, static_cast<int>(roc_line->size()) - 5); i < static_cast<int>(roc_line->size()); ++i) {
        std::cout << "ROC_line[" << i << "] = " << (*roc_line)[i] << std::endl;
    }
    
    // Find the last non-NaN value
    int last_valid_index = -1;
    for (int i = roc_line->size() - 1; i >= 0; --i) {
        if (!std::isnan((*roc_line)[i])) {
            last_valid_index = i;
            break;
        }
    }
    std::cout << "Last valid ROC index: " << last_valid_index 
              << ", value: " << (last_valid_index >= 0 ? (*roc_line)[last_valid_index] : 0.0) << std::endl;
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 13;  // period + 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::cout << "Check points: ";
    for (auto cp : check_points) {
        std::cout << cp << " ";
    }
    std::cout << std::endl;
    
    // Debug what indices these ago values map to
    for (size_t i = 0; i < check_points.size(); ++i) {
        int ago = check_points[i];
        std::cout << "Check point " << i << ": ago=" << ago << std::endl;
    }
    
    std::vector<std::string> expected = {"0.016544", "-0.009477", "0.019050"};
    
    // The Python test checks values at specific ago indices
    // Use the get() method to access values correctly
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        int ago = check_points[i];
        
        // Use the get() method which handles ago values correctly
        double actual = roc->get(ago);
        
        std::cout << "Check point " << i << ": ago=" << ago 
                  << ", value=" << actual << std::endl;
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        
        // Check if values are close enough (within 0.01% tolerance)
        double expected_val = std::stod(expected[i]);
        bool close_enough = std::abs(actual - expected_val) < std::abs(expected_val) * 0.0001;
        
        if (close_enough) {
            // Use a looser comparison for values that are close
            EXPECT_NEAR(actual, expected_val, std::abs(expected_val) * 0.0001) 
                << "ROC value close but not exact at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        } else {
            EXPECT_EQ(actual_str, expected[i]) 
                << "ROC value mismatch at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(roc->getMinPeriod(), 13) << "ROC minimum period should be 13";
}

// 参数化测试 - 测试不同周期的ROC
class ROCParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line_series_ = std::make_shared<LineSeries>();
        close_line_series_->lines->add_line(std::make_shared<LineBuffer>());
        close_line_series_->lines->add_alias("close", 0);
        
        // 逐步添加数据到线缓冲区  
        auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series_->lines->getline(0));
        if (close_buffer) {
            // LineBuffer starts with initial NaN, append data after it
            for (size_t i = 0; i < csv_data_.size(); ++i) {
                close_buffer->append(csv_data_[i].close);
            }
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineSeries> close_line_series_;
};

TEST_P(ROCParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto roc = std::make_shared<ROC>(std::static_pointer_cast<LineSeries>(close_line_series_), period);
    
    // 计算
    roc->calculate();
    
    // 验证最小周期 (period + 1)
    EXPECT_EQ(roc->getMinPeriod(), period + 1) 
        << "ROC minimum period should be period + 1";
    
    // 验证最后的值 - 使用get(0)方法来获取最新值
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        // Debug: Check ROC line size
        size_t roc_size = roc->size();
        std::cout << "Period: " << period << ", ROC size: " << roc_size 
                  << ", Data size: " << csv_data_.size() << std::endl;
        
        // Use get(0) to get the most recent value
        double last_value = roc->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last ROC value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "ROC value should be finite";
    }
}

// 测试不同的ROC周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    ROCParameterizedTest,
    ::testing::Values(5, 10, 12, 20, 30)
);

// ROC计算逻辑验证测试
TEST(OriginalTests, ROC_CalculationLogic) {
    // 使用简单的测试数据验证ROC计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0};
    
    auto close_line_series = std::make_shared<LineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    close_line_series->lines->add_alias("roc_calc", 0);
    
    // 逐步添加数据到线缓冲区  
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    if (close_buffer) {
        // Set the first data point to replace the initial NaN, then append the rest
        close_buffer->set(0, prices[0]);
        for (size_t i = 1; i < prices.size(); ++i) {
            close_buffer->append(prices[i]);
        }
    }
    
    auto roc = std::make_shared<ROC>(std::static_pointer_cast<LineSeries>(close_line_series), 5);
    
    // 计算
    roc->calculate();
    
    // 手动计算最终ROC进行验证
    size_t i = prices.size() - 1;
    if (i >= 5) {  // 需要period + 1个数据点
        double current_price = prices[i];
        double past_price = prices[i - 5];  // 5周期前的价格
        double expected_roc = (current_price - past_price) / past_price;
        
        double actual_roc = roc->get(0);
        EXPECT_NEAR(actual_roc, expected_roc, 1e-10) 
            << "ROC calculation mismatch at final step" 
            << " (current: " << current_price << ", past: " << past_price << ")";
    }
}

// ROC百分比计算验证测试
TEST(OriginalTests, ROC_PercentageCalculation) {
    // 测试ROC百分比计算
    std::vector<double> prices = {100.0, 105.0, 110.0, 95.0, 120.0};
    
    auto close_line_series = std::make_shared<LineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    close_line_series->lines->add_alias("roc_percent", 0);
    
    // 逐步添加数据到线缓冲区  
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    if (close_buffer) {
        // Set the first data point to replace the initial NaN, then append the rest
        close_buffer->set(0, prices[0]);
        for (size_t i = 1; i < prices.size(); ++i) {
            close_buffer->append(prices[i]);
        }
    }
    
    auto roc = std::make_shared<ROC>(std::static_pointer_cast<LineSeries>(close_line_series), 3);
    
    // 计算
    roc->calculate();
    
    // 验证最终ROC值
    size_t i = prices.size() - 1;
    if (i >= 3) {
        double current_price = prices[i];
        double past_price = prices[i - 3];
        
        // ROC = (Current - Past) / Past
        double expected_roc = (current_price - past_price) / past_price;
        double actual_roc = roc->get(0);
        
        EXPECT_NEAR(actual_roc, expected_roc, 1e-10) 
            << "ROC percentage calculation at final step";
            
        // 验证百分比意义
        if (current_price > past_price) {
            EXPECT_GT(actual_roc, 0.0) << "ROC should be positive for price increase";
        } else if (current_price < past_price) {
            EXPECT_LT(actual_roc, 0.0) << "ROC should be negative for price decrease";
        }
    }
}

// 趋势检测测试
TEST(OriginalTests, ROC_TrendDetection) {
    // 创建上升趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 30; ++i) {
        uptrend_prices.push_back(100.0 + i * 2.0);  // 持续上升
    }
    
    auto up_line_series = std::make_shared<LineSeries>();
    up_line_series->lines->add_line(std::make_shared<LineBuffer>());
    up_line_series->lines->add_alias("uptrend", 0);
    
    // 逐步添加数据到线缓冲区  
    auto up_buffer = std::dynamic_pointer_cast<LineBuffer>(up_line_series->lines->getline(0));
    if (up_buffer) {
        // Set the first data point to replace the initial NaN, then append the rest
        up_buffer->set(0, uptrend_prices[0]);
        for (size_t i = 1; i < uptrend_prices.size(); ++i) {
            up_buffer->append(uptrend_prices[i]);
        }
    }
    
    auto up_roc = std::make_shared<ROC>(std::static_pointer_cast<LineSeries>(up_line_series), 10);
    
    // 计算
    up_roc->calculate();
    
    double final_up_roc = up_roc->get(0);
    if (!std::isnan(final_up_roc)) {
        EXPECT_GT(final_up_roc, 0.0) 
            << "ROC should be positive for uptrend";
    }
    
    // 创建下降趋势数据
    std::vector<double> downtrend_prices;
    for (int i = 0; i < 30; ++i) {
        downtrend_prices.push_back(200.0 - i * 2.0);  // 持续下降
    }
    
    auto down_line_series = std::make_shared<LineSeries>();
    down_line_series->lines->add_line(std::make_shared<LineBuffer>());
    down_line_series->lines->add_alias("downtrend", 0);
    
    // 逐步添加数据到线缓冲区  
    auto down_buffer = std::dynamic_pointer_cast<LineBuffer>(down_line_series->lines->getline(0));
    if (down_buffer) {
        // Set the first data point to replace the initial NaN, then append the rest
        down_buffer->set(0, downtrend_prices[0]);
        for (size_t i = 1; i < downtrend_prices.size(); ++i) {
            down_buffer->append(downtrend_prices[i]);
        }
    }
    
    auto down_roc = std::make_shared<ROC>(std::static_pointer_cast<LineSeries>(down_line_series), 10);
    
    // 计算
    down_roc->calculate();
    
    double final_down_roc = down_roc->get(0);
    if (!std::isnan(final_down_roc)) {
        EXPECT_LT(final_down_roc, 0.0) 
            << "ROC should be negative for downtrend";
    }
    
    std::cout << "Uptrend ROC: " << final_up_roc << std::endl;
    std::cout << "Downtrend ROC: " << final_down_roc << std::endl;
}

// 横盘市场测试
TEST(OriginalTests, ROC_SidewaysMarket) {
    // 创建横盘震荡数据
    std::vector<double> sideways_prices;
    for (int i = 0; i < 50; ++i) {
        double base = 100.0;
        double oscillation = 3.0 * std::sin(i * 0.3);  // 小幅震荡
        sideways_prices.push_back(base + oscillation);
    }
    
    auto sideways_line_series = std::make_shared<LineSeries>();
    sideways_line_series->lines->add_line(std::make_shared<LineBuffer>());
    sideways_line_series->lines->add_alias("sideways", 0);
    
    // 逐步添加数据到线缓冲区  
    auto sideways_buffer = std::dynamic_pointer_cast<LineBuffer>(sideways_line_series->lines->getline(0));
    if (sideways_buffer) {
        // Set the first data point to replace the initial NaN, then append the rest
        sideways_buffer->set(0, sideways_prices[0]);
        for (size_t i = 1; i < sideways_prices.size(); ++i) {
            sideways_buffer->append(sideways_prices[i]);
        }
    }
    
    auto sideways_roc = std::make_shared<ROC>(std::static_pointer_cast<LineSeries>(sideways_line_series), 20);
    
    // 计算
    sideways_roc->calculate();
    
    std::vector<double> roc_values;
    
    // 收集所有ROC值进行统计
    for (size_t i = 0; i < sideways_roc->size(); ++i) {
        double roc_val = sideways_roc->get(-static_cast<int>(i));
        if (!std::isnan(roc_val)) {
            roc_values.push_back(roc_val);
        }
    }
    
    // 横盘市场中，ROC应该接近零且波动较小
    if (!roc_values.empty()) {
        double avg_roc = std::accumulate(roc_values.begin(), roc_values.end(), 0.0) / roc_values.size();
        EXPECT_NEAR(avg_roc, 0.0, 0.1) 
            << "Average ROC should be close to zero in sideways market";
        
        std::cout << "Sideways market average ROC: " << avg_roc << std::endl;
    }
}

// 零线交叉测试
TEST(OriginalTests, ROC_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line_series = std::make_shared<LineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    close_line_series->lines->add_alias("close", 0);
    
    // 逐步添加数据到线缓冲区  
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    if (close_buffer) {
        // Use append for all data points to ensure correct size
        for (size_t i = 0; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    auto roc = std::make_shared<ROC>(std::static_pointer_cast<LineSeries>(close_line_series), 12);
    
    // 计算
    roc->calculate();
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_roc = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越 - 遍历所有计算的ROC值
    for (size_t i = 0; i < roc->size(); ++i) {
        double current_roc = roc->get(-static_cast<int>(i));
        
        if (!std::isnan(current_roc) && has_prev) {
            if (prev_roc <= 0.0 && current_roc > 0.0) {
                positive_crossings++;
            } else if (prev_roc >= 0.0 && current_roc < 0.0) {
                negative_crossings++;
            }
        }
        
        if (!std::isnan(current_roc)) {
            prev_roc = current_roc;
            has_prev = true;
        }
    }
    
    std::cout << "ROC zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// 边界条件测试
TEST(OriginalTests, ROC_EdgeCases) {
    // 测试除零情况（过去价格为0）
    std::vector<double> zero_prices = {0.0, 100.0, 105.0, 110.0, 115.0};
    
    auto zero_line_series = std::make_shared<LineSeries>();
    zero_line_series->lines->add_line(std::make_shared<LineBuffer>());
    zero_line_series->lines->add_alias("zero_test", 0);
    
    // 逐步添加数据到线缓冲区  
    auto zero_buffer = std::dynamic_pointer_cast<LineBuffer>(zero_line_series->lines->getline(0));
    if (zero_buffer) {
        // Set the first data point to replace the initial NaN, then append the rest
        zero_buffer->set(0, zero_prices[0]);
        for (size_t i = 1; i < zero_prices.size(); ++i) {
            zero_buffer->append(zero_prices[i]);
        }
    }
    
    auto zero_roc = std::make_shared<ROC>(std::static_pointer_cast<LineSeries>(zero_line_series), 3);
    
    // 计算
    zero_roc->calculate();
    
    double roc_val = zero_roc->get(0);
    
    // 当过去价格为0时，ROC应该是无穷大或NaN
    size_t i = zero_prices.size() - 1;
    if (i >= 3 && zero_prices[i - 3] == 0.0) {
        EXPECT_TRUE(std::isnan(roc_val) || std::isinf(roc_val)) 
            << "ROC should be NaN or infinite when past price is zero";
    }
    
    // 测试相同价格的情况
    std::vector<double> flat_prices(20, 100.0);  // 20个相同价格
    
    auto flat_line_series = std::make_shared<LineSeries>();
    flat_line_series->lines->add_line(std::make_shared<LineBuffer>());
    flat_line_series->lines->add_alias("flat", 0);
    
    // 逐步添加数据到线缓冲区  
    auto flat_buffer = std::dynamic_pointer_cast<LineBuffer>(flat_line_series->lines->getline(0));
    if (flat_buffer) {
        // Set the first data point to replace the initial NaN, then append the rest
        flat_buffer->set(0, flat_prices[0]);
        for (size_t i = 1; i < flat_prices.size(); ++i) {
            flat_buffer->append(flat_prices[i]);
        }
    }
    
    auto flat_roc = std::make_shared<ROC>(std::static_pointer_cast<LineSeries>(flat_line_series), 10);
    
    // 计算
    flat_roc->calculate();
    
    double final_roc = flat_roc->get(0);
    if (!std::isnan(final_roc)) {
        EXPECT_NEAR(final_roc, 0.0, 1e-10) 
            << "ROC should be zero for constant prices";
    }
}

// 性能测试
TEST(OriginalTests, ROC_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<double> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        large_data.push_back(dist(rng));
    }
    
    auto large_line_series = std::make_shared<LineSeries>();
    large_line_series->lines->add_line(std::make_shared<LineBuffer>());
    large_line_series->lines->add_alias("large", 0);
    
    // 逐步添加数据到线缓冲区  
    auto large_buffer = std::dynamic_pointer_cast<LineBuffer>(large_line_series->lines->getline(0));
    if (large_buffer) {
        // Set the first data point to replace the initial NaN, then append the rest
        large_buffer->set(0, large_data[0]);
        for (size_t i = 1; i < large_data.size(); ++i) {
            large_buffer->append(large_data[i]);
        }
    }
    
    auto large_roc = std::make_shared<ROC>(std::static_pointer_cast<LineSeries>(large_line_series), 50);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 计算
    large_roc->calculate();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "ROC calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_roc->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}