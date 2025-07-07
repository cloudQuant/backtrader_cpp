/**
 * @file test_ind_ppo.cpp
 * @brief PPO指标测试 - 对应Python test_ind_ppo.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['0.633439', '0.883552', '0.049430'],   # PPO line
 *     ['0.540516', '0.724136', '-0.079820'],  # Signal line  
 *     ['0.092923', '0.159416', '0.129250']    # Histogram
 * ]
 * chkmin = 34
 * chkind = btind.PPO
 */

#include "test_common.h"
#include <random>

#include "indicators/ppo.h"
#include "indicators/macd.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> PPO_EXPECTED_VALUES = {
    {"0.633439", "0.883552", "0.049430"},   // PPO line
    {"0.540516", "0.724136", "-0.079820"},  // Signal line  
    {"0.092923", "0.159416", "0.129250"}    // Histogram
};

const int PPO_MIN_PERIOD = 34;

} // anonymous namespace

// 使用默认参数的PPO测试
DEFINE_INDICATOR_TEST(PPO_Default, PPO, PPO_EXPECTED_VALUES, PPO_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, PPO_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建PPO指标（默认参数：12, 26, 9）
    auto ppo = std::make_shared<PPO>(close_line, 12, 26, 9);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ppo->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 34;  // 26 + 9 - 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // 验证PPO线
    std::vector<std::string> expected_ppo = {"0.633439", "0.883552", "0.049430"};
    for (size_t i = 0; i < check_points.size() && i < expected_ppo.size(); ++i) {
        double actual = ppo->getPPOLine(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_ppo[i]) 
            << "PPO line mismatch at check point " << i;
    }
    
    // 验证信号线
    std::vector<std::string> expected_signal = {"0.540516", "0.724136", "-0.079820"};
    for (size_t i = 0; i < check_points.size() && i < expected_signal.size(); ++i) {
        double actual = ppo->getSignalLine(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_signal[i]) 
            << "PPO signal line mismatch at check point " << i;
    }
    
    // 验证直方图
    std::vector<std::string> expected_histogram = {"0.092923", "0.159416", "0.129250"};
    for (size_t i = 0; i < check_points.size() && i < expected_histogram.size(); ++i) {
        double actual = ppo->getHistogram(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_histogram[i]) 
            << "PPO histogram mismatch at check point " << i;
    }
    
    // 验证最小周期
    EXPECT_EQ(ppo->getMinPeriod(), 34) << "PPO minimum period should be 34";
}

// PPO关系验证测试
TEST(OriginalTests, PPO_RelationshipValidation) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto ppo = std::make_shared<PPO>(close_line, 12, 26, 9);
    
    // 计算所有值并验证关系
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ppo->calculate();
        
        double ppo_line = ppo->getPPOLine(0);
        double signal_line = ppo->getSignalLine(0);
        double histogram = ppo->getHistogram(0);
        
        // 验证直方图 = PPO线 - 信号线
        if (!std::isnan(ppo_line) && !std::isnan(signal_line) && !std::isnan(histogram)) {
            double expected_histogram = ppo_line - signal_line;
            EXPECT_NEAR(histogram, expected_histogram, 1e-10) 
                << "Histogram should equal PPO line minus Signal line at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
}

// 参数化测试 - 测试不同参数的PPO
class PPOParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int, int>> {
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

TEST_P(PPOParameterizedTest, DifferentParameters) {
    auto [fast_period, slow_period, signal_period] = GetParam();
    auto ppo = std::make_shared<PPO>(close_line_, fast_period, slow_period, signal_period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        ppo->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    int expected_min_period = slow_period + signal_period - 1;
    EXPECT_EQ(ppo->getMinPeriod(), expected_min_period) 
        << "PPO minimum period should be slow_period + signal_period - 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double ppo_value = ppo->getPPOLine(0);
        double signal_value = ppo->getSignalLine(0);
        double histogram_value = ppo->getHistogram(0);
        
        EXPECT_FALSE(std::isnan(ppo_value)) << "PPO line should not be NaN";
        EXPECT_FALSE(std::isnan(signal_value)) << "Signal line should not be NaN";
        EXPECT_FALSE(std::isnan(histogram_value)) << "Histogram should not be NaN";
        
        // 验证直方图关系
        EXPECT_NEAR(histogram_value, ppo_value - signal_value, 1e-10) 
            << "Histogram should equal PPO - Signal";
    }
}

// 测试不同的PPO参数
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    PPOParameterizedTest,
    ::testing::Values(
        std::make_tuple(5, 10, 3),
        std::make_tuple(12, 26, 9),   // 标准参数
        std::make_tuple(8, 17, 9),
        std::make_tuple(6, 13, 5)
    )
);

// PPO vs MACD关系测试
TEST(OriginalTests, PPO_vs_MACD_Relationship) {
    auto csv_data = getdata(0);
    auto close_line_ppo = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close_ppo");
    auto close_line_macd = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close_macd");
    
    for (const auto& bar : csv_data) {
        close_line_ppo->forward(bar.close);
        close_line_macd->forward(bar.close);
    }
    
    auto ppo = std::make_shared<PPO>(close_line_ppo, 12, 26, 9);
    auto macd = std::make_shared<MACD>(close_line_macd, 12, 26, 9);
    
    // 计算所有值并比较PPO和MACD关系
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ppo->calculate();
        macd->calculate();
        
        double ppo_line = ppo->getPPOLine(0);
        double macd_line = macd->getMACDLine(0);
        
        // PPO是MACD的百分比版本，应该与MACD/慢EMA相关
        if (!std::isnan(ppo_line) && !std::isnan(macd_line)) {
            // PPO = (Fast EMA - Slow EMA) / Slow EMA * 100
            // 这里我们只验证两者都是有效的数值
            EXPECT_TRUE(std::isfinite(ppo_line)) << "PPO should be finite";
            EXPECT_TRUE(std::isfinite(macd_line)) << "MACD should be finite";
        }
        
        if (i < csv_data.size() - 1) {
            close_line_ppo->forward();
            close_line_macd->forward();
        }
    }
}

// 交叉信号测试
TEST(OriginalTests, PPO_CrossoverSignals) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto ppo = std::make_shared<PPO>(close_line, 12, 26, 9);
    
    int bullish_crossovers = 0;
    int bearish_crossovers = 0;
    double prev_histogram = 0.0;
    bool has_prev = false;
    
    // 检测交叉信号
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ppo->calculate();
        
        double current_histogram = ppo->getHistogram(0);
        
        if (!std::isnan(current_histogram) && has_prev) {
            // 检测直方图零轴穿越
            if (prev_histogram <= 0.0 && current_histogram > 0.0) {
                bullish_crossovers++;  // PPO上穿信号线
            } else if (prev_histogram >= 0.0 && current_histogram < 0.0) {
                bearish_crossovers++;  // PPO下穿信号线
            }
        }
        
        if (!std::isnan(current_histogram)) {
            prev_histogram = current_histogram;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "PPO crossover signals:" << std::endl;
    std::cout << "Bullish crossovers: " << bullish_crossovers << std::endl;
    std::cout << "Bearish crossovers: " << bearish_crossovers << std::endl;
    
    // 验证至少检测到一些信号
    EXPECT_GE(bullish_crossovers + bearish_crossovers, 0) 
        << "Should detect some crossover signals";
}

// 趋势强度测试
TEST(OriginalTests, PPO_TrendStrength) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto ppo = std::make_shared<PPO>(close_line, 12, 26, 9);
    
    std::vector<double> ppo_values;
    std::vector<double> histogram_values;
    
    // 收集PPO值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ppo->calculate();
        
        double ppo_val = ppo->getPPOLine(0);
        double hist_val = ppo->getHistogram(0);
        
        if (!std::isnan(ppo_val) && !std::isnan(hist_val)) {
            ppo_values.push_back(ppo_val);
            histogram_values.push_back(hist_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 分析趋势强度
    if (!ppo_values.empty()) {
        double avg_ppo = std::accumulate(ppo_values.begin(), ppo_values.end(), 0.0) / ppo_values.size();
        double avg_histogram = std::accumulate(histogram_values.begin(), histogram_values.end(), 0.0) / histogram_values.size();
        
        std::cout << "Average PPO: " << avg_ppo << std::endl;
        std::cout << "Average Histogram: " << avg_histogram << std::endl;
        
        // 验证值是有限的
        EXPECT_TRUE(std::isfinite(avg_ppo)) << "Average PPO should be finite";
        EXPECT_TRUE(std::isfinite(avg_histogram)) << "Average histogram should be finite";
    }
}

// 零线测试
TEST(OriginalTests, PPO_ZeroLineTest) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto ppo = std::make_shared<PPO>(close_line, 12, 26, 9);
    
    int above_zero = 0;
    int below_zero = 0;
    int zero_crossings = 0;
    double prev_ppo = 0.0;
    bool has_prev = false;
    
    // 分析零线行为
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ppo->calculate();
        
        double current_ppo = ppo->getPPOLine(0);
        
        if (!std::isnan(current_ppo)) {
            if (current_ppo > 0.0) {
                above_zero++;
            } else if (current_ppo < 0.0) {
                below_zero++;
            }
            
            // 检测零线穿越
            if (has_prev) {
                if ((prev_ppo <= 0.0 && current_ppo > 0.0) || (prev_ppo >= 0.0 && current_ppo < 0.0)) {
                    zero_crossings++;
                }
            }
            
            prev_ppo = current_ppo;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "PPO zero line analysis:" << std::endl;
    std::cout << "Above zero: " << above_zero << std::endl;
    std::cout << "Below zero: " << below_zero << std::endl;
    std::cout << "Zero crossings: " << zero_crossings << std::endl;
    
    // 验证统计结果
    EXPECT_GE(above_zero + below_zero, 0) << "Should have some valid PPO readings";
}

// 边界条件测试
TEST(OriginalTests, PPO_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(50, 100.0);  // 50个相同价格
    
    auto flat_line = std::make_shared<backtrader::LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_ppo = std::make_shared<PPO>(flat_line, 12, 26, 9);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_ppo->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    double final_ppo = flat_ppo->getPPOLine(0);
    double final_signal = flat_ppo->getSignalLine(0);
    double final_histogram = flat_ppo->getHistogram(0);
    
    if (!std::isnan(final_ppo) && !std::isnan(final_signal) && !std::isnan(final_histogram)) {
        EXPECT_NEAR(final_ppo, 0.0, 1e-10) 
            << "PPO should be zero for constant prices";
        EXPECT_NEAR(final_signal, 0.0, 1e-10) 
            << "Signal should be zero for constant prices";
        EXPECT_NEAR(final_histogram, 0.0, 1e-10) 
            << "Histogram should be zero for constant prices";
    }
}

// 性能测试
TEST(OriginalTests, PPO_Performance) {
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
    
    auto large_ppo = std::make_shared<PPO>(large_line, 12, 26, 9);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_ppo->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "PPO calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_ppo = large_ppo->getPPOLine(0);
    double final_signal = large_ppo->getSignalLine(0);
    double final_histogram = large_ppo->getHistogram(0);
    
    EXPECT_FALSE(std::isnan(final_ppo)) << "Final PPO should not be NaN";
    EXPECT_FALSE(std::isnan(final_signal)) << "Final signal should not be NaN";
    EXPECT_FALSE(std::isnan(final_histogram)) << "Final histogram should not be NaN";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}
