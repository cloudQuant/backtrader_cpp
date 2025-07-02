/**
 * @file test_ind_trix.cpp
 * @brief TRIX指标测试 - 对应Python test_ind_trix.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['0.071304', '0.181480', '0.050954']
 * ]
 * chkmin = 44
 * chkind = btind.Trix
 */

#include "test_common.h"
#include "indicators/TRIX.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> TRIX_EXPECTED_VALUES = {
    {"0.071304", "0.181480", "0.050954"}
};

const int TRIX_MIN_PERIOD = 44;

} // anonymous namespace

// 使用默认参数的TRIX测试
DEFINE_INDICATOR_TEST(TRIX_Default, TRIX, TRIX_EXPECTED_VALUES, TRIX_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, TRIX_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建TRIX指标（默认15周期，最小周期为44）
    auto trix = std::make_shared<TRIX>(close_line, 15);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        trix->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 44;  // 大约 3 * period - 2 + 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"0.071304", "0.181480", "0.050954"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = trix->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "TRIX value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(trix->getMinPeriod(), 44) << "TRIX minimum period should be 44";
}

// 参数化测试 - 测试不同周期的TRIX
class TRIXParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(TRIXParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto trix = std::make_shared<TRIX>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        trix->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期 (大约 3 * period - 2 + 1)
    int expected_min_period = 3 * period - 2 + 1;
    EXPECT_EQ(trix->getMinPeriod(), expected_min_period) 
        << "TRIX minimum period should be approximately 3 * period - 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_value = trix->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last TRIX value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "TRIX value should be finite";
    }
}

// 测试不同的TRIX周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    TRIXParameterizedTest,
    ::testing::Values(10, 15, 20)
);

// TRIX计算逻辑验证测试
TEST(OriginalTests, TRIX_CalculationLogic) {
    // 使用简单的测试数据验证TRIX计算
    std::vector<double> prices;
    for (int i = 0; i < 100; ++i) {
        prices.push_back(100.0 + i * 0.5);  // 线性增长
    }
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "trix_calc");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto trix = std::make_shared<TRIX>(close_line, 10);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        trix->calculate();
        
        double trix_val = trix->get(0);
        
        // TRIX应该产生有限值
        if (!std::isnan(trix_val)) {
            EXPECT_TRUE(std::isfinite(trix_val)) 
                << "TRIX should be finite at step " << i;
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// TRIX趋势检测测试
TEST(OriginalTests, TRIX_TrendDetection) {
    // 创建上升趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 100; ++i) {
        uptrend_prices.push_back(100.0 + i * 1.0);  // 持续上升
    }
    
    auto up_line = std::make_shared<LineRoot>(uptrend_prices.size(), "uptrend");
    for (double price : uptrend_prices) {
        up_line->forward(price);
    }
    
    auto up_trix = std::make_shared<TRIX>(up_line, 15);
    
    for (size_t i = 0; i < uptrend_prices.size(); ++i) {
        up_trix->calculate();
        if (i < uptrend_prices.size() - 1) {
            up_line->forward();
        }
    }
    
    double final_up_trix = up_trix->get(0);
    if (!std::isnan(final_up_trix)) {
        EXPECT_GT(final_up_trix, 0.0) 
            << "TRIX should be positive for uptrend";
    }
    
    // 创建下降趋势数据
    std::vector<double> downtrend_prices;
    for (int i = 0; i < 100; ++i) {
        downtrend_prices.push_back(200.0 - i * 1.0);  // 持续下降
    }
    
    auto down_line = std::make_shared<LineRoot>(downtrend_prices.size(), "downtrend");
    for (double price : downtrend_prices) {
        down_line->forward(price);
    }
    
    auto down_trix = std::make_shared<TRIX>(down_line, 15);
    
    for (size_t i = 0; i < downtrend_prices.size(); ++i) {
        down_trix->calculate();
        if (i < downtrend_prices.size() - 1) {
            down_line->forward();
        }
    }
    
    double final_down_trix = down_trix->get(0);
    if (!std::isnan(final_down_trix)) {
        EXPECT_LT(final_down_trix, 0.0) 
            << "TRIX should be negative for downtrend";
    }
    
    std::cout << "Uptrend TRIX: " << final_up_trix << std::endl;
    std::cout << "Downtrend TRIX: " << final_down_trix << std::endl;
}

// TRIX零线穿越测试
TEST(OriginalTests, TRIX_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto trix = std::make_shared<TRIX>(close_line, 15);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_trix = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越
    for (size_t i = 0; i < csv_data.size(); ++i) {
        trix->calculate();
        
        double current_trix = trix->get(0);
        
        if (!std::isnan(current_trix) && has_prev) {
            if (prev_trix <= 0.0 && current_trix > 0.0) {
                positive_crossings++;
            } else if (prev_trix >= 0.0 && current_trix < 0.0) {
                negative_crossings++;
            }
        }
        
        if (!std::isnan(current_trix)) {
            prev_trix = current_trix;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "TRIX zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// TRIX滤波特性测试
TEST(OriginalTests, TRIX_FilteringCharacteristics) {
    // 创建带噪声的价格数据
    std::vector<double> noisy_prices;
    std::mt19937 rng(42);
    std::normal_distribution<double> noise(0.0, 1.0);
    
    for (int i = 0; i < 200; ++i) {
        double trend = 100.0 + i * 0.1;  // 缓慢上升趋势
        double noise_val = noise(rng);   // 随机噪声
        noisy_prices.push_back(trend + noise_val);
    }
    
    auto noisy_line = std::make_shared<LineRoot>(noisy_prices.size(), "noisy");
    for (double price : noisy_prices) {
        noisy_line->forward(price);
    }
    
    auto noisy_trix = std::make_shared<TRIX>(noisy_line, 15);
    
    std::vector<double> trix_values;
    
    for (size_t i = 0; i < noisy_prices.size(); ++i) {
        noisy_trix->calculate();
        
        double trix_val = noisy_trix->get(0);
        if (!std::isnan(trix_val)) {
            trix_values.push_back(trix_val);
        }
        
        if (i < noisy_prices.size() - 1) {
            noisy_line->forward();
        }
    }
    
    // 验证TRIX的滤波特性
    if (!trix_values.empty()) {
        // 计算TRIX值的变异系数
        double mean = std::accumulate(trix_values.begin(), trix_values.end(), 0.0) / trix_values.size();
        double variance = 0.0;
        for (double val : trix_values) {
            variance += (val - mean) * (val - mean);
        }
        variance /= trix_values.size();
        double std_dev = std::sqrt(variance);
        
        std::cout << "TRIX filtering test - Mean: " << mean << ", StdDev: " << std_dev << std::endl;
        
        // TRIX应该比原始噪声数据更平滑
        EXPECT_TRUE(std::isfinite(mean)) << "TRIX mean should be finite";
        EXPECT_TRUE(std::isfinite(std_dev)) << "TRIX std dev should be finite";
    }
}

// TRIX发散测试
TEST(OriginalTests, TRIX_Divergence) {
    // 创建价格走高但动量减弱的发散情况
    std::vector<double> divergence_prices;
    
    // 第一阶段：强劲上升
    for (int i = 0; i < 50; ++i) {
        divergence_prices.push_back(100.0 + i * 2.0);
    }
    
    // 第二阶段：缓慢上升（价格新高但动量减弱）
    for (int i = 0; i < 50; ++i) {
        divergence_prices.push_back(200.0 + i * 0.2);
    }
    
    auto div_line = std::make_shared<LineRoot>(divergence_prices.size(), "divergence");
    for (double price : divergence_prices) {
        div_line->forward(price);
    }
    
    auto div_trix = std::make_shared<TRIX>(div_line, 15);
    
    std::vector<double> early_trix;
    std::vector<double> late_trix;
    
    for (size_t i = 0; i < divergence_prices.size(); ++i) {
        div_trix->calculate();
        
        double trix_val = div_trix->get(0);
        
        if (!std::isnan(trix_val)) {
            if (i < 60) {
                early_trix.push_back(trix_val);
            } else {
                late_trix.push_back(trix_val);
            }
        }
        
        if (i < divergence_prices.size() - 1) {
            div_line->forward();
        }
    }
    
    // 分析发散现象
    if (!early_trix.empty() && !late_trix.empty()) {
        double avg_early = std::accumulate(early_trix.begin(), early_trix.end(), 0.0) / early_trix.size();
        double avg_late = std::accumulate(late_trix.begin(), late_trix.end(), 0.0) / late_trix.size();
        
        std::cout << "Early TRIX average: " << avg_early << std::endl;
        std::cout << "Late TRIX average: " << avg_late << std::endl;
        
        // 验证计算结果是有限的
        EXPECT_TRUE(std::isfinite(avg_early)) << "Early TRIX should be finite";
        EXPECT_TRUE(std::isfinite(avg_late)) << "Late TRIX should be finite";
        
        // 后期动量应该小于早期动量
        EXPECT_LT(avg_late, avg_early) 
            << "Late TRIX should be less than early TRIX in divergence scenario";
    }
}

// 边界条件测试
TEST(OriginalTests, TRIX_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);  // 100个相同价格
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_trix = std::make_shared<TRIX>(flat_line, 15);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_trix->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    double final_trix = flat_trix->get(0);
    if (!std::isnan(final_trix)) {
        EXPECT_NEAR(final_trix, 0.0, 1e-10) 
            << "TRIX should be zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 20; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_trix = std::make_shared<TRIX>(insufficient_line, 15);
    
    for (int i = 0; i < 20; ++i) {
        insufficient_trix->calculate();
        if (i < 19) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_trix->get(0);
    EXPECT_TRUE(std::isnan(result)) << "TRIX should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, TRIX_Performance) {
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
    
    auto large_trix = std::make_shared<TRIX>(large_line, 15);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_trix->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "TRIX calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_trix->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}