/**
 * @file test_ind_sumn.cpp
 * @brief SumN指标测试 - 对应Python test_ind_sumn.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['57406.490000', '50891.010000', '50424.690000']
 * ]
 * chkmin = 14
 * chkind = btind.SumN
 * chkargs = dict(period=14)
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/sumn.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> SUMN_EXPECTED_VALUES = {
    {"57406.490000", "50891.010000", "50424.690000"}
};

const int SUMN_MIN_PERIOD = 14;

} // anonymous namespace

// 使用默认参数的SumN测试
DEFINE_INDICATOR_TEST(SumN_Default, SumN, SUMN_EXPECTED_VALUES, SUMN_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, SumN_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建SumN指标（14周期）
    auto sumn = std::make_shared<SumN>(close_line, 14);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        sumn->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 14;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"57406.490000", "50891.010000", "50424.690000"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = sumn->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "SumN value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(sumn->getMinPeriod(), 14) << "SumN minimum period should be 14";
}

// 参数化测试 - 测试不同周期的SumN
class SumNParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(SumNParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto sumn = std::make_shared<SumN>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        sumn->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(sumn->getMinPeriod(), period) 
        << "SumN minimum period should equal period parameter";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_value = sumn->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last SumN value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last SumN value should be finite";
        EXPECT_GT(last_value, 0.0) << "SumN should be positive for positive prices";
    }
}

// 测试不同的SumN周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    SumNParameterizedTest,
    ::testing::Values(5, 10, 14, 20, 30)
);

// SumN计算逻辑验证测试
TEST(OriginalTests, SumN_CalculationLogic) {
    // 使用简单的测试数据验证SumN计算
    std::vector<double> prices = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "sumn_calc");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto sumn = std::make_shared<SumN>(close_line, 5);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        sumn->calculate();
        
        // 手动计算期望的SumN值进行验证
        if (i >= 4) {  // 需要至少5个数据点
            double expected_sum = 0.0;
            for (int j = 0; j < 5; ++j) {
                expected_sum += prices[i - j];
            }
            
            double actual_sum = sumn->get(0);
            
            if (!std::isnan(actual_sum)) {
                EXPECT_NEAR(actual_sum, expected_sum, 1e-10) 
                    << "SumN calculation mismatch at step " << i;
            }
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// 滚动窗口测试
TEST(OriginalTests, SumN_RollingWindow) {
    // 测试滚动窗口机制
    std::vector<double> test_prices = {10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0};
    
    auto test_line = std::make_shared<LineRoot>(test_prices.size(), "rolling");
    for (double price : test_prices) {
        test_line->forward(price);
    }
    
    auto sumn = std::make_shared<SumN>(test_line, 3);
    
    std::vector<double> expected_sums = {
        // 前两个值为NaN（数据不足）
        // 第3个值：10+20+30 = 60
        // 第4个值：20+30+40 = 90
        // 第5个值：30+40+50 = 120
        // 第6个值：40+50+60 = 150
        // 第7个值：50+60+70 = 180
        // 第8个值：60+70+80 = 210
        // 第9个值：70+80+90 = 240
        // 第10个值：80+90+100 = 270
        60.0, 90.0, 120.0, 150.0, 180.0, 210.0, 240.0, 270.0
    };
    
    size_t expected_idx = 0;
    for (size_t i = 0; i < test_prices.size(); ++i) {
        sumn->calculate();
        
        double actual_sum = sumn->get(0);
        
        if (i >= 2) {  // 从第3个值开始有效
            EXPECT_FALSE(std::isnan(actual_sum)) << "SumN should not be NaN at step " << i;
            if (expected_idx < expected_sums.size()) {
                EXPECT_NEAR(actual_sum, expected_sums[expected_idx], 1e-10) 
                    << "SumN rolling window test at step " << i;
                expected_idx++;
            }
        } else {
            EXPECT_TRUE(std::isnan(actual_sum)) << "SumN should be NaN at step " << i;
        }
        
        if (i < test_prices.size() - 1) {
            test_line->forward();
        }
    }
}

// 累积效应测试
TEST(OriginalTests, SumN_AccumulationEffect) {
    // 测试累积效应：价格增加应该导致SumN增加
    std::vector<double> increasing_prices;
    for (int i = 1; i <= 50; ++i) {
        increasing_prices.push_back(static_cast<double>(i));
    }
    
    auto inc_line = std::make_shared<LineRoot>(increasing_prices.size(), "increasing");
    for (double price : increasing_prices) {
        inc_line->forward(price);
    }
    
    auto inc_sumn = std::make_shared<SumN>(inc_line, 10);
    
    double prev_sum = 0.0;
    bool has_prev = false;
    int increasing_count = 0;
    int total_count = 0;
    
    for (size_t i = 0; i < increasing_prices.size(); ++i) {
        inc_sumn->calculate();
        
        double current_sum = inc_sumn->get(0);
        
        if (!std::isnan(current_sum)) {
            if (has_prev) {
                total_count++;
                if (current_sum > prev_sum) {
                    increasing_count++;
                }
            }
            prev_sum = current_sum;
            has_prev = true;
        }
        
        if (i < increasing_prices.size() - 1) {
            inc_line->forward();
        }
    }
    
    // 对于递增序列，SumN应该主要呈增长趋势
    if (total_count > 0) {
        double increasing_ratio = static_cast<double>(increasing_count) / total_count;
        EXPECT_GT(increasing_ratio, 0.9) 
            << "SumN should increase for increasing price sequence";
        
        std::cout << "Increasing sequence - SumN increasing ratio: " << increasing_ratio << std::endl;
    }
}

// 周期性数据测试
TEST(OriginalTests, SumN_PeriodicData) {
    // 创建周期性数据
    std::vector<double> periodic_prices;
    for (int i = 0; i < 100; ++i) {
        double angle = i * 2.0 * M_PI / 20.0;  // 20个点为一个周期
        periodic_prices.push_back(50.0 + 10.0 * std::sin(angle));
    }
    
    auto periodic_line = std::make_shared<LineRoot>(periodic_prices.size(), "periodic");
    for (double price : periodic_prices) {
        periodic_line->forward(price);
    }
    
    auto periodic_sumn = std::make_shared<SumN>(periodic_line, 20);  // 窗口等于周期
    
    std::vector<double> sumn_values;
    
    for (size_t i = 0; i < periodic_prices.size(); ++i) {
        periodic_sumn->calculate();
        
        double sumn_val = periodic_sumn->get(0);
        if (!std::isnan(sumn_val)) {
            sumn_values.push_back(sumn_val);
        }
        
        if (i < periodic_prices.size() - 1) {
            periodic_line->forward();
        }
    }
    
    // 对于周期性数据，当窗口等于周期时，SumN应该相对稳定
    if (sumn_values.size() > 20) {
        double sum_of_sums = std::accumulate(sumn_values.end() - 20, sumn_values.end(), 0.0);
        double avg_sum = sum_of_sums / 20.0;
        
        // 计算标准差
        double variance = 0.0;
        for (auto it = sumn_values.end() - 20; it != sumn_values.end(); ++it) {
            variance += (*it - avg_sum) * (*it - avg_sum);
        }
        variance /= 20.0;
        double std_dev = std::sqrt(variance);
        
        std::cout << "Periodic data - SumN average: " << avg_sum 
                  << ", std dev: " << std_dev << std::endl;
        
        // 对于完整周期窗口，标准差应该较小
        EXPECT_LT(std_dev / avg_sum, 0.1) 
            << "SumN should be relatively stable for periodic data with period-length window";
    }
}

// 边界值测试
TEST(OriginalTests, SumN_BoundaryValues) {
    // 测试极值
    std::vector<double> extreme_prices = {0.0, 1e6, -1e6, 1e-6, -1e-6};
    
    auto extreme_line = std::make_shared<LineRoot>(extreme_prices.size(), "extreme");
    for (double price : extreme_prices) {
        extreme_line->forward(price);
    }
    
    auto extreme_sumn = std::make_shared<SumN>(extreme_line, 3);
    
    for (size_t i = 0; i < extreme_prices.size(); ++i) {
        extreme_sumn->calculate();
        
        double sumn_val = extreme_sumn->get(0);
        
        if (!std::isnan(sumn_val)) {
            EXPECT_TRUE(std::isfinite(sumn_val)) 
                << "SumN should be finite even for extreme values at step " << i;
        }
        
        if (i < extreme_prices.size() - 1) {
            extreme_line->forward();
        }
    }
}

// 边界条件测试
TEST(OriginalTests, SumN_EdgeCases) {
    // 测试所有价格为零的情况
    std::vector<double> zero_prices(50, 0.0);
    
    auto zero_line = std::make_shared<LineRoot>(zero_prices.size(), "zero");
    for (double price : zero_prices) {
        zero_line->forward(price);
    }
    
    auto zero_sumn = std::make_shared<SumN>(zero_line, 10);
    
    for (size_t i = 0; i < zero_prices.size(); ++i) {
        zero_sumn->calculate();
        if (i < zero_prices.size() - 1) {
            zero_line->forward();
        }
    }
    
    // 当所有价格为零时，SumN应该为零
    double final_sum = zero_sumn->get(0);
    if (!std::isnan(final_sum)) {
        EXPECT_NEAR(final_sum, 0.0, 1e-10) 
            << "SumN should be zero for all-zero prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 5; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_sumn = std::make_shared<SumN>(insufficient_line, 10);
    
    for (int i = 0; i < 5; ++i) {
        insufficient_sumn->calculate();
        if (i < 4) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_sumn->get(0);
    EXPECT_TRUE(std::isnan(result)) << "SumN should return NaN when insufficient data";
}

// 与SMA关系测试
TEST(OriginalTests, SumN_vs_SMA_Relationship) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    int period = 20;
    auto sumn = std::make_shared<SumN>(close_line, period);
    auto sma = std::make_shared<SMA>(close_line, period);
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        sumn->calculate();
        sma->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // SumN应该等于SMA * period
    double final_sumn = sumn->get(0);
    double final_sma = sma->get(0);
    
    if (!std::isnan(final_sumn) && !std::isnan(final_sma)) {
        double expected_sumn = final_sma * period;
        EXPECT_NEAR(final_sumn, expected_sumn, 1e-6) 
            << "SumN should equal SMA * period";
        
        std::cout << "SumN: " << final_sumn << ", SMA * period: " << expected_sumn << std::endl;
    }
}

// 性能测试
TEST(OriginalTests, SumN_Performance) {
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
    
    auto large_sumn = std::make_shared<SumN>(large_line, 100);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_sumn->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "SumN calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_sumn->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}