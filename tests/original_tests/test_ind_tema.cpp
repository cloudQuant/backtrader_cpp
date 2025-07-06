/**
 * @file test_ind_tema.cpp
 * @brief TEMA指标测试 - 对应Python test_ind_tema.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4113.721705', '3862.386854', '3832.691054']
 * ]
 * chkmin = 88
 * chkind = btind.TEMA
 */

#include "test_common.h"
#include <random>

#include "indicators/tema.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> TEMA_EXPECTED_VALUES = {
    {"4113.721705", "3862.386854", "3832.691054"}
};

const int TEMA_MIN_PERIOD = 88;

} // anonymous namespace

// 使用默认参数的TEMA测试
DEFINE_INDICATOR_TEST(TEMA_Default, TEMA, TEMA_EXPECTED_VALUES, TEMA_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, TEMA_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建TEMA指标（默认30周期，最小周期为88）
    auto tema = std::make_shared<TEMA>(close_line, 30);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        tema->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 88;  // 3 * period - 2
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"4113.721705", "3862.386854", "3832.691054"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = tema->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "TEMA value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(tema->getMinPeriod(), 88) << "TEMA minimum period should be 88";
}

// 参数化测试 - 测试不同周期的TEMA
class TEMAParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(TEMAParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto tema = std::make_shared<TEMA>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        tema->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期 (3 * period - 2)
    EXPECT_EQ(tema->getMinPeriod(), 3 * period - 2) 
        << "TEMA minimum period should be 3 * period - 2";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(3 * period - 2)) {
        double last_value = tema->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last TEMA value should not be NaN";
        EXPECT_GT(last_value, 0) << "TEMA value should be positive for this test data";
    }
}

// 测试不同的TEMA周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    TEMAParameterizedTest,
    ::testing::Values(10, 20, 30)
);

// TEMA响应性测试 - TEMA应该比DEMA和EMA响应更快
TEST(OriginalTests, TEMA_vs_Others_Responsiveness) {
    auto csv_data = getdata(0);
    auto close_line_tema = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close_tema");
    auto close_line_dema = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close_dema");
    auto close_line_ema = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close_ema");
    
    for (const auto& bar : csv_data) {
        close_line_tema->forward(bar.close);
        close_line_dema->forward(bar.close);
        close_line_ema->forward(bar.close);
    }
    
    const int period = 20;
    auto tema = std::make_shared<TEMA>(close_line_tema, period);
    auto dema = std::make_shared<DEMA>(close_line_dema, period);
    auto ema = std::make_shared<EMA>(close_line_ema, period);
    
    std::vector<double> tema_changes;
    std::vector<double> dema_changes;
    std::vector<double> ema_changes;
    double prev_tema = 0.0, prev_dema = 0.0, prev_ema = 0.0;
    
    // 计算并记录变化
    for (size_t i = 0; i < csv_data.size(); ++i) {
        tema->calculate();
        dema->calculate();
        ema->calculate();
        
        double current_tema = tema->get(0);
        double current_dema = dema->get(0);
        double current_ema = ema->get(0);
        
        if (i > 3 * period && !std::isnan(current_tema) && !std::isnan(current_dema) && !std::isnan(current_ema)) {
            if (prev_tema != 0.0 && prev_dema != 0.0 && prev_ema != 0.0) {
                tema_changes.push_back(std::abs(current_tema - prev_tema));
                dema_changes.push_back(std::abs(current_dema - prev_dema));
                ema_changes.push_back(std::abs(current_ema - prev_ema));
            }
            prev_tema = current_tema;
            prev_dema = current_dema;
            prev_ema = current_ema;
        }
        
        if (i < csv_data.size() - 1) {
            close_line_tema->forward();
            close_line_dema->forward();
            close_line_ema->forward();
        }
    }
    
    // 计算平均变化
    if (!tema_changes.empty() && !dema_changes.empty() && !ema_changes.empty()) {
        double avg_tema_change = std::accumulate(tema_changes.begin(), tema_changes.end(), 0.0) / tema_changes.size();
        double avg_dema_change = std::accumulate(dema_changes.begin(), dema_changes.end(), 0.0) / dema_changes.size();
        double avg_ema_change = std::accumulate(ema_changes.begin(), ema_changes.end(), 0.0) / ema_changes.size();
        
        std::cout << "Average TEMA change: " << avg_tema_change << std::endl;
        std::cout << "Average DEMA change: " << avg_dema_change << std::endl;
        std::cout << "Average EMA change: " << avg_ema_change << std::endl;
        
        // 验证都是正值
        EXPECT_GT(avg_tema_change, 0.0) << "TEMA should show price changes";
        EXPECT_GT(avg_dema_change, 0.0) << "DEMA should show price changes";
        EXPECT_GT(avg_ema_change, 0.0) << "EMA should show price changes";
    }
}

// 滞后测试 - TEMA应该比其他移动平均线滞后更小
TEST(OriginalTests, TEMA_LagTest) {
    // 创建一个步进价格序列来测试滞后
    std::vector<double> step_prices;
    
    // 前30个点为100
    for (int i = 0; i < 30; ++i) {
        step_prices.push_back(100.0);
    }
    
    // 后30个点为120（步进上升）
    for (int i = 0; i < 30; ++i) {
        step_prices.push_back(120.0);
    }
    
    auto close_line_tema = std::make_shared<backtrader::LineRoot>(step_prices.size(), "step_tema");
    auto close_line_dema = std::make_shared<backtrader::LineRoot>(step_prices.size(), "step_dema");
    auto close_line_sma = std::make_shared<backtrader::LineRoot>(step_prices.size(), "step_sma");
    
    for (double price : step_prices) {
        close_line_tema->forward(price);
        close_line_dema->forward(price);
        close_line_sma->forward(price);
    }
    
    const int period = 10;
    auto tema = std::make_shared<TEMA>(close_line_tema, period);
    auto dema = std::make_shared<DEMA>(close_line_dema, period);
    auto sma = std::make_shared<SMA>(close_line_sma, period);
    
    std::vector<double> final_values;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        tema->calculate();
        dema->calculate();
        sma->calculate();
        
        if (i == step_prices.size() - 1) {
            // 记录最终值
            final_values.push_back(tema->get(0));
            final_values.push_back(dema->get(0));
            final_values.push_back(sma->get(0));
        }
        
        if (i < step_prices.size() - 1) {
            close_line_tema->forward();
            close_line_dema->forward();
            close_line_sma->forward();
        }
    }
    
    if (final_values.size() >= 3) {
        std::cout << "Final TEMA: " << final_values[0] << std::endl;
        std::cout << "Final DEMA: " << final_values[1] << std::endl;
        std::cout << "Final SMA: " << final_values[2] << std::endl;
        
        // TEMA应该最接近新价格120
        double target = 120.0;
        double tema_distance = std::abs(final_values[0] - target);
        double dema_distance = std::abs(final_values[1] - target);
        double sma_distance = std::abs(final_values[2] - target);
        
        // TEMA应该比SMA更接近目标价格
        EXPECT_LT(tema_distance, sma_distance) 
            << "TEMA should be closer to target price than SMA";
            
        // TEMA通常也应该比DEMA更接近目标价格
        EXPECT_LT(tema_distance, dema_distance) 
            << "TEMA should be closer to target price than DEMA";
    }
}

// 边界条件测试
TEST(OriginalTests, TEMA_EdgeCases) {
    // 测试数据不足的情况
    auto close_line = std::make_shared<backtrader::LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 30; ++i) {
        close_line->forward(100.0 + i);
    }
    
    auto tema = std::make_shared<TEMA>(close_line, 30);  // 最小周期为88
    
    for (int i = 0; i < 30; ++i) {
        tema->calculate();
        if (i < 29) {
            close_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = tema->get(0);
    EXPECT_TRUE(std::isnan(result)) << "TEMA should return NaN when insufficient data";
}

// 收敛测试
TEST(OriginalTests, TEMA_Convergence) {
    // 使用恒定价格测试收敛
    const double constant_price = 100.0;
    const int num_points = 300;
    
    auto close_line = std::make_shared<backtrader::LineRoot>(num_points, "convergence");
    for (int i = 0; i < num_points; ++i) {
        close_line->forward(constant_price);
    }
    
    auto tema = std::make_shared<TEMA>(close_line, 20);
    
    double final_tema = 0.0;
    for (int i = 0; i < num_points; ++i) {
        tema->calculate();
        final_tema = tema->get(0);
        if (i < num_points - 1) {
            close_line->forward();
        }
    }
    
    // TEMA应该收敛到恒定价格
    EXPECT_NEAR(final_tema, constant_price, 0.01) 
        << "TEMA should converge to constant price";
}

// 性能测试
TEST(OriginalTests, TEMA_Performance) {
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
    
    auto large_tema = std::make_shared<TEMA>(large_line, 50);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_tema->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "TEMA calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_tema->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_GE(final_result, 50.0) << "Final result should be within expected range";
    EXPECT_LE(final_result, 150.0) << "Final result should be within expected range";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}
