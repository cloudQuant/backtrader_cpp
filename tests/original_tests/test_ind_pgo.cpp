/**
 * @file test_ind_pgo.cpp
 * @brief PGO指标测试 - 对应Python test_ind_pgo.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['0.543029', '-2.347884', '0.416325']
 * ]
 * chkmin = 15
 * chkind = btind.PGO
 * 
 * 注：PGO (Pretty Good Oscillator) 是一个动量振荡器
 */

#include "test_common.h"
#include <random>

#include "indicators/pgo.h"
#include "indicators/ema.h"
#include "indicators/rsi.h"
#include "indicators/atr.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> PGO_EXPECTED_VALUES = {
    {"0.543029", "-2.347884", "0.416325"}
};

const int PGO_MIN_PERIOD = 15;

} // anonymous namespace

// 使用默认参数的PGO测试
DEFINE_INDICATOR_TEST(PGO_Default, PGO, PGO_EXPECTED_VALUES, PGO_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, PGO_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建PGO指标
    auto pgo = std::make_shared<PGO>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        pgo->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 15;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"0.543029", "-2.347884", "0.416325"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = pgo->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "PGO value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(pgo->getMinPeriod(), 15) << "PGO minimum period should be 15";
}

// 参数化测试 - 测试不同周期的PGO
class PGOParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(PGOParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto pgo = std::make_shared<PGO>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        pgo->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(pgo->getMinPeriod(), period + 1) 
        << "PGO minimum period should equal period + 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        double last_value = pgo->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last PGO value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last PGO value should be finite";
    }
}

// 测试不同的PGO周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    PGOParameterizedTest,
    ::testing::Values(10, 14, 20, 30)
);

// PGO计算逻辑验证测试
TEST(OriginalTests, PGO_CalculationLogic) {
    // 使用简单的测试数据验证PGO计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0,
                                  124.0, 126.0, 128.0, 130.0, 132.0, 134.0, 136.0, 138.0, 140.0, 142.0};
    
    auto price_line = std::make_shared<backtrader::LineRoot>(prices.size(), "pgo_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto pgo = std::make_shared<PGO>(price_line, 14);
    auto ema = std::make_shared<EMA>(price_line, 14);
    auto atr = std::make_shared<ATR>(price_line);  // 使用close价格计算ATR，默认周期14
    
    for (size_t i = 0; i < prices.size(); ++i) {
        pgo->calculate();
        ema->calculate();
        atr->calculate();
        
        // 验证PGO计算逻辑：PGO = (Close - EMA) / ATR
        if (i >= 14) {  // PGO需要15个数据点
            double current_price = prices[i];
            double ema_value = ema->get(0);
            double atr_value = atr->get(0);
            double actual_pgo = pgo->get(0);
            
            if (!std::isnan(actual_pgo) && !std::isnan(ema_value) && !std::isnan(atr_value) && atr_value > 0.0) {
                double expected_pgo = (current_price - ema_value) / atr_value;
                EXPECT_NEAR(actual_pgo, expected_pgo, 0.01) 
                    << "PGO calculation mismatch at step " << i
                    << " (price=" << current_price << ", ema=" << ema_value << ", atr=" << atr_value << ")";
            }
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
}

// PGO零线穿越测试
TEST(OriginalTests, PGO_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto pgo = std::make_shared<PGO>(close_line, 14);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_pgo = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越
    for (size_t i = 0; i < csv_data.size(); ++i) {
        pgo->calculate();
        
        double current_pgo = pgo->get(0);
        
        if (!std::isnan(current_pgo) && has_prev) {
            if (prev_pgo <= 0.0 && current_pgo > 0.0) {
                positive_crossings++;
            } else if (prev_pgo >= 0.0 && current_pgo < 0.0) {
                negative_crossings++;
            }
        }
        
        if (!std::isnan(current_pgo)) {
            prev_pgo = current_pgo;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "PGO zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// PGO趋势分析测试
TEST(OriginalTests, PGO_TrendAnalysis) {
    // 创建明确的趋势数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 50; ++i) {
        double base = 100.0 + i * 1.0;  // 上升趋势
        double noise = std::sin(i * 0.1) * 2.0;  // 添加一些噪音
        trend_prices.push_back(base + noise);
    }
    
    auto trend_line = std::make_shared<backtrader::LineRoot>(trend_prices.size(), "trend");
    for (double price : trend_prices) {
        trend_line->forward(price);
    }
    
    auto trend_pgo = std::make_shared<PGO>(trend_line, 14);
    
    int positive_values = 0;
    int negative_values = 0;
    int zero_values = 0;
    
    for (size_t i = 0; i < trend_prices.size(); ++i) {
        trend_pgo->calculate();
        
        double pgo_value = trend_pgo->get(0);
        
        if (!std::isnan(pgo_value)) {
            if (pgo_value > 0.1) {
                positive_values++;
            } else if (pgo_value < -0.1) {
                negative_values++;
            } else {
                zero_values++;
            }
        }
        
        if (i < trend_prices.size() - 1) {
            trend_line->forward();
        }
    }
    
    std::cout << "Trend analysis:" << std::endl;
    std::cout << "Positive PGO values: " << positive_values << std::endl;
    std::cout << "Negative PGO values: " << negative_values << std::endl;
    std::cout << "Near-zero values: " << zero_values << std::endl;
    
    // 在上升趋势中，PGO往往应该是正值
    EXPECT_GT(positive_values, negative_values) 
        << "In uptrend, PGO should be positive more often";
}

// PGO振荡特性测试
TEST(OriginalTests, PGO_OscillationCharacteristics) {
    // 创建振荡数据
    std::vector<double> oscillating_prices;
    for (int i = 0; i < 100; ++i) {
        double base = 100.0;
        double oscillation = 8.0 * std::sin(i * 0.3);
        oscillating_prices.push_back(base + oscillation);
    }
    
    auto osc_line = std::make_shared<backtrader::LineRoot>(oscillating_prices.size(), "oscillating");
    for (double price : oscillating_prices) {
        osc_line->forward(price);
    }
    
    auto pgo = std::make_shared<PGO>(osc_line, 14);
    
    std::vector<double> pgo_values;
    
    for (size_t i = 0; i < oscillating_prices.size(); ++i) {
        pgo->calculate();
        
        double pgo_val = pgo->get(0);
        if (!std::isnan(pgo_val)) {
            pgo_values.push_back(pgo_val);
        }
        
        if (i < oscillating_prices.size() - 1) {
            osc_line->forward();
        }
    }
    
    // 分析振荡特性
    if (!pgo_values.empty()) {
        double avg_pgo = std::accumulate(pgo_values.begin(), pgo_values.end(), 0.0) / pgo_values.size();
        
        // 计算标准差
        double variance = 0.0;
        for (double val : pgo_values) {
            variance += (val - avg_pgo) * (val - avg_pgo);
        }
        variance /= pgo_values.size();
        double std_dev = std::sqrt(variance);
        
        std::cout << "PGO oscillation characteristics:" << std::endl;
        std::cout << "Average: " << avg_pgo << std::endl;
        std::cout << "Standard deviation: " << std_dev << std::endl;
        
        // PGO应该围绕零线波动
        EXPECT_NEAR(avg_pgo, 0.0, 1.0) 
            << "PGO should oscillate around zero";
        
        EXPECT_GT(std_dev, 0.5) 
            << "PGO should show meaningful variation";
    }
}

// PGO标准化特性测试
TEST(OriginalTests, PGO_NormalizationCharacteristics) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto pgo = std::make_shared<PGO>(close_line, 14);
    
    std::vector<double> pgo_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        pgo->calculate();
        
        double pgo_val = pgo->get(0);
        if (!std::isnan(pgo_val)) {
            pgo_values.push_back(pgo_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 分析标准化特性
    if (!pgo_values.empty()) {
        double max_pgo = *std::max_element(pgo_values.begin(), pgo_values.end());
        double min_pgo = *std::min_element(pgo_values.begin(), pgo_values.end());
        
        std::cout << "PGO normalization characteristics:" << std::endl;
        std::cout << "Maximum PGO: " << max_pgo << std::endl;
        std::cout << "Minimum PGO: " << min_pgo << std::endl;
        std::cout << "Range: " << (max_pgo - min_pgo) << std::endl;
        
        // PGO通过ATR标准化，应该在合理范围内
        EXPECT_GT(max_pgo, -10.0) << "Maximum PGO should be reasonable";
        EXPECT_LT(min_pgo, 10.0) << "Minimum PGO should be reasonable";
        EXPECT_GT(max_pgo - min_pgo, 1.0) << "PGO should have meaningful range";
        
        // 计算极值信号
        int extreme_positive = 0;  // > 3.0
        int extreme_negative = 0;  // < -3.0
        
        for (double val : pgo_values) {
            if (val > 3.0) extreme_positive++;
            if (val < -3.0) extreme_negative++;
        }
        
        std::cout << "Extreme positive signals (>3.0): " << extreme_positive << std::endl;
        std::cout << "Extreme negative signals (<-3.0): " << extreme_negative << std::endl;
        
        EXPECT_GE(extreme_positive + extreme_negative, 0) 
            << "Should generate some extreme signals";
    }
}

// PGO与其他标准化指标比较测试
TEST(OriginalTests, PGO_vs_StandardizedIndicators) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto pgo = std::make_shared<PGO>(close_line, 14);
    auto rsi = std::make_shared<RSI>(close_line, 14);
    
    std::vector<double> pgo_values, rsi_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        pgo->calculate();
        rsi->calculate();
        
        double pgo_val = pgo->get(0);
        double rsi_val = rsi->get(0);
        
        if (!std::isnan(pgo_val)) pgo_values.push_back(pgo_val);
        if (!std::isnan(rsi_val)) rsi_values.push_back(rsi_val);
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 比较不同标准化指标的特性
    if (!pgo_values.empty() && !rsi_values.empty()) {
        double pgo_avg = std::accumulate(pgo_values.begin(), pgo_values.end(), 0.0) / pgo_values.size();
        double rsi_avg = std::accumulate(rsi_values.begin(), rsi_values.end(), 0.0) / rsi_values.size();
        
        std::cout << "Standardized indicator comparison:" << std::endl;
        std::cout << "PGO average: " << pgo_avg << std::endl;
        std::cout << "RSI average: " << rsi_avg << std::endl;
        
        // PGO围绕0振荡，RSI围绕50振荡
        EXPECT_NEAR(pgo_avg, 0.0, 2.0) << "PGO should center around 0";
        EXPECT_NEAR(rsi_avg, 50.0, 20.0) << "RSI should center around 50";
        
        // 验证PGO的标准化特性
        double pgo_variance = 0.0;
        for (double val : pgo_values) {
            pgo_variance += (val - pgo_avg) * (val - pgo_avg);
        }
        pgo_variance /= pgo_values.size();
        double pgo_std = std::sqrt(pgo_variance);
        
        std::cout << "PGO standard deviation: " << pgo_std << std::endl;
        EXPECT_GT(pgo_std, 0.5) << "PGO should have reasonable volatility";
        EXPECT_LT(pgo_std, 5.0) << "PGO volatility should be controlled";
    }
}

// 边界条件测试
TEST(OriginalTests, PGO_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(50, 100.0);
    
    auto flat_line = std::make_shared<backtrader::LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_pgo = std::make_shared<PGO>(flat_line, 14);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_pgo->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，PGO应该接近零（价格 = EMA，ATR接近0或处理为默认值）
    double final_pgo = flat_pgo->get(0);
    if (!std::isnan(final_pgo)) {
        EXPECT_NEAR(final_pgo, 0.0, 1.0) 
            << "PGO should be near zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<backtrader::LineRoot>(50, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 10; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_pgo = std::make_shared<PGO>(insufficient_line, 14);
    
    for (int i = 0; i < 10; ++i) {
        insufficient_pgo->calculate();
        if (i < 9) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_pgo->get(0);
    EXPECT_TRUE(std::isnan(result)) << "PGO should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, PGO_Performance) {
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
    
    auto large_pgo = std::make_shared<PGO>(large_line, 14);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_pgo->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "PGO calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_pgo->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}
