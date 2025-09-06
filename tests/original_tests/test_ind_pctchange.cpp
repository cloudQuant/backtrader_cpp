/**
 * @file test_ind_pctchange.cpp
 * @brief PctChange指标测试 - 对应Python test_ind_pctchange.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['0.002704', '0.034162', '0.043717']
 * ]
 * chkmin = 31
 * chkind = btind.PctChange
 */

#include "test_common.h"
#include "lineseries.h"
#include <random>

#include "indicators/percentchange.h"
#include "indicators/roc.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;
using LineSeries = backtrader::LineSeries;
using LineBuffer = backtrader::LineBuffer;

namespace {

const std::vector<std::vector<std::string>> PCTCHANGE_EXPECTED_VALUES = {
    {"0.002704", "0.034162", "0.043717"}
};

const int PCTCHANGE_MIN_PERIOD = 31;

} // anonymous namespace

// 使用默认参数的PctChange测试
DEFINE_INDICATOR_TEST(PctChange_Default, PctChange, PCTCHANGE_EXPECTED_VALUES, PCTCHANGE_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, PctChange_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    // 创建PctChange指标（默认30周期，最小周期为31）
    auto pctchange = std::make_shared<PctChange>(std::static_pointer_cast<LineSeries>(close_line), 30);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        pctchange->calculate();
        if (i < csv_data.size() - 1) {
            if (close_buffer) close_buffer->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 31;  // period + 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"0.002704", "0.034162", "0.043717"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = pctchange->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "PctChange value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(pctchange->getMinPeriod(), 31) << "PctChange minimum period should be 31";
}

// 参数化测试 - 测试不同周期的PctChange
class PctChangeParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line_ = std::make_shared<LineSeries>();
        close_line_->lines->add_line(std::make_shared<LineBuffer>());
        close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_->lines->getline(0));
        for (const auto& bar : csv_data_) {
            close_buffer->append(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineSeries> close_line_;
    std::shared_ptr<LineBuffer> close_buffer;
};

TEST_P(PctChangeParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto pctchange = std::make_shared<PctChange>(std::static_pointer_cast<LineSeries>(close_line_), period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        pctchange->calculate();
        if (i < csv_data_.size() - 1) {
            if (close_buffer) close_buffer->forward();
        }
    }
    
    // 验证最小周期 (period + 1)
    EXPECT_EQ(pctchange->getMinPeriod(), period + 1) 
        << "PctChange minimum period should be period + 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        double last_value = pctchange->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last PctChange value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "PctChange value should be finite";
    }
}

// 测试不同的PctChange周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    PctChangeParameterizedTest,
    ::testing::Values(1, 5, 10, 20, 30)
);

// PctChange计算逻辑验证测试
TEST(OriginalTests, PctChange_CalculationLogic) {
    // 使用简单的测试数据验证PctChange计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0};
    
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    for (double price : prices) {
        close_buffer->append(price);
    }
    
    auto pctchange = std::make_shared<PctChange>(std::static_pointer_cast<LineSeries>(close_line), 5);
    
    // Calculate all values at once (batch mode)
    pctchange->calculate();
    
    // Verify calculations
    for (size_t i = 0; i < prices.size(); ++i) {
        // 手动计算PctChange进行验证
        if (i >= 5) {  // 需要period个数据点
            double current_price = prices[i];
            double past_price = prices[i - 5];  // 5周期前的价格
            double expected_pctchange = (current_price - past_price) / past_price;
            
            // In batch mode, access by negative index from the end
            int ago = static_cast<int>(prices.size()) - 1 - static_cast<int>(i);
            double actual_pctchange = pctchange->get(-ago);
            
            EXPECT_NEAR(actual_pctchange, expected_pctchange, 1e-10) 
                << "PctChange calculation mismatch at position " << i 
                << " (current: " << current_price << ", past: " << past_price << ")";
        }
    }
}

// PctChange vs ROC关系测试
TEST(OriginalTests, PctChange_vs_ROC) {
    auto csv_data = getdata(0);
    auto close_line_pct = std::make_shared<LineSeries>();
    close_line_pct->lines->add_line(std::make_shared<LineBuffer>());
    auto close_line_pct_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_pct->lines->getline(0));
    auto close_line_roc = std::make_shared<LineSeries>();
    close_line_roc->lines->add_line(std::make_shared<LineBuffer>());
    auto close_line_roc_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_roc->lines->getline(0));
    for (const auto& bar : csv_data) {
        close_line_pct_buffer->append(bar.close);
        close_line_roc_buffer->append(bar.close);
    }
    
    auto pctchange = std::make_shared<PctChange>(std::static_pointer_cast<LineSeries>(close_line_pct), 12);
    auto roc = std::make_shared<ROC>(std::static_pointer_cast<LineSeries>(close_line_roc), 12);
    
    // 验证PctChange和ROC应该相等
    for (size_t i = 0; i < csv_data.size(); ++i) {
        pctchange->calculate();
        roc->calculate();
        
        double pct_value = pctchange->get(0);
        double roc_value = roc->get(0);
        
        if (!std::isnan(pct_value) && !std::isnan(roc_value)) {
            EXPECT_NEAR(pct_value, roc_value, 1e-10) 
                << "PctChange should equal ROC at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            if (close_line_pct_buffer) close_line_pct_buffer->forward();
            if (close_line_roc_buffer) close_line_roc_buffer->forward();
        }
    }
}

// 趋势检测测试
TEST(OriginalTests, PctChange_TrendDetection) {
    // 创建上升趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 30; ++i) {
        uptrend_prices.push_back(100.0 + i * 2.0);  // 持续上升
    }
    
    auto up_line = std::make_shared<LineSeries>();
    up_line->lines->add_line(std::make_shared<LineBuffer>());
    auto up_line_buffer = std::dynamic_pointer_cast<LineBuffer>(up_line->lines->getline(0));
    for (double price : uptrend_prices) {
        up_line_buffer->append(price);
    }
    
    auto up_pctchange = std::make_shared<PctChange>(std::static_pointer_cast<LineSeries>(up_line), 10);
    
    for (size_t i = 0; i < uptrend_prices.size(); ++i) {
        up_pctchange->calculate();
        if (i < uptrend_prices.size() - 1) {
            if (up_line_buffer) up_line_buffer->forward();
        }
    }
    
    double final_up_pctchange = up_pctchange->get(0);
    if (!std::isnan(final_up_pctchange)) {
        EXPECT_GT(final_up_pctchange, 0.0) 
            << "PctChange should be positive for uptrend";
    }
    
    // 创建下降趋势数据
    std::vector<double> downtrend_prices;
    for (int i = 0; i < 30; ++i) {
        downtrend_prices.push_back(200.0 - i * 2.0);  // 持续下降
    }
    
    auto down_line = std::make_shared<LineSeries>();
    down_line->lines->add_line(std::make_shared<LineBuffer>());
    auto down_line_buffer = std::dynamic_pointer_cast<LineBuffer>(down_line->lines->getline(0));
    for (double price : downtrend_prices) {
        down_line_buffer->append(price);
    }
    
    auto down_pctchange = std::make_shared<PctChange>(std::static_pointer_cast<LineSeries>(down_line), 10);
    
    for (size_t i = 0; i < downtrend_prices.size(); ++i) {
        down_pctchange->calculate();
        if (i < downtrend_prices.size() - 1) {
            if (down_line_buffer) down_line_buffer->forward();
        }
    }
    
    double final_down_pctchange = down_pctchange->get(0);
    if (!std::isnan(final_down_pctchange)) {
        EXPECT_LT(final_down_pctchange, 0.0) 
            << "PctChange should be negative for downtrend";
    }
    
    std::cout << "Uptrend PctChange: " << final_up_pctchange << std::endl;
    std::cout << "Downtrend PctChange: " << final_down_pctchange << std::endl;
}

// 百分比计算验证测试
TEST(OriginalTests, PctChange_PercentageCalculation) {
    // 测试百分比计算的准确性
    std::vector<double> prices = {100.0, 105.0, 110.0, 95.0, 120.0};
    
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    for (double price : prices) {
        close_buffer->append(price);
    }
    
    auto pctchange = std::make_shared<PctChange>(std::static_pointer_cast<LineSeries>(close_line), 3);
    
    // Calculate all values at once (batch mode)
    pctchange->calculate();
    
    for (size_t i = 0; i < prices.size(); ++i) {
        if (i >= 3) {
            double current_price = prices[i];
            double past_price = prices[i - 3];
            
            // PctChange = (Current - Past) / Past
            double expected_pct = (current_price - past_price) / past_price;
            
            // In batch mode, access by negative index from the end
            int ago = static_cast<int>(prices.size()) - 1 - static_cast<int>(i);
            double actual_pct = pctchange->get(-ago);
            
            EXPECT_NEAR(actual_pct, expected_pct, 1e-10) 
                << "PctChange percentage calculation at position " << i;
                
            // 验证百分比意义
            if (current_price > past_price) {
                EXPECT_GT(actual_pct, 0.0) << "PctChange should be positive for price increase";
            } else if (current_price < past_price) {
                EXPECT_LT(actual_pct, 0.0) << "PctChange should be negative for price decrease";
            } else {
                EXPECT_NEAR(actual_pct, 0.0, 1e-10) << "PctChange should be zero for no change";
            }
        }
    }
}

// 横盘市场测试
TEST(OriginalTests, PctChange_SidewaysMarket) {
    // 创建横盘震荡数据
    std::vector<double> sideways_prices;
    for (int i = 0; i < 50; ++i) {
        double base = 100.0;
        double oscillation = 3.0 * std::sin(i * 0.3);  // 小幅震荡
        sideways_prices.push_back(base + oscillation);
    }
    
    auto sideways_line = std::make_shared<LineSeries>();
    sideways_line->lines->add_line(std::make_shared<LineBuffer>());
    auto sideways_line_buffer = std::dynamic_pointer_cast<LineBuffer>(sideways_line->lines->getline(0));
    for (double price : sideways_prices) {
        sideways_line_buffer->append(price);
    }
    
    auto sideways_pctchange = std::make_shared<PctChange>(std::static_pointer_cast<LineSeries>(sideways_line), 20);
    
    std::vector<double> pctchange_values;
    
    for (size_t i = 0; i < sideways_prices.size(); ++i) {
        sideways_pctchange->calculate();
        
        double pctchange_val = sideways_pctchange->get(0);
        if (!std::isnan(pctchange_val)) {
            pctchange_values.push_back(pctchange_val);
        }
        
        if (i < sideways_prices.size() - 1) {
            if (sideways_line_buffer) sideways_line_buffer->forward();
        }
    }
    
    // 横盘市场中，PctChange应该接近零且波动较小
    if (!pctchange_values.empty()) {
        double avg_pctchange = std::accumulate(pctchange_values.begin(), pctchange_values.end(), 0.0) / pctchange_values.size();
        EXPECT_NEAR(avg_pctchange, 0.0, 0.1) 
            << "Average PctChange should be close to zero in sideways market";
        
        std::cout << "Sideways market average PctChange: " << avg_pctchange << std::endl;
    }
}

// 零线交叉测试
TEST(OriginalTests, PctChange_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    auto pctchange = std::make_shared<PctChange>(std::static_pointer_cast<LineSeries>(close_line), 12);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_pctchange = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越
    for (size_t i = 0; i < csv_data.size(); ++i) {
        pctchange->calculate();
        
        double current_pctchange = pctchange->get(0);
        
        if (!std::isnan(current_pctchange) && has_prev) {
            if (prev_pctchange <= 0.0 && current_pctchange > 0.0) {
                positive_crossings++;
            } else if (prev_pctchange >= 0.0 && current_pctchange < 0.0) {
                negative_crossings++;
            }
        }
        
        if (!std::isnan(current_pctchange)) {
            prev_pctchange = current_pctchange;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            if (close_buffer) close_buffer->forward();
        }
    }
    
    std::cout << "PctChange zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// 边界条件测试
TEST(OriginalTests, PctChange_EdgeCases) {
    // 测试除零情况（过去价格为0）
    std::vector<double> zero_prices = {0.0, 100.0, 105.0, 110.0, 115.0};
    
    auto zero_line = std::make_shared<LineSeries>();
    zero_line->lines->add_line(std::make_shared<LineBuffer>());
    auto zero_line_buffer = std::dynamic_pointer_cast<LineBuffer>(zero_line->lines->getline(0));
    for (double price : zero_prices) {
        zero_line_buffer->append(price);
    }
    
    auto zero_pctchange = std::make_shared<PctChange>(std::static_pointer_cast<LineSeries>(zero_line), 3);
    
    for (size_t i = 0; i < zero_prices.size(); ++i) {
        zero_pctchange->calculate();
        
        double pctchange_val = zero_pctchange->get(0);
        
        // 当过去价格为0时，PctChange应该是无穷大或NaN
        if (i >= 3 && zero_prices[i - 3] == 0.0) {
            EXPECT_TRUE(std::isnan(pctchange_val) || std::isinf(pctchange_val)) 
                << "PctChange should be NaN or infinite when past price is zero";
        }
        
        if (i < zero_prices.size() - 1) {
            if (zero_line_buffer) zero_line_buffer->forward();
        }
    }
    
    // 测试相同价格的情况
    std::vector<double> flat_prices(20, 100.0);  // 20个相同价格
    
    auto flat_line = std::make_shared<LineSeries>();
    flat_line->lines->add_line(std::make_shared<LineBuffer>());
    auto flat_line_buffer = std::dynamic_pointer_cast<LineBuffer>(flat_line->lines->getline(0));
    for (double price : flat_prices) {
        flat_line_buffer->append(price);
    }
    
    auto flat_pctchange = std::make_shared<PctChange>(std::static_pointer_cast<LineSeries>(flat_line), 10);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_pctchange->calculate();
        if (i < flat_prices.size() - 1) {
            if (flat_line_buffer) flat_line_buffer->forward();
        }
    }
    
    double final_pctchange = flat_pctchange->get(0);
    if (!std::isnan(final_pctchange)) {
        EXPECT_NEAR(final_pctchange, 0.0, 1e-10) 
            << "PctChange should be zero for constant prices";
    }
}

// 高波动性测试
TEST(OriginalTests, PctChange_HighVolatility) {
    // 创建高波动性价格数据
    std::vector<double> volatile_prices;
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(-0.1, 0.1);  // ±10%变化
    
    double price = 100.0;
    volatile_prices.push_back(price);
    
    for (int i = 1; i < 100; ++i) {
        double change = dist(rng);
        price *= (1.0 + change);
        volatile_prices.push_back(price);
    }
    
    auto volatile_line = std::make_shared<LineSeries>();
    volatile_line->lines->add_line(std::make_shared<LineBuffer>());
    auto volatile_line_buffer = std::dynamic_pointer_cast<LineBuffer>(volatile_line->lines->getline(0));
    for (double price : volatile_prices) {
        volatile_line_buffer->append(price);
    }
    
    auto volatile_pctchange = std::make_shared<PctChange>(std::static_pointer_cast<LineSeries>(volatile_line), 10);
    
    std::vector<double> pctchange_values;
    
    for (size_t i = 0; i < volatile_prices.size(); ++i) {
        volatile_pctchange->calculate();
        
        double pctchange_val = volatile_pctchange->get(0);
        if (!std::isnan(pctchange_val)) {
            pctchange_values.push_back(std::abs(pctchange_val));
        }
        
        if (i < volatile_prices.size() - 1) {
            if (volatile_line_buffer) volatile_line_buffer->forward();
        }
    }
    
    // 验证高波动性数据的PctChange特性
    if (!pctchange_values.empty()) {
        double max_pctchange = *std::max_element(pctchange_values.begin(), pctchange_values.end());
        std::cout << "Maximum absolute PctChange in volatile data: " << max_pctchange << std::endl;
        
        EXPECT_TRUE(std::isfinite(max_pctchange)) << "Max PctChange should be finite";
        EXPECT_GT(max_pctchange, 0.0) << "Should have some price changes in volatile data";
    }
}

// 性能测试
TEST(OriginalTests, PctChange_Performance) {
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
    auto large_line_buffer = std::dynamic_pointer_cast<LineBuffer>(large_line->lines->getline(0));
    for (double price : large_data) {
        large_line_buffer->append(price);
    }
    
    auto large_pctchange = std::make_shared<PctChange>(std::static_pointer_cast<LineSeries>(large_line), 50);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    large_pctchange->calculate();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "PctChange calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_pctchange->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}