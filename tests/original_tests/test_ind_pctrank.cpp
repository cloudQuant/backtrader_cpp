/**
 * @file test_ind_pctrank.cpp
 * @brief PercentRank指标测试 - 对应Python test_ind_pctrank.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['0.900000', '0.880000', '0.980000'],
 * ]
 * chkmin = 50
 * chkind = btind.PercentRank
 */

#include "test_common.h"
#include <random>

#include "indicators/percentrank.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> PCTRANK_EXPECTED_VALUES = {
    {"0.900000", "0.880000", "0.980000"}
};

const int PCTRANK_MIN_PERIOD = 50;

} // anonymous namespace

// 使用默认参数的PercentRank测试
DEFINE_INDICATOR_TEST(PercentRank_Default, PercentRank, PCTRANK_EXPECTED_VALUES, PCTRANK_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, PercentRank_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建PercentRank指标（默认50周期）
    auto pctrank = std::make_shared<PercentRank>(close_line, 50);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        pctrank->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 50;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"0.900000", "0.880000", "0.980000"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = pctrank->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "PercentRank value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(pctrank->getMinPeriod(), 50) << "PercentRank minimum period should be 50";
}

// PercentRank范围验证测试
TEST(OriginalTests, PercentRank_RangeValidation) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto pctrank = std::make_shared<PercentRank>(close_line, 50);
    
    // 计算所有值并验证范围
    for (size_t i = 0; i < csv_data.size(); ++i) {
        pctrank->calculate();
        
        double rank_value = pctrank->get(0);
        
        // 验证PercentRank在0到1范围内
        if (!std::isnan(rank_value)) {
            EXPECT_GE(rank_value, 0.0) << "PercentRank should be >= 0 at step " << i;
            EXPECT_LE(rank_value, 1.0) << "PercentRank should be <= 1 at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
}

// 参数化测试 - 测试不同周期的PercentRank
class PercentRankParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(PercentRankParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto pctrank = std::make_shared<PercentRank>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        pctrank->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(pctrank->getMinPeriod(), period) 
        << "PercentRank minimum period should match parameter";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_value = pctrank->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last PercentRank value should not be NaN";
        EXPECT_GE(last_value, 0.0) << "PercentRank should be >= 0";
        EXPECT_LE(last_value, 1.0) << "PercentRank should be <= 1";
    }
}

// 测试不同的PercentRank周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    PercentRankParameterizedTest,
    ::testing::Values(10, 20, 50, 100)
);

// PercentRank计算逻辑验证测试
TEST(OriginalTests, PercentRank_CalculationLogic) {
    // 使用简单的测试数据验证PercentRank计算
    std::vector<double> prices = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
    
    auto close_line = std::make_shared<backtrader::LineRoot>(prices.size(), "rank_calc");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto pctrank = std::make_shared<PercentRank>(close_line, 5);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        pctrank->calculate();
        
        // 手动计算PercentRank进行验证
        if (i >= 4) {  // 需要至少5个数据点
            double current_price = prices[i];
            int count_below = 0;
            
            // 统计在窗口中有多少值小于当前值
            for (int j = 0; j < 5; ++j) {
                if (prices[i - j] < current_price) {
                    count_below++;
                }
            }
            
            // PercentRank = count_below / (period - 1)
            double expected_rank = static_cast<double>(count_below) / 4.0;
            double actual_rank = pctrank->get(0);
            
            if (!std::isnan(actual_rank)) {
                EXPECT_NEAR(actual_rank, expected_rank, 1e-10) 
                    << "PercentRank calculation mismatch at step " << i;
            }
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
}

// 单调递增序列测试
TEST(OriginalTests, PercentRank_MonotonicIncreasing) {
    // 对于单调递增序列，当前值应该总是最高的
    std::vector<double> monotonic_prices;
    for (int i = 0; i < 100; ++i) {
        monotonic_prices.push_back(100.0 + i);
    }
    
    auto monotonic_line = std::make_shared<backtrader::LineRoot>(monotonic_prices.size(), "monotonic");
    for (double price : monotonic_prices) {
        monotonic_line->forward(price);
    }
    
    auto monotonic_rank = std::make_shared<PercentRank>(monotonic_line, 20);
    
    for (size_t i = 0; i < monotonic_prices.size(); ++i) {
        monotonic_rank->calculate();
        
        double rank_value = monotonic_rank->get(0);
        
        // 对于单调递增序列，当前值总是最高的，所以rank应该是1.0
        if (!std::isnan(rank_value) && i >= 19) {
            EXPECT_NEAR(rank_value, 1.0, 1e-10) 
                << "PercentRank should be 1.0 for monotonic increasing at step " << i;
        }
        
        if (i < monotonic_prices.size() - 1) {
            monotonic_line->forward();
        }
    }
}

// 单调递减序列测试
TEST(OriginalTests, PercentRank_MonotonicDecreasing) {
    // 对于单调递减序列，当前值应该总是最低的
    std::vector<double> monotonic_prices;
    for (int i = 0; i < 100; ++i) {
        monotonic_prices.push_back(200.0 - i);
    }
    
    auto monotonic_line = std::make_shared<backtrader::LineRoot>(monotonic_prices.size(), "monotonic_dec");
    for (double price : monotonic_prices) {
        monotonic_line->forward(price);
    }
    
    auto monotonic_rank = std::make_shared<PercentRank>(monotonic_line, 20);
    
    for (size_t i = 0; i < monotonic_prices.size(); ++i) {
        monotonic_rank->calculate();
        
        double rank_value = monotonic_rank->get(0);
        
        // 对于单调递减序列，当前值总是最低的，所以rank应该是0.0
        if (!std::isnan(rank_value) && i >= 19) {
            EXPECT_NEAR(rank_value, 0.0, 1e-10) 
                << "PercentRank should be 0.0 for monotonic decreasing at step " << i;
        }
        
        if (i < monotonic_prices.size() - 1) {
            monotonic_line->forward();
        }
    }
}

// 重复值测试
TEST(OriginalTests, PercentRank_DuplicateValues) {
    // 测试包含重复值的序列
    std::vector<double> duplicate_prices = {100.0, 105.0, 105.0, 110.0, 110.0, 110.0, 115.0, 115.0, 120.0, 120.0};
    
    auto duplicate_line = std::make_shared<backtrader::LineRoot>(duplicate_prices.size(), "duplicate");
    for (double price : duplicate_prices) {
        duplicate_line->forward(price);
    }
    
    auto duplicate_rank = std::make_shared<PercentRank>(duplicate_line, 5);
    
    for (size_t i = 0; i < duplicate_prices.size(); ++i) {
        duplicate_rank->calculate();
        
        double rank_value = duplicate_rank->get(0);
        
        // 验证rank值在有效范围内
        if (!std::isnan(rank_value)) {
            EXPECT_GE(rank_value, 0.0) << "PercentRank should be >= 0 with duplicates";
            EXPECT_LE(rank_value, 1.0) << "PercentRank should be <= 1 with duplicates";
        }
        
        if (i < duplicate_prices.size() - 1) {
            duplicate_line->forward();
        }
    }
}

// 震荡市场测试
TEST(OriginalTests, PercentRank_OscillatingMarket) {
    // 创建震荡数据
    std::vector<double> oscillating_prices;
    for (int i = 0; i < 100; ++i) {
        double base = 100.0;
        double oscillation = 10.0 * std::sin(i * 0.2);
        oscillating_prices.push_back(base + oscillation);
    }
    
    auto oscillating_line = std::make_shared<backtrader::LineRoot>(oscillating_prices.size(), "oscillating");
    for (double price : oscillating_prices) {
        oscillating_line->forward(price);
    }
    
    auto oscillating_rank = std::make_shared<PercentRank>(oscillating_line, 20);
    
    std::vector<double> rank_values;
    
    for (size_t i = 0; i < oscillating_prices.size(); ++i) {
        oscillating_rank->calculate();
        
        double rank_value = oscillating_rank->get(0);
        if (!std::isnan(rank_value)) {
            rank_values.push_back(rank_value);
        }
        
        if (i < oscillating_prices.size() - 1) {
            oscillating_line->forward();
        }
    }
    
    // 震荡市场中，rank值应该在0和1之间波动
    if (!rank_values.empty()) {
        double avg_rank = std::accumulate(rank_values.begin(), rank_values.end(), 0.0) / rank_values.size();
        EXPECT_GT(avg_rank, 0.0) << "Average rank should be > 0 in oscillating market";
        EXPECT_LT(avg_rank, 1.0) << "Average rank should be < 1 in oscillating market";
        
        std::cout << "Oscillating market average PercentRank: " << avg_rank << std::endl;
    }
}

// 极值测试
TEST(OriginalTests, PercentRank_ExtremeValues) {
    // 创建包含极值的数据
    std::vector<double> extreme_prices = {100.0, 101.0, 102.0, 1000.0, 103.0, 104.0, 105.0, 10.0, 106.0, 107.0};
    
    auto extreme_line = std::make_shared<backtrader::LineRoot>(extreme_prices.size(), "extreme");
    for (double price : extreme_prices) {
        extreme_line->forward(price);
    }
    
    auto extreme_rank = std::make_shared<PercentRank>(extreme_line, 5);
    
    for (size_t i = 0; i < extreme_prices.size(); ++i) {
        extreme_rank->calculate();
        
        double rank_value = extreme_rank->get(0);
        
        if (!std::isnan(rank_value)) {
            // 当当前值是极大值时（如1000.0），rank应该接近1.0
            if (i >= 4 && extreme_prices[i] == 1000.0) {
                EXPECT_NEAR(rank_value, 1.0, 1e-10) 
                    << "PercentRank should be 1.0 for maximum value";
            }
            // 当当前值是极小值时（如10.0），rank应该接近0.0
            else if (i >= 4 && extreme_prices[i] == 10.0) {
                EXPECT_NEAR(rank_value, 0.0, 1e-10) 
                    << "PercentRank should be 0.0 for minimum value";
            }
        }
        
        if (i < extreme_prices.size() - 1) {
            extreme_line->forward();
        }
    }
}

// 边界条件测试
TEST(OriginalTests, PercentRank_EdgeCases) {
    // 测试所有价格相同的情况
    std::vector<double> flat_prices(50, 100.0);
    
    auto flat_line = std::make_shared<backtrader::LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_rank = std::make_shared<PercentRank>(flat_line, 20);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_rank->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，rank的具体值取决于实现
    // 但应该在有效范围内
    double final_rank = flat_rank->get(0);
    if (!std::isnan(final_rank)) {
        EXPECT_GE(final_rank, 0.0) << "PercentRank should be >= 0 for constant prices";
        EXPECT_LE(final_rank, 1.0) << "PercentRank should be <= 1 for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<backtrader::LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 5; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_rank = std::make_shared<PercentRank>(insufficient_line, 10);
    
    for (int i = 0; i < 5; ++i) {
        insufficient_rank->calculate();
        if (i < 4) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_rank->get(0);
    EXPECT_TRUE(std::isnan(result)) << "PercentRank should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, PercentRank_Performance) {
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
    
    auto large_rank = std::make_shared<PercentRank>(large_line, 100);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_rank->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "PercentRank calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_rank->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_GE(final_result, 0.0) << "Final result should be >= 0";
    EXPECT_LE(final_result, 1.0) << "Final result should be <= 1";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}