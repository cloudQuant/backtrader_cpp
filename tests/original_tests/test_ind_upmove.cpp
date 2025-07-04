/**
 * @file test_ind_upmove.cpp
 * @brief UpMove指标测试 - 对应Python test_ind_upmove.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['-10.720000', '10.010000', '14.000000']
 * ]
 * chkmin = 2
 * chkind = btind.UpMove
 */

#include "test_common_simple.h"

#include "indicators/upmove.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> UPMOVE_EXPECTED_VALUES = {
    {"-10.720000", "10.010000", "14.000000"}
};

const int UPMOVE_MIN_PERIOD = 2;

} // anonymous namespace

// 使用默认参数的UpMove测试
DEFINE_INDICATOR_TEST(UpMove_Default, UpMove, UPMOVE_EXPECTED_VALUES, UPMOVE_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, UpMove_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
    }
    
    // 创建UpMove指标
    auto upmove = std::make_shared<UpMove>(high_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        upmove->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 2;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"-10.720000", "10.010000", "14.000000"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = upmove->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "UpMove value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(upmove->getMinPeriod(), 2) << "UpMove minimum period should be 2";
}

// UpMove计算逻辑验证测试
TEST(OriginalTests, UpMove_CalculationLogic) {
    // 使用简单的测试数据验证UpMove计算
    std::vector<double> high_prices = {100.0, 105.0, 102.0, 108.0, 104.0, 110.0, 106.0, 112.0, 109.0, 115.0};
    
    auto high_line = std::make_shared<LineRoot>(high_prices.size(), "upmove_calc");
    for (double price : high_prices) {
        high_line->forward(price);
    }
    
    auto upmove = std::make_shared<UpMove>(high_line);
    
    for (size_t i = 0; i < high_prices.size(); ++i) {
        upmove->calculate();
        
        // 手动计算期望的UpMove值进行验证
        if (i >= 1) {  // 需要至少2个数据点
            double current_high = high_prices[i];
            double prev_high = high_prices[i - 1];
            double expected_upmove = current_high - prev_high;  // 只有当current > prev时才为正值
            
            // UpMove = max(0, current_high - prev_high)
            expected_upmove = std::max(0.0, expected_upmove);
            
            double actual_upmove = upmove->get(0);
            
            if (!std::isnan(actual_upmove)) {
                EXPECT_NEAR(actual_upmove, expected_upmove, 1e-10) 
                    << "UpMove calculation mismatch at step " << i
                    << " (prev=" << prev_high << ", current=" << current_high << ")";
            }
        }
        
        if (i < high_prices.size() - 1) {
            high_line->forward();
        }
    }
}

// UpMove向上运动识别测试
TEST(OriginalTests, UpMove_UpwardMovementDetection) {
    // 创建明确的向上运动数据
    std::vector<double> upward_highs = {100.0, 105.0, 110.0, 115.0, 120.0, 125.0, 130.0};
    
    auto up_line = std::make_shared<LineRoot>(upward_highs.size(), "upward");
    for (double price : upward_highs) {
        up_line->forward(price);
    }
    
    auto up_upmove = std::make_shared<UpMove>(up_line);
    
    std::vector<double> upmove_values;
    
    for (size_t i = 0; i < upward_highs.size(); ++i) {
        up_upmove->calculate();
        
        double upmove_val = up_upmove->get(0);
        if (!std::isnan(upmove_val)) {
            upmove_values.push_back(upmove_val);
        }
        
        if (i < upward_highs.size() - 1) {
            up_line->forward();
        }
    }
    
    // 在持续上升的数据中，所有UpMove值都应该为正
    for (size_t i = 0; i < upmove_values.size(); ++i) {
        EXPECT_GE(upmove_values[i], 0.0) 
            << "UpMove should be non-negative at step " << i;
        
        if (i > 0) {  // 除了第一个值
            EXPECT_GT(upmove_values[i], 0.0) 
                << "UpMove should be positive for upward movement at step " << i;
        }
    }
    
    std::cout << "Upward movement UpMove values:" << std::endl;
    for (size_t i = 0; i < upmove_values.size(); ++i) {
        std::cout << "Step " << i << ": " << upmove_values[i] << std::endl;
    }
}

// UpMove向下运动测试
TEST(OriginalTests, UpMove_DownwardMovementTest) {
    // 创建向下运动数据
    std::vector<double> downward_highs = {130.0, 125.0, 120.0, 115.0, 110.0, 105.0, 100.0};
    
    auto down_line = std::make_shared<LineRoot>(downward_highs.size(), "downward");
    for (double price : downward_highs) {
        down_line->forward(price);
    }
    
    auto down_upmove = std::make_shared<UpMove>(down_line);
    
    std::vector<double> upmove_values;
    
    for (size_t i = 0; i < downward_highs.size(); ++i) {
        down_upmove->calculate();
        
        double upmove_val = down_upmove->get(0);
        if (!std::isnan(upmove_val)) {
            upmove_values.push_back(upmove_val);
        }
        
        if (i < downward_highs.size() - 1) {
            down_line->forward();
        }
    }
    
    // 在持续下降的数据中，所有UpMove值都应该为零
    for (size_t i = 1; i < upmove_values.size(); ++i) {  // 跳过第一个值
        EXPECT_NEAR(upmove_values[i], 0.0, 1e-10) 
            << "UpMove should be zero for downward movement at step " << i;
    }
    
    std::cout << "Downward movement UpMove values:" << std::endl;
    for (size_t i = 0; i < upmove_values.size(); ++i) {
        std::cout << "Step " << i << ": " << upmove_values[i] << std::endl;
    }
}

// UpMove混合运动测试
TEST(OriginalTests, UpMove_MixedMovement) {
    // 创建混合运动数据
    std::vector<double> mixed_highs = {100.0, 105.0, 102.0, 108.0, 104.0, 112.0, 107.0, 115.0};
    
    auto mixed_line = std::make_shared<LineRoot>(mixed_highs.size(), "mixed");
    for (double price : mixed_highs) {
        mixed_line->forward(price);
    }
    
    auto mixed_upmove = std::make_shared<UpMove>(mixed_line);
    
    std::vector<double> expected_upmoves = {
        // 100.0 -> 105.0: 105-100 = 5.0 (向上)
        // 105.0 -> 102.0: max(0, 102-105) = 0.0 (向下)
        // 102.0 -> 108.0: 108-102 = 6.0 (向上)
        // 108.0 -> 104.0: max(0, 104-108) = 0.0 (向下)
        // 104.0 -> 112.0: 112-104 = 8.0 (向上)
        // 112.0 -> 107.0: max(0, 107-112) = 0.0 (向下)
        // 107.0 -> 115.0: 115-107 = 8.0 (向上)
        5.0, 0.0, 6.0, 0.0, 8.0, 0.0, 8.0
    };
    
    std::vector<double> actual_upmoves;
    
    for (size_t i = 0; i < mixed_highs.size(); ++i) {
        mixed_upmove->calculate();
        
        double upmove_val = mixed_upmove->get(0);
        if (!std::isnan(upmove_val)) {
            actual_upmoves.push_back(upmove_val);
        }
        
        if (i < mixed_highs.size() - 1) {
            mixed_line->forward();
        }
    }
    
    // 验证计算结果
    for (size_t i = 0; i < expected_upmoves.size() && i < actual_upmoves.size(); ++i) {
        EXPECT_NEAR(actual_upmoves[i], expected_upmoves[i], 1e-10) 
            << "UpMove mismatch at step " << (i + 1)
            << " (expected=" << expected_upmoves[i] 
            << ", actual=" << actual_upmoves[i] << ")";
    }
    
    std::cout << "Mixed movement analysis:" << std::endl;
    for (size_t i = 0; i < actual_upmoves.size(); ++i) {
        std::cout << "Step " << (i + 1) << ": expected=" << expected_upmoves[i] 
                  << ", actual=" << actual_upmoves[i] << std::endl;
    }
}

// UpMove累积效应测试
TEST(OriginalTests, UpMove_CumulativeEffect) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
    }
    
    auto upmove = std::make_shared<UpMove>(high_line);
    
    double total_upmove = 0.0;
    int up_periods = 0;
    int down_periods = 0;
    
    // 分析UpMove的累积效应
    for (size_t i = 0; i < csv_data.size(); ++i) {
        upmove->calculate();
        
        double upmove_val = upmove->get(0);
        if (!std::isnan(upmove_val)) {
            total_upmove += upmove_val;
            
            if (upmove_val > 0.0) {
                up_periods++;
            } else {
                down_periods++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
        }
    }
    
    std::cout << "UpMove cumulative analysis:" << std::endl;
    std::cout << "Total upward movement: " << total_upmove << std::endl;
    std::cout << "Up periods: " << up_periods << std::endl;
    std::cout << "Down periods: " << down_periods << std::endl;
    
    if (up_periods > 0) {
        double avg_upmove = total_upmove / up_periods;
        std::cout << "Average upward move: " << avg_upmove << std::endl;
        
        EXPECT_GT(avg_upmove, 0.0) << "Average upward move should be positive";
    }
    
    // 验证至少有一些有效的计算
    EXPECT_GT(up_periods + down_periods, 0) 
        << "Should have some valid UpMove calculations";
}

// UpMove与DownMove对称性测试
TEST(OriginalTests, UpMove_DownMoveSymmetry) {
    // 使用相同的数据测试UpMove和DownMove
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    auto upmove = std::make_shared<UpMove>(high_line);
    auto downmove = std::make_shared<DownMove>(low_line);
    
    double total_upmove = 0.0;
    double total_downmove = 0.0;
    int valid_count = 0;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        upmove->calculate();
        downmove->calculate();
        
        double up_val = upmove->get(0);
        double down_val = downmove->get(0);
        
        if (!std::isnan(up_val) && !std::isnan(down_val)) {
            total_upmove += up_val;
            total_downmove += down_val;
            valid_count++;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
        }
    }
    
    std::cout << "UpMove vs DownMove comparison:" << std::endl;
    std::cout << "Total UpMove: " << total_upmove << std::endl;
    std::cout << "Total DownMove: " << total_downmove << std::endl;
    
    if (valid_count > 0) {
        double avg_upmove = total_upmove / valid_count;
        double avg_downmove = total_downmove / valid_count;
        
        std::cout << "Average UpMove: " << avg_upmove << std::endl;
        std::cout << "Average DownMove: " << avg_downmove << std::endl;
        
        // 验证两者都是非负的
        EXPECT_GE(avg_upmove, 0.0) << "Average UpMove should be non-negative";
        EXPECT_GE(avg_downmove, 0.0) << "Average DownMove should be non-negative";
    }
}

// UpMove与价格波动关系测试
TEST(OriginalTests, UpMove_PriceVolatilityRelation) {
    // 创建高波动性数据
    std::vector<double> volatile_highs;
    for (int i = 0; i < 50; ++i) {
        double base = 100.0;
        double volatility = 10.0 * std::sin(i * 0.5);  // 大幅波动
        volatile_highs.push_back(base + volatility);
    }
    
    // 创建低波动性数据
    std::vector<double> stable_highs;
    for (int i = 0; i < 50; ++i) {
        double base = 100.0;
        double volatility = 1.0 * std::sin(i * 0.5);  // 小幅波动
        stable_highs.push_back(base + volatility);
    }
    
    auto volatile_line = std::make_shared<LineRoot>(volatile_highs.size(), "volatile");
    for (double price : volatile_highs) {
        volatile_line->forward(price);
    }
    
    auto stable_line = std::make_shared<LineRoot>(stable_highs.size(), "stable");
    for (double price : stable_highs) {
        stable_line->forward(price);
    }
    
    auto volatile_upmove = std::make_shared<UpMove>(volatile_line);
    auto stable_upmove = std::make_shared<UpMove>(stable_line);
    
    double volatile_total = 0.0;
    double stable_total = 0.0;
    int volatile_count = 0;
    int stable_count = 0;
    
    for (size_t i = 0; i < 50; ++i) {
        volatile_upmove->calculate();
        stable_upmove->calculate();
        
        double volatile_val = volatile_upmove->get(0);
        double stable_val = stable_upmove->get(0);
        
        if (!std::isnan(volatile_val)) {
            volatile_total += volatile_val;
            volatile_count++;
        }
        
        if (!std::isnan(stable_val)) {
            stable_total += stable_val;
            stable_count++;
        }
        
        if (i < 49) {
            volatile_line->forward();
            stable_line->forward();
        }
    }
    
    std::cout << "Volatility comparison:" << std::endl;
    
    if (volatile_count > 0) {
        double volatile_avg = volatile_total / volatile_count;
        std::cout << "High volatility average UpMove: " << volatile_avg << std::endl;
    }
    
    if (stable_count > 0) {
        double stable_avg = stable_total / stable_count;
        std::cout << "Low volatility average UpMove: " << stable_avg << std::endl;
    }
    
    // 验证高波动性产生更大的UpMove
    if (volatile_count > 0 && stable_count > 0) {
        double volatile_avg = volatile_total / volatile_count;
        double stable_avg = stable_total / stable_count;
        
        EXPECT_GE(volatile_avg, stable_avg) 
            << "High volatility should produce larger UpMove values";
    }
}

// 边界条件测试
TEST(OriginalTests, UpMove_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_upmove = std::make_shared<UpMove>(flat_line);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_upmove->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，UpMove应该为零
    double final_upmove = flat_upmove->get(0);
    if (!std::isnan(final_upmove)) {
        EXPECT_NEAR(final_upmove, 0.0, 1e-10) 
            << "UpMove should be zero for constant prices";
    }
    
    // 测试单个数据点
    auto single_line = std::make_shared<LineRoot>(10, "single");
    single_line->forward(100.0);
    
    auto single_upmove = std::make_shared<UpMove>(single_line);
    single_upmove->calculate();
    
    // 单个数据点时应该返回NaN
    double single_result = single_upmove->get(0);
    EXPECT_TRUE(std::isnan(single_result)) << "UpMove should return NaN for single data point";
    
    // 测试极值
    std::vector<double> extreme_prices = {1e-6, 1e6, 0.0, -1e6};
    
    auto extreme_line = std::make_shared<LineRoot>(extreme_prices.size(), "extreme");
    for (double price : extreme_prices) {
        extreme_line->forward(price);
    }
    
    auto extreme_upmove = std::make_shared<UpMove>(extreme_line);
    
    for (size_t i = 0; i < extreme_prices.size(); ++i) {
        extreme_upmove->calculate();
        
        double upmove_val = extreme_upmove->get(0);
        
        if (!std::isnan(upmove_val)) {
            EXPECT_TRUE(std::isfinite(upmove_val)) 
                << "UpMove should be finite even for extreme values at step " << i;
            EXPECT_GE(upmove_val, 0.0) 
                << "UpMove should be non-negative at step " << i;
        }
        
        if (i < extreme_prices.size() - 1) {
            extreme_line->forward();
        }
    }
}

// 性能测试
TEST(OriginalTests, UpMove_Performance) {
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
    
    auto large_upmove = std::make_shared<UpMove>(large_line);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_upmove->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "UpMove calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_upmove->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    EXPECT_GE(final_result, 0.0) << "Final result should be non-negative";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}