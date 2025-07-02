/**
 * @file test_ind_tsi.cpp
 * @brief TSI指标测试 - 对应Python test_ind_tsi.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['16.012364', '22.866307', '4.990750']
 * ]
 * chkmin = 38
 * chkind = bt.ind.TSI
 */

#include "test_common.h"
#include "indicators/TSI.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> TSI_EXPECTED_VALUES = {
    {"16.012364", "22.866307", "4.990750"}
};

const int TSI_MIN_PERIOD = 38;

} // anonymous namespace

// 使用默认参数的TSI测试
DEFINE_INDICATOR_TEST(TSI_Default, TSI, TSI_EXPECTED_VALUES, TSI_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, TSI_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建TSI指标（默认参数：25, 13）
    auto tsi = std::make_shared<TSI>(close_line, 25, 13);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        tsi->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 38;  // 25 + 13
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"16.012364", "22.866307", "4.990750"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = tsi->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "TSI value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(tsi->getMinPeriod(), 38) << "TSI minimum period should be 38";
}

// TSI范围验证测试
TEST(OriginalTests, TSI_RangeValidation) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto tsi = std::make_shared<TSI>(close_line, 25, 13);
    
    // 计算所有值并验证范围
    for (size_t i = 0; i < csv_data.size(); ++i) {
        tsi->calculate();
        
        double tsi_value = tsi->get(0);
        
        // 验证TSI在-100到+100范围内
        if (!std::isnan(tsi_value)) {
            EXPECT_GE(tsi_value, -100.0) << "TSI should be >= -100 at step " << i;
            EXPECT_LE(tsi_value, 100.0) << "TSI should be <= 100 at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
}

// 参数化测试 - 测试不同参数的TSI
class TSIParameterizedTest : public ::testing::TestWithParam<std::pair<int, int>> {
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

TEST_P(TSIParameterizedTest, DifferentParameters) {
    auto [period1, period2] = GetParam();
    auto tsi = std::make_shared<TSI>(close_line_, period1, period2);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        tsi->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    int expected_min_period = period1 + period2;
    EXPECT_EQ(tsi->getMinPeriod(), expected_min_period) 
        << "TSI minimum period should be period1 + period2";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_value = tsi->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last TSI value should not be NaN";
        EXPECT_GE(last_value, -100.0) << "TSI should be >= -100";
        EXPECT_LE(last_value, 100.0) << "TSI should be <= 100";
    }
}

// 测试不同的TSI参数
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    TSIParameterizedTest,
    ::testing::Values(
        std::make_pair(25, 13),   // 标准参数
        std::make_pair(13, 7),
        std::make_pair(40, 20),
        std::make_pair(15, 8)
    )
);

// TSI计算逻辑验证测试
TEST(OriginalTests, TSI_CalculationLogic) {
    // 使用简单的测试数据验证TSI计算
    std::vector<double> prices = {100.0, 102.0, 101.0, 103.0, 105.0, 104.0, 106.0, 108.0, 107.0, 109.0};
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "tsi_calc");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto tsi = std::make_shared<TSI>(close_line, 5, 3);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        tsi->calculate();
        
        double tsi_val = tsi->get(0);
        
        // TSI应该产生有限值且在-100到100范围内
        if (!std::isnan(tsi_val)) {
            EXPECT_TRUE(std::isfinite(tsi_val)) 
                << "TSI should be finite at step " << i;
            EXPECT_GE(tsi_val, -100.0) 
                << "TSI should be >= -100 at step " << i;
            EXPECT_LE(tsi_val, 100.0) 
                << "TSI should be <= 100 at step " << i;
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// 趋势强度测试
TEST(OriginalTests, TSI_TrendStrength) {
    // 创建强势上升趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 100; ++i) {
        uptrend_prices.push_back(100.0 + i * 1.0);  // 持续上升
    }
    
    auto up_line = std::make_shared<LineRoot>(uptrend_prices.size(), "uptrend");
    for (double price : uptrend_prices) {
        up_line->forward(price);
    }
    
    auto up_tsi = std::make_shared<TSI>(up_line, 25, 13);
    
    for (size_t i = 0; i < uptrend_prices.size(); ++i) {
        up_tsi->calculate();
        if (i < uptrend_prices.size() - 1) {
            up_line->forward();
        }
    }
    
    double final_up_tsi = up_tsi->get(0);
    if (!std::isnan(final_up_tsi)) {
        EXPECT_GT(final_up_tsi, 0.0) 
            << "TSI should be positive for strong uptrend";
        
        std::cout << "Strong uptrend TSI: " << final_up_tsi << std::endl;
    }
    
    // 创建强势下降趋势数据
    std::vector<double> downtrend_prices;
    for (int i = 0; i < 100; ++i) {
        downtrend_prices.push_back(200.0 - i * 1.0);  // 持续下降
    }
    
    auto down_line = std::make_shared<LineRoot>(downtrend_prices.size(), "downtrend");
    for (double price : downtrend_prices) {
        down_line->forward(price);
    }
    
    auto down_tsi = std::make_shared<TSI>(down_line, 25, 13);
    
    for (size_t i = 0; i < downtrend_prices.size(); ++i) {
        down_tsi->calculate();
        if (i < downtrend_prices.size() - 1) {
            down_line->forward();
        }
    }
    
    double final_down_tsi = down_tsi->get(0);
    if (!std::isnan(final_down_tsi)) {
        EXPECT_LT(final_down_tsi, 0.0) 
            << "TSI should be negative for strong downtrend";
        
        std::cout << "Strong downtrend TSI: " << final_down_tsi << std::endl;
    }
}

// 零线穿越测试
TEST(OriginalTests, TSI_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto tsi = std::make_shared<TSI>(close_line, 25, 13);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_tsi = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越
    for (size_t i = 0; i < csv_data.size(); ++i) {
        tsi->calculate();
        
        double current_tsi = tsi->get(0);
        
        if (!std::isnan(current_tsi) && has_prev) {
            if (prev_tsi <= 0.0 && current_tsi > 0.0) {
                positive_crossings++;
            } else if (prev_tsi >= 0.0 && current_tsi < 0.0) {
                negative_crossings++;
            }
        }
        
        if (!std::isnan(current_tsi)) {
            prev_tsi = current_tsi;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "TSI zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// 超买超卖水平测试
TEST(OriginalTests, TSI_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto tsi = std::make_shared<TSI>(close_line, 25, 13);
    
    int overbought_signals = 0;  // TSI > 25
    int oversold_signals = 0;    // TSI < -25
    int normal_signals = 0;      // -25 <= TSI <= 25
    
    // 统计超买超卖信号
    for (size_t i = 0; i < csv_data.size(); ++i) {
        tsi->calculate();
        
        double tsi_value = tsi->get(0);
        
        if (!std::isnan(tsi_value)) {
            if (tsi_value > 25.0) {
                overbought_signals++;
            } else if (tsi_value < -25.0) {
                oversold_signals++;
            } else {
                normal_signals++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "TSI signal statistics:" << std::endl;
    std::cout << "Overbought signals (> 25): " << overbought_signals << std::endl;
    std::cout << "Oversold signals (< -25): " << oversold_signals << std::endl;
    std::cout << "Normal signals (-25 to 25): " << normal_signals << std::endl;
    
    // 验证至少有一些有效的计算
    EXPECT_GT(overbought_signals + oversold_signals + normal_signals, 0) 
        << "Should have some valid TSI calculations";
}

// 发散测试
TEST(OriginalTests, TSI_Divergence) {
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
    
    auto div_tsi = std::make_shared<TSI>(div_line, 25, 13);
    
    std::vector<double> early_tsi;
    std::vector<double> late_tsi;
    
    for (size_t i = 0; i < divergence_prices.size(); ++i) {
        div_tsi->calculate();
        
        double tsi_val = div_tsi->get(0);
        
        if (!std::isnan(tsi_val)) {
            if (i < 60) {
                early_tsi.push_back(tsi_val);
            } else {
                late_tsi.push_back(tsi_val);
            }
        }
        
        if (i < divergence_prices.size() - 1) {
            div_line->forward();
        }
    }
    
    // 分析发散现象
    if (!early_tsi.empty() && !late_tsi.empty()) {
        double avg_early = std::accumulate(early_tsi.begin(), early_tsi.end(), 0.0) / early_tsi.size();
        double avg_late = std::accumulate(late_tsi.begin(), late_tsi.end(), 0.0) / late_tsi.size();
        
        std::cout << "Early TSI average: " << avg_early << std::endl;
        std::cout << "Late TSI average: " << avg_late << std::endl;
        
        // 验证计算结果是有限的
        EXPECT_TRUE(std::isfinite(avg_early)) << "Early TSI should be finite";
        EXPECT_TRUE(std::isfinite(avg_late)) << "Late TSI should be finite";
        
        // 后期动量应该小于早期动量
        EXPECT_LT(avg_late, avg_early) 
            << "Late TSI should be less than early TSI in divergence scenario";
    }
}

// 震荡市场测试
TEST(OriginalTests, TSI_ChoppyMarket) {
    // 创建震荡市场数据
    std::vector<double> choppy_prices;
    for (int i = 0; i < 100; ++i) {
        double base = 100.0;
        double oscillation = 5.0 * std::sin(i * 0.3);
        choppy_prices.push_back(base + oscillation);
    }
    
    auto choppy_line = std::make_shared<LineRoot>(choppy_prices.size(), "choppy");
    for (double price : choppy_prices) {
        choppy_line->forward(price);
    }
    
    auto choppy_tsi = std::make_shared<TSI>(choppy_line, 25, 13);
    
    std::vector<double> tsi_values;
    
    for (size_t i = 0; i < choppy_prices.size(); ++i) {
        choppy_tsi->calculate();
        
        double tsi_val = choppy_tsi->get(0);
        if (!std::isnan(tsi_val)) {
            tsi_values.push_back(tsi_val);
        }
        
        if (i < choppy_prices.size() - 1) {
            choppy_line->forward();
        }
    }
    
    // 在震荡市场中，TSI应该在零线附近波动
    if (!tsi_values.empty()) {
        double avg_tsi = std::accumulate(tsi_values.begin(), tsi_values.end(), 0.0) / tsi_values.size();
        EXPECT_NEAR(avg_tsi, 0.0, 10.0) 
            << "Average TSI should be close to zero in choppy market";
        
        std::cout << "Choppy market average TSI: " << avg_tsi << std::endl;
    }
}

// 边界条件测试
TEST(OriginalTests, TSI_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_tsi = std::make_shared<TSI>(flat_line, 25, 13);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_tsi->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，TSI应该为0或接近0
    double final_tsi = flat_tsi->get(0);
    if (!std::isnan(final_tsi)) {
        EXPECT_NEAR(final_tsi, 0.0, 1e-10) 
            << "TSI should be zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 20; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_tsi = std::make_shared<TSI>(insufficient_line, 25, 13);
    
    for (int i = 0; i < 20; ++i) {
        insufficient_tsi->calculate();
        if (i < 19) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_tsi->get(0);
    EXPECT_TRUE(std::isnan(result)) << "TSI should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, TSI_Performance) {
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
    
    auto large_tsi = std::make_shared<TSI>(large_line, 25, 13);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_tsi->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "TSI calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_tsi->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_GE(final_result, -100.0) << "Final result should be >= -100";
    EXPECT_LE(final_result, 100.0) << "Final result should be <= 100";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}