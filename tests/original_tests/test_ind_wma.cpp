/**
 * @file test_ind_wma.cpp
 * @brief WMA指标测试 - 对应Python test_ind_wma.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4076.212366', '3655.193634', '3576.228000'],
 * ]
 * chkmin = 30
 * chkind = btind.WMA
 */

#include "test_common.h"
#include "indicators/WMA.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> WMA_EXPECTED_VALUES = {
    {"4076.212366", "3655.193634", "3576.228000"}
};

const int WMA_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的WMA测试
DEFINE_INDICATOR_TEST(WMA_Default, WMA, WMA_EXPECTED_VALUES, WMA_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, WMA_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建WMA指标（默认30周期）
    auto wma = std::make_shared<WMA>(close_line, 30);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        wma->calculate();
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
    
    std::vector<std::string> expected = {"4076.212366", "3655.193634", "3576.228000"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = wma->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "WMA value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(wma->getMinPeriod(), 30) << "WMA minimum period should be 30";
}

// 参数化测试 - 测试不同周期的WMA
class WMAParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(WMAParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto wma = std::make_shared<WMA>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        wma->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(wma->getMinPeriod(), period) 
        << "WMA minimum period should match parameter";
    
    // 验证最后的值不是NaN
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_value = wma->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last WMA value should not be NaN";
        EXPECT_GT(last_value, 0) << "WMA value should be positive for this test data";
    }
}

// 测试不同的WMA周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    WMAParameterizedTest,
    ::testing::Values(5, 10, 20, 30, 50)
);

// WMA计算逻辑验证测试
TEST(OriginalTests, WMA_CalculationLogic) {
    // 使用简单的测试数据验证WMA计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0};
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "wma_calc");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto wma = std::make_shared<WMA>(close_line, 5);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        wma->calculate();
        
        // 手动计算WMA进行验证
        if (i >= 4) {  // 需要5个数据点
            double numerator = 0.0;
            double denominator = 0.0;
            
            for (int j = 0; j < 5; ++j) {
                double weight = 5 - j;  // 权重从5到1
                numerator += prices[i - j] * weight;
                denominator += weight;
            }
            
            double expected_wma = numerator / denominator;
            double actual_wma = wma->get(0);
            
            EXPECT_NEAR(actual_wma, expected_wma, 1e-10) 
                << "WMA calculation mismatch at step " << i;
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// WMA权重验证测试
TEST(OriginalTests, WMA_WeightValidation) {
    // 创建一个简单的价格序列来验证权重
    std::vector<double> prices = {10.0, 20.0, 30.0};
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "weight_test");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto wma = std::make_shared<WMA>(close_line, 3);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        wma->calculate();
        
        if (i >= 2) {  // 需要3个数据点
            // 手动计算：最新价格权重最高
            // WMA = (30*3 + 20*2 + 10*1) / (3+2+1) = (90+40+10) / 6 = 140/6 = 23.333333
            double expected = (30.0 * 3 + 20.0 * 2 + 10.0 * 1) / (3 + 2 + 1);
            double actual = wma->get(0);
            
            EXPECT_NEAR(actual, expected, 1e-10) 
                << "WMA weight calculation should be correct";
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// WMA vs SMA响应性测试
TEST(OriginalTests, WMA_vs_SMA_Responsiveness) {
    auto csv_data = getdata(0);
    auto close_line_wma = std::make_shared<LineRoot>(csv_data.size(), "close_wma");
    auto close_line_sma = std::make_shared<LineRoot>(csv_data.size(), "close_sma");
    
    for (const auto& bar : csv_data) {
        close_line_wma->forward(bar.close);
        close_line_sma->forward(bar.close);
    }
    
    const int period = 20;
    auto wma = std::make_shared<WMA>(close_line_wma, period);
    auto sma = std::make_shared<SMA>(close_line_sma, period);
    
    std::vector<double> wma_changes;
    std::vector<double> sma_changes;
    double prev_wma = 0.0, prev_sma = 0.0;
    
    // 计算并记录变化
    for (size_t i = 0; i < csv_data.size(); ++i) {
        wma->calculate();
        sma->calculate();
        
        double current_wma = wma->get(0);
        double current_sma = sma->get(0);
        
        if (i > period && !std::isnan(current_wma) && !std::isnan(current_sma)) {
            if (prev_wma != 0.0 && prev_sma != 0.0) {
                wma_changes.push_back(std::abs(current_wma - prev_wma));
                sma_changes.push_back(std::abs(current_sma - prev_sma));
            }
            prev_wma = current_wma;
            prev_sma = current_sma;
        }
        
        if (i < csv_data.size() - 1) {
            close_line_wma->forward();
            close_line_sma->forward();
        }
    }
    
    // 计算平均变化
    if (!wma_changes.empty() && !sma_changes.empty()) {
        double avg_wma_change = std::accumulate(wma_changes.begin(), wma_changes.end(), 0.0) / wma_changes.size();
        double avg_sma_change = std::accumulate(sma_changes.begin(), sma_changes.end(), 0.0) / sma_changes.size();
        
        std::cout << "Average WMA change: " << avg_wma_change << std::endl;
        std::cout << "Average SMA change: " << avg_sma_change << std::endl;
        
        // 验证都是正值
        EXPECT_GT(avg_wma_change, 0.0) << "WMA should show price changes";
        EXPECT_GT(avg_sma_change, 0.0) << "SMA should show price changes";
    }
}

// WMA vs EMA比较测试
TEST(OriginalTests, WMA_vs_EMA_Comparison) {
    auto csv_data = getdata(0);
    auto close_line_wma = std::make_shared<LineRoot>(csv_data.size(), "close_wma");
    auto close_line_ema = std::make_shared<LineRoot>(csv_data.size(), "close_ema");
    
    for (const auto& bar : csv_data) {
        close_line_wma->forward(bar.close);
        close_line_ema->forward(bar.close);
    }
    
    const int period = 20;
    auto wma = std::make_shared<WMA>(close_line_wma, period);
    auto ema = std::make_shared<EMA>(close_line_ema, period);
    
    std::vector<double> wma_values;
    std::vector<double> ema_values;
    
    // 计算并收集值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        wma->calculate();
        ema->calculate();
        
        double wma_val = wma->get(0);
        double ema_val = ema->get(0);
        
        if (!std::isnan(wma_val) && !std::isnan(ema_val)) {
            wma_values.push_back(wma_val);
            ema_values.push_back(ema_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line_wma->forward();
            close_line_ema->forward();
        }
    }
    
    // 验证两个指标都产生了有效值
    EXPECT_FALSE(wma_values.empty()) << "WMA should produce values";
    EXPECT_FALSE(ema_values.empty()) << "EMA should produce values";
    
    if (!wma_values.empty() && !ema_values.empty()) {
        double avg_wma = std::accumulate(wma_values.begin(), wma_values.end(), 0.0) / wma_values.size();
        double avg_ema = std::accumulate(ema_values.begin(), ema_values.end(), 0.0) / ema_values.size();
        
        std::cout << "Average WMA: " << avg_wma << std::endl;
        std::cout << "Average EMA: " << avg_ema << std::endl;
        
        // 验证平均值在合理范围内
        EXPECT_TRUE(std::isfinite(avg_wma)) << "WMA average should be finite";
        EXPECT_TRUE(std::isfinite(avg_ema)) << "EMA average should be finite";
    }
}

// 线性权重测试
TEST(OriginalTests, WMA_LinearWeights) {
    // 验证WMA使用线性递减权重
    std::vector<double> prices = {1.0, 2.0, 3.0, 4.0, 5.0};
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "linear_weights");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto wma = std::make_shared<WMA>(close_line, 5);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        wma->calculate();
        
        if (i >= 4) {  // 有足够数据点
            // 手动计算：权重为 5, 4, 3, 2, 1
            // WMA = (5*5 + 4*4 + 3*3 + 2*2 + 1*1) / (5+4+3+2+1)
            //     = (25 + 16 + 9 + 4 + 1) / 15 = 55/15 = 3.666667
            double expected = (5.0*5 + 4.0*4 + 3.0*3 + 2.0*2 + 1.0*1) / 15.0;
            double actual = wma->get(0);
            
            EXPECT_NEAR(actual, expected, 1e-10) 
                << "WMA with linear weights calculation";
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// 边界条件测试
TEST(OriginalTests, WMA_EdgeCases) {
    // 测试数据不足的情况
    auto close_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 5; ++i) {
        close_line->forward(100.0 + i);
    }
    
    auto wma = std::make_shared<WMA>(close_line, 10);  // 周期大于数据量
    
    for (int i = 0; i < 5; ++i) {
        wma->calculate();
        if (i < 4) {
            close_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = wma->get(0);
    EXPECT_TRUE(std::isnan(result)) << "WMA should return NaN when insufficient data";
    
    // 测试单个数据点的情况
    auto single_line = std::make_shared<LineRoot>(1, "single");
    single_line->forward(123.45);
    
    auto single_wma = std::make_shared<WMA>(single_line, 1);
    single_wma->calculate();
    
    double single_result = single_wma->get(0);
    EXPECT_NEAR(single_result, 123.45, 1e-10) 
        << "WMA of single value should equal that value";
}

// 收敛测试
TEST(OriginalTests, WMA_Convergence) {
    // 使用恒定价格测试收敛
    const double constant_price = 100.0;
    const int num_points = 50;
    
    auto close_line = std::make_shared<LineRoot>(num_points, "convergence");
    for (int i = 0; i < num_points; ++i) {
        close_line->forward(constant_price);
    }
    
    auto wma = std::make_shared<WMA>(close_line, 10);
    
    double final_wma = 0.0;
    for (int i = 0; i < num_points; ++i) {
        wma->calculate();
        final_wma = wma->get(0);
        if (i < num_points - 1) {
            close_line->forward();
        }
    }
    
    // WMA应该收敛到恒定价格
    EXPECT_NEAR(final_wma, constant_price, 1e-10) 
        << "WMA should converge to constant price";
}

// 性能测试
TEST(OriginalTests, WMA_Performance) {
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
    
    auto large_wma = std::make_shared<WMA>(large_line, 100);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_wma->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "WMA calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_wma->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_GE(final_result, 50.0) << "Final result should be within expected range";
    EXPECT_LE(final_result, 150.0) << "Final result should be within expected range";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}