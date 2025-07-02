/**
 * @file test_ind_dv2.cpp
 * @brief DV2指标测试 - 对应Python test_ind_dv2.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['17.460317', '55.952381', '80.555556']
 * ]
 * chkmin = 253
 * chkind = btind.DV2
 */

#include "test_common.h"
#include "indicators/DV2.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> DV2_EXPECTED_VALUES = {
    {"17.460317", "55.952381", "80.555556"}
};

const int DV2_MIN_PERIOD = 253;

} // anonymous namespace

// 使用默认参数的DV2测试
DEFINE_INDICATOR_TEST(DV2_Default, DV2, DV2_EXPECTED_VALUES, DV2_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, DV2_Manual) {
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
    
    // 创建DV2指标（默认252周期）
    auto dv2 = std::make_shared<DV2>(high_line, low_line, close_line, 252);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        dv2->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 253;  // period + 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"17.460317", "55.952381", "80.555556"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = dv2->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "DV2 value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(dv2->getMinPeriod(), 253) << "DV2 minimum period should be 253";
}

// DV2范围验证测试
TEST(OriginalTests, DV2_RangeValidation) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto dv2 = std::make_shared<DV2>(high_line, low_line, close_line, 252);
    
    // 计算所有值并验证范围
    for (size_t i = 0; i < csv_data.size(); ++i) {
        dv2->calculate();
        
        double dv2_value = dv2->get(0);
        
        // 验证DV2在0到100范围内
        if (!std::isnan(dv2_value)) {
            EXPECT_GE(dv2_value, 0.0) << "DV2 should be >= 0 at step " << i;
            EXPECT_LE(dv2_value, 100.0) << "DV2 should be <= 100 at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}

// 参数化测试 - 测试不同周期的DV2
class DV2ParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(DV2ParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto dv2 = std::make_shared<DV2>(high_line_, low_line_, close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        dv2->calculate();
        if (i < csv_data_.size() - 1) {
            high_line_->forward();
            low_line_->forward();
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(dv2->getMinPeriod(), period + 1) 
        << "DV2 minimum period should be period + 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        double last_value = dv2->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last DV2 value should not be NaN";
        EXPECT_GE(last_value, 0.0) << "DV2 should be >= 0";
        EXPECT_LE(last_value, 100.0) << "DV2 should be <= 100";
    }
}

// 测试不同的DV2周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    DV2ParameterizedTest,
    ::testing::Values(20, 50, 126, 252)
);

// DV2计算逻辑验证测试
TEST(OriginalTests, DV2_CalculationLogic) {
    // 使用简单的测试数据验证DV2计算
    std::vector<CSVDataReader::OHLCVData> test_data;
    
    // 创建测试数据：交替的上涨和下跌日
    for (int i = 0; i < 30; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        
        if (i % 2 == 0) {
            // 上涨日：收盘价靠近高点
            bar.high = 100.0 + i;
            bar.low = 95.0 + i;
            bar.close = 99.0 + i;  // 接近高点
        } else {
            // 下跌日：收盘价靠近低点
            bar.high = 100.0 + i;
            bar.low = 95.0 + i;
            bar.close = 96.0 + i;  // 接近低点
        }
        
        bar.open = (bar.high + bar.low) / 2.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        test_data.push_back(bar);
    }
    
    auto high_line = std::make_shared<LineRoot>(test_data.size(), "test_high");
    auto low_line = std::make_shared<LineRoot>(test_data.size(), "test_low");
    auto close_line = std::make_shared<LineRoot>(test_data.size(), "test_close");
    
    for (const auto& bar : test_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto dv2 = std::make_shared<DV2>(high_line, low_line, close_line, 10);
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        dv2->calculate();
        
        double dv2_val = dv2->get(0);
        
        // DV2应该产生有限值且在0-100范围内
        if (!std::isnan(dv2_val)) {
            EXPECT_TRUE(std::isfinite(dv2_val)) 
                << "DV2 should be finite at step " << i;
            EXPECT_GE(dv2_val, 0.0) 
                << "DV2 should be >= 0 at step " << i;
            EXPECT_LE(dv2_val, 100.0) 
                << "DV2 should be <= 100 at step " << i;
        }
        
        if (i < test_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}

// 牛市熊市行为测试
TEST(OriginalTests, DV2_BullBearBehavior) {
    // 创建牛市数据（收盘价持续接近高点）
    std::vector<CSVDataReader::OHLCVData> bull_data;
    for (int i = 0; i < 50; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 100.0 + i * 2.0;
        bar.low = 95.0 + i * 2.0;
        bar.close = bar.high - 0.5;  // 接近高点
        bar.open = (bar.high + bar.low) / 2.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        bull_data.push_back(bar);
    }
    
    auto bull_high = std::make_shared<LineRoot>(bull_data.size(), "bull_high");
    auto bull_low = std::make_shared<LineRoot>(bull_data.size(), "bull_low");
    auto bull_close = std::make_shared<LineRoot>(bull_data.size(), "bull_close");
    
    for (const auto& bar : bull_data) {
        bull_high->forward(bar.high);
        bull_low->forward(bar.low);
        bull_close->forward(bar.close);
    }
    
    auto bull_dv2 = std::make_shared<DV2>(bull_high, bull_low, bull_close, 20);
    
    for (size_t i = 0; i < bull_data.size(); ++i) {
        bull_dv2->calculate();
        if (i < bull_data.size() - 1) {
            bull_high->forward();
            bull_low->forward();
            bull_close->forward();
        }
    }
    
    double final_bull_dv2 = bull_dv2->get(0);
    if (!std::isnan(final_bull_dv2)) {
        EXPECT_GT(final_bull_dv2, 50.0) 
            << "DV2 should be high in bullish conditions";
        
        std::cout << "Bull market DV2: " << final_bull_dv2 << std::endl;
    }
    
    // 创建熊市数据（收盘价持续接近低点）
    std::vector<CSVDataReader::OHLCVData> bear_data;
    for (int i = 0; i < 50; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 200.0 - i * 2.0;
        bar.low = 195.0 - i * 2.0;
        bar.close = bar.low + 0.5;  // 接近低点
        bar.open = (bar.high + bar.low) / 2.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        bear_data.push_back(bar);
    }
    
    auto bear_high = std::make_shared<LineRoot>(bear_data.size(), "bear_high");
    auto bear_low = std::make_shared<LineRoot>(bear_data.size(), "bear_low");
    auto bear_close = std::make_shared<LineRoot>(bear_data.size(), "bear_close");
    
    for (const auto& bar : bear_data) {
        bear_high->forward(bar.high);
        bear_low->forward(bar.low);
        bear_close->forward(bar.close);
    }
    
    auto bear_dv2 = std::make_shared<DV2>(bear_high, bear_low, bear_close, 20);
    
    for (size_t i = 0; i < bear_data.size(); ++i) {
        bear_dv2->calculate();
        if (i < bear_data.size() - 1) {
            bear_high->forward();
            bear_low->forward();
            bear_close->forward();
        }
    }
    
    double final_bear_dv2 = bear_dv2->get(0);
    if (!std::isnan(final_bear_dv2)) {
        EXPECT_LT(final_bear_dv2, 50.0) 
            << "DV2 should be low in bearish conditions";
        
        std::cout << "Bear market DV2: " << final_bear_dv2 << std::endl;
    }
}

// 中性市场测试
TEST(OriginalTests, DV2_NeutralMarket) {
    // 创建中性市场数据（收盘价在高低点中间）
    std::vector<CSVDataReader::OHLCVData> neutral_data;
    for (int i = 0; i < 50; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 105.0;
        bar.low = 95.0;
        bar.close = 100.0;  // 正好在中间
        bar.open = 100.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        neutral_data.push_back(bar);
    }
    
    auto neutral_high = std::make_shared<LineRoot>(neutral_data.size(), "neutral_high");
    auto neutral_low = std::make_shared<LineRoot>(neutral_data.size(), "neutral_low");
    auto neutral_close = std::make_shared<LineRoot>(neutral_data.size(), "neutral_close");
    
    for (const auto& bar : neutral_data) {
        neutral_high->forward(bar.high);
        neutral_low->forward(bar.low);
        neutral_close->forward(bar.close);
    }
    
    auto neutral_dv2 = std::make_shared<DV2>(neutral_high, neutral_low, neutral_close, 20);
    
    for (size_t i = 0; i < neutral_data.size(); ++i) {
        neutral_dv2->calculate();
        if (i < neutral_data.size() - 1) {
            neutral_high->forward();
            neutral_low->forward();
            neutral_close->forward();
        }
    }
    
    double final_neutral_dv2 = neutral_dv2->get(0);
    if (!std::isnan(final_neutral_dv2)) {
        EXPECT_NEAR(final_neutral_dv2, 50.0, 10.0) 
            << "DV2 should be around 50 in neutral market";
        
        std::cout << "Neutral market DV2: " << final_neutral_dv2 << std::endl;
    }
}

// 均值回归信号测试
TEST(OriginalTests, DV2_MeanReversionSignals) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto dv2 = std::make_shared<DV2>(high_line, low_line, close_line, 252);
    
    int oversold_signals = 0;    // DV2 < 25
    int overbought_signals = 0;  // DV2 > 75
    int neutral_signals = 0;     // 25 <= DV2 <= 75
    
    // 统计不同信号的分布
    for (size_t i = 0; i < csv_data.size(); ++i) {
        dv2->calculate();
        
        double dv2_value = dv2->get(0);
        
        if (!std::isnan(dv2_value)) {
            if (dv2_value < 25.0) {
                oversold_signals++;
            } else if (dv2_value > 75.0) {
                overbought_signals++;
            } else {
                neutral_signals++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    std::cout << "DV2 signal distribution:" << std::endl;
    std::cout << "Oversold signals (< 25): " << oversold_signals << std::endl;
    std::cout << "Overbought signals (> 75): " << overbought_signals << std::endl;
    std::cout << "Neutral signals (25-75): " << neutral_signals << std::endl;
    
    // 验证至少有一些有效的计算
    EXPECT_GT(oversold_signals + overbought_signals + neutral_signals, 0) 
        << "Should have some valid DV2 calculations";
}

// 边界条件测试
TEST(OriginalTests, DV2_EdgeCases) {
    // 测试所有价格相同的情况
    std::vector<CSVDataReader::OHLCVData> flat_data;
    for (int i = 0; i < 300; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-01";
        bar.high = 100.0;
        bar.low = 100.0;
        bar.close = 100.0;
        bar.open = 100.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        flat_data.push_back(bar);
    }
    
    auto flat_high = std::make_shared<LineRoot>(flat_data.size(), "flat_high");
    auto flat_low = std::make_shared<LineRoot>(flat_data.size(), "flat_low");
    auto flat_close = std::make_shared<LineRoot>(flat_data.size(), "flat_close");
    
    for (const auto& bar : flat_data) {
        flat_high->forward(bar.high);
        flat_low->forward(bar.low);
        flat_close->forward(bar.close);
    }
    
    auto flat_dv2 = std::make_shared<DV2>(flat_high, flat_low, flat_close, 252);
    
    for (size_t i = 0; i < flat_data.size(); ++i) {
        flat_dv2->calculate();
        if (i < flat_data.size() - 1) {
            flat_high->forward();
            flat_low->forward();
            flat_close->forward();
        }
    }
    
    // 当所有价格相同时，DV2的行为取决于具体实现
    double final_dv2 = flat_dv2->get(0);
    if (!std::isnan(final_dv2)) {
        EXPECT_GE(final_dv2, 0.0) << "DV2 should be >= 0 for constant prices";
        EXPECT_LE(final_dv2, 100.0) << "DV2 should be <= 100 for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_high = std::make_shared<LineRoot>(100, "insufficient_high");
    auto insufficient_low = std::make_shared<LineRoot>(100, "insufficient_low");
    auto insufficient_close = std::make_shared<LineRoot>(100, "insufficient_close");
    
    // 只添加几个数据点
    for (int i = 0; i < 100; ++i) {
        insufficient_high->forward(105.0 + i);
        insufficient_low->forward(95.0 + i);
        insufficient_close->forward(100.0 + i);
    }
    
    auto insufficient_dv2 = std::make_shared<DV2>(insufficient_high, insufficient_low, insufficient_close, 252);
    
    for (int i = 0; i < 100; ++i) {
        insufficient_dv2->calculate();
        if (i < 99) {
            insufficient_high->forward();
            insufficient_low->forward();
            insufficient_close->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_dv2->get(0);
    EXPECT_TRUE(std::isnan(result)) << "DV2 should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, DV2_Performance) {
    // 生成大量测试数据
    const size_t data_size = 5000;  // DV2需要较多数据，使用5K而不是10K
    std::vector<CSVDataReader::OHLCVData> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> price_dist(50.0, 150.0);
    std::uniform_real_distribution<double> range_dist(1.0, 5.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-01";
        double base_price = price_dist(rng);
        double range = range_dist(rng);
        
        bar.high = base_price + range;
        bar.low = base_price - range;
        bar.close = base_price + (range * 2.0 * rng() / rng.max() - range);  // Random close within range
        bar.open = base_price;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        large_data.push_back(bar);
    }
    
    auto large_high = std::make_shared<LineRoot>(large_data.size(), "large_high");
    auto large_low = std::make_shared<LineRoot>(large_data.size(), "large_low");
    auto large_close = std::make_shared<LineRoot>(large_data.size(), "large_close");
    
    for (const auto& bar : large_data) {
        large_high->forward(bar.high);
        large_low->forward(bar.low);
        large_close->forward(bar.close);
    }
    
    auto large_dv2 = std::make_shared<DV2>(large_high, large_low, large_close, 252);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_dv2->calculate();
        if (i < large_data.size() - 1) {
            large_high->forward();
            large_low->forward();
            large_close->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "DV2 calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_dv2->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_GE(final_result, 0.0) << "Final result should be >= 0";
    EXPECT_LE(final_result, 100.0) << "Final result should be <= 100";
    
    // 性能要求：5K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}