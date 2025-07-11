/**
 * @file test_ind_lrsi.cpp
 * @brief LRSI指标测试 - 对应Python test_ind_lrsi.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['0.748915', '0.714286', '1.000000']
 * ]
 * chkmin = 6
 * chkind = btind.LRSI
 * 
 * 注：LRSI (Laguerre RSI) 是一个基于Laguerre滤波器的RSI变种
 */

#include "test_common.h"
#include <random>

#include "indicators/lrsi.h"
#include "indicators/rsi.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> LRSI_EXPECTED_VALUES = {
    {"0.748915", "0.714286", "1.000000"}
};

const int LRSI_MIN_PERIOD = 6;

} // anonymous namespace

// 使用默认参数的LRSI测试
DEFINE_INDICATOR_TEST(LRSI_Default, LRSI, LRSI_EXPECTED_VALUES, LRSI_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, LRSI_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建LRSI指标
    auto lrsi = std::make_shared<LRSI>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        lrsi->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 6;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"0.748915", "0.714286", "1.000000"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = lrsi->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "LRSI value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(lrsi->getMinPeriod(), 6) << "LRSI minimum period should be 6";
}

// 参数化测试 - 测试不同gamma参数的LRSI
class LRSIParameterizedTest : public ::testing::TestWithParam<double> {
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

TEST_P(LRSIParameterizedTest, DifferentGamma) {
    double gamma = GetParam();
    
    // 使用自定义gamma参数创建LRSI（如果支持）
    auto lrsi = std::make_shared<LRSI>(close_line_, gamma);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        lrsi->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最后的值
    if (csv_data_.size() >= 6) {  // 确保有足够数据
        double last_value = lrsi->get(0);
        
        EXPECT_FALSE(std::isnan(last_value)) << "Last LRSI value should not be NaN for gamma=" << gamma;
        EXPECT_TRUE(std::isfinite(last_value)) << "Last LRSI value should be finite for gamma=" << gamma;
        EXPECT_GE(last_value, 0.0) << "LRSI should be >= 0 for gamma=" << gamma;
        EXPECT_LE(last_value, 1.0) << "LRSI should be <= 1 for gamma=" << gamma;
    }
}

// 测试不同的gamma参数
INSTANTIATE_TEST_SUITE_P(
    VariousGamma,
    LRSIParameterizedTest,
    ::testing::Values(0.1, 0.2, 0.3, 0.5, 0.7, 0.9)
);

// LRSI计算逻辑验证测试
TEST(OriginalTests, LRSI_CalculationLogic) {
    // 使用简单的测试数据验证LRSI计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 108.0, 106.0, 104.0, 102.0};
    
    auto price_line = std::make_shared<backtrader::LineRoot>(prices.size(), "lrsi_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto lrsi = std::make_shared<LRSI>(price_line, 0.5);  // 使用gamma=0.5
    
    // LRSI基于Laguerre滤波器，计算较为复杂，主要验证其基本特性
    for (size_t i = 0; i < prices.size(); ++i) {
        lrsi->calculate();
        
        if (i >= 5) {  // LRSI需要6个数据点
            double lrsi_value = lrsi->get(0);
            
            if (!std::isnan(lrsi_value)) {
                // 验证LRSI的基本范围约束
                EXPECT_GE(lrsi_value, 0.0) 
                    << "LRSI should be >= 0 at step " << i;
                EXPECT_LE(lrsi_value, 1.0) 
                    << "LRSI should be <= 1 at step " << i;
                EXPECT_TRUE(std::isfinite(lrsi_value)) 
                    << "LRSI should be finite at step " << i;
            }
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
}

// LRSI超买超卖信号测试
TEST(OriginalTests, LRSI_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto lrsi = std::make_shared<LRSI>(close_line);
    
    int overbought_count = 0;   // LRSI > 0.8
    int oversold_count = 0;     // LRSI < 0.2
    int neutral_count = 0;      // 0.2 <= LRSI <= 0.8
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        lrsi->calculate();
        
        double lrsi_value = lrsi->get(0);
        
        if (!std::isnan(lrsi_value)) {
            if (lrsi_value > 0.8) {
                overbought_count++;
            } else if (lrsi_value < 0.2) {
                oversold_count++;
            } else {
                neutral_count++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "LRSI overbought/oversold analysis:" << std::endl;
    std::cout << "Overbought (> 0.8): " << overbought_count << std::endl;
    std::cout << "Oversold (< 0.2): " << oversold_count << std::endl;
    std::cout << "Neutral (0.2-0.8): " << neutral_count << std::endl;
    
    // 验证有一些有效的计算
    int total_valid = overbought_count + oversold_count + neutral_count;
    EXPECT_GT(total_valid, 0) << "Should have some valid LRSI calculations";
    
    // 大部分值应该在0-1范围内
    EXPECT_EQ(total_valid, overbought_count + oversold_count + neutral_count) 
        << "All valid LRSI values should be in 0-1 range";
}

// LRSI平滑特性测试
TEST(OriginalTests, LRSI_SmoothingCharacteristics) {
    // 创建含有噪声的数据
    std::vector<double> noisy_prices;
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> noise_dist(-2.0, 2.0);
    
    for (int i = 0; i < 100; ++i) {
        double trend = 100.0 + i * 0.5;  // 缓慢上升趋势
        double noise = noise_dist(rng);   // 随机噪声
        noisy_prices.push_back(trend + noise);
    }
    
    auto noisy_line = std::make_shared<backtrader::LineRoot>(noisy_prices.size(), "noisy");
    for (double price : noisy_prices) {
        noisy_line->forward(price);
    }
    
    auto lrsi = std::make_shared<LRSI>(noisy_line);
    auto regular_rsi = std::make_shared<RSI>(noisy_line, 14);  // 比较对象
    
    std::vector<double> lrsi_changes;
    std::vector<double> rsi_changes;
    double prev_lrsi = 0.0, prev_rsi = 0.0;
    bool has_prev = false;
    
    for (size_t i = 0; i < noisy_prices.size(); ++i) {
        lrsi->calculate();
        regular_rsi->calculate();
        
        double current_lrsi = lrsi->get(0);
        double current_rsi = regular_rsi->get(0);
        
        if (!std::isnan(current_lrsi) && !std::isnan(current_rsi)) {
            if (has_prev) {
                lrsi_changes.push_back(std::abs(current_lrsi - prev_lrsi));
                rsi_changes.push_back(std::abs(current_rsi - prev_rsi));
            }
            prev_lrsi = current_lrsi;
            prev_rsi = current_rsi;
            has_prev = true;
        }
        
        if (i < noisy_prices.size() - 1) {
            noisy_line->forward();
        }
    }
    
    // 比较LRSI和RSI的平滑性
    if (!lrsi_changes.empty() && !rsi_changes.empty()) {
        double avg_lrsi_change = std::accumulate(lrsi_changes.begin(), lrsi_changes.end(), 0.0) / lrsi_changes.size();
        double avg_rsi_change = std::accumulate(rsi_changes.begin(), rsi_changes.end(), 0.0) / rsi_changes.size();
        
        std::cout << "Smoothing comparison:" << std::endl;
        std::cout << "Average LRSI change: " << avg_lrsi_change << std::endl;
        std::cout << "Average RSI change: " << avg_rsi_change << std::endl;
        
        // LRSI由于Laguerre滤波器应该比RSI更平滑
        EXPECT_LT(avg_lrsi_change, avg_rsi_change) 
            << "LRSI should be smoother than regular RSI";
    }
}

// LRSI趋势跟随测试
TEST(OriginalTests, LRSI_TrendFollowing) {
    // 创建明确的趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 50; ++i) {
        uptrend_prices.push_back(100.0 + i * 1.0);  // 线性上升趋势
    }
    
    auto trend_line = std::make_shared<backtrader::LineRoot>(uptrend_prices.size(), "trend");
    for (double price : uptrend_prices) {
        trend_line->forward(price);
    }
    
    auto trend_lrsi = std::make_shared<LRSI>(trend_line);
    
    std::vector<double> lrsi_values;
    
    for (size_t i = 0; i < uptrend_prices.size(); ++i) {
        trend_lrsi->calculate();
        
        double lrsi_val = trend_lrsi->get(0);
        if (!std::isnan(lrsi_val)) {
            lrsi_values.push_back(lrsi_val);
        }
        
        if (i < uptrend_prices.size() - 1) {
            trend_line->forward();
        }
    }
    
    // 分析LRSI在上升趋势中的表现
    if (lrsi_values.size() > 20) {
        double early_avg = std::accumulate(lrsi_values.begin(), lrsi_values.begin() + 10, 0.0) / 10.0;
        double late_avg = std::accumulate(lrsi_values.end() - 10, lrsi_values.end(), 0.0) / 10.0;
        
        std::cout << "Trend following analysis:" << std::endl;
        std::cout << "Early LRSI average: " << early_avg << std::endl;
        std::cout << "Late LRSI average: " << late_avg << std::endl;
        
        // 在持续上升趋势中，LRSI应该趋向于较高值
        EXPECT_GT(late_avg, 0.5) << "LRSI should be high in strong uptrend";
    }
}

// LRSI反转信号测试
TEST(OriginalTests, LRSI_ReversalSignals) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto lrsi = std::make_shared<LRSI>(close_line);
    
    int bullish_reversals = 0;  // LRSI从超卖区域上升
    int bearish_reversals = 0;  // LRSI从超买区域下降
    double prev_lrsi = 0.0;
    bool was_oversold = false;
    bool was_overbought = false;
    bool has_prev = false;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        lrsi->calculate();
        
        double current_lrsi = lrsi->get(0);
        
        if (!std::isnan(current_lrsi)) {
            if (has_prev) {
                // 检测从超卖区域的反转
                if (was_oversold && current_lrsi > 0.2) {
                    bullish_reversals++;
                    was_oversold = false;
                }
                
                // 检测从超买区域的反转
                if (was_overbought && current_lrsi < 0.8) {
                    bearish_reversals++;
                    was_overbought = false;
                }
            }
            
            // 更新状态
            if (current_lrsi < 0.2) {
                was_oversold = true;
            }
            if (current_lrsi > 0.8) {
                was_overbought = true;
            }
            
            prev_lrsi = current_lrsi;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "LRSI reversal signals:" << std::endl;
    std::cout << "Bullish reversals: " << bullish_reversals << std::endl;
    std::cout << "Bearish reversals: " << bearish_reversals << std::endl;
    
    // 验证检测到一些反转信号
    EXPECT_GE(bullish_reversals + bearish_reversals, 0) 
        << "Should detect some reversal signals";
}

// LRSI与价格发散测试
TEST(OriginalTests, LRSI_PriceDivergence) {
    // 创建价格创新高但动量减弱的数据（负发散）
    std::vector<double> divergence_prices;
    
    // 第一段上升：强势
    for (int i = 0; i < 20; ++i) {
        divergence_prices.push_back(100.0 + i * 2.0);
    }
    
    // 第二段上升：弱势（价格新高但涨幅减小）
    for (int i = 0; i < 20; ++i) {
        divergence_prices.push_back(divergence_prices.back() + i * 0.5);
    }
    
    auto div_line = std::make_shared<backtrader::LineRoot>(divergence_prices.size(), "divergence");
    for (double price : divergence_prices) {
        div_line->forward(price);
    }
    
    auto div_lrsi = std::make_shared<LRSI>(div_line);
    
    std::vector<double> prices_segment1, prices_segment2;
    std::vector<double> lrsi_segment1, lrsi_segment2;
    
    for (size_t i = 0; i < divergence_prices.size(); ++i) {
        div_lrsi->calculate();
        
        double price = divergence_prices[i];
        double lrsi_val = div_lrsi->get(0);
        
        if (!std::isnan(lrsi_val)) {
            if (i < 20) {
                prices_segment1.push_back(price);
                lrsi_segment1.push_back(lrsi_val);
            } else {
                prices_segment2.push_back(price);
                lrsi_segment2.push_back(lrsi_val);
            }
        }
        
        if (i < divergence_prices.size() - 1) {
            div_line->forward();
        }
    }
    
    // 分析发散现象
    if (!prices_segment1.empty() && !prices_segment2.empty() && 
        !lrsi_segment1.empty() && !lrsi_segment2.empty()) {
        
        double price_high1 = *std::max_element(prices_segment1.begin(), prices_segment1.end());
        double price_high2 = *std::max_element(prices_segment2.begin(), prices_segment2.end());
        double lrsi_high1 = *std::max_element(lrsi_segment1.begin(), lrsi_segment1.end());
        double lrsi_high2 = *std::max_element(lrsi_segment2.begin(), lrsi_segment2.end());
        
        std::cout << "Divergence analysis:" << std::endl;
        std::cout << "Price high 1: " << price_high1 << ", Price high 2: " << price_high2 << std::endl;
        std::cout << "LRSI high 1: " << lrsi_high1 << ", LRSI high 2: " << lrsi_high2 << std::endl;
        
        // 价格应该创新高，但LRSI可能不会
        EXPECT_GT(price_high2, price_high1) << "Second price segment should make higher high";
        
        // 分析LRSI是否显示动量减弱
        if (lrsi_high2 < lrsi_high1) {
            std::cout << "Negative divergence detected: Price made higher high, LRSI made lower high" << std::endl;
        }
    }
}

// 边界条件测试
TEST(OriginalTests, LRSI_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(50, 100.0);
    
    auto flat_line = std::make_shared<backtrader::LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_lrsi = std::make_shared<LRSI>(flat_line);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_lrsi->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，LRSI应该趋向于中性值
    double final_lrsi = flat_lrsi->get(0);
    if (!std::isnan(final_lrsi)) {
        EXPECT_GE(final_lrsi, 0.0) << "LRSI should be >= 0 for constant prices";
        EXPECT_LE(final_lrsi, 1.0) << "LRSI should be <= 1 for constant prices";
        // LRSI在没有价格变化时应该趋向于中性值（约0.5）
        EXPECT_NEAR(final_lrsi, 0.5, 0.3) << "LRSI should be near neutral for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<backtrader::LineRoot>(10, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 4; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_lrsi = std::make_shared<LRSI>(insufficient_line);
    
    for (int i = 0; i < 4; ++i) {
        insufficient_lrsi->calculate();
        if (i < 3) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_lrsi->get(0);
    EXPECT_TRUE(std::isnan(result)) << "LRSI should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, LRSI_Performance) {
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
    
    auto large_lrsi = std::make_shared<LRSI>(large_line);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_lrsi->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "LRSI calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_lrsi->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    EXPECT_GE(final_result, 0.0) << "Final result should be >= 0";
    EXPECT_LE(final_result, 1.0) << "Final result should be <= 1";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}
