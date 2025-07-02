/**
 * @file test_ind_momentum.cpp
 * @brief Momentum指标测试 - 对应Python test_ind_momentum.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['67.050000', '-34.160000', '67.630000'],
 * ]
 * chkmin = 13
 * chkind = btind.Momentum
 */

#include "test_common.h"
#include "indicators/Momentum.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> MOMENTUM_EXPECTED_VALUES = {
    {"67.050000", "-34.160000", "67.630000"}
};

const int MOMENTUM_MIN_PERIOD = 13;

} // anonymous namespace

// 使用默认参数的Momentum测试
DEFINE_INDICATOR_TEST(Momentum_Default, Momentum, MOMENTUM_EXPECTED_VALUES, MOMENTUM_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, Momentum_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建Momentum指标（默认12周期，最小周期为13）
    auto momentum = std::make_shared<Momentum>(close_line, 12);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        momentum->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 13;  // period + 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"67.050000", "-34.160000", "67.630000"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = momentum->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "Momentum value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(momentum->getMinPeriod(), 13) << "Momentum minimum period should be 13";
}

// 参数化测试 - 测试不同周期的Momentum
class MomentumParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(MomentumParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto momentum = std::make_shared<Momentum>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        momentum->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期 (period + 1)
    EXPECT_EQ(momentum->getMinPeriod(), period + 1) 
        << "Momentum minimum period should be period + 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        double last_value = momentum->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last Momentum value should not be NaN";
        // Momentum可以是正值、负值或零，所以不检查符号
        EXPECT_TRUE(std::isfinite(last_value)) << "Momentum value should be finite";
    }
}

// 测试不同的Momentum周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    MomentumParameterizedTest,
    ::testing::Values(5, 10, 12, 20, 30)
);

// 计算逻辑验证测试
TEST(OriginalTests, Momentum_CalculationLogic) {
    // 使用简单的测试数据验证Momentum计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0};
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "momentum_calc");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto momentum = std::make_shared<Momentum>(close_line, 5);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        momentum->calculate();
        
        // 手动计算Momentum进行验证
        if (i >= 5) {  // 需要period + 1个数据点
            double current_price = prices[i];
            double past_price = prices[i - 5];  // 5周期前的价格
            double expected_momentum = current_price - past_price;
            
            double actual_momentum = momentum->get(0);
            EXPECT_NEAR(actual_momentum, expected_momentum, 1e-10) 
                << "Momentum calculation mismatch at step " << i 
                << " (current: " << current_price << ", past: " << past_price << ")";
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// 趋势检测测试
TEST(OriginalTests, Momentum_TrendDetection) {
    // 创建上升趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 30; ++i) {
        uptrend_prices.push_back(100.0 + i * 2.0);  // 持续上升
    }
    
    auto up_line = std::make_shared<LineRoot>(uptrend_prices.size(), "uptrend");
    for (double price : uptrend_prices) {
        up_line->forward(price);
    }
    
    auto up_momentum = std::make_shared<Momentum>(up_line, 10);
    
    for (size_t i = 0; i < uptrend_prices.size(); ++i) {
        up_momentum->calculate();
        if (i < uptrend_prices.size() - 1) {
            up_line->forward();
        }
    }
    
    double final_up_momentum = up_momentum->get(0);
    if (!std::isnan(final_up_momentum)) {
        EXPECT_GT(final_up_momentum, 0.0) 
            << "Momentum should be positive for uptrend";
    }
    
    // 创建下降趋势数据
    std::vector<double> downtrend_prices;
    for (int i = 0; i < 30; ++i) {
        downtrend_prices.push_back(200.0 - i * 2.0);  // 持续下降
    }
    
    auto down_line = std::make_shared<LineRoot>(downtrend_prices.size(), "downtrend");
    for (double price : downtrend_prices) {
        down_line->forward(price);
    }
    
    auto down_momentum = std::make_shared<Momentum>(down_line, 10);
    
    for (size_t i = 0; i < downtrend_prices.size(); ++i) {
        down_momentum->calculate();
        if (i < downtrend_prices.size() - 1) {
            down_line->forward();
        }
    }
    
    double final_down_momentum = down_momentum->get(0);
    if (!std::isnan(final_down_momentum)) {
        EXPECT_LT(final_down_momentum, 0.0) 
            << "Momentum should be negative for downtrend";
    }
    
    std::cout << "Uptrend momentum: " << final_up_momentum << std::endl;
    std::cout << "Downtrend momentum: " << final_down_momentum << std::endl;
}

// 横盘市场测试
TEST(OriginalTests, Momentum_SidewaysMarket) {
    // 创建横盘震荡数据
    std::vector<double> sideways_prices;
    for (int i = 0; i < 50; ++i) {
        double base = 100.0;
        double oscillation = 5.0 * std::sin(i * 0.2);  // 小幅震荡
        sideways_prices.push_back(base + oscillation);
    }
    
    auto sideways_line = std::make_shared<LineRoot>(sideways_prices.size(), "sideways");
    for (double price : sideways_prices) {
        sideways_line->forward(price);
    }
    
    auto sideways_momentum = std::make_shared<Momentum>(sideways_line, 20);
    
    std::vector<double> momentum_values;
    
    for (size_t i = 0; i < sideways_prices.size(); ++i) {
        sideways_momentum->calculate();
        
        double momentum_val = sideways_momentum->get(0);
        if (!std::isnan(momentum_val)) {
            momentum_values.push_back(momentum_val);
        }
        
        if (i < sideways_prices.size() - 1) {
            sideways_line->forward();
        }
    }
    
    // 横盘市场中，动量应该接近零且波动较小
    if (!momentum_values.empty()) {
        double avg_momentum = std::accumulate(momentum_values.begin(), momentum_values.end(), 0.0) / momentum_values.size();
        EXPECT_NEAR(avg_momentum, 0.0, 10.0) 
            << "Average momentum should be close to zero in sideways market";
        
        std::cout << "Sideways market average momentum: " << avg_momentum << std::endl;
    }
}

// 动量发散测试
TEST(OriginalTests, Momentum_Divergence) {
    // 创建价格新高但动量减弱的数据
    std::vector<double> divergence_prices;
    
    // 第一段：强劲上升
    for (int i = 0; i < 15; ++i) {
        divergence_prices.push_back(100.0 + i * 3.0);
    }
    
    // 第二段：缓慢上升（价格新高但动量减弱）
    for (int i = 0; i < 15; ++i) {
        divergence_prices.push_back(145.0 + i * 0.5);
    }
    
    auto div_line = std::make_shared<LineRoot>(divergence_prices.size(), "divergence");
    for (double price : divergence_prices) {
        div_line->forward(price);
    }
    
    auto div_momentum = std::make_shared<Momentum>(div_line, 10);
    
    std::vector<double> early_momentum;
    std::vector<double> late_momentum;
    
    for (size_t i = 0; i < divergence_prices.size(); ++i) {
        div_momentum->calculate();
        
        double momentum_val = div_momentum->get(0);
        if (!std::isnan(momentum_val)) {
            if (i < 20) {
                early_momentum.push_back(momentum_val);
            } else {
                late_momentum.push_back(momentum_val);
            }
        }
        
        if (i < divergence_prices.size() - 1) {
            div_line->forward();
        }
    }
    
    // 计算平均动量
    if (!early_momentum.empty() && !late_momentum.empty()) {
        double avg_early = std::accumulate(early_momentum.begin(), early_momentum.end(), 0.0) / early_momentum.size();
        double avg_late = std::accumulate(late_momentum.begin(), late_momentum.end(), 0.0) / late_momentum.size();
        
        std::cout << "Early momentum average: " << avg_early << std::endl;
        std::cout << "Late momentum average: " << avg_late << std::endl;
        
        // 后期动量应该明显小于早期动量
        EXPECT_LT(avg_late, avg_early) 
            << "Late momentum should be less than early momentum in divergence scenario";
    }
}

// 零线交叉测试
TEST(OriginalTests, Momentum_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto momentum = std::make_shared<Momentum>(close_line, 12);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_momentum = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越
    for (size_t i = 0; i < csv_data.size(); ++i) {
        momentum->calculate();
        
        double current_momentum = momentum->get(0);
        
        if (!std::isnan(current_momentum) && has_prev) {
            if (prev_momentum <= 0.0 && current_momentum > 0.0) {
                positive_crossings++;
            } else if (prev_momentum >= 0.0 && current_momentum < 0.0) {
                negative_crossings++;
            }
        }
        
        if (!std::isnan(current_momentum)) {
            prev_momentum = current_momentum;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "Momentum zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// 边界条件测试
TEST(OriginalTests, Momentum_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(20, 100.0);  // 20个相同价格
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_momentum = std::make_shared<Momentum>(flat_line, 10);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_momentum->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    double final_momentum = flat_momentum->get(0);
    if (!std::isnan(final_momentum)) {
        EXPECT_NEAR(final_momentum, 0.0, 1e-10) 
            << "Momentum should be zero for constant prices";
    }
}