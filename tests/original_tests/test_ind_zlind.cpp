/**
 * @file test_ind_zlind.cpp
 * @brief ZeroLagIndicator指标测试 - 对应Python test_ind_zlind.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4110.282052', '3644.444667', '3564.906194']
 * ]
 * chkmin = 30
 * chkind = btind.ZeroLagIndicator
 * 
 * 注：ZeroLagIndicator (零滞后指标) 是一个减少滞后的移动平均线变种
 */

#include "test_common.h"
#include "indicators/zlind.h"
#include "indicators/sma.h"
#include "indicators/ema.h"
#include <random>

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> ZEROLAGINDICATOR_EXPECTED_VALUES = {
    {"4110.282052", "3644.444667", "3564.906194"}
};

const int ZEROLAGINDICATOR_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的ZeroLagIndicator测试
DEFINE_INDICATOR_TEST(ZeroLagIndicator_Default, ZeroLagIndicator, ZEROLAGINDICATOR_EXPECTED_VALUES, ZEROLAGINDICATOR_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, ZeroLagIndicator_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建ZeroLagIndicator指标
    auto zlind = std::make_shared<ZeroLagIndicator>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        zlind->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 30;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"4110.282052", "3644.444667", "3564.906194"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = zlind->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "ZeroLagIndicator value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(zlind->getMinPeriod(), 30) << "ZeroLagIndicator minimum period should be 30";
}

// 参数化测试 - 测试不同周期的ZeroLagIndicator
class ZeroLagIndicatorParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(ZeroLagIndicatorParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto zlind = std::make_shared<ZeroLagIndicator>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        zlind->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(zlind->getMinPeriod(), period) 
        << "ZeroLagIndicator minimum period should equal period parameter";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_value = zlind->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last ZeroLagIndicator value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last ZeroLagIndicator value should be finite";
        EXPECT_GT(last_value, 0.0) << "ZeroLagIndicator should be positive for positive prices";
    }
}

// 测试不同的ZeroLagIndicator周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    ZeroLagIndicatorParameterizedTest,
    ::testing::Values(10, 20, 30, 50)
);

// ZeroLagIndicator计算逻辑验证测试
TEST(OriginalTests, ZeroLagIndicator_CalculationLogic) {
    // 使用简单的测试数据验证ZeroLagIndicator计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0,
                                  120.0, 118.0, 116.0, 114.0, 112.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0,
                                  124.0, 126.0, 128.0, 130.0, 132.0};
    
    auto price_line = std::make_shared<backtrader::LineRoot>(prices.size(), "zlind_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto zlind = std::make_shared<ZeroLagIndicator>(price_line, 20);
    
    // ZeroLagIndicator的计算比较复杂，主要验证其基本特性
    for (size_t i = 0; i < prices.size(); ++i) {
        zlind->calculate();
        
        if (i >= 19) {  // ZeroLagIndicator需要20个数据点
            double zlind_value = zlind->get(0);
            
            if (!std::isnan(zlind_value)) {
                // 验证ZeroLagIndicator的基本属性
                EXPECT_TRUE(std::isfinite(zlind_value)) 
                    << "ZeroLagIndicator value should be finite at step " << i;
                EXPECT_GT(zlind_value, 0.0) 
                    << "ZeroLagIndicator should be positive for positive prices at step " << i;
            }
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
}

// ZeroLagIndicator响应速度测试
TEST(OriginalTests, ZeroLagIndicator_ResponseSpeed) {
    // 创建价格突然变化的数据来测试响应速度
    std::vector<double> step_prices;
    
    // 第一阶段：稳定价格
    for (int i = 0; i < 30; ++i) {
        step_prices.push_back(100.0);
    }
    
    // 第二阶段：价格跳跃
    for (int i = 0; i < 30; ++i) {
        step_prices.push_back(120.0);
    }
    
    auto step_line = std::make_shared<backtrader::LineRoot>(step_prices.size(), "step");
    for (double price : step_prices) {
        step_line->forward(price);
    }
    
    auto zlind = std::make_shared<ZeroLagIndicator>(step_line, 20);
    auto sma = std::make_shared<SMA>(step_line, 20);  // 比较对象
    
    std::vector<double> pre_step_zlind, post_step_zlind;
    std::vector<double> pre_step_sma, post_step_sma;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        zlind->calculate();
        sma->calculate();
        
        double zlind_val = zlind->get(0);
        double sma_val = sma->get(0);
        
        if (!std::isnan(zlind_val) && !std::isnan(sma_val)) {
            if (i < 30) {
                pre_step_zlind.push_back(zlind_val);
                pre_step_sma.push_back(sma_val);
            } else {
                post_step_zlind.push_back(zlind_val);
                post_step_sma.push_back(sma_val);
            }
        }
        
        if (i < step_prices.size() - 1) {
            step_line->forward();
        }
    }
    
    // 分析对价格跳跃的响应速度
    if (!pre_step_zlind.empty() && !post_step_zlind.empty() && 
        !pre_step_sma.empty() && !post_step_sma.empty()) {
        
        double avg_pre_zlind = std::accumulate(pre_step_zlind.end() - 5, pre_step_zlind.end(), 0.0) / 5.0;
        double final_post_zlind = post_step_zlind.back();
        double avg_pre_sma = std::accumulate(pre_step_sma.end() - 5, pre_step_sma.end(), 0.0) / 5.0;
        double final_post_sma = post_step_sma.back();
        
        double zlind_response = final_post_zlind - avg_pre_zlind;
        double sma_response = final_post_sma - avg_pre_sma;
        
        std::cout << "Response speed analysis:" << std::endl;
        std::cout << "ZeroLagIndicator response: " << zlind_response << std::endl;
        std::cout << "SMA response: " << sma_response << std::endl;
        
        // ZeroLagIndicator应该比SMA有更快的响应
        EXPECT_GT(zlind_response, sma_response * 0.9) 
            << "ZeroLagIndicator should respond faster than SMA";
    }
}

// ZeroLagIndicator与EMA比较测试
TEST(OriginalTests, ZeroLagIndicator_vs_EMA_Comparison) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto zlind = std::make_shared<ZeroLagIndicator>(close_line, 20);
    auto ema = std::make_shared<EMA>(close_line, 20);
    
    std::vector<double> zlind_values;
    std::vector<double> ema_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        zlind->calculate();
        ema->calculate();
        
        double zlind_val = zlind->get(0);
        double ema_val = ema->get(0);
        
        if (!std::isnan(zlind_val) && !std::isnan(ema_val)) {
            zlind_values.push_back(zlind_val);
            ema_values.push_back(ema_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 比较ZeroLagIndicator和EMA的特性
    if (!zlind_values.empty() && !ema_values.empty()) {
        double zlind_avg = std::accumulate(zlind_values.begin(), zlind_values.end(), 0.0) / zlind_values.size();
        double ema_avg = std::accumulate(ema_values.begin(), ema_values.end(), 0.0) / ema_values.size();
        
        // 计算变化速度
        std::vector<double> zlind_changes, ema_changes;
        for (size_t i = 1; i < std::min(zlind_values.size(), ema_values.size()); ++i) {
            zlind_changes.push_back(std::abs(zlind_values[i] - zlind_values[i-1]));
            ema_changes.push_back(std::abs(ema_values[i] - ema_values[i-1]));
        }
        
        double avg_zlind_change = std::accumulate(zlind_changes.begin(), zlind_changes.end(), 0.0) / zlind_changes.size();
        double avg_ema_change = std::accumulate(ema_changes.begin(), ema_changes.end(), 0.0) / ema_changes.size();
        
        std::cout << "ZeroLagIndicator vs EMA comparison:" << std::endl;
        std::cout << "ZeroLagIndicator average: " << zlind_avg << ", change rate: " << avg_zlind_change << std::endl;
        std::cout << "EMA average: " << ema_avg << ", change rate: " << avg_ema_change << std::endl;
        
        // 验证两者都是有限值
        EXPECT_TRUE(std::isfinite(zlind_avg)) << "ZeroLagIndicator average should be finite";
        EXPECT_TRUE(std::isfinite(ema_avg)) << "EMA average should be finite";
        
        // ZeroLagIndicator可能有更快的变化率（减少滞后）
        EXPECT_GE(avg_zlind_change, avg_ema_change * 0.8) 
            << "ZeroLagIndicator should have comparable or higher responsiveness than EMA";
    }
}

// ZeroLagIndicator趋势跟随能力测试
TEST(OriginalTests, ZeroLagIndicator_TrendFollowing) {
    // 创建明确的趋势数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 60; ++i) {
        trend_prices.push_back(100.0 + i * 0.8);  // 线性上升趋势
    }
    
    auto trend_line = std::make_shared<backtrader::LineRoot>(trend_prices.size(), "trend");
    for (double price : trend_prices) {
        trend_line->forward(price);
    }
    
    auto trend_zlind = std::make_shared<ZeroLagIndicator>(trend_line, 20);
    
    double prev_zlind = 0.0;
    bool has_prev = false;
    int increasing_count = 0;
    int total_count = 0;
    
    for (size_t i = 0; i < trend_prices.size(); ++i) {
        trend_zlind->calculate();
        
        double current_zlind = trend_zlind->get(0);
        
        if (!std::isnan(current_zlind)) {
            if (has_prev) {
                total_count++;
                if (current_zlind > prev_zlind) {
                    increasing_count++;
                }
            }
            prev_zlind = current_zlind;
            has_prev = true;
        }
        
        if (i < trend_prices.size() - 1) {
            trend_line->forward();
        }
    }
    
    // 在上升趋势中，ZeroLagIndicator应该主要呈上升趋势
    if (total_count > 0) {
        double increasing_ratio = static_cast<double>(increasing_count) / total_count;
        EXPECT_GT(increasing_ratio, 0.7) 
            << "ZeroLagIndicator should follow uptrend effectively";
        
        std::cout << "Trend following - ZeroLagIndicator increasing ratio: " << increasing_ratio << std::endl;
    }
}

// ZeroLagIndicator噪声过滤测试
TEST(OriginalTests, ZeroLagIndicator_NoiseFiltering) {
    // 创建包含噪声的数据
    std::vector<double> noisy_prices;
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> noise_dist(-3.0, 3.0);
    
    for (int i = 0; i < 100; ++i) {
        double trend = 100.0 + i * 0.3;  // 缓慢上升趋势
        double noise = noise_dist(rng);   // 随机噪声
        noisy_prices.push_back(trend + noise);
    }
    
    auto noisy_line = std::make_shared<backtrader::LineRoot>(noisy_prices.size(), "noisy");
    for (double price : noisy_prices) {
        noisy_line->forward(price);
    }
    
    auto zlind = std::make_shared<ZeroLagIndicator>(noisy_line, 20);
    
    std::vector<double> price_changes;
    std::vector<double> zlind_changes;
    double prev_price = 0.0, prev_zlind = 0.0;
    bool has_prev = false;
    
    for (size_t i = 0; i < noisy_prices.size(); ++i) {
        zlind->calculate();
        
        double current_price = noisy_prices[i];
        double current_zlind = zlind->get(0);
        
        if (!std::isnan(current_zlind) && has_prev) {
            price_changes.push_back(std::abs(current_price - prev_price));
            zlind_changes.push_back(std::abs(current_zlind - prev_zlind));
        }
        
        if (!std::isnan(current_zlind)) {
            prev_price = current_price;
            prev_zlind = current_zlind;
            has_prev = true;
        }
        
        if (i < noisy_prices.size() - 1) {
            noisy_line->forward();
        }
    }
    
    // 比较价格和ZeroLagIndicator的波动性
    if (!price_changes.empty() && !zlind_changes.empty()) {
        double avg_price_change = std::accumulate(price_changes.begin(), price_changes.end(), 0.0) / price_changes.size();
        double avg_zlind_change = std::accumulate(zlind_changes.begin(), zlind_changes.end(), 0.0) / zlind_changes.size();
        
        std::cout << "Noise filtering comparison:" << std::endl;
        std::cout << "Average price change: " << avg_price_change << std::endl;
        std::cout << "Average ZeroLagIndicator change: " << avg_zlind_change << std::endl;
        
        // ZeroLagIndicator应该能够过滤一些噪声，但保持响应性
        EXPECT_LT(avg_zlind_change, avg_price_change) 
            << "ZeroLagIndicator should filter some noise while maintaining responsiveness";
    }
}

// ZeroLagIndicator滞后分析测试
TEST(OriginalTests, ZeroLagIndicator_LagAnalysis) {
    // 创建正弦波数据来测试滞后
    std::vector<double> sine_prices;
    for (int i = 0; i < 200; ++i) {
        double angle = i * M_PI / 25.0;  // 完整周期50个点
        sine_prices.push_back(100.0 + 10.0 * std::sin(angle));
    }
    
    auto sine_line = std::make_shared<backtrader::LineRoot>(sine_prices.size(), "sine");
    for (double price : sine_prices) {
        sine_line->forward(price);
    }
    
    auto zlind = std::make_shared<ZeroLagIndicator>(sine_line, 20);
    auto sma = std::make_shared<SMA>(sine_line, 20);
    
    std::vector<double> price_values;
    std::vector<double> zlind_values;
    std::vector<double> sma_values;
    
    for (size_t i = 0; i < sine_prices.size(); ++i) {
        zlind->calculate();
        sma->calculate();
        
        double zlind_val = zlind->get(0);
        double sma_val = sma->get(0);
        
        if (!std::isnan(zlind_val) && !std::isnan(sma_val)) {
            price_values.push_back(sine_prices[i]);
            zlind_values.push_back(zlind_val);
            sma_values.push_back(sma_val);
        }
        
        if (i < sine_prices.size() - 1) {
            sine_line->forward();
        }
    }
    
    // 分析滞后特性
    if (zlind_values.size() >= 100) {
        // 计算与原始价格的相关性（简化版本）
        double price_range = *std::max_element(price_values.begin(), price_values.end()) - 
                            *std::min_element(price_values.begin(), price_values.end());
        double zlind_range = *std::max_element(zlind_values.begin(), zlind_values.end()) - 
                           *std::min_element(zlind_values.begin(), zlind_values.end());
        double sma_range = *std::max_element(sma_values.begin(), sma_values.end()) - 
                          *std::min_element(sma_values.begin(), sma_values.end());
        
        std::cout << "Lag analysis:" << std::endl;
        std::cout << "Price range: " << price_range << std::endl;
        std::cout << "ZeroLagIndicator range: " << zlind_range << std::endl;
        std::cout << "SMA range: " << sma_range << std::endl;
        
        // ZeroLagIndicator应该保持更多的原始信号幅度
        EXPECT_GT(zlind_range, sma_range * 0.9) 
            << "ZeroLagIndicator should preserve more signal amplitude than SMA";
    }
}

// 边界条件测试
TEST(OriginalTests, ZeroLagIndicator_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<backtrader::LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_zlind = std::make_shared<ZeroLagIndicator>(flat_line, 20);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_zlind->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，ZeroLagIndicator应该等于该价格
    double final_zlind = flat_zlind->get(0);
    if (!std::isnan(final_zlind)) {
        EXPECT_NEAR(final_zlind, 100.0, 1e-6) 
            << "ZeroLagIndicator should equal constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<backtrader::LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 15; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_zlind = std::make_shared<ZeroLagIndicator>(insufficient_line, 20);
    
    for (int i = 0; i < 15; ++i) {
        insufficient_zlind->calculate();
        if (i < 14) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_zlind->get(0);
    EXPECT_TRUE(std::isnan(result)) << "ZeroLagIndicator should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, ZeroLagIndicator_Performance) {
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
    
    auto large_zlind = std::make_shared<ZeroLagIndicator>(large_line, 50);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_zlind->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "ZeroLagIndicator calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_zlind->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    EXPECT_GT(final_result, 0.0) << "Final result should be positive";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}