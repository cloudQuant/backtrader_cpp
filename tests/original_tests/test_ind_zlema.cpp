/**
 * @file test_ind_zlema.cpp
 * @brief ZLEMA指标测试 - 对应Python test_ind_zlema.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4125.487746', '3778.694000', '3620.284712']
 * ]
 * chkmin = 44
 * chkind = btind.ZLEMA
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/zlema.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> ZLEMA_EXPECTED_VALUES = {
    {"4125.487746", "3778.694000", "3620.284712"}
};

const int ZLEMA_MIN_PERIOD = 44;

} // anonymous namespace

// 使用默认参数的ZLEMA测试
DEFINE_INDICATOR_TEST(ZLEMA_Default, ZLEMA, ZLEMA_EXPECTED_VALUES, ZLEMA_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, ZLEMA_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建ZLEMA指标（默认参数，产生44周期的最小周期）
    auto zlema = std::make_shared<ZLEMA>(close_line, 21);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        zlema->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 44;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"4125.487746", "3778.694000", "3620.284712"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = zlema->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "ZLEMA value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(zlema->getMinPeriod(), 44) << "ZLEMA minimum period should be 44";
}

// 参数化测试 - 测试不同周期的ZLEMA
class ZLEMAParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line_ = std::make_shared<LineRoot>(csv_data_.size(), "close");
        for (const auto& bar : csv_data_) {
            close_line_->forward(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineRoot> close_line_;
};

TEST_P(ZLEMAParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto zlema = std::make_shared<ZLEMA>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        zlema->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期（ZLEMA需要额外的lag period）
    int expected_min_period = 2 * period + 2;  // 简化的计算
    EXPECT_EQ(zlema->getMinPeriod(), expected_min_period) 
        << "ZLEMA minimum period for period " << period;
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_value = zlema->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last ZLEMA value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last ZLEMA value should be finite";
    }
}

// 测试不同的ZLEMA周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    ZLEMAParameterizedTest,
    ::testing::Values(10, 15, 21, 30)
);

// ZLEMA计算逻辑验证测试
TEST(OriginalTests, ZLEMA_CalculationLogic) {
    // 使用简单的测试数据验证ZLEMA计算
    std::vector<double> prices = {100.0, 102.0, 101.0, 103.0, 105.0, 104.0, 106.0, 108.0, 107.0, 109.0};
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "zlema_calc");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto zlema = std::make_shared<ZLEMA>(close_line, 5);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        zlema->calculate();
        
        double zlema_val = zlema->get(0);
        
        // ZLEMA应该产生有限值
        if (!std::isnan(zlema_val)) {
            EXPECT_TRUE(std::isfinite(zlema_val)) 
                << "ZLEMA should be finite at step " << i;
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// 与EMA比较测试 - ZLEMA应该更快响应
TEST(OriginalTests, ZLEMA_vs_EMA_Responsiveness) {
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
    
    auto step_line = std::make_shared<LineRoot>(step_prices.size(), "step");
    for (double price : step_prices) {
        step_line->forward(price);
    }
    
    auto zlema = std::make_shared<ZLEMA>(step_line, 20);
    auto ema = std::make_shared<EMA>(step_line, 20);
    
    std::vector<double> zlema_post_step;
    std::vector<double> ema_post_step;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        zlema->calculate();
        ema->calculate();
        
        double zlema_val = zlema->get(0);
        double ema_val = ema->get(0);
        
        if (!std::isnan(zlema_val) && !std::isnan(ema_val) && i >= 50) {
            zlema_post_step.push_back(zlema_val);
            ema_post_step.push_back(ema_val);
        }
        
        if (i < step_prices.size() - 1) {
            step_line->forward();
        }
    }
    
    // 分析ZLEMA和EMA对价格跳跃的响应
    if (!zlema_post_step.empty() && !ema_post_step.empty()) {
        double final_zlema = zlema_post_step.back();
        double final_ema = ema_post_step.back();
        
        std::cout << "Step response - Final ZLEMA: " << final_zlema 
                  << ", Final EMA: " << final_ema << std::endl;
        
        // ZLEMA应该更接近新价格水平
        double zlema_distance = std::abs(final_zlema - 120.0);
        double ema_distance = std::abs(final_ema - 120.0);
        
        EXPECT_LE(zlema_distance, ema_distance) 
            << "ZLEMA should be closer to new price level than EMA";
    }
}

// 滞后减少测试
TEST(OriginalTests, ZLEMA_LagReduction) {
    // 创建正弦波数据来测试滞后
    std::vector<double> sine_prices;
    for (int i = 0; i < 200; ++i) {
        double angle = i * M_PI / 50.0;  // 完整周期100个点
        sine_prices.push_back(100.0 + 10.0 * std::sin(angle));
    }
    
    auto sine_line = std::make_shared<LineRoot>(sine_prices.size(), "sine");
    for (double price : sine_prices) {
        sine_line->forward(price);
    }
    
    auto zlema = std::make_shared<ZLEMA>(sine_line, 20);
    auto ema = std::make_shared<EMA>(sine_line, 20);
    
    std::vector<double> zlema_values;
    std::vector<double> ema_values;
    std::vector<double> price_values;
    
    for (size_t i = 0; i < sine_prices.size(); ++i) {
        zlema->calculate();
        ema->calculate();
        
        double zlema_val = zlema->get(0);
        double ema_val = ema->get(0);
        
        if (!std::isnan(zlema_val) && !std::isnan(ema_val)) {
            zlema_values.push_back(zlema_val);
            ema_values.push_back(ema_val);
            price_values.push_back(sine_prices[i]);
        }
        
        if (i < sine_prices.size() - 1) {
            sine_line->forward();
        }
    }
    
    // 计算相关性和滞后
    if (zlema_values.size() >= 100) {
        // 取后100个点进行分析
        size_t start_idx = zlema_values.size() - 100;
        
        double zlema_price_corr = 0.0;
        double ema_price_corr = 0.0;
        
        // 简化的相关性计算
        for (size_t i = start_idx; i < zlema_values.size() - 1; ++i) {
            double price_change = price_values[i+1] - price_values[i];
            double zlema_change = zlema_values[i+1] - zlema_values[i];
            double ema_change = ema_values[i+1] - ema_values[i];
            
            if (price_change * zlema_change > 0) zlema_price_corr += 1.0;
            if (price_change * ema_change > 0) ema_price_corr += 1.0;
        }
        
        zlema_price_corr /= (zlema_values.size() - start_idx - 1);
        ema_price_corr /= (zlema_values.size() - start_idx - 1);
        
        std::cout << "Direction correlation - ZLEMA: " << zlema_price_corr 
                  << ", EMA: " << ema_price_corr << std::endl;
        
        // ZLEMA应该与价格变化方向有更好的相关性
        EXPECT_GE(zlema_price_corr, ema_price_corr) 
            << "ZLEMA should have better directional correlation than EMA";
    }
}

// 趋势跟踪能力测试
TEST(OriginalTests, ZLEMA_TrendTracking) {
    // 创建趋势数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 100; ++i) {
        trend_prices.push_back(100.0 + i * 0.5);  // 缓慢上升趋势
    }
    
    auto trend_line = std::make_shared<LineRoot>(trend_prices.size(), "trend");
    for (double price : trend_prices) {
        trend_line->forward(price);
    }
    
    auto zlema = std::make_shared<ZLEMA>(trend_line, 20);
    
    double prev_zlema = 0.0;
    bool has_prev = false;
    int increasing_count = 0;
    int total_count = 0;
    
    for (size_t i = 0; i < trend_prices.size(); ++i) {
        zlema->calculate();
        
        double current_zlema = zlema->get(0);
        
        if (!std::isnan(current_zlema)) {
            if (has_prev) {
                total_count++;
                if (current_zlema > prev_zlema) {
                    increasing_count++;
                }
            }
            prev_zlema = current_zlema;
            has_prev = true;
        }
        
        if (i < trend_prices.size() - 1) {
            trend_line->forward();
        }
    }
    
    // 在上升趋势中，ZLEMA应该主要呈上升趋势
    if (total_count > 0) {
        double increasing_ratio = static_cast<double>(increasing_count) / total_count;
        EXPECT_GT(increasing_ratio, 0.8) 
            << "ZLEMA should track uptrend effectively";
        
        std::cout << "Trend tracking - ZLEMA increasing ratio: " << increasing_ratio << std::endl;
    }
}

// 平滑性测试
TEST(OriginalTests, ZLEMA_Smoothness) {
    // 创建包含噪声的数据
    std::vector<double> noisy_prices;
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> noise_dist(-3.0, 3.0);
    
    for (int i = 0; i < 100; ++i) {
        double trend = 100.0 + i * 0.3;  // 缓慢上升趋势
        double noise = noise_dist(rng);   // 随机噪声
        noisy_prices.push_back(trend + noise);
    }
    
    auto noisy_line = std::make_shared<LineRoot>(noisy_prices.size(), "noisy");
    for (double price : noisy_prices) {
        noisy_line->forward(price);
    }
    
    auto zlema = std::make_shared<ZLEMA>(noisy_line, 15);
    
    std::vector<double> zlema_values;
    
    for (size_t i = 0; i < noisy_prices.size(); ++i) {
        zlema->calculate();
        
        double zlema_val = zlema->get(0);
        if (!std::isnan(zlema_val)) {
            zlema_values.push_back(zlema_val);
        }
        
        if (i < noisy_prices.size() - 1) {
            noisy_line->forward();
        }
    }
    
    // 计算ZLEMA的平滑性
    if (zlema_values.size() > 1) {
        std::vector<double> changes;
        for (size_t i = 1; i < zlema_values.size(); ++i) {
            changes.push_back(std::abs(zlema_values[i] - zlema_values[i-1]));
        }
        
        double avg_change = std::accumulate(changes.begin(), changes.end(), 0.0) / changes.size();
        std::cout << "ZLEMA smoothness (avg change): " << avg_change << std::endl;
        
        // ZLEMA应该比原始数据更平滑
        EXPECT_LT(avg_change, 3.0) << "ZLEMA should smooth out noise";
    }
}

// 边界条件测试
TEST(OriginalTests, ZLEMA_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_zlema = std::make_shared<ZLEMA>(flat_line, 20);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_zlema->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，ZLEMA应该等于该价格
    double final_zlema = flat_zlema->get(0);
    if (!std::isnan(final_zlema)) {
        EXPECT_NEAR(final_zlema, 100.0, 1e-6) 
            << "ZLEMA should equal constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 30; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_zlema = std::make_shared<ZLEMA>(insufficient_line, 21);
    
    for (int i = 0; i < 30; ++i) {
        insufficient_zlema->calculate();
        if (i < 29) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_zlema->get(0);
    EXPECT_TRUE(std::isnan(result)) << "ZLEMA should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, ZLEMA_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<double> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        large_data.push_back(dist(rng));
    }
    
    auto large_line = std::make_shared<LineRoot>(large_data.size(), "large");
    for (double price : large_data) {
        large_line->forward(price);
    }
    
    auto large_zlema = std::make_shared<ZLEMA>(large_line, 21);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_zlema->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "ZLEMA calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_zlema->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}