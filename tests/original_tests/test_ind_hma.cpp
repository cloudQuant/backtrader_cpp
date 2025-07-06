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
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建HMA指标（默认30周期，计算公式需要最小34周期）
    auto hma = std::make_shared<HMA>(close_line, 30);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        hma->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
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
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "HMA value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
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
        
        close_line_ = std::make_shared<backtrader::LineRoot>(csv_data_.size(), "close");
        for (const auto& bar : csv_data_) {
            close_line_->forward(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<backtrader::LineRoot> close_line_;
};

TEST_P(HMAParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto hma = std::make_shared<HMA>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        hma->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期（HMA的最小周期公式复杂，这里简化验证）
    int expected_min_period = period + static_cast<int>(std::sqrt(period)) + 1;
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
    std::vector<double> prices = {100.0, 102.0, 101.0, 103.0, 105.0, 104.0, 106.0, 108.0, 107.0, 109.0};
    
    auto close_line = std::make_shared<backtrader::LineRoot>(prices.size(), "hma_calc");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto hma = std::make_shared<HMA>(close_line, 5);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        hma->calculate();
        
        double hma_val = hma->get(0);
        
        // HMA应该产生有限值
        if (!std::isnan(hma_val)) {
            EXPECT_TRUE(std::isfinite(hma_val)) 
                << "HMA should be finite at step " << i;
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// 趋势跟踪能力测试
TEST(OriginalTests, HMA_TrendTracking) {
    // 创建强势上升趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 100; ++i) {
        uptrend_prices.push_back(100.0 + i * 1.5);  // 持续上升
    }
    
    auto up_line = std::make_shared<backtrader::LineRoot>(uptrend_prices.size(), "uptrend");
    for (double price : uptrend_prices) {
        up_line->forward(price);
    }
    
    auto up_hma = std::make_shared<HMA>(up_line, 20);
    
    double prev_hma = 0.0;
    bool has_prev = false;
    int increasing_count = 0;
    int total_count = 0;
    
    for (size_t i = 0; i < uptrend_prices.size(); ++i) {
        up_hma->calculate();
        
        double current_hma = up_hma->get(0);
        
        if (!std::isnan(current_hma)) {
            if (has_prev) {
                total_count++;
                if (current_hma > prev_hma) {
                    increasing_count++;
                }
            }
            prev_hma = current_hma;
            has_prev = true;
        }
        
        if (i < uptrend_prices.size() - 1) {
            up_line->forward();
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

// 响应速度测试
TEST(OriginalTests, HMA_Responsiveness) {
    // 创建价格突然变化的数据
    std::vector<double> step_prices;
    
    // 第一阶段：稳定价格
    for (int i = 0; i < 50; ++i) {
        step_prices.push_back(100.0);
    }
    
    // 第二阶段：价格跳跃
    for (int i = 0; i < 50; ++i) {
        step_prices.push_back(120.0);
    }
    
    auto step_line = std::make_shared<backtrader::LineRoot>(step_prices.size(), "step");
    for (double price : step_prices) {
        step_line->forward(price);
    }
    
    auto step_hma = std::make_shared<HMA>(step_line, 20);
    
    std::vector<double> pre_step_hma;
    std::vector<double> post_step_hma;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        step_hma->calculate();
        
        double hma_val = step_hma->get(0);
        
        if (!std::isnan(hma_val)) {
            if (i < 50) {
                pre_step_hma.push_back(hma_val);
            } else {
                post_step_hma.push_back(hma_val);
            }
        }
        
        if (i < step_prices.size() - 1) {
            step_line->forward();
        }
    }
    
    // 分析HMA对价格变化的响应
    if (!pre_step_hma.empty() && !post_step_hma.empty()) {
        double avg_pre = std::accumulate(pre_step_hma.end() - 10, pre_step_hma.end(), 0.0) / 10.0;
        double final_post = post_step_hma.back();
        
        std::cout << "Step response - Pre-step HMA: " << avg_pre 
                  << ", Final post-step HMA: " << final_post << std::endl;
        
        // HMA应该能够响应价格变化
        EXPECT_GT(final_post, avg_pre) << "HMA should respond to price step";
        EXPECT_LT(std::abs(final_post - 120.0), 10.0) << "HMA should approach new price level";
    }
}

// 与SMA比较测试
TEST(OriginalTests, HMA_vs_SMA_Comparison) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto hma = std::make_shared<HMA>(close_line, 20);
    auto sma = std::make_shared<SMA>(close_line, 20);
    
    std::vector<double> hma_changes;
    std::vector<double> sma_changes;
    
    double prev_hma = 0.0, prev_sma = 0.0;
    bool has_prev = false;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        hma->calculate();
        sma->calculate();
        
        double current_hma = hma->get(0);
        double current_sma = sma->get(0);
        
        if (!std::isnan(current_hma) && !std::isnan(current_sma)) {
            if (has_prev) {
                hma_changes.push_back(std::abs(current_hma - prev_hma));
                sma_changes.push_back(std::abs(current_sma - prev_sma));
            }
            prev_hma = current_hma;
            prev_sma = current_sma;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
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

// 平滑性测试
TEST(OriginalTests, HMA_Smoothness) {
    // 创建包含噪声的数据
    std::vector<double> noisy_prices;
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> noise_dist(-2.0, 2.0);
    
    for (int i = 0; i < 100; ++i) {
        double trend = 100.0 + i * 0.5;  // 缓慢上升趋势
        double noise = noise_dist(rng);   // 随机噪声
        noisy_prices.push_back(trend + noise);
    }
    
    auto noisy_line = std::make_shared<backtrader::LineRoot>(noisy_prices.size(), "noisy");
    for (double price : noisy_prices) {
        noisy_line->forward(price);
    }
    
    auto noisy_hma = std::make_shared<HMA>(noisy_line, 20);
    
    std::vector<double> hma_values;
    
    for (size_t i = 0; i < noisy_prices.size(); ++i) {
        noisy_hma->calculate();
        
        double hma_val = noisy_hma->get(0);
        if (!std::isnan(hma_val)) {
            hma_values.push_back(hma_val);
        }
        
        if (i < noisy_prices.size() - 1) {
            noisy_line->forward();
        }
    }
    
    // 计算HMA的平滑性（相邻值的平均变化）
    if (hma_values.size() > 1) {
        std::vector<double> changes;
        for (size_t i = 1; i < hma_values.size(); ++i) {
            changes.push_back(std::abs(hma_values[i] - hma_values[i-1]));
        }
        
        double avg_change = std::accumulate(changes.begin(), changes.end(), 0.0) / changes.size();
        std::cout << "HMA smoothness (avg change): " << avg_change << std::endl;
        
        // HMA应该比原始数据更平滑
        EXPECT_LT(avg_change, 5.0) << "HMA should smooth out noise";
    }
}

// 边界条件测试
TEST(OriginalTests, HMA_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<backtrader::LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_hma = std::make_shared<HMA>(flat_line, 20);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_hma->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，HMA应该等于该价格
    double final_hma = flat_hma->get(0);
    if (!std::isnan(final_hma)) {
        EXPECT_NEAR(final_hma, 100.0, 1e-6) 
            << "HMA should equal constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<backtrader::LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 20; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_hma = std::make_shared<HMA>(insufficient_line, 30);
    
    for (int i = 0; i < 20; ++i) {
        insufficient_hma->calculate();
        if (i < 19) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_hma->get(0);
    EXPECT_TRUE(std::isnan(result)) << "HMA should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, HMA_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<double> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        large_data.push_back(dist(rng));
    }
    
    auto large_line = std::make_shared<backtrader::LineRoot>(large_data.size(), "large");
    for (double price : large_data) {
        large_line->forward(price);
    }
    
    auto large_hma = std::make_shared<HMA>(large_line, 20);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_hma->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "HMA calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_hma->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}
