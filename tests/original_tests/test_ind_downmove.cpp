/**
 * @file test_ind_downmove.cpp
 * @brief DownMove指标测试 - 对应Python test_ind_downmove.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['10.720000', '-10.010000', '-14.000000']
 * ]
 * chkmin = 2
 * chkind = btind.DownMove
 */

#include "test_common.h"
#include <random>

#include "indicators/downmove.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> DOWNMOVE_EXPECTED_VALUES = {
    {"10.720000", "-10.010000", "-14.000000"}
};

const int DOWNMOVE_MIN_PERIOD = 2;

} // anonymous namespace

// 使用默认参数的DownMove测试
DEFINE_INDICATOR_TEST(DownMove_Default, DownMove, DOWNMOVE_EXPECTED_VALUES, DOWNMOVE_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, DownMove_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    for (const auto& bar : csv_data) {
        low_line->forward(bar.low);
    }
    
    // 创建DownMove指标
    auto downmove = std::make_shared<DownMove>(low_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        downmove->calculate();
        if (i < csv_data.size() - 1) {
            low_line->forward();
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
    
    std::vector<std::string> expected = {"10.720000", "-10.010000", "-14.000000"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = downmove->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "DownMove value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(downmove->getMinPeriod(), 2) << "DownMove minimum period should be 2";
}

// DownMove计算逻辑验证测试
TEST(OriginalTests, DownMove_CalculationLogic) {
    // 使用简单的测试数据验证DownMove计算
    std::vector<double> low_prices = {100.0, 95.0, 98.0, 92.0, 96.0, 90.0, 94.0, 88.0, 91.0, 85.0};
    
    auto low_line = std::make_shared<backtrader::LineRoot>(low_prices.size(), "downmove_calc");
    for (double price : low_prices) {
        low_line->forward(price);
    }
    
    auto downmove = std::make_shared<DownMove>(low_line);
    
    for (size_t i = 0; i < low_prices.size(); ++i) {
        downmove->calculate();
        
        // 手动计算期望的DownMove值进行验证
        if (i >= 1) {  // 需要至少2个数据点
            double current_low = low_prices[i];
            double prev_low = low_prices[i - 1];
            double expected_downmove = prev_low - current_low;  // 只有当current < prev时才为正值
            
            // DownMove = max(0, prev_low - current_low)
            expected_downmove = std::max(0.0, expected_downmove);
            
            double actual_downmove = downmove->get(0);
            
            if (!std::isnan(actual_downmove)) {
                EXPECT_NEAR(actual_downmove, expected_downmove, 1e-10) 
                    << "DownMove calculation mismatch at step " << i
                    << " (prev=" << prev_low << ", current=" << current_low << ")";
            }
        }
        
        if (i < low_prices.size() - 1) {
            low_line->forward();
        }
    }
}

// DownMove向下运动识别测试
TEST(OriginalTests, DownMove_DownwardMovementDetection) {
    // 创建明确的向下运动数据
    std::vector<double> downward_lows = {100.0, 95.0, 90.0, 85.0, 80.0, 75.0, 70.0};
    
    auto down_line = std::make_shared<backtrader::LineRoot>(downward_lows.size(), "downward");
    for (double price : downward_lows) {
        down_line->forward(price);
    }
    
    auto down_downmove = std::make_shared<DownMove>(down_line);
    
    std::vector<double> downmove_values;
    
    for (size_t i = 0; i < downward_lows.size(); ++i) {
        down_downmove->calculate();
        
        double downmove_val = down_downmove->get(0);
        if (!std::isnan(downmove_val)) {
            downmove_values.push_back(downmove_val);
        }
        
        if (i < downward_lows.size() - 1) {
            down_line->forward();
        }
    }
    
    // 在持续下降的数据中，所有DownMove值都应该为正
    for (size_t i = 0; i < downmove_values.size(); ++i) {
        EXPECT_GE(downmove_values[i], 0.0) 
            << "DownMove should be non-negative at step " << i;
        
        if (i > 0) {  // 除了第一个值
            EXPECT_GT(downmove_values[i], 0.0) 
                << "DownMove should be positive for downward movement at step " << i;
        }
    }
    
    std::cout << "Downward movement DownMove values:" << std::endl;
    for (size_t i = 0; i < downmove_values.size(); ++i) {
        std::cout << "Step " << i << ": " << downmove_values[i] << std::endl;
    }
}

// DownMove向上运动测试
TEST(OriginalTests, DownMove_UpwardMovementTest) {
    // 创建向上运动数据
    std::vector<double> upward_lows = {70.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100.0};
    
    auto up_line = std::make_shared<backtrader::LineRoot>(upward_lows.size(), "upward");
    for (double price : upward_lows) {
        up_line->forward(price);
    }
    
    auto up_downmove = std::make_shared<DownMove>(up_line);
    
    std::vector<double> downmove_values;
    
    for (size_t i = 0; i < upward_lows.size(); ++i) {
        up_downmove->calculate();
        
        double downmove_val = up_downmove->get(0);
        if (!std::isnan(downmove_val)) {
            downmove_values.push_back(downmove_val);
        }
        
        if (i < upward_lows.size() - 1) {
            up_line->forward();
        }
    }
    
    // 在持续上升的数据中，所有DownMove值都应该为零
    for (size_t i = 1; i < downmove_values.size(); ++i) {  // 跳过第一个值
        EXPECT_NEAR(downmove_values[i], 0.0, 1e-10) 
            << "DownMove should be zero for upward movement at step " << i;
    }
    
    std::cout << "Upward movement DownMove values:" << std::endl;
    for (size_t i = 0; i < downmove_values.size(); ++i) {
        std::cout << "Step " << i << ": " << downmove_values[i] << std::endl;
    }
}

// DownMove混合运动测试
TEST(OriginalTests, DownMove_MixedMovement) {
    // 创建混合运动数据
    std::vector<double> mixed_lows = {100.0, 95.0, 98.0, 92.0, 96.0, 88.0, 93.0, 85.0};
    
    auto mixed_line = std::make_shared<backtrader::LineRoot>(mixed_lows.size(), "mixed");
    for (double price : mixed_lows) {
        mixed_line->forward(price);
    }
    
    auto mixed_downmove = std::make_shared<DownMove>(mixed_line);
    
    std::vector<double> expected_downmoves = {
        // 100.0 -> 95.0: 100-95 = 5.0 (向下)
        // 95.0 -> 98.0: max(0, 95-98) = 0.0 (向上)
        // 98.0 -> 92.0: 98-92 = 6.0 (向下)
        // 92.0 -> 96.0: max(0, 92-96) = 0.0 (向上)
        // 96.0 -> 88.0: 96-88 = 8.0 (向下)
        // 88.0 -> 93.0: max(0, 88-93) = 0.0 (向上)
        // 93.0 -> 85.0: 93-85 = 8.0 (向下)
        5.0, 0.0, 6.0, 0.0, 8.0, 0.0, 8.0
    };
    
    std::vector<double> actual_downmoves;
    
    for (size_t i = 0; i < mixed_lows.size(); ++i) {
        mixed_downmove->calculate();
        
        double downmove_val = mixed_downmove->get(0);
        if (!std::isnan(downmove_val)) {
            actual_downmoves.push_back(downmove_val);
        }
        
        if (i < mixed_lows.size() - 1) {
            mixed_line->forward();
        }
    }
    
    // 验证计算结果
    for (size_t i = 0; i < expected_downmoves.size() && i < actual_downmoves.size(); ++i) {
        EXPECT_NEAR(actual_downmoves[i], expected_downmoves[i], 1e-10) 
            << "DownMove mismatch at step " << (i + 1)
            << " (expected=" << expected_downmoves[i] 
            << ", actual=" << actual_downmoves[i] << ")";
    }
    
    std::cout << "Mixed movement analysis:" << std::endl;
    for (size_t i = 0; i < actual_downmoves.size(); ++i) {
        std::cout << "Step " << (i + 1) << ": expected=" << expected_downmoves[i] 
                  << ", actual=" << actual_downmoves[i] << std::endl;
    }
}

// DownMove累积效应测试
TEST(OriginalTests, DownMove_CumulativeEffect) {
    auto csv_data = getdata(0);
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    for (const auto& bar : csv_data) {
        low_line->forward(bar.low);
    }
    
    auto downmove = std::make_shared<DownMove>(low_line);
    
    double total_downmove = 0.0;
    int down_periods = 0;
    int up_periods = 0;
    
    // 分析DownMove的累积效应
    for (size_t i = 0; i < csv_data.size(); ++i) {
        downmove->calculate();
        
        double downmove_val = downmove->get(0);
        if (!std::isnan(downmove_val)) {
            total_downmove += downmove_val;
            
            if (downmove_val > 0.0) {
                down_periods++;
            } else {
                up_periods++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            low_line->forward();
        }
    }
    
    std::cout << "DownMove cumulative analysis:" << std::endl;
    std::cout << "Total downward movement: " << total_downmove << std::endl;
    std::cout << "Down periods: " << down_periods << std::endl;
    std::cout << "Up periods: " << up_periods << std::endl;
    
    if (down_periods > 0) {
        double avg_downmove = total_downmove / down_periods;
        std::cout << "Average downward move: " << avg_downmove << std::endl;
        
        EXPECT_GT(avg_downmove, 0.0) << "Average downward move should be positive";
    }
    
    // 验证至少有一些有效的计算
    EXPECT_GT(down_periods + up_periods, 0) 
        << "Should have some valid DownMove calculations";
}

// DownMove与价格波动关系测试
TEST(OriginalTests, DownMove_PriceVolatilityRelation) {
    // 创建高波动性数据
    std::vector<double> volatile_lows;
    for (int i = 0; i < 50; ++i) {
        double base = 100.0;
        double volatility = 10.0 * std::sin(i * 0.5);  // 大幅波动
        volatile_lows.push_back(base + volatility);
    }
    
    // 创建低波动性数据
    std::vector<double> stable_lows;
    for (int i = 0; i < 50; ++i) {
        double base = 100.0;
        double volatility = 1.0 * std::sin(i * 0.5);  // 小幅波动
        stable_lows.push_back(base + volatility);
    }
    
    auto volatile_line = std::make_shared<backtrader::LineRoot>(volatile_lows.size(), "volatile");
    for (double price : volatile_lows) {
        volatile_line->forward(price);
    }
    
    auto stable_line = std::make_shared<backtrader::LineRoot>(stable_lows.size(), "stable");
    for (double price : stable_lows) {
        stable_line->forward(price);
    }
    
    auto volatile_downmove = std::make_shared<DownMove>(volatile_line);
    auto stable_downmove = std::make_shared<DownMove>(stable_line);
    
    double volatile_total = 0.0;
    double stable_total = 0.0;
    int volatile_count = 0;
    int stable_count = 0;
    
    for (size_t i = 0; i < 50; ++i) {
        volatile_downmove->calculate();
        stable_downmove->calculate();
        
        double volatile_val = volatile_downmove->get(0);
        double stable_val = stable_downmove->get(0);
        
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
        std::cout << "High volatility average DownMove: " << volatile_avg << std::endl;
    }
    
    if (stable_count > 0) {
        double stable_avg = stable_total / stable_count;
        std::cout << "Low volatility average DownMove: " << stable_avg << std::endl;
    }
    
    // 验证高波动性产生更大的DownMove
    if (volatile_count > 0 && stable_count > 0) {
        double volatile_avg = volatile_total / volatile_count;
        double stable_avg = stable_total / stable_count;
        
        EXPECT_GE(volatile_avg, stable_avg) 
            << "High volatility should produce larger DownMove values";
    }
}

// 边界条件测试
TEST(OriginalTests, DownMove_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<backtrader::LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_downmove = std::make_shared<DownMove>(flat_line);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_downmove->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，DownMove应该为零
    double final_downmove = flat_downmove->get(0);
    if (!std::isnan(final_downmove)) {
        EXPECT_NEAR(final_downmove, 0.0, 1e-10) 
            << "DownMove should be zero for constant prices";
    }
    
    // 测试单个数据点
    auto single_line = std::make_shared<backtrader::LineRoot>(10, "single");
    single_line->forward(100.0);
    
    auto single_downmove = std::make_shared<DownMove>(single_line);
    single_downmove->calculate();
    
    // 单个数据点时应该返回NaN
    double single_result = single_downmove->get(0);
    EXPECT_TRUE(std::isnan(single_result)) << "DownMove should return NaN for single data point";
    
    // 测试极值
    std::vector<double> extreme_prices = {1e6, 0.0, 1e-6, -1e6};
    
    auto extreme_line = std::make_shared<backtrader::LineRoot>(extreme_prices.size(), "extreme");
    for (double price : extreme_prices) {
        extreme_line->forward(price);
    }
    
    auto extreme_downmove = std::make_shared<DownMove>(extreme_line);
    
    for (size_t i = 0; i < extreme_prices.size(); ++i) {
        extreme_downmove->calculate();
        
        double downmove_val = extreme_downmove->get(0);
        
        if (!std::isnan(downmove_val)) {
            EXPECT_TRUE(std::isfinite(downmove_val)) 
                << "DownMove should be finite even for extreme values at step " << i;
            EXPECT_GE(downmove_val, 0.0) 
                << "DownMove should be non-negative at step " << i;
        }
        
        if (i < extreme_prices.size() - 1) {
            extreme_line->forward();
        }
    }
}

// 性能测试
TEST(OriginalTests, DownMove_Performance) {
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
    
    auto large_downmove = std::make_shared<DownMove>(large_line);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_downmove->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "DownMove calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_downmove->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    EXPECT_GE(final_result, 0.0) << "Final result should be non-negative";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}