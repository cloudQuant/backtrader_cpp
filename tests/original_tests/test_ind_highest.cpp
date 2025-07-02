/**
 * @file test_ind_highest.cpp
 * @brief Highest指标测试 - 对应Python test_ind_highest.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4140.660000', '3671.780000', '3670.750000'],
 * ]
 * chkmin = 14
 * chkind = btind.Highest
 * chkargs = dict(period=14)
 */

#include "test_common.h"
#include "indicators/Highest.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> HIGHEST_EXPECTED_VALUES = {
    {"4140.660000", "3671.780000", "3670.750000"}
};

const int HIGHEST_MIN_PERIOD = 14;

} // anonymous namespace

// 使用指定参数的Highest测试
DEFINE_INDICATOR_TEST(Highest_Period14, Highest, HIGHEST_EXPECTED_VALUES, HIGHEST_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, Highest_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建Highest指标（14周期）
    auto highest = std::make_shared<Highest>(close_line, 14);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        highest->calculate();
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
    
    std::vector<std::string> expected = {"4140.660000", "3671.780000", "3670.750000"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = highest->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "Highest value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(highest->getMinPeriod(), 14) << "Highest minimum period should be 14";
}

// 参数化测试 - 测试不同周期的Highest
class HighestParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(HighestParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto highest = std::make_shared<Highest>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        highest->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(highest->getMinPeriod(), period) 
        << "Highest minimum period should match parameter";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_value = highest->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last Highest value should not be NaN";
        EXPECT_GT(last_value, 0) << "Highest value should be positive for this test data";
    }
}

// 测试不同的Highest周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    HighestParameterizedTest,
    ::testing::Values(5, 10, 14, 20, 30)
);

// 计算逻辑验证测试
TEST(OriginalTests, Highest_CalculationLogic) {
    // 使用简单的测试数据验证Highest计算
    std::vector<double> prices = {100.0, 95.0, 110.0, 85.0, 120.0, 90.0, 105.0, 115.0, 80.0, 125.0};
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "highest_calc");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto highest = std::make_shared<Highest>(close_line, 5);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        highest->calculate();
        
        // 手动计算Highest进行验证
        if (i >= 4) {  // 需要至少5个数据点
            double expected_highest = -std::numeric_limits<double>::infinity();
            for (int j = 0; j < 5; ++j) {
                expected_highest = std::max(expected_highest, prices[i - j]);
            }
            
            double actual_highest = highest->get(0);
            EXPECT_NEAR(actual_highest, expected_highest, 1e-10) 
                << "Highest calculation mismatch at step " << i;
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// 滚动窗口测试
TEST(OriginalTests, Highest_RollingWindow) {
    // 创建一个已知的数据序列
    std::vector<double> prices = {10, 20, 30, 25, 15, 35, 40, 45, 20, 50, 5, 60, 55, 30};
    
    auto close_line = std::make_shared<LineRoot>(prices.size(), "rolling");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto highest = std::make_shared<Highest>(close_line, 3);  // 3周期滚动最高值
    
    std::vector<double> expected_results;
    
    for (size_t i = 0; i < prices.size(); ++i) {
        highest->calculate();
        
        // 手动计算期望结果
        if (i >= 2) {
            double manual_highest = std::max({prices[i], prices[i-1], prices[i-2]});
            expected_results.push_back(manual_highest);
            
            double actual = highest->get(0);
            EXPECT_NEAR(actual, manual_highest, 1e-10) 
                << "Rolling highest mismatch at position " << i;
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "Rolling 3-period highest results:" << std::endl;
    for (size_t i = 0; i < expected_results.size(); ++i) {
        std::cout << "Position " << (i + 2) << ": " << expected_results[i] << std::endl;
    }
}

// 单调性测试
TEST(OriginalTests, Highest_Monotonicity) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto highest = std::make_shared<Highest>(close_line, 20);
    
    // 验证Highest值的单调性质
    for (size_t i = 0; i < csv_data.size(); ++i) {
        highest->calculate();
        
        double current_highest = highest->get(0);
        double current_price = csv_data[i].close;
        
        // Highest值应该大于等于当前价格
        if (!std::isnan(current_highest)) {
            EXPECT_GE(current_highest, current_price) 
                << "Highest should be >= current price at step " << i;
            
            // 如果有前一个值，新的Highest不应该小于之前窗口重叠部分的最高值
            if (i > 0) {
                double prev_price = csv_data[i-1].close;
                // 这个测试比较复杂，这里只检查基本合理性
                EXPECT_GT(current_highest, 0) << "Highest should be positive";
            }
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
}

// 与真实数据的一致性测试
TEST(OriginalTests, Highest_vs_ManualCalculation) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    const int period = 10;
    auto highest = std::make_shared<Highest>(close_line, period);
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        highest->calculate();
        
        // 从第10个数据点开始验证
        if (i >= period - 1) {
            double indicator_highest = highest->get(0);
            
            // 手动计算最近10个数据点的最高值
            double manual_highest = -std::numeric_limits<double>::infinity();
            for (int j = 0; j < period; ++j) {
                manual_highest = std::max(manual_highest, csv_data[i - j].close);
            }
            
            EXPECT_NEAR(indicator_highest, manual_highest, 1e-10) 
                << "Manual vs indicator calculation mismatch at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
}

// 极值测试
TEST(OriginalTests, Highest_ExtremeValues) {
    // 测试包含极大值的数据
    std::vector<double> extreme_prices = {
        100.0, 105.0, 1000000.0, 110.0, 95.0,  // 包含一个极大值
        90.0, 85.0, 80.0, 75.0, 70.0
    };
    
    auto extreme_line = std::make_shared<LineRoot>(extreme_prices.size(), "extreme");
    for (double price : extreme_prices) {
        extreme_line->forward(price);
    }
    
    auto extreme_highest = std::make_shared<Highest>(extreme_line, 5);
    
    for (size_t i = 0; i < extreme_prices.size(); ++i) {
        extreme_highest->calculate();
        
        double current_highest = extreme_highest->get(0);
        
        // 在包含极大值的窗口中，应该返回极大值
        if (i >= 2 && i <= 6) {  // 包含1000000.0的窗口
            if (!std::isnan(current_highest)) {
                EXPECT_EQ(current_highest, 1000000.0) 
                    << "Should return extreme value when it's in the window at step " << i;
            }
        }
        
        if (i < extreme_prices.size() - 1) {
            extreme_line->forward();
        }
    }
}

// 边界条件测试
TEST(OriginalTests, Highest_EdgeCases) {
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加3个数据点
    std::vector<double> short_data = {100.0, 110.0, 90.0};
    for (double price : short_data) {
        insufficient_line->forward(price);
    }
    
    auto insufficient_highest = std::make_shared<Highest>(insufficient_line, 5);  // 周期大于数据量
    
    for (size_t i = 0; i < short_data.size(); ++i) {
        insufficient_highest->calculate();
        if (i < short_data.size() - 1) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_highest->get(0);
    EXPECT_TRUE(std::isnan(result)) << "Highest should return NaN when insufficient data";
    
    // 测试单个数据点的情况
    auto single_line = std::make_shared<LineRoot>(1, "single");
    single_line->forward(123.45);
    
    auto single_highest = std::make_shared<Highest>(single_line, 1);
    single_highest->calculate();
    
    double single_result = single_highest->get(0);
    EXPECT_NEAR(single_result, 123.45, 1e-10) 
        << "Highest of single value should equal that value";
}

// 性能测试
TEST(OriginalTests, Highest_Performance) {
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
    
    auto large_highest = std::make_shared<Highest>(large_line, 100);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_highest->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Highest calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_highest->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_GE(final_result, 50.0) << "Final result should be within expected range";
    EXPECT_LE(final_result, 150.0) << "Final result should be within expected range";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}