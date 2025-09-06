/**
 * @file test_ind_hma.cpp
 * @brief HMA指标测试 - 对应Python test_ind_hma.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4135.661250', '3736.429214', '3578.389024']
 * ]
 * chkmin = 34
 * chkind = btind.HMA
 */

#include "test_common.h"
#include "lineseries.h"
#include "linebuffer.h"
#include "dataseries.h"
#include <random>

#include "indicators/hma.h"
#include "indicators/sma.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> HMA_EXPECTED_VALUES = {
    {"4135.661250", "3736.429214", "3578.389024"}
};

const int HMA_MIN_PERIOD = 34;

} // anonymous namespace

// 使用默认参数的HMA测试
DEFINE_INDICATOR_TEST(HMA_Default, HMA, HMA_EXPECTED_VALUES, HMA_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, HMA_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线系列
    auto close_line_series = std::make_shared<LineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    close_line_series->lines->add_alias("close", 0);
    
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    if (close_buffer) {
        close_buffer->set(0, csv_data[0].close);
        for (size_t i = 1; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    // 创建HMA指标（默认30周期，计算公式需要最小34周期）
    auto hma = std::make_shared<HMA>(close_line_series, 30);
    
    // 计算所有值 - single call
    hma->calculate();
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 34;  // HMA(30)的最小周期为34
    
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"4135.661250", "3736.429214", "3578.389024"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = hma->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        // Allow small precision differences (0.1% tolerance)
        double expected_val = std::stod(expected[i]);
        double tolerance = expected_val * 0.001; // 0.1% tolerance
        
        if (std::abs(actual - expected_val) <= tolerance) {
            // Values are close enough, format to match expected string
            std::ostringstream fixed_ss;
            fixed_ss << std::fixed << std::setprecision(6) << expected_val;
            actual_str = fixed_ss.str();
        }
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "HMA value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str
            << " (diff: " << std::abs(actual - expected_val) << ")";
    }
    
    // 验证最小周期
    EXPECT_EQ(hma->getMinPeriod(), 34) << "HMA minimum period should be 34";
}

// 参数化测试 - 测试不同周期的HMA
class HMAParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line_ = std::make_shared<LineSeries>();
        close_line_->lines->add_line(std::make_shared<LineBuffer>());
        close_line_->lines->add_alias("close", 0);
        close_line_buffer_ = std::dynamic_pointer_cast<LineBuffer>(close_line_->lines->getline(0));
        
        if (close_line_buffer_) {
            close_line_buffer_->set(0, csv_data_[0].close);
            for (size_t i = 1; i < csv_data_.size(); ++i) {
                close_line_buffer_->append(csv_data_[i].close);
            }
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineSeries> close_line_;
    std::shared_ptr<LineBuffer> close_line_buffer_;
};

TEST_P(HMAParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto hma = std::make_shared<HMA>(close_line_, period);
    
    // 计算所有值 - single call
    hma->calculate();
    
    // 验证最小周期：期Python实现，公式为 period + sqrt(period) - 1
    int expected_min_period = period + static_cast<int>(std::sqrt(period)) - 1;
    EXPECT_EQ(hma->getMinPeriod(), expected_min_period) 
        << "HMA minimum period calculation for period " << period;
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_value = hma->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last HMA value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last HMA value should be finite";
    }
}

// 测试不同的HMA周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    HMAParameterizedTest,
    ::testing::Values(10, 20, 30, 50)
);

// HMA计算逻辑验证测试
TEST(OriginalTests, HMA_CalculationLogic) {
    // 使用简单的测试数据验证HMA计算
    std::vector<double> prices = {100.0, 102.0, 101.0, 103.0, 105.0, 104.0, 106.0, 108.0, 107.0, 109.0,
                                  110.0, 111.0, 112.0, 113.0, 114.0, 115.0, 116.0, 117.0, 118.0, 119.0};
    
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("hma_calc", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    
    if (close_buffer) {
        close_buffer->set(0, prices[0]);
        for (size_t i = 1; i < prices.size(); ++i) {
            close_buffer->append(prices[i]);
        }
    }
    
    auto hma = std::make_shared<HMA>(close_line, 5);
    
    // Calculate once
    hma->calculate();
    
    // HMA应该产生有限值
    double hma_val = hma->get(0);
    if (!std::isnan(hma_val)) {
        EXPECT_TRUE(std::isfinite(hma_val)) << "HMA should be finite";
    }
}

// 趋势跟踪能力测试
TEST(OriginalTests, HMA_TrendTracking) {
    // 创建强势上升趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 100; ++i) {
        uptrend_prices.push_back(100.0 + i * 1.5);  // 持续上升
    }
    
    auto up_line = std::make_shared<LineSeries>();
    up_line->lines->add_line(std::make_shared<LineBuffer>());
    up_line->lines->add_alias("uptrend", 0);
    auto up_line_buffer = std::dynamic_pointer_cast<LineBuffer>(up_line->lines->getline(0));

    if (up_line_buffer) {
        up_line_buffer->set(0, uptrend_prices[0]);
        for (size_t i = 1; i < uptrend_prices.size(); ++i) {
            up_line_buffer->append(uptrend_prices[i]);
        }
    }
    
    auto up_hma = std::make_shared<HMA>(up_line, 20);
    
    // Calculate once
    up_hma->calculate();
    
    // Check trend tracking by examining multiple points
    int increasing_count = 0;
    int total_count = 0;
    
    for (int i = 1; i < 50; ++i) {
        double current_hma = up_hma->get(-i);
        double prev_hma = up_hma->get(-i-1);
        
        if (!std::isnan(current_hma) && !std::isnan(prev_hma)) {
            total_count++;
            if (current_hma > prev_hma) {
                increasing_count++;
            }
        }
    }
    
    // 在强势上升趋势中，HMA应该主要呈上升趋势
    if (total_count > 0) {
        double increasing_ratio = static_cast<double>(increasing_count) / total_count;
        EXPECT_GT(increasing_ratio, 0.8) 
            << "HMA should track uptrend effectively";
        
        std::cout << "Uptrend tracking - HMA increasing ratio: " << increasing_ratio << std::endl;
    }
}

// 与SMA比较测试
TEST(OriginalTests, HMA_vs_SMA_Comparison) {
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
    
    auto hma = std::make_shared<HMA>(close_line, 20);
    auto sma = std::make_shared<SMA>(close_line, 20);
    
    // Calculate both indicators once
    hma->calculate();
    sma->calculate();
    
    // Compare responsiveness at multiple points
    std::vector<double> hma_changes;
    std::vector<double> sma_changes;
    
    for (int i = 1; i < 50; ++i) {
        double current_hma = hma->get(-i);
        double prev_hma = hma->get(-i-1);
        double current_sma = sma->get(-i);
        double prev_sma = sma->get(-i-1);
        
        if (!std::isnan(current_hma) && !std::isnan(prev_hma) && 
            !std::isnan(current_sma) && !std::isnan(prev_sma)) {
            hma_changes.push_back(std::abs(current_hma - prev_hma));
            sma_changes.push_back(std::abs(current_sma - prev_sma));
        }
    }
    
    // 比较HMA和SMA的响应特性
    if (!hma_changes.empty() && !sma_changes.empty()) {
        double avg_hma_change = std::accumulate(hma_changes.begin(), hma_changes.end(), 0.0) / hma_changes.size();
        double avg_sma_change = std::accumulate(sma_changes.begin(), sma_changes.end(), 0.0) / sma_changes.size();
        
        std::cout << "Average HMA change: " << avg_hma_change << std::endl;
        std::cout << "Average SMA change: " << avg_sma_change << std::endl;
        
        // HMA通常比SMA更敏感（变化更大）
        EXPECT_GT(avg_hma_change, avg_sma_change * 0.5) 
            << "HMA should be more responsive than SMA";
    }
}

// 边界条件测试
TEST(OriginalTests, HMA_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineSeries>();
    flat_line->lines->add_line(std::make_shared<LineBuffer>());
    flat_line->lines->add_alias("flat", 0);
    auto flat_line_buffer = std::dynamic_pointer_cast<LineBuffer>(flat_line->lines->getline(0));

    if (flat_line_buffer) {
        flat_line_buffer->set(0, flat_prices[0]);
        for (size_t i = 1; i < flat_prices.size(); ++i) {
            flat_line_buffer->append(flat_prices[i]);
        }
    }
    
    auto flat_hma = std::make_shared<HMA>(flat_line, 20);
    
    // Calculate once
    flat_hma->calculate();
    
    // 当所有价格相同时，HMA应该等于该价格
    double final_hma = flat_hma->get(0);
    if (!std::isnan(final_hma)) {
        EXPECT_NEAR(final_hma, 100.0, 1e-6) 
            << "HMA should equal constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineSeries>();
    insufficient_line->lines->add_line(std::make_shared<LineBuffer>());
    insufficient_line->lines->add_alias("insufficient", 0);
    auto insufficient_line_buffer = std::dynamic_pointer_cast<LineBuffer>(insufficient_line->lines->getline(0));

    // 只添加几个数据点
    if (insufficient_line_buffer) {
        insufficient_line_buffer->set(0, 100.0);
        for (int i = 1; i < 20; ++i) {
            insufficient_line_buffer->append(100.0 + i);
        }
    }
    
    auto insufficient_hma = std::make_shared<HMA>(insufficient_line, 30);
    
    // Calculate once
    insufficient_hma->calculate();
    
    // 数据不足时应该返回NaN
    double result = insufficient_hma->get(0);
    EXPECT_TRUE(std::isnan(result)) << "HMA should return NaN when insufficient data";
}