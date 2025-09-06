/**
 * @file test_ind_lowest.cpp
 * @brief Lowest指标测试 - 对应Python test_ind_lowest.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4019.890000', '3570.170000', '3506.070000'],
 * ]
 * chkmin = 14
 * chkind = btind.Lowest
 * chkargs = dict(period=14)
 */

#include "test_common.h"
#include "lineseries.h"
#include <random>
#include <chrono>

#include "indicators/lowest.h"
#include "indicators/highest.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;
using LineSeries = backtrader::LineSeries;
using LineBuffer = backtrader::LineBuffer;

namespace {

const std::vector<std::vector<std::string>> LOWEST_EXPECTED_VALUES = {
    {"4019.890000", "3570.170000", "3506.070000"}
};

const int LOWEST_MIN_PERIOD = 14;

} // anonymous namespace

// 使用指定参数的Lowest测试
DEFINE_INDICATOR_TEST(Lowest_Period14, Lowest, LOWEST_EXPECTED_VALUES, LOWEST_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, Lowest_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    std::cerr << "DEBUG: csv_data size = " << csv_data.size() << std::endl;
    
    // 创建数据线 - 使用LineSeries+LineBuffer模式
    auto data_series = std::make_shared<backtrader::LineSeries>();
    data_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    data_series->lines->add_alias("close", 0);
    
    auto close_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(data_series->lines->getline(0));
    
    
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    // 创建Lowest指标（14周期）
    auto lowest = std::make_shared<Lowest>(data_series, 14);
    
    // 计算所有值
    lowest->calculate();
    
    // Debug: check buffer content
    auto lowest_line = lowest->lines->getline(0);
    auto linebuf = std::dynamic_pointer_cast<backtrader::LineBuffer>(lowest_line);
    if (linebuf) {
        const auto& arr = linebuf->array();
        std::cerr << "DEBUG: result buffer size = " << arr.size() << std::endl;
        if (arr.size() > 3) {
            std::cerr << "  Last 3 values: " 
                      << arr[arr.size()-3] << ", "
                      << arr[arr.size()-2] << ", "
                      << arr[arr.size()-1] << std::endl;
        }
        
        // Debug: print values at the check positions
        std::cerr << "DEBUG: Values at check positions:" << std::endl;
        std::cerr << "  Position 14: " << (14 < arr.size() ? std::to_string(arr[14]) : "N/A") << std::endl;
        std::cerr << "  Position 134: " << (134 < arr.size() ? std::to_string(arr[134]) : "N/A") << std::endl;
        std::cerr << "  Position 254: " << (254 < arr.size() ? std::to_string(arr[254]) : "N/A") << std::endl;
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 14;
    
    // Python测试的检查点转换为buffer位置:
    // Python: [0, -l + mp, (-l + mp) // 2]
    // Buffer has 256 values (including initial NaN), csv_data has 255
    // 0 -> 最新值 (buffer position 255)
    // -l + mp -> -(255-14) = -241 -> buffer position 14
    // (-l + mp) // 2 -> -120 -> buffer position 134
    std::vector<int> buffer_positions = {
        data_length,                          // 最新值位置 (255)
        min_period,                           // 第一个有效值位置 (14)
        min_period + (data_length - min_period) / 2  // 中间值位置 (134)
    };
    
    std::vector<std::string> expected = {"4019.890000", "3570.170000", "3506.070000"};
    for (size_t i = 0; i < buffer_positions.size() && i < expected.size(); ++i) {
        // 计算相应的ago值来访问buffer中的位置
        // buffer的_idx在位置255，要访问位置i，需要ago = 255 - i
        int ago = data_length - buffer_positions[i];
        double actual = lowest->get(ago);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "Lowest value mismatch at check point " << i 
            << " (buffer position=" << buffer_positions[i] << ", ago=" << ago << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(lowest->getMinPeriod(), 14) << "Lowest minimum period should be 14";
}

// 参数化测试 - 测试不同周期的Lowest
class LowestParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        // 使用LineSeries+LineBuffer模式
        data_series_ = std::make_shared<backtrader::LineSeries>();
        data_series_->lines->add_line(std::make_shared<backtrader::LineBuffer>());
        data_series_->lines->add_alias("close", 0);
        
        auto close_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(data_series_->lines->getline(0));
    
    
    for (const auto& bar : csv_data_) {
            close_buffer->append(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<backtrader::LineSeries> data_series_;
};

TEST_P(LowestParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto lowest = std::make_shared<Lowest>(data_series_, period);
    
    // 计算所有值
    lowest->calculate();
    
    // 验证最小周期
    EXPECT_EQ(lowest->getMinPeriod(), period) 
        << "Lowest minimum period should match parameter";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_value = lowest->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last Lowest value should not be NaN";
        EXPECT_GT(last_value, 0) << "Lowest value should be positive for this test data";
    }
}

// 测试不同的Lowest周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    LowestParameterizedTest,
    ::testing::Values(5, 10, 14, 20, 30)
);

// 计算逻辑验证测试
TEST(OriginalTests, Lowest_CalculationLogic) {
    // 使用简单的测试数据验证Lowest计算
    std::vector<double> prices = {100.0, 95.0, 110.0, 85.0, 120.0, 90.0, 105.0, 115.0, 80.0, 125.0};
    
    auto data_series = std::make_shared<backtrader::LineSeries>();
    data_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    data_series->lines->add_alias("close", 0);
    
    auto close_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(data_series->lines->getline(0));
    
    
    for (double price : prices) {
        close_buffer->append(price);
    }
    
    auto lowest = std::make_shared<Lowest>(data_series, 5);
    lowest->calculate();
    
    // Debug: check buffer content after calculate
    auto lowest_line = lowest->lines->getline(0);
    auto linebuf = std::dynamic_pointer_cast<backtrader::LineBuffer>(lowest_line);
    if (linebuf) {
        const auto& arr = linebuf->array();
        std::cout << "DEBUG: Lowest buffer size = " << arr.size() << std::endl;
        std::cout << "DEBUG: Buffer content: ";
        for (size_t k = 0; k < arr.size(); ++k) {
            std::cout << arr[k] << " ";
        }
        std::cout << std::endl;
        std::cout << "DEBUG: Current _idx = " << linebuf->get_idx() << std::endl;
        
        // Test accessing with different ago values
        std::cout << "DEBUG: Testing ago values:" << std::endl;
        std::cout << "  get(0) = " << lowest->get(0) << std::endl;
        std::cout << "  get(-1) = " << lowest->get(-1) << std::endl;
        std::cout << "  get(-4) = " << lowest->get(-4) << std::endl;
    }
    
    // 手动验证最后几个计算值;
    for (size_t i = 4; i < prices.size(); ++i) {
        double expected_lowest = std::numeric_limits<double>::infinity();
    
    int j;
    for (int j = 0; j < 5; ++j) {
            expected_lowest = std::min(expected_lowest, prices[i - j]);
        }
        
        // The test expects to access calculated values based on position
        // But our buffer has _idx at the end after once() completes
        // We need to adjust ago to account for this
        // Buffer position for calculation at step i is just i
        // To access buffer[i] when _idx=10, we need ago = 10-i
        int ago = static_cast<int>(prices.size() - 1 - i);
        double actual_lowest = lowest->get(ago);
        
        if (i < 6) { // 调试前几个值
            std::cout << "Debug i=" << i << ": expected=" << expected_lowest 
                      << ", actual=" << actual_lowest << ", ago=" << ago << std::endl;
        }
        
        EXPECT_NEAR(actual_lowest, expected_lowest, 1e-10) 
            << "Lowest calculation mismatch at step " << i << " (ago=" << ago << ")";
    }
}

// 滚动窗口测试
TEST(OriginalTests, Lowest_RollingWindow) {
    // 创建一个已知的数据序列
    std::vector<double> prices = {50, 40, 30, 35, 45, 25, 20, 15, 40, 10, 55, 5, 15, 30};
    
    auto data_series = std::make_shared<backtrader::LineSeries>();
    data_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    data_series->lines->add_alias("close", 0);
    
    auto close_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(data_series->lines->getline(0));
    
    
    for (double price : prices) {
        close_buffer->append(price);
    }
    
    auto lowest = std::make_shared<Lowest>(data_series, 3);  // 3周期滚动最低值
    lowest->calculate();
    
    std::vector<double> expected_results;
    
    // 手动计算期望结果并验证;
    for (size_t i = 2; i < prices.size(); ++i) {
        double manual_lowest = std::min({prices[i], prices[i-1], prices[i-2]});
        expected_results.push_back(manual_lowest);
        
        // Adjust ago to account for buffer indexing
        // Buffer position for calculation at step i is just i
        // To access buffer[i] when _idx is at end, we need ago = buffer_size - 1 - i
        int ago = static_cast<int>(prices.size() - 1 - i);
        double actual = lowest->get(ago);
        EXPECT_NEAR(actual, manual_lowest, 1e-10) 
            << "Rolling lowest mismatch at position " << i << " (ago=" << ago << ")";
    }
    
    std::cout << "Rolling 3-period lowest results:" << std::endl;
    for (size_t i = 0; i < expected_results.size(); ++i) {
        std::cout << "Position " << (i + 2) << ": " << expected_results[i] << std::endl;
    }
}

// 单调性测试
TEST(OriginalTests, Lowest_Monotonicity) {
    auto csv_data = getdata(0);
    auto data_series = std::make_shared<backtrader::LineSeries>();
    data_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    data_series->lines->add_alias("close", 0);
    
    auto close_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(data_series->lines->getline(0));
    
    
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    auto lowest = std::make_shared<Lowest>(data_series, 20);
    lowest->calculate();
    
    // 验证Lowest值的单调性质;
    for (size_t i = 19; i < csv_data.size(); ++i) {
        int ago = static_cast<int>(csv_data.size() - 1 - i);
        double current_lowest = lowest->get(ago);
        double current_price = csv_data[i].close;
        
        // Lowest值应该小于等于当前价格
        if (!std::isnan(current_lowest)) {
            EXPECT_LE(current_lowest, current_price) 
                << "Lowest should be <= current price at step " << i;
            
            // 基本合理性检查
            EXPECT_GT(current_lowest, 0) << "Lowest should be positive";
        }
    }
}

// 与真实数据的一致性测试
TEST(OriginalTests, Lowest_vs_ManualCalculation) {
    auto csv_data = getdata(0);
    auto data_series = std::make_shared<backtrader::LineSeries>();
    data_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    data_series->lines->add_alias("close", 0);
    
    auto close_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(data_series->lines->getline(0));
    
    
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    const int period = 10;
    auto lowest = std::make_shared<Lowest>(data_series, period);
    lowest->calculate();
    
    // 从第10个数据点开始验证;
    for (size_t i = period - 1; i < csv_data.size(); ++i) {
        int ago = static_cast<int>(csv_data.size() - 1 - i);
        double indicator_lowest = lowest->get(ago);
        
        // 手动计算最近10个数据点的最低值
        double manual_lowest = std::numeric_limits<double>::infinity();
    for (int j = 0; j < period; ++j) {
            manual_lowest = std::min(manual_lowest, csv_data[i - j].close);
        }
    EXPECT_NEAR(indicator_lowest, manual_lowest, 1e-10) 
            << "Manual vs indicator calculation mismatch at step " << i << " (ago=" << ago << ")";
    }
}

// 极值测试
TEST(OriginalTests, Lowest_ExtremeValues) {
    // 测试包含极小值的数据
    std::vector<double> extreme_prices = {
        100.0, 95.0, 0.001, 90.0, 105.0,  // 包含一个极小值
        110.0, 115.0, 120.0, 125.0, 130.0
    };
    
    auto data_series = std::make_shared<backtrader::LineSeries>();
    data_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    data_series->lines->add_alias("close", 0);
    
    auto close_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(data_series->lines->getline(0));
    
    
    for (double price : extreme_prices) {
        close_buffer->append(price);
    }
    
    auto extreme_lowest = std::make_shared<Lowest>(data_series, 5);
    extreme_lowest->calculate();
    
    // 在包含极小值的窗口中，应该返回极小值;
    for (size_t i = 4; i <= 6; ++i) {  // positions 4,5,6 have windows that include index 2
        int ago = static_cast<int>(extreme_prices.size() - 1 - i);
        double current_lowest = extreme_lowest->get(ago);
        
        if (!std::isnan(current_lowest)) {
            EXPECT_EQ(current_lowest, 0.001) 
                << "Should return extreme value when it's in the window at position " << i << " (ago=" << ago << ")";
        }
    }
}

// 边界条件测试
TEST(OriginalTests, Lowest_EdgeCases) {
    // 测试数据不足的情况
    auto insufficient_series = std::make_shared<backtrader::LineSeries>();
    insufficient_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    insufficient_series->lines->add_alias("close", 0);
    
    // 只添加3个数据点
    std::vector<double> short_data = {100.0, 90.0, 110.0};
    auto insufficient_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(insufficient_series->lines->getline(0));
    
    
    for (double price : short_data) {
        insufficient_buffer->append(price);
    }
    
    auto insufficient_lowest = std::make_shared<Lowest>(insufficient_series, 5);  // 周期大于数据量
    insufficient_lowest->calculate();
    
    // 数据不足时应该返回NaN
    double result = insufficient_lowest->get(0);
    EXPECT_TRUE(std::isnan(result)) << "Lowest should return NaN when insufficient data";
    
    // 测试单个数据点的情况
    auto single_series = std::make_shared<backtrader::LineSeries>();
    single_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    single_series->lines->add_alias("close", 0);
    
    auto single_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(single_series->lines->getline(0));
    single_buffer->append(123.45);
    
    auto single_lowest = std::make_shared<Lowest>(single_series, 1);
    single_lowest->calculate();
    
    double single_result = single_lowest->get(0);
    EXPECT_NEAR(single_result, 123.45, 1e-10) 
        << "Lowest of single value should equal that value";
}

// 与Highest的对称性测试
TEST(OriginalTests, Lowest_vs_Highest_Symmetry) {
    auto csv_data = getdata(0);
    
    // 创建两个独立的数据系列
    auto data_series_lowest = std::make_shared<backtrader::LineSeries>();
    data_series_lowest->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    data_series_lowest->lines->add_alias("close", 0);
    
    auto data_series_highest = std::make_shared<backtrader::LineSeries>();
    data_series_highest->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    data_series_highest->lines->add_alias("close", 0);
    
    auto lowest_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(data_series_lowest->lines->getline(0));
    auto highest_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(data_series_highest->lines->getline(0));
    
    
    for (const auto& bar : csv_data) {
        lowest_buffer->append(bar.close);
        highest_buffer->append(bar.close);
    }
    
    auto lowest = std::make_shared<Lowest>(data_series_lowest, 14);
    auto highest = std::make_shared<Highest>(data_series_highest, 14);
    
    lowest->calculate();
    highest->calculate();
    
    // 验证Lowest <= Highest;
    for (size_t i = 13; i < csv_data.size(); ++i) {
        int ago = static_cast<int>(csv_data.size() - 1 - i);
        double lowest_val = lowest->get(ago);
        double highest_val = highest->get(ago);
        
        if (!std::isnan(lowest_val) && !std::isnan(highest_val)) {
            EXPECT_LE(lowest_val, highest_val) 
                << "Lowest should be <= Highest at step " << i << " (ago=" << ago << ")";
        }
    }
}

// 性能测试
TEST(OriginalTests, Lowest_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<double> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    for (size_t i = 0; i < data_size; ++i) {
        large_data.push_back(dist(rng));
    }
    
    auto data_series = std::make_shared<backtrader::LineSeries>();
    data_series->lines->add_line(std::make_shared<backtrader::LineBuffer>());
    data_series->lines->add_alias("close", 0);
    
    auto large_buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(data_series->lines->getline(0));
    
    
    for (double price : large_data) {
        large_buffer->append(price);
    }
    
    auto large_lowest = std::make_shared<Lowest>(data_series, 100);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    large_lowest->calculate();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Lowest calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_lowest->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_GE(final_result, 50.0) << "Final result should be within expected range";
    EXPECT_LE(final_result, 150.0) << "Final result should be within expected range";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}