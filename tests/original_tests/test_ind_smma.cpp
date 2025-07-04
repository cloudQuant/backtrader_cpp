/**
 * @file test_ind_smma.cpp
 * @brief SMMA指标测试 - 对应Python test_ind_smma.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4021.569725', '3644.444667', '3616.427648']
 * ]
 * chkmin = 30
 * chkind = btind.SMMA
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/smma.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> SMMA_EXPECTED_VALUES = {
    {"4021.569725", "3644.444667", "3616.427648"}
};

const int SMMA_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的SMMA测试
DEFINE_INDICATOR_TEST(SMMA_Default, SMMA, SMMA_EXPECTED_VALUES, SMMA_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, SMMA_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建SMMA指标（30周期）
    auto smma = std::make_shared<SMMA>(close_line, 30);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        smma->calculate();
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
    
    std::vector<std::string> expected = {"4021.569725", "3644.444667", "3616.427648"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = smma->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "SMMA value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(smma->getMinPeriod(), 30) << "SMMA minimum period should be 30";
}

// 参数化测试 - 测试不同周期的SMMA
class SMMAParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(SMMAParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto smma = std::make_shared<SMMA>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        smma->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(smma->getMinPeriod(), period) 
        << "SMMA minimum period should equal period parameter";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_value = smma->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last SMMA value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last SMMA value should be finite";
        EXPECT_GT(last_value, 0.0) << "SMMA should be positive for positive prices";
    }
}

// 测试不同的SMMA周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    SMMAParameterizedTest,
    ::testing::Values(10, 20, 30, 50)
);

// SMMA计算逻辑验证测试
TEST(OriginalTests, SMMA_CalculationLogic) {
    // 使用简单的测试数据验证SMMA计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 108.0, 106.0, 104.0, 102.0};
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "smma_calc");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto smma = std::make_shared<SMMA>(close_line, 5);
    
    // 手动计算SMMA进行验证
    double manual_smma = 0.0;
    bool first_calculation = true;
    
    for (size_t i = 0; i < prices.size(); ++i) {
        smma->calculate();
        
        if (i >= 4) {  // SMMA需要5个数据点
            if (first_calculation) {
                // 第一个SMMA值是SMA
                manual_smma = (prices[0] + prices[1] + prices[2] + prices[3] + prices[4]) / 5.0;
                first_calculation = false;
            } else {
                // 后续SMMA值：(prev_smma * (period - 1) + current_price) / period
                manual_smma = (manual_smma * 4.0 + prices[i]) / 5.0;
            }
            
            double actual_smma = smma->get(0);
            
            if (!std::isnan(actual_smma)) {
                EXPECT_NEAR(actual_smma, manual_smma, 1e-10) 
                    << "SMMA calculation mismatch at step " << i;
            }
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// SMMA平滑特性测试
TEST(OriginalTests, SMMA_SmoothingCharacteristics) {
    // 创建包含噪声的数据
    std::vector<double> noisy_prices;
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> noise_dist(-2.0, 2.0);
    
    for (int i = 0; i < 100; ++i) {
        double trend = 100.0 + i * 0.5;  // 缓慢上升趋势
        double noise = noise_dist(rng);   // 随机噪声
        noisy_prices.push_back(trend + noise);
    }
    
    auto noisy_line = std::make_shared<LineRoot>(noisy_prices.size(), "noisy");
    for (double price : noisy_prices) {
        noisy_line->forward(price);
    }
    
    auto smma = std::make_shared<SMMA>(noisy_line, 20);
    auto sma = std::make_shared<SMA>(noisy_line, 20);  // 比较对象
    
    std::vector<double> smma_changes;
    std::vector<double> sma_changes;
    double prev_smma = 0.0, prev_sma = 0.0;
    bool has_prev = false;
    
    for (size_t i = 0; i < noisy_prices.size(); ++i) {
        smma->calculate();
        sma->calculate();
        
        double current_smma = smma->get(0);
        double current_sma = sma->get(0);
        
        if (!std::isnan(current_smma) && !std::isnan(current_sma)) {
            if (has_prev) {
                smma_changes.push_back(std::abs(current_smma - prev_smma));
                sma_changes.push_back(std::abs(current_sma - prev_sma));
            }
            prev_smma = current_smma;
            prev_sma = current_sma;
            has_prev = true;
        }
        
        if (i < noisy_prices.size() - 1) {
            noisy_line->forward();
        }
    }
    
    // 比较SMMA和SMA的平滑性
    if (!smma_changes.empty() && !sma_changes.empty()) {
        double avg_smma_change = std::accumulate(smma_changes.begin(), smma_changes.end(), 0.0) / smma_changes.size();
        double avg_sma_change = std::accumulate(sma_changes.begin(), sma_changes.end(), 0.0) / sma_changes.size();
        
        std::cout << "Smoothing comparison:" << std::endl;
        std::cout << "Average SMMA change: " << avg_smma_change << std::endl;
        std::cout << "Average SMA change: " << avg_sma_change << std::endl;
        
        // SMMA应该比SMA更平滑（变化更小）
        EXPECT_LT(avg_smma_change, avg_sma_change) 
            << "SMMA should be smoother than SMA";
    }
}

// SMMA趋势跟随能力测试
TEST(OriginalTests, SMMA_TrendFollowing) {
    // 创建明确的趋势数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 100; ++i) {
        trend_prices.push_back(100.0 + i * 1.0);  // 线性上升趋势
    }
    
    auto trend_line = std::make_shared<LineRoot>(trend_prices.size(), "trend");
    for (double price : trend_prices) {
        trend_line->forward(price);
    }
    
    auto smma = std::make_shared<SMMA>(trend_line, 20);
    
    double prev_smma = 0.0;
    bool has_prev = false;
    int increasing_count = 0;
    int total_count = 0;
    
    for (size_t i = 0; i < trend_prices.size(); ++i) {
        smma->calculate();
        
        double current_smma = smma->get(0);
        
        if (!std::isnan(current_smma)) {
            if (has_prev) {
                total_count++;
                if (current_smma > prev_smma) {
                    increasing_count++;
                }
            }
            prev_smma = current_smma;
            has_prev = true;
        }
        
        if (i < trend_prices.size() - 1) {
            trend_line->forward();
        }
    }
    
    // 在上升趋势中，SMMA应该主要呈上升趋势
    if (total_count > 0) {
        double increasing_ratio = static_cast<double>(increasing_count) / total_count;
        EXPECT_GT(increasing_ratio, 0.8) 
            << "SMMA should follow uptrend effectively";
        
        std::cout << "Trend following - SMMA increasing ratio: " << increasing_ratio << std::endl;
    }
}

// SMMA与EMA比较测试
TEST(OriginalTests, SMMA_vs_EMA_Comparison) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto smma = std::make_shared<SMMA>(close_line, 20);
    auto ema = std::make_shared<EMA>(close_line, 20);
    
    std::vector<double> smma_values;
    std::vector<double> ema_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        smma->calculate();
        ema->calculate();
        
        double smma_val = smma->get(0);
        double ema_val = ema->get(0);
        
        if (!std::isnan(smma_val) && !std::isnan(ema_val)) {
            smma_values.push_back(smma_val);
            ema_values.push_back(ema_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 比较SMMA和EMA的特性
    if (!smma_values.empty() && !ema_values.empty()) {
        double final_smma = smma_values.back();
        double final_ema = ema_values.back();
        
        std::cout << "SMMA vs EMA comparison:" << std::endl;
        std::cout << "Final SMMA: " << final_smma << std::endl;
        std::cout << "Final EMA: " << final_ema << std::endl;
        
        // 验证两者都是有限值
        EXPECT_TRUE(std::isfinite(final_smma)) << "Final SMMA should be finite";
        EXPECT_TRUE(std::isfinite(final_ema)) << "Final EMA should be finite";
    }
}

// SMMA响应速度测试
TEST(OriginalTests, SMMA_ResponseSpeed) {
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
    
    auto smma = std::make_shared<SMMA>(step_line, 20);
    
    std::vector<double> pre_step_smma;
    std::vector<double> post_step_smma;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        smma->calculate();
        
        double smma_val = smma->get(0);
        
        if (!std::isnan(smma_val)) {
            if (i < 50) {
                pre_step_smma.push_back(smma_val);
            } else {
                post_step_smma.push_back(smma_val);
            }
        }
        
        if (i < step_prices.size() - 1) {
            step_line->forward();
        }
    }
    
    // 分析SMMA对价格跳跃的响应
    if (!pre_step_smma.empty() && !post_step_smma.empty()) {
        double avg_pre = std::accumulate(pre_step_smma.end() - 10, pre_step_smma.end(), 0.0) / 10.0;
        double final_post = post_step_smma.back();
        
        std::cout << "Step response - Pre-step SMMA: " << avg_pre 
                  << ", Final post-step SMMA: " << final_post << std::endl;
        
        // SMMA应该能够响应价格变化，但比较平滑
        EXPECT_GT(final_post, avg_pre) << "SMMA should respond to price step";
        EXPECT_LT(final_post, 120.0) << "SMMA should lag behind price step";
        EXPECT_GT(final_post, 110.0) << "SMMA should partially adapt to new price level";
    }
}

// SMMA滞后特性测试
TEST(OriginalTests, SMMA_LagCharacteristics) {
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
    
    auto smma = std::make_shared<SMMA>(sine_line, 20);
    auto sma = std::make_shared<SMA>(sine_line, 20);
    
    std::vector<double> price_values;
    std::vector<double> smma_values;
    std::vector<double> sma_values;
    
    for (size_t i = 0; i < sine_prices.size(); ++i) {
        smma->calculate();
        sma->calculate();
        
        double smma_val = smma->get(0);
        double sma_val = sma->get(0);
        
        if (!std::isnan(smma_val) && !std::isnan(sma_val)) {
            price_values.push_back(sine_prices[i]);
            smma_values.push_back(smma_val);
            sma_values.push_back(sma_val);
        }
        
        if (i < sine_prices.size() - 1) {
            sine_line->forward();
        }
    }
    
    // 分析滞后特性
    if (smma_values.size() >= 100) {
        // 计算价格和SMMA的相关性（简化版本）
        double price_range = *std::max_element(price_values.begin(), price_values.end()) - 
                            *std::min_element(price_values.begin(), price_values.end());
        double smma_range = *std::max_element(smma_values.begin(), smma_values.end()) - 
                           *std::min_element(smma_values.begin(), smma_values.end());
        
        std::cout << "Lag characteristics:" << std::endl;
        std::cout << "Price range: " << price_range << std::endl;
        std::cout << "SMMA range: " << smma_range << std::endl;
        
        // SMMA的波动范围应该小于原始价格
        EXPECT_LT(smma_range, price_range) 
            << "SMMA should have smaller range than original prices";
    }
}

// 边界条件测试
TEST(OriginalTests, SMMA_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_smma = std::make_shared<SMMA>(flat_line, 20);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_smma->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，SMMA应该等于该价格
    double final_smma = flat_smma->get(0);
    if (!std::isnan(final_smma)) {
        EXPECT_NEAR(final_smma, 100.0, 1e-6) 
            << "SMMA should equal constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 15; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_smma = std::make_shared<SMMA>(insufficient_line, 20);
    
    for (int i = 0; i < 15; ++i) {
        insufficient_smma->calculate();
        if (i < 14) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_smma->get(0);
    EXPECT_TRUE(std::isnan(result)) << "SMMA should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, SMMA_Performance) {
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
    
    auto large_smma = std::make_shared<SMMA>(large_line, 50);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_smma->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "SMMA calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_smma->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    EXPECT_GT(final_result, 0.0) << "Final result should be positive";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}