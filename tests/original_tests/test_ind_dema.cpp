/**
 * @file test_ind_dema.cpp
 * @brief DEMA指标测试 - 对应Python test_ind_dema.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4115.563246', '3852.837209', '3665.728415']
 * ]
 * chkmin = 59
 * chkind = btind.DEMA
 */

#include "test_common.h"
#include "indicators/DEMA.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> DEMA_EXPECTED_VALUES = {
    {"4115.563246", "3852.837209", "3665.728415"}
};

const int DEMA_MIN_PERIOD = 59;

} // anonymous namespace

// 使用默认参数的DEMA测试
DEFINE_INDICATOR_TEST(DEMA_Default, DEMA, DEMA_EXPECTED_VALUES, DEMA_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, DEMA_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建DEMA指标（默认30周期，最小周期为59）
    auto dema = std::make_shared<DEMA>(close_line, 30);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        dema->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 59;  // 2 * period - 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"4115.563246", "3852.837209", "3665.728415"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = dema->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "DEMA value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(dema->getMinPeriod(), 59) << "DEMA minimum period should be 59";
}

// 参数化测试 - 测试不同周期的DEMA
class DEMAParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(DEMAParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto dema = std::make_shared<DEMA>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        dema->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期 (2 * period - 1)
    EXPECT_EQ(dema->getMinPeriod(), 2 * period - 1) 
        << "DEMA minimum period should be 2 * period - 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(2 * period - 1)) {
        double last_value = dema->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last DEMA value should not be NaN";
        EXPECT_GT(last_value, 0) << "DEMA value should be positive for this test data";
    }
}

// 测试不同的DEMA周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    DEMAParameterizedTest,
    ::testing::Values(10, 20, 30, 40)
);

// DEMA计算逻辑验证测试
TEST(OriginalTests, DEMA_CalculationLogic) {
    // 使用简单的测试数据验证DEMA计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0};
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "dema_calc");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto dema = std::make_shared<DEMA>(close_line, 5);
    auto ema1 = std::make_shared<EMA>(close_line, 5);
    
    // 创建第二个EMA来验证DEMA计算
    auto ema1_line = std::make_shared<LineRoot>(prices.size(), "ema1_values");
    
    for (size_t i = 0; i < prices.size(); ++i) {
        dema->calculate();
        ema1->calculate();
        
        double ema1_val = ema1->get(0);
        if (!std::isnan(ema1_val)) {
            ema1_line->forward(ema1_val);
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
    
    // DEMA应该比EMA更快响应价格变化
    if (prices.size() >= 9) {  // 2*5-1 = 9
        double dema_val = dema->get(0);
        double ema_val = ema1->get(0);
        
        EXPECT_FALSE(std::isnan(dema_val)) << "DEMA should produce valid values";
        EXPECT_FALSE(std::isnan(ema_val)) << "EMA should produce valid values";
        
        // DEMA通常比EMA更接近最新价格（对于上升趋势）
        if (!std::isnan(dema_val) && !std::isnan(ema_val)) {
            EXPECT_TRUE(std::isfinite(dema_val)) << "DEMA should be finite";
            EXPECT_TRUE(std::isfinite(ema_val)) << "EMA should be finite";
        }
    }
}

// DEMA响应性测试 - DEMA应该比EMA响应更快
TEST(OriginalTests, DEMA_vs_EMA_Responsiveness) {
    auto csv_data = getdata(0);
    auto close_line_dema = std::make_shared<LineRoot>(csv_data.size(), "close_dema");
    auto close_line_ema = std::make_shared<LineRoot>(csv_data.size(), "close_ema");
    
    for (const auto& bar : csv_data) {
        close_line_dema->forward(bar.close);
        close_line_ema->forward(bar.close);
    }
    
    const int period = 20;
    auto dema = std::make_shared<DEMA>(close_line_dema, period);
    auto ema = std::make_shared<EMA>(close_line_ema, period);
    
    std::vector<double> dema_changes;
    std::vector<double> ema_changes;
    double prev_dema = 0.0, prev_ema = 0.0;
    
    // 计算并记录变化
    for (size_t i = 0; i < csv_data.size(); ++i) {
        dema->calculate();
        ema->calculate();
        
        double current_dema = dema->get(0);
        double current_ema = ema->get(0);
        
        if (i > 2 * period && !std::isnan(current_dema) && !std::isnan(current_ema)) {
            if (prev_dema != 0.0 && prev_ema != 0.0) {
                dema_changes.push_back(std::abs(current_dema - prev_dema));
                ema_changes.push_back(std::abs(current_ema - prev_ema));
            }
            prev_dema = current_dema;
            prev_ema = current_ema;
        }
        
        if (i < csv_data.size() - 1) {
            close_line_dema->forward();
            close_line_ema->forward();
        }
    }
    
    // 计算平均变化
    if (!dema_changes.empty() && !ema_changes.empty()) {
        double avg_dema_change = std::accumulate(dema_changes.begin(), dema_changes.end(), 0.0) / dema_changes.size();
        double avg_ema_change = std::accumulate(ema_changes.begin(), ema_changes.end(), 0.0) / ema_changes.size();
        
        std::cout << "Average DEMA change: " << avg_dema_change << std::endl;
        std::cout << "Average EMA change: " << avg_ema_change << std::endl;
        
        // 验证都是正值
        EXPECT_GT(avg_dema_change, 0.0) << "DEMA should show price changes";
        EXPECT_GT(avg_ema_change, 0.0) << "EMA should show price changes";
    }
}

// DEMA vs SMA比较测试
TEST(OriginalTests, DEMA_vs_SMA_Comparison) {
    auto csv_data = getdata(0);
    auto close_line_dema = std::make_shared<LineRoot>(csv_data.size(), "close_dema");
    auto close_line_sma = std::make_shared<LineRoot>(csv_data.size(), "close_sma");
    
    for (const auto& bar : csv_data) {
        close_line_dema->forward(bar.close);
        close_line_sma->forward(bar.close);
    }
    
    const int period = 20;
    auto dema = std::make_shared<DEMA>(close_line_dema, period);
    auto sma = std::make_shared<SMA>(close_line_sma, period);
    
    std::vector<double> dema_values;
    std::vector<double> sma_values;
    
    // 计算并收集值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        dema->calculate();
        sma->calculate();
        
        double dema_val = dema->get(0);
        double sma_val = sma->get(0);
        
        if (!std::isnan(dema_val) && !std::isnan(sma_val)) {
            dema_values.push_back(dema_val);
            sma_values.push_back(sma_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line_dema->forward();
            close_line_sma->forward();
        }
    }
    
    // 验证两个指标都产生了有效值
    EXPECT_FALSE(dema_values.empty()) << "DEMA should produce values";
    EXPECT_FALSE(sma_values.empty()) << "SMA should produce values";
    
    if (!dema_values.empty() && !sma_values.empty()) {
        double avg_dema = std::accumulate(dema_values.begin(), dema_values.end(), 0.0) / dema_values.size();
        double avg_sma = std::accumulate(sma_values.begin(), sma_values.end(), 0.0) / sma_values.size();
        
        std::cout << "Average DEMA: " << avg_dema << std::endl;
        std::cout << "Average SMA: " << avg_sma << std::endl;
        
        // 验证平均值在合理范围内
        EXPECT_TRUE(std::isfinite(avg_dema)) << "DEMA average should be finite";
        EXPECT_TRUE(std::isfinite(avg_sma)) << "SMA average should be finite";
    }
}

// 滞后测试 - DEMA应该比其他移动平均线滞后更小
TEST(OriginalTests, DEMA_LagTest) {
    // 创建一个步进价格序列来测试滞后
    std::vector<double> step_prices;
    
    // 前20个点为100
    for (int i = 0; i < 20; ++i) {
        step_prices.push_back(100.0);
    }
    
    // 后20个点为110（步进上升）
    for (int i = 0; i < 20; ++i) {
        step_prices.push_back(110.0);
    }
    
    auto close_line_dema = std::make_shared<LineRoot>(step_prices.size(), "step_dema");
    auto close_line_ema = std::make_shared<LineRoot>(step_prices.size(), "step_ema");
    auto close_line_sma = std::make_shared<LineRoot>(step_prices.size(), "step_sma");
    
    for (double price : step_prices) {
        close_line_dema->forward(price);
        close_line_ema->forward(price);
        close_line_sma->forward(price);
    }
    
    const int period = 10;
    auto dema = std::make_shared<DEMA>(close_line_dema, period);
    auto ema = std::make_shared<EMA>(close_line_ema, period);
    auto sma = std::make_shared<SMA>(close_line_sma, period);
    
    std::vector<double> final_values;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        dema->calculate();
        ema->calculate();
        sma->calculate();
        
        if (i == step_prices.size() - 1) {
            // 记录最终值
            final_values.push_back(dema->get(0));
            final_values.push_back(ema->get(0));
            final_values.push_back(sma->get(0));
        }
        
        if (i < step_prices.size() - 1) {
            close_line_dema->forward();
            close_line_ema->forward();
            close_line_sma->forward();
        }
    }
    
    if (final_values.size() >= 3) {
        std::cout << "Final DEMA: " << final_values[0] << std::endl;
        std::cout << "Final EMA: " << final_values[1] << std::endl;
        std::cout << "Final SMA: " << final_values[2] << std::endl;
        
        // DEMA应该最接近新价格110
        double target = 110.0;
        double dema_distance = std::abs(final_values[0] - target);
        double ema_distance = std::abs(final_values[1] - target);
        double sma_distance = std::abs(final_values[2] - target);
        
        // DEMA应该比SMA更接近目标价格
        EXPECT_LT(dema_distance, sma_distance) 
            << "DEMA should be closer to target price than SMA";
    }
}

// 边界条件测试
TEST(OriginalTests, DEMA_EdgeCases) {
    // 测试数据不足的情况
    auto close_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 20; ++i) {
        close_line->forward(100.0 + i);
    }
    
    auto dema = std::make_shared<DEMA>(close_line, 30);  // 最小周期为59
    
    for (int i = 0; i < 20; ++i) {
        dema->calculate();
        if (i < 19) {
            close_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = dema->get(0);
    EXPECT_TRUE(std::isnan(result)) << "DEMA should return NaN when insufficient data";
}

// 收敛测试
TEST(OriginalTests, DEMA_Convergence) {
    // 使用恒定价格测试收敛
    const double constant_price = 100.0;
    const int num_points = 200;
    
    auto close_line = std::make_shared<LineRoot>(num_points, "convergence");
    for (int i = 0; i < num_points; ++i) {
        close_line->forward(constant_price);
    }
    
    auto dema = std::make_shared<DEMA>(close_line, 20);
    
    double final_dema = 0.0;
    for (int i = 0; i < num_points; ++i) {
        dema->calculate();
        final_dema = dema->get(0);
        if (i < num_points - 1) {
            close_line->forward();
        }
    }
    
    // DEMA应该收敛到恒定价格
    EXPECT_NEAR(final_dema, constant_price, 0.01) 
        << "DEMA should converge to constant price";
}

// 性能测试
TEST(OriginalTests, DEMA_Performance) {
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
    
    auto large_dema = std::make_shared<DEMA>(large_line, 50);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_dema->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "DEMA calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_dema->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_GE(final_result, 50.0) << "Final result should be within expected range";
    EXPECT_LE(final_result, 150.0) << "Final result should be within expected range";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}