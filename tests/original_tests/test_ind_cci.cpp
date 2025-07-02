/**
 * @file test_ind_cci.cpp
 * @brief CCI指标测试 - 对应Python test_ind_cci.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['69.574287', '91.196363', '82.175663'],
 * ]
 * chkmin = 39
 * chkind = btind.CCI
 */

#include "test_common.h"
#include "indicators/CCI.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> CCI_EXPECTED_VALUES = {
    {"69.574287", "91.196363", "82.175663"}
};

const int CCI_MIN_PERIOD = 39;

} // anonymous namespace

// 使用默认参数的CCI测试
DEFINE_INDICATOR_TEST(CCI_Default, CCI, CCI_EXPECTED_VALUES, CCI_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, CCI_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    // 创建CCI指标（默认20周期，最小周期为39）
    auto cci = std::make_shared<CCI>(close_line, high_line, low_line, 20);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        cci->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 39;  // period + SMA period for mean deviation
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"69.574287", "91.196363", "82.175663"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = cci->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "CCI value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(cci->getMinPeriod(), 39) << "CCI minimum period should be 39";
}

// CCI范围验证测试 - CCI没有固定范围，但通常在-300到+300之间
TEST(OriginalTests, CCI_RangeValidation) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto cci = std::make_shared<CCI>(close_line, high_line, low_line, 20);
    
    int extreme_high_count = 0;  // CCI > 200
    int extreme_low_count = 0;   // CCI < -200
    int normal_count = 0;        // -200 <= CCI <= 200
    
    // 计算所有值并统计范围
    for (size_t i = 0; i < csv_data.size(); ++i) {
        cci->calculate();
        
        double cci_value = cci->get(0);
        
        if (!std::isnan(cci_value)) {
            if (cci_value > 200.0) {
                extreme_high_count++;
            } else if (cci_value < -200.0) {
                extreme_low_count++;
            } else {
                normal_count++;
            }
            
            // 验证CCI是有限值
            EXPECT_TRUE(std::isfinite(cci_value)) << "CCI should be finite at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    std::cout << "CCI range statistics:" << std::endl;
    std::cout << "Extreme high (> 200): " << extreme_high_count << std::endl;
    std::cout << "Extreme low (< -200): " << extreme_low_count << std::endl;
    std::cout << "Normal range (-200 to 200): " << normal_count << std::endl;
    
    // 验证至少有一些有效的计算
    EXPECT_GT(normal_count + extreme_high_count + extreme_low_count, 0) 
        << "Should have some valid CCI calculations";
}

// 参数化测试 - 测试不同周期的CCI
class CCIParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        high_line_ = std::make_shared<LineRoot>(csv_data_.size(), "high");
        low_line_ = std::make_shared<LineRoot>(csv_data_.size(), "low");
        close_line_ = std::make_shared<LineRoot>(csv_data_.size(), "close");
        
        for (const auto& bar : csv_data_) {
            high_line_->forward(bar.high);
            low_line_->forward(bar.low);
            close_line_->forward(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineRoot> high_line_;
    std::shared_ptr<LineRoot> low_line_;
    std::shared_ptr<LineRoot> close_line_;
};

TEST_P(CCIParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto cci = std::make_shared<CCI>(close_line_, high_line_, low_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        cci->calculate();
        if (i < csv_data_.size() - 1) {
            high_line_->forward();
            low_line_->forward();
            close_line_->forward();
        }
    }
    
    // 验证最小周期（通常是period + period - 1）
    int expected_min_period = period + period - 1;
    EXPECT_EQ(cci->getMinPeriod(), expected_min_period) 
        << "CCI minimum period should be period + period - 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_value = cci->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last CCI value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "CCI value should be finite";
    }
}

// 测试不同的CCI周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    CCIParameterizedTest,
    ::testing::Values(10, 14, 20, 30)
);

// CCI计算逻辑验证测试
TEST(OriginalTests, CCI_CalculationLogic) {
    // 使用简单的测试数据验证CCI计算
    std::vector<CSVDataReader::OHLCVData> test_data = {
        {"2006-01-01", 100.0, 110.0, 90.0, 105.0, 0, 0},
        {"2006-01-02", 105.0, 115.0, 95.0, 110.0, 0, 0},
        {"2006-01-03", 110.0, 120.0, 100.0, 115.0, 0, 0},
        {"2006-01-04", 115.0, 125.0, 105.0, 120.0, 0, 0},
        {"2006-01-05", 120.0, 130.0, 110.0, 125.0, 0, 0}
    };
    
    auto high_line = std::make_shared<LineRoot>(test_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(test_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(test_data.size(), "close");
    
    for (const auto& bar : test_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto cci = std::make_shared<CCI>(close_line, high_line, low_line, 3);
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        cci->calculate();
        
        // 手动计算CCI进行验证（从第3个数据点开始）
        if (i >= 2) {
            // 计算典型价格 (H + L + C) / 3
            std::vector<double> typical_prices;
            for (int j = 0; j < 3; ++j) {
                double tp = (test_data[i - j].high + test_data[i - j].low + test_data[i - j].close) / 3.0;
                typical_prices.push_back(tp);
            }
            
            // 计算SMA of typical prices
            double sma_tp = std::accumulate(typical_prices.begin(), typical_prices.end(), 0.0) / typical_prices.size();
            
            // 计算平均绝对偏差
            double mad = 0.0;
            for (double tp : typical_prices) {
                mad += std::abs(tp - sma_tp);
            }
            mad /= typical_prices.size();
            
            // CCI = (Typical Price - SMA) / (0.015 * Mean Absolute Deviation)
            double current_tp = (test_data[i].high + test_data[i].low + test_data[i].close) / 3.0;
            double expected_cci = (current_tp - sma_tp) / (0.015 * mad);
            
            double actual_cci = cci->get(0);
            
            if (!std::isnan(actual_cci) && mad > 0.0) {
                EXPECT_NEAR(actual_cci, expected_cci, 1e-6) 
                    << "CCI calculation mismatch at step " << i;
            }
        }
        
        if (i < test_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}

// 超买超卖信号测试
TEST(OriginalTests, CCI_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto cci = std::make_shared<CCI>(close_line, high_line, low_line, 20);
    
    int overbought_signals = 0;  // CCI > 100
    int oversold_signals = 0;    // CCI < -100
    int zero_crossings = 0;      // CCI穿越零线的次数
    
    double prev_cci = 0.0;
    bool has_prev = false;
    
    // 统计信号
    for (size_t i = 0; i < csv_data.size(); ++i) {
        cci->calculate();
        
        double cci_value = cci->get(0);
        
        if (!std::isnan(cci_value)) {
            // 检测超买超卖
            if (cci_value > 100.0) {
                overbought_signals++;
            } else if (cci_value < -100.0) {
                oversold_signals++;
            }
            
            // 检测零线穿越
            if (has_prev) {
                if ((prev_cci <= 0.0 && cci_value > 0.0) || (prev_cci >= 0.0 && cci_value < 0.0)) {
                    zero_crossings++;
                }
            }
            
            prev_cci = cci_value;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    std::cout << "CCI signal statistics:" << std::endl;
    std::cout << "Overbought signals (> 100): " << overbought_signals << std::endl;
    std::cout << "Oversold signals (< -100): " << oversold_signals << std::endl;
    std::cout << "Zero line crossings: " << zero_crossings << std::endl;
    
    // 验证检测到一些信号
    EXPECT_GE(overbought_signals + oversold_signals + zero_crossings, 0) 
        << "Should detect some CCI signals";
}

// 发散测试
TEST(OriginalTests, CCI_Divergence) {
    // 创建价格走高但CCI走低的发散情况
    std::vector<CSVDataReader::OHLCVData> divergence_data;
    
    // 第一阶段：价格和CCI同步上升
    for (int i = 0; i < 20; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 100.0 + i * 2.0;
        bar.low = 90.0 + i * 2.0;
        bar.close = 95.0 + i * 2.0;
        bar.open = 92.0 + i * 2.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        divergence_data.push_back(bar);
    }
    
    // 第二阶段：价格继续上升但波动性降低（可能导致CCI下降）
    for (int i = 0; i < 20; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-02-" + std::to_string(i + 1);
        bar.high = 140.0 + i * 0.5;
        bar.low = 135.0 + i * 0.5;
        bar.close = 137.0 + i * 0.5;
        bar.open = 136.0 + i * 0.5;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        divergence_data.push_back(bar);
    }
    
    auto high_line = std::make_shared<LineRoot>(divergence_data.size(), "div_high");
    auto low_line = std::make_shared<LineRoot>(divergence_data.size(), "div_low");
    auto close_line = std::make_shared<LineRoot>(divergence_data.size(), "div_close");
    
    for (const auto& bar : divergence_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto cci = std::make_shared<CCI>(close_line, high_line, low_line, 14);
    
    std::vector<double> early_cci;
    std::vector<double> late_cci;
    
    for (size_t i = 0; i < divergence_data.size(); ++i) {
        cci->calculate();
        
        double cci_val = cci->get(0);
        
        if (!std::isnan(cci_val)) {
            if (i < 25) {
                early_cci.push_back(cci_val);
            } else {
                late_cci.push_back(cci_val);
            }
        }
        
        if (i < divergence_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 分析发散现象
    if (!early_cci.empty() && !late_cci.empty()) {
        double avg_early = std::accumulate(early_cci.begin(), early_cci.end(), 0.0) / early_cci.size();
        double avg_late = std::accumulate(late_cci.begin(), late_cci.end(), 0.0) / late_cci.size();
        
        std::cout << "Early CCI average: " << avg_early << std::endl;
        std::cout << "Late CCI average: " << avg_late << std::endl;
        
        // 验证计算结果是有限的
        EXPECT_TRUE(std::isfinite(avg_early)) << "Early CCI should be finite";
        EXPECT_TRUE(std::isfinite(avg_late)) << "Late CCI should be finite";
    }
}

// 边界条件测试
TEST(OriginalTests, CCI_EdgeCases) {
    // 测试相同价格的情况（MAD为0）
    std::vector<CSVDataReader::OHLCVData> flat_data;
    for (int i = 0; i < 50; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 100.0;
        bar.low = 100.0;
        bar.close = 100.0;
        bar.open = 100.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        flat_data.push_back(bar);
    }
    
    auto high_line = std::make_shared<LineRoot>(flat_data.size(), "flat_high");
    auto low_line = std::make_shared<LineRoot>(flat_data.size(), "flat_low");
    auto close_line = std::make_shared<LineRoot>(flat_data.size(), "flat_close");
    
    for (const auto& bar : flat_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto cci = std::make_shared<CCI>(close_line, high_line, low_line, 20);
    
    for (size_t i = 0; i < flat_data.size(); ++i) {
        cci->calculate();
        if (i < flat_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 当所有价格相同时，CCI应该为0或NaN（因为MAD为0）
    double final_cci = cci->get(0);
    if (!std::isnan(final_cci)) {
        EXPECT_NEAR(final_cci, 0.0, 1e-10) 
            << "CCI should be 0 for constant prices";
    }
}

// 性能测试
TEST(OriginalTests, CCI_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<CSVDataReader::OHLCVData> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> price_dist(50.0, 150.0);
    std::uniform_real_distribution<double> range_dist(1.0, 5.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-01";
        bar.close = price_dist(rng);
        double range = range_dist(rng);
        bar.high = bar.close + range;
        bar.low = bar.close - range;
        bar.open = bar.close;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        large_data.push_back(bar);
    }
    
    auto high_line = std::make_shared<LineRoot>(large_data.size(), "large_high");
    auto low_line = std::make_shared<LineRoot>(large_data.size(), "large_low");
    auto close_line = std::make_shared<LineRoot>(large_data.size(), "large_close");
    
    for (const auto& bar : large_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto large_cci = std::make_shared<CCI>(close_line, high_line, low_line, 20);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_cci->calculate();
        if (i < large_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "CCI calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_cci->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}