/**
 * @file test_ind_kama.cpp
 * @brief KAMA指标测试 - 对应Python test_ind_kama.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4054.187922', '3648.549000', '3592.979190']
 * ]
 * chkmin = 31
 * chkind = btind.KAMA
 */

#include "test_common_simple.h"

#include "indicators/kama.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> KAMA_EXPECTED_VALUES = {
    {"4054.187922", "3648.549000", "3592.979190"}
};

const int KAMA_MIN_PERIOD = 31;

} // anonymous namespace

// 使用默认参数的KAMA测试
DEFINE_INDICATOR_TEST(KAMA_Default, KAMA, KAMA_EXPECTED_VALUES, KAMA_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, KAMA_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建KAMA指标（默认参数：period=30, fast=2, slow=30）
    auto kama = std::make_shared<KAMA>(close_line, 30, 2, 30);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        kama->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
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
    
    std::vector<std::string> expected = {"4054.187922", "3648.549000", "3592.979190"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = kama->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "KAMA value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(kama->getMinPeriod(), 31) << "KAMA minimum period should be 31";
}

// 参数化测试 - 测试不同参数的KAMA
class KAMAParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int, int>> {
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

TEST_P(KAMAParameterizedTest, DifferentParameters) {
    auto [period, fast, slow] = GetParam();
    auto kama = std::make_shared<KAMA>(close_line_, period, fast, slow);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        kama->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    int expected_min_period = period + 1;
    EXPECT_EQ(kama->getMinPeriod(), expected_min_period) 
        << "KAMA minimum period should be period + 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_value = kama->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last KAMA value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last KAMA value should be finite";
    }
}

// 测试不同的KAMA参数
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    KAMAParameterizedTest,
    ::testing::Values(
        std::make_tuple(10, 2, 30),   // 短周期
        std::make_tuple(30, 2, 30),   // 标准参数
        std::make_tuple(50, 2, 30),   // 长周期
        std::make_tuple(20, 1, 15)    // 不同的快慢参数
    )
);

// KAMA计算逻辑验证测试
TEST(OriginalTests, KAMA_CalculationLogic) {
    // 使用简单的测试数据验证KAMA计算
    std::vector<double> prices = {100.0, 102.0, 101.0, 103.0, 105.0, 104.0, 106.0, 108.0, 107.0, 109.0};
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "kama_calc");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto kama = std::make_shared<KAMA>(close_line, 5, 2, 10);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        kama->calculate();
        
        double kama_val = kama->get(0);
        
        // KAMA应该产生有限值
        if (!std::isnan(kama_val)) {
            EXPECT_TRUE(std::isfinite(kama_val)) 
                << "KAMA should be finite at step " << i;
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// 趋势适应性测试
TEST(OriginalTests, KAMA_TrendAdaptivity) {
    // 创建强势趋势数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 100; ++i) {
        trend_prices.push_back(100.0 + i * 2.0);  // 强势上升趋势
    }
    
    auto trend_line = std::make_shared<LineRoot>(trend_prices.size(), "trend");
    for (double price : trend_prices) {
        trend_line->forward(price);
    }
    
    auto trend_kama = std::make_shared<KAMA>(trend_line, 20, 2, 30);
    
    double prev_kama = 0.0;
    bool has_prev = false;
    int increasing_count = 0;
    
    for (size_t i = 0; i < trend_prices.size(); ++i) {
        trend_kama->calculate();
        
        double current_kama = trend_kama->get(0);
        
        if (!std::isnan(current_kama)) {
            if (has_prev && current_kama > prev_kama) {
                increasing_count++;
            }
            prev_kama = current_kama;
            has_prev = true;
        }
        
        if (i < trend_prices.size() - 1) {
            trend_line->forward();
        }
    }
    
    // 在强势趋势中，KAMA应该主要呈上升趋势
    int total_valid_points = static_cast<int>(trend_prices.size()) - 21;  // 减去最小周期
    if (total_valid_points > 0) {
        double increasing_ratio = static_cast<double>(increasing_count) / total_valid_points;
        EXPECT_GT(increasing_ratio, 0.7) 
            << "KAMA should increase most of the time in strong uptrend";
        
        std::cout << "Strong trend - KAMA increasing ratio: " << increasing_ratio << std::endl;
    }
}

// 震荡市场适应性测试
TEST(OriginalTests, KAMA_ChoppyMarket) {
    // 创建震荡市场数据
    std::vector<double> choppy_prices;
    for (int i = 0; i < 100; ++i) {
        double base = 100.0;
        double noise = 5.0 * std::sin(i * 0.5) + 2.0 * std::cos(i * 0.3);
        choppy_prices.push_back(base + noise);
    }
    
    auto choppy_line = std::make_shared<LineRoot>(choppy_prices.size(), "choppy");
    for (double price : choppy_prices) {
        choppy_line->forward(price);
    }
    
    auto choppy_kama = std::make_shared<KAMA>(choppy_line, 20, 2, 30);
    
    std::vector<double> kama_changes;
    double prev_kama = 0.0;
    bool has_prev = false;
    
    for (size_t i = 0; i < choppy_prices.size(); ++i) {
        choppy_kama->calculate();
        
        double current_kama = choppy_kama->get(0);
        
        if (!std::isnan(current_kama)) {
            if (has_prev) {
                kama_changes.push_back(std::abs(current_kama - prev_kama));
            }
            prev_kama = current_kama;
            has_prev = true;
        }
        
        if (i < choppy_prices.size() - 1) {
            choppy_line->forward();
        }
    }
    
    // 在震荡市场中，KAMA的变化应该相对较小
    if (!kama_changes.empty()) {
        double avg_change = std::accumulate(kama_changes.begin(), kama_changes.end(), 0.0) / kama_changes.size();
        std::cout << "Choppy market - Average KAMA change: " << avg_change << std::endl;
        
        EXPECT_LT(avg_change, 2.0) 
            << "KAMA should have small changes in choppy market";
    }
}

// 效率比测试
TEST(OriginalTests, KAMA_EfficiencyRatio) {
    // 测试不同市场条件下的效率比特性
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto kama = std::make_shared<KAMA>(close_line, 20, 2, 30);
    
    std::vector<double> kama_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        kama->calculate();
        
        double kama_val = kama->get(0);
        if (!std::isnan(kama_val)) {
            kama_values.push_back(kama_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证KAMA值的合理性
    if (!kama_values.empty()) {
        double min_kama = *std::min_element(kama_values.begin(), kama_values.end());
        double max_kama = *std::max_element(kama_values.begin(), kama_values.end());
        
        std::cout << "KAMA range: [" << min_kama << ", " << max_kama << "]" << std::endl;
        
        // KAMA应该在价格范围内或接近
        EXPECT_GT(max_kama, min_kama) << "KAMA should have some variation";
    }
}

// 快慢移动平均对比测试
TEST(OriginalTests, KAMA_FastSlowComparison) {
    // 比较快速和慢速KAMA的行为
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto fast_kama = std::make_shared<KAMA>(close_line, 10, 2, 30);
    auto slow_kama = std::make_shared<KAMA>(close_line, 30, 2, 30);
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        fast_kama->calculate();
        slow_kama->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证两个KAMA的有效性
    double fast_val = fast_kama->get(0);
    double slow_val = slow_kama->get(0);
    
    if (!std::isnan(fast_val) && !std::isnan(slow_val)) {
        EXPECT_TRUE(std::isfinite(fast_val)) << "Fast KAMA should be finite";
        EXPECT_TRUE(std::isfinite(slow_val)) << "Slow KAMA should be finite";
        
        std::cout << "Fast KAMA: " << fast_val << ", Slow KAMA: " << slow_val << std::endl;
    }
}

// 价格跟踪测试
TEST(OriginalTests, KAMA_PriceTracking) {
    // 测试KAMA对价格变化的跟踪能力
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
    
    auto step_kama = std::make_shared<KAMA>(step_line, 20, 2, 30);
    
    std::vector<double> pre_jump_kama;
    std::vector<double> post_jump_kama;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        step_kama->calculate();
        
        double kama_val = step_kama->get(0);
        
        if (!std::isnan(kama_val)) {
            if (i < 50) {
                pre_jump_kama.push_back(kama_val);
            } else {
                post_jump_kama.push_back(kama_val);
            }
        }
        
        if (i < step_prices.size() - 1) {
            step_line->forward();
        }
    }
    
    // 分析价格跳跃前后的KAMA行为
    if (!pre_jump_kama.empty() && !post_jump_kama.empty()) {
        double avg_pre = std::accumulate(pre_jump_kama.end() - 10, pre_jump_kama.end(), 0.0) / 10.0;
        double avg_post = std::accumulate(post_jump_kama.end() - 10, post_jump_kama.end(), 0.0) / 10.0;
        
        std::cout << "Pre-jump KAMA: " << avg_pre << ", Post-jump KAMA: " << avg_post << std::endl;
        
        // KAMA应该能够适应价格变化
        EXPECT_GT(avg_post, avg_pre) << "KAMA should adapt to price increase";
        EXPECT_LT(std::abs(avg_post - 120.0), 10.0) << "KAMA should track new price level";
    }
}

// 边界条件测试
TEST(OriginalTests, KAMA_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_kama = std::make_shared<KAMA>(flat_line, 20, 2, 30);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_kama->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，KAMA应该等于该价格
    double final_kama = flat_kama->get(0);
    if (!std::isnan(final_kama)) {
        EXPECT_NEAR(final_kama, 100.0, 1e-6) 
            << "KAMA should equal constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 20; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_kama = std::make_shared<KAMA>(insufficient_line, 30, 2, 30);
    
    for (int i = 0; i < 20; ++i) {
        insufficient_kama->calculate();
        if (i < 19) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_kama->get(0);
    EXPECT_TRUE(std::isnan(result)) << "KAMA should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, KAMA_Performance) {
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
    
    auto large_kama = std::make_shared<KAMA>(large_line, 30, 2, 30);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_kama->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "KAMA calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_kama->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}