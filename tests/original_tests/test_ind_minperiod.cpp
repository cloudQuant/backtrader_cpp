/**
 * @file test_ind_minperiod.cpp
 * @brief MinPeriod指标测试 - 对应Python test_ind_minperiod.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = []
 * chkmin = 34  # from MACD
 * chkind = [btind.SMA, btind.Stochastic, btind.MACD, btind.Highest]
 * chkargs = dict()
 * 
 * 注：这个测试验证多个指标组合时的最小周期计算
 */

#include "test_common.h"
#include <random>

#include "indicators/sma.h"
#include "indicators/stochastic.h"
#include "indicators/macd.h"
#include "indicators/highest.h"
#include "indicators/rsi.h"

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

// 多指标最小周期测试
TEST(OriginalTests, MinPeriod_MultipleIndicators) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    // 创建多个指标（使用默认参数）
    auto sma = std::make_shared<SMA>(close_line);          // 默认30周期
    auto stochastic = std::make_shared<Stochastic>(high_line, low_line, close_line);  // 默认参数
    auto macd = std::make_shared<MACD>(close_line);        // 默认参数
    auto highest = std::make_shared<Highest>(high_line);   // 默认30周期
    
    // 验证各个指标的最小周期
    int sma_minperiod = sma->getMinPeriod();
    int stochastic_minperiod = stochastic->getMinPeriod();
    int macd_minperiod = macd->getMinPeriod();
    int highest_minperiod = highest->getMinPeriod();
    
    std::cout << "Individual indicator minimum periods:" << std::endl;
    std::cout << "SMA: " << sma_minperiod << std::endl;
    std::cout << "Stochastic: " << stochastic_minperiod << std::endl;
    std::cout << "MACD: " << macd_minperiod << std::endl;
    std::cout << "Highest: " << highest_minperiod << std::endl;
    
    // 验证SMA最小周期
    EXPECT_EQ(sma_minperiod, 30) << "SMA minimum period should be 30";
    
    // 验证Stochastic最小周期
    EXPECT_GE(stochastic_minperiod, 14) << "Stochastic minimum period should be at least 14";
    
    // 验证MACD最小周期（根据原始测试，MACD的最小周期是34）
    EXPECT_EQ(macd_minperiod, 34) << "MACD minimum period should be 34";
    
    // 验证Highest最小周期
    EXPECT_EQ(highest_minperiod, 30) << "Highest minimum period should be 30";
    
    // 计算组合的最小周期（应该是所有指标中的最大值）
    int combined_minperiod = std::max({sma_minperiod, stochastic_minperiod, macd_minperiod, highest_minperiod});
    std::cout << "Combined minimum period: " << combined_minperiod << std::endl;
    
    // 根据原始测试，组合的最小周期应该是34（来自MACD）
    EXPECT_EQ(combined_minperiod, 34) << "Combined minimum period should be 34 (from MACD)";
    
    // 计算所有值，直到达到最小周期
    for (size_t i = 0; i < csv_data.size(); ++i) {
        sma->calculate();
        stochastic->calculate();
        macd->calculate();
        highest->calculate();
        
        // 在达到最小周期之前，所有指标都应该返回NaN
        if (static_cast<int>(i) < combined_minperiod - 1) {
            if (static_cast<int>(i) < sma_minperiod - 1) {
                EXPECT_TRUE(std::isnan(sma->get(0))) << "SMA should return NaN before minimum period at step " << i;
            }
            if (static_cast<int>(i) < stochastic_minperiod - 1) {
                EXPECT_TRUE(std::isnan(stochastic->getLine(0)->get(0))) << "Stochastic %K should return NaN before minimum period at step " << i;
            }
            if (static_cast<int>(i) < macd_minperiod - 1) {
                EXPECT_TRUE(std::isnan(macd->getLine(0)->get(0))) << "MACD should return NaN before minimum period at step " << i;
            }
            if (static_cast<int>(i) < highest_minperiod - 1) {
                EXPECT_TRUE(std::isnan(highest->get(0))) << "Highest should return NaN before minimum period at step " << i;
            }
        }
        
        // 在达到各自最小周期后，指标应该返回有效值
        if (static_cast<int>(i) >= sma_minperiod - 1) {
            EXPECT_FALSE(std::isnan(sma->get(0))) << "SMA should return valid value after minimum period at step " << i;
        }
        if (static_cast<int>(i) >= stochastic_minperiod - 1) {
            EXPECT_FALSE(std::isnan(stochastic->getLine(0)->get(0))) << "Stochastic %K should return valid value after minimum period at step " << i;
        }
        if (static_cast<int>(i) >= macd_minperiod - 1) {
            EXPECT_FALSE(std::isnan(macd->getLine(0)->get(0))) << "MACD should return valid value after minimum period at step " << i;
        }
        if (static_cast<int>(i) >= highest_minperiod - 1) {
            EXPECT_FALSE(std::isnan(highest->get(0))) << "Highest should return valid value after minimum period at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
            high_line->forward();
            low_line->forward();
        }
    }
}

// 测试不同参数下的最小周期
TEST(OriginalTests, MinPeriod_DifferentParameters) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    // 测试不同周期的SMA
    std::vector<int> sma_periods = {10, 20, 50, 100};
    for (int period : sma_periods) {
        auto sma = std::make_shared<SMA>(close_line, period);
        EXPECT_EQ(sma->getMinPeriod(), period) << "SMA minimum period should equal period parameter";
    }
    
    // 测试不同参数的Stochastic
    std::vector<std::tuple<int, int, int>> stoch_params = {
        {14, 1, 3},   // 默认参数
        {10, 1, 3},   // 更短周期
        {20, 3, 5}    // 更长周期
    };
    for (auto [k_period, k_slowing, d_period] : stoch_params) {
        auto stochastic = std::make_shared<Stochastic>(high_line, low_line, close_line, k_period, k_slowing, d_period);
        int expected_minperiod = k_period + k_slowing + d_period - 2;
        EXPECT_EQ(stochastic->getMinPeriod(), expected_minperiod) 
            << "Stochastic minimum period should be " << expected_minperiod 
            << " for parameters (" << k_period << ", " << k_slowing << ", " << d_period << ")";
    }
    
    // 测试不同参数的MACD
    std::vector<std::tuple<int, int, int>> macd_params = {
        {12, 26, 9},  // 默认参数
        {8, 17, 9},   // 更快参数
        {19, 39, 9}   // 更慢参数
    };
    for (auto [fast, slow, signal] : macd_params) {
        auto macd = std::make_shared<MACD>(close_line, fast, slow, signal);
        int expected_minperiod = slow + signal - 1;
        EXPECT_EQ(macd->getMinPeriod(), expected_minperiod) 
            << "MACD minimum period should be " << expected_minperiod 
            << " for parameters (" << fast << ", " << slow << ", " << signal << ")";
    }
    
    // 测试不同周期的Highest
    std::vector<int> highest_periods = {10, 20, 30, 50};
    for (int period : highest_periods) {
        auto highest = std::make_shared<Highest>(high_line, period);
        EXPECT_EQ(highest->getMinPeriod(), period) << "Highest minimum period should equal period parameter";
    }
}

// 测试嵌套指标的最小周期计算
TEST(OriginalTests, MinPeriod_NestedIndicators) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建基础SMA
    auto base_sma = std::make_shared<SMA>(close_line, 20);
    
    // 创建基于SMA的另一个SMA（嵌套）
    auto nested_sma = SMA::fromIndicator(base_sma, 10);
    
    // 嵌套指标的最小周期应该是基础指标的最小周期 + 自身周期 - 1
    int expected_nested_minperiod = base_sma->getMinPeriod() + 10 - 1;
    EXPECT_EQ(nested_sma->getMinPeriod(), expected_nested_minperiod) 
        << "Nested SMA minimum period should be " << expected_nested_minperiod;
    
    std::cout << "Base SMA minimum period: " << base_sma->getMinPeriod() << std::endl;
    std::cout << "Nested SMA minimum period: " << nested_sma->getMinPeriod() << std::endl;
    
    // 测试嵌套指标的计算
    for (size_t i = 0; i < csv_data.size(); ++i) {
        base_sma->calculate();
        nested_sma->calculate();
        
        // 在达到最小周期之前应该返回NaN
        if (static_cast<int>(i) < nested_sma->getMinPeriod() - 1) {
            EXPECT_TRUE(std::isnan(nested_sma->get(0))) 
                << "Nested SMA should return NaN before minimum period at step " << i;
        } else {
            EXPECT_FALSE(std::isnan(nested_sma->get(0))) 
                << "Nested SMA should return valid value after minimum period at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
}

// 测试复杂组合指标的最小周期
TEST(OriginalTests, MinPeriod_ComplexCombination) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
        high_line->forward(bar.high);
        low_line->forward(bar.low);
    }
    
    // 创建复杂的指标组合
    auto sma_short = std::make_shared<SMA>(close_line, 10);
    auto sma_long = std::make_shared<SMA>(close_line, 30);
    auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
    auto stochastic = std::make_shared<Stochastic>(high_line, low_line, close_line, 14, 1, 3);
    auto rsi = std::make_shared<RSI>(close_line, 14);
    
    // 收集所有指标的最小周期
    std::vector<std::shared_ptr<IndicatorBase>> indicators = {sma_short, sma_long, macd, stochastic, rsi};
    std::vector<int> min_periods;
    
    for (auto& indicator : indicators) {
        min_periods.push_back(indicator->getMinPeriod());
    }
    
    // 计算组合的最小周期
    int combined_minperiod = *std::max_element(min_periods.begin(), min_periods.end());
    
    std::cout << "Complex combination minimum periods:" << std::endl;
    std::cout << "SMA(10): " << sma_short->getMinPeriod() << std::endl;
    std::cout << "SMA(30): " << sma_long->getMinPeriod() << std::endl;
    std::cout << "MACD: " << macd->getMinPeriod() << std::endl;
    std::cout << "Stochastic: " << stochastic->getMinPeriod() << std::endl;
    std::cout << "RSI: " << rsi->getMinPeriod() << std::endl;
    std::cout << "Combined: " << combined_minperiod << std::endl;
    
    // 验证组合最小周期是正确的
    EXPECT_GE(combined_minperiod, 30) << "Combined minimum period should be at least 30";
    
    // 测试计算
    int valid_count = 0;
    for (size_t i = 0; i < csv_data.size(); ++i) {
        for (auto& indicator : indicators) {
            indicator->calculate();
        }
        
        // 检查在达到组合最小周期后所有指标都有效
        if (static_cast<int>(i) >= combined_minperiod - 1) {
            bool all_valid = true;
            
            if (static_cast<int>(i) >= sma_short->getMinPeriod() - 1) {
                all_valid &= !std::isnan(sma_short->get(0));
            }
            if (static_cast<int>(i) >= sma_long->getMinPeriod() - 1) {
                all_valid &= !std::isnan(sma_long->get(0));
            }
            if (static_cast<int>(i) >= macd->getMinPeriod() - 1) {
                all_valid &= !std::isnan(macd->getLine(0)->get(0));
            }
            if (static_cast<int>(i) >= stochastic->getMinPeriod() - 1) {
                all_valid &= !std::isnan(stochastic->getLine(0)->get(0));
            }
            if (static_cast<int>(i) >= rsi->getMinPeriod() - 1) {
                all_valid &= !std::isnan(rsi->get(0));
            }
            
            if (all_valid) {
                valid_count++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
            high_line->forward();
            low_line->forward();
        }
    }
    
    std::cout << "Valid combinations after minimum period: " << valid_count << std::endl;
    EXPECT_GT(valid_count, 0) << "Should have some valid combinations after minimum period";
}

// 测试最小周期边界条件
TEST(OriginalTests, MinPeriod_EdgeCases) {
    // 测试单数据点的情况
    std::vector<double> single_price = {100.0};
    
    auto single_line = std::make_shared<backtrader::LineRoot>(single_price.size(), "single");
    single_line->forward(100.0);
    
    auto sma_single = std::make_shared<SMA>(single_line, 1);
    EXPECT_EQ(sma_single->getMinPeriod(), 1) << "SMA(1) minimum period should be 1";
    
    sma_single->calculate();
    EXPECT_FALSE(std::isnan(sma_single->get(0))) << "SMA(1) should return valid value with single data point";
    EXPECT_NEAR(sma_single->get(0), 100.0, 1e-10) << "SMA(1) should equal the single input value";
    
    // 测试周期为0的情况（如果支持）
    try {
        auto sma_zero = std::make_shared<SMA>(single_line, 0);
        // 如果没有抛出异常，检查最小周期
        EXPECT_GE(sma_zero->getMinPeriod(), 1) << "Minimum period should be at least 1";
    } catch (const std::exception& e) {
        // 预期可能会抛出异常
        std::cout << "Expected exception for zero period: " << e.what() << std::endl;
    }
    
    // 测试负周期的情况（如果支持）
    try {
        auto sma_negative = std::make_shared<SMA>(single_line, -1);
        // 如果没有抛出异常，检查最小周期
        EXPECT_GE(sma_negative->getMinPeriod(), 1) << "Minimum period should be at least 1";
    } catch (const std::exception& e) {
        // 预期可能会抛出异常
        std::cout << "Expected exception for negative period: " << e.what() << std::endl;
    }
}

// 测试动态最小周期更新
TEST(OriginalTests, MinPeriod_DynamicUpdate) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建指标并检查最小周期在计算过程中是否保持一致
    auto sma = std::make_shared<SMA>(close_line, 20);
    int initial_minperiod = sma->getMinPeriod();
    
    for (size_t i = 0; i < std::min(csv_data.size(), size_t(50)); ++i) {
        sma->calculate();
        
        // 最小周期在计算过程中应该保持不变
        EXPECT_EQ(sma->getMinPeriod(), initial_minperiod) 
            << "Minimum period should remain constant during calculation at step " << i;
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
}

// 性能测试：大量指标的最小周期计算
TEST(OriginalTests, MinPeriod_Performance) {
    const size_t data_size = 1000;
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
    
    // 创建大量不同的指标
    std::vector<std::shared_ptr<IndicatorBase>> many_indicators;
    
    for (int period = 5; period <= 50; period += 5) {
        many_indicators.push_back(std::make_shared<SMA>(large_line, period));
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 计算所有指标
    for (size_t i = 0; i < large_data.size(); ++i) {
        for (auto& indicator : many_indicators) {
            indicator->calculate();
        }
        
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Multiple indicators calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证所有指标的最终值都是有效的
    for (size_t i = 0; i < many_indicators.size(); ++i) {
        double final_value = many_indicators[i]->get(0);
        EXPECT_FALSE(std::isnan(final_value)) << "Indicator " << i << " should have valid final value";
        EXPECT_TRUE(std::isfinite(final_value)) << "Indicator " << i << " should have finite final value";
    }
    
    // 性能要求：应该在合理时间内完成
    EXPECT_LT(duration.count(), 2000) << "Performance test: should complete within 2 seconds";
}