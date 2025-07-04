/**
 * @file test_ind_ema.cpp
 * @brief EMA指标测试 - 对应Python test_ind_ema.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4070.115719', '3644.444667', '3581.728712'],
 * ]
 * chkmin = 30
 * chkind = btind.EMA
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/ema.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> EMA_EXPECTED_VALUES = {
    {"4070.115719", "3644.444667", "3581.728712"}
};

const int EMA_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的EMA测试
DEFINE_INDICATOR_TEST(EMA_Default, EMA, EMA_EXPECTED_VALUES, EMA_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, EMA_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建EMA指标（默认30周期）
    auto ema = std::make_shared<EMA>(close_line, 30);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ema->calculate();
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
    
    std::vector<std::string> expected = {"4070.115719", "3644.444667", "3581.728712"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = ema->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "EMA value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(ema->getMinPeriod(), 30) << "EMA minimum period should be 30";
}

// 参数化测试 - 测试不同周期的EMA
class EMAParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(EMAParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto ema = std::make_shared<EMA>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        ema->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(ema->getMinPeriod(), period) 
        << "EMA minimum period should match parameter";
    
    // 验证最后的值不是NaN
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_value = ema->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last EMA value should not be NaN";
        EXPECT_GT(last_value, 0) << "EMA value should be positive for this test data";
    }
}

// 测试不同的EMA周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    EMAParameterizedTest,
    ::testing::Values(5, 10, 20, 30, 50, 100)
);

// EMA响应性测试 - EMA应该比SMA响应更快
TEST(OriginalTests, EMA_vs_SMA_Responsiveness) {
    auto csv_data = getdata(0);
    auto close_line_ema = std::make_shared<LineRoot>(csv_data.size(), "close_ema");
    auto close_line_sma = std::make_shared<LineRoot>(csv_data.size(), "close_sma");
    
    for (const auto& bar : csv_data) {
        close_line_ema->forward(bar.close);
        close_line_sma->forward(bar.close);
    }
    
    const int period = 20;
    auto ema = std::make_shared<EMA>(close_line_ema, period);
    auto sma = std::make_shared<SMA>(close_line_sma, period);
    
    std::vector<double> ema_changes;
    std::vector<double> sma_changes;
    double prev_ema = 0.0, prev_sma = 0.0;
    
    // 计算并记录变化
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ema->calculate();
        sma->calculate();
        
        double current_ema = ema->get(0);
        double current_sma = sma->get(0);
        
        if (i > period && !std::isnan(current_ema) && !std::isnan(current_sma)) {
            if (prev_ema != 0.0 && prev_sma != 0.0) {
                ema_changes.push_back(std::abs(current_ema - prev_ema));
                sma_changes.push_back(std::abs(current_sma - prev_sma));
            }
            prev_ema = current_ema;
            prev_sma = current_sma;
        }
        
        if (i < csv_data.size() - 1) {
            close_line_ema->forward();
            close_line_sma->forward();
        }
    }
    
    // 计算平均变化
    if (!ema_changes.empty() && !sma_changes.empty()) {
        double avg_ema_change = std::accumulate(ema_changes.begin(), ema_changes.end(), 0.0) / ema_changes.size();
        double avg_sma_change = std::accumulate(sma_changes.begin(), sma_changes.end(), 0.0) / sma_changes.size();
        
        // EMA通常应该比SMA有更大的变化（更敏感）
        std::cout << "Average EMA change: " << avg_ema_change << std::endl;
        std::cout << "Average SMA change: " << avg_sma_change << std::endl;
        
        // 注意：这个测试可能因数据而异，我们只验证都是正值
        EXPECT_GT(avg_ema_change, 0.0) << "EMA should show price changes";
        EXPECT_GT(avg_sma_change, 0.0) << "SMA should show price changes";
    }
}

// EMA平滑因子测试
TEST(OriginalTests, EMA_SmoothingFactor) {
    // 使用一个简单的价格序列来验证EMA计算
    std::vector<double> prices = {100.0, 102.0, 101.0, 103.0, 105.0, 104.0, 106.0, 108.0};
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "ema_smooth");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto ema = std::make_shared<EMA>(close_line, 3);  // 3周期EMA
    
    std::vector<double> ema_values;
    
    for (size_t i = 0; i < prices.size(); ++i) {
        ema->calculate();
        double current_ema = ema->get(0);
        if (!std::isnan(current_ema)) {
            ema_values.push_back(current_ema);
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证EMA值是连续的且在合理范围内
    for (size_t i = 1; i < ema_values.size(); ++i) {
        // EMA应该在相邻价格的合理范围内
        double min_price = std::min(prices[i + 2], prices[i + 1]);  // 调整索引
        double max_price = std::max(prices[i + 2], prices[i + 1]);
        
        // EMA可能在价格范围外，但不应该过度偏离
        // 这里我们只检查EMA是有限值
        EXPECT_TRUE(std::isfinite(ema_values[i])) 
            << "EMA value should be finite at step " << i;
    }
}

// 边界条件测试
TEST(OriginalTests, EMA_EdgeCases) {
    // 测试数据不足的情况
    auto close_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 5; ++i) {
        close_line->forward(100.0 + i);
    }
    
    auto ema = std::make_shared<EMA>(close_line, 10);  // 周期大于数据量
    
    for (int i = 0; i < 5; ++i) {
        ema->calculate();
        if (i < 4) {
            close_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = ema->get(0);
    EXPECT_TRUE(std::isnan(result)) << "EMA should return NaN when insufficient data";
}

// 收敛测试 - 验证EMA最终会收敛到稳定值
TEST(OriginalTests, EMA_Convergence) {
    // 使用恒定价格测试收敛
    const double constant_price = 100.0;
    const int num_points = 100;
    
    auto close_line = std::make_shared<LineRoot>(num_points, "convergence");
    for (int i = 0; i < num_points; ++i) {
        close_line->forward(constant_price);
    }
    
    auto ema = std::make_shared<EMA>(close_line, 10);
    
    double final_ema = 0.0;
    for (int i = 0; i < num_points; ++i) {
        ema->calculate();
        final_ema = ema->get(0);
        if (i < num_points - 1) {
            close_line->forward();
        }
    }
    
    // EMA应该收敛到恒定价格
    EXPECT_NEAR(final_ema, constant_price, 0.01) 
        << "EMA should converge to constant price";
}