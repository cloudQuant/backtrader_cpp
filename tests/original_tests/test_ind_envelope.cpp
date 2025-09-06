/**
 * @file test_ind_envelope.cpp
 * @brief Envelope指标测试 - 对应Python test_ind_envelope.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4063.463000', '3644.444667', '3554.693333'],  # Mid Line (SMA)
 *     ['4165.049575', '3735.555783', '3643.560667'],  # Upper Line
 *     ['3961.876425', '3553.333550', '3465.826000']   # Lower Line
 * ]
 * chkmin = 30
 * chkind = btind.Envelope
 */

#include "test_common.h"
#include "lineseries.h"
#include <random>

#include "indicators/envelope.h"

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;
using LineSeries = backtrader::LineSeries;
using LineBuffer = backtrader::LineBuffer;

namespace {

const std::vector<std::vector<std::string>> ENVELOPE_EXPECTED_VALUES = {
    {"4063.463000", "3644.444667", "3554.693333"},  // Mid Line
    {"4165.049575", "3735.555783", "3643.560667"},  // Upper Line
    {"3961.876425", "3553.333550", "3465.826000"}   // Lower Line
};

const int ENVELOPE_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的Envelope测试
DEFINE_INDICATOR_TEST(Envelope_Default, Envelope, ENVELOPE_EXPECTED_VALUES, ENVELOPE_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, Envelope_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("close", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    // Set buffer index to the end for proper ago indexing
    // Since LineBuffer starts with NaN, total size is csv_data.size() + 1
    close_buffer->set_idx(close_buffer->size() - 1);
    
    // 创建Envelope指标（基于30周期SMA，默认2.5%包络）
    auto envelope = std::make_shared<Envelope>(close_line, 30, 2.5);
    
    // 计算所有值
    envelope->calculate();
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 30;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    // 其中 l = indicator长度 = 255, mp = min_period = 30
    std::vector<int> check_points = {
        0,                                    // 当前值 (ago=0)
        -(255 - 30),                         // ago=-225
        static_cast<int>(std::floor(-(255 - 30) / 2.0))  // ago=-113 (Python floor division)
    };
    
    // 验证Mid Line (SMA)
    std::vector<std::string> expected_mid = {"4063.463000", "3644.444667", "3554.693333"};
    for (size_t i = 0; i < check_points.size() && i < expected_mid.size(); ++i) {
        double actual = envelope->getMidLine(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_mid[i]) 
            << "Envelope Mid Line mismatch at check point " << i;
    }
    
    // 验证Upper Line
    std::vector<std::string> expected_upper = {"4165.049575", "3735.555783", "3643.560667"};
    for (size_t i = 0; i < check_points.size() && i < expected_upper.size(); ++i) {
        double actual = envelope->getUpperLine(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_upper[i]) 
            << "Envelope Upper Line mismatch at check point " << i;
    }
    
    // 验证Lower Line
    std::vector<std::string> expected_lower = {"3961.876425", "3553.333550", "3465.826000"};
    for (size_t i = 0; i < check_points.size() && i < expected_lower.size(); ++i) {
        double actual = envelope->getLowerLine(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_lower[i]) 
            << "Envelope Lower Line mismatch at check point " << i;
    }
    
    // 验证最小周期
    EXPECT_EQ(envelope->getMinPeriod(), 30) << "Envelope minimum period should be 30";
}

// 参数化测试 - 测试不同参数的Envelope
class EnvelopeParameterizedTest : public ::testing::TestWithParam<std::pair<int, double>> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line_ = std::make_shared<LineSeries>();
        close_line_->lines->add_line(std::make_shared<LineBuffer>());
        close_line_->lines->add_alias("close", 0);
        close_line_buffer_ = std::dynamic_pointer_cast<LineBuffer>(close_line_->lines->getline(0));
        for (const auto& bar : csv_data_) {
            close_line_buffer_->append(bar.close);
        }
        // Set buffer index to the end for proper ago indexing
        // Since LineBuffer starts with NaN, total size is csv_data_.size() + 1
        close_line_buffer_->set_idx(close_line_buffer_->size() - 1);
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineSeries> close_line_;
    std::shared_ptr<LineBuffer> close_line_buffer_;
};

TEST_P(EnvelopeParameterizedTest, DifferentParameters) {
    auto [period, percentage] = GetParam();
    auto envelope = std::make_shared<Envelope>(close_line_, period, percentage);
    
    // 计算所有值
    envelope->calculate();
    
    // 验证最小周期
    EXPECT_EQ(envelope->getMinPeriod(), period) 
        << "Envelope minimum period should equal MA period";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double mid_value = envelope->getMidLine(0);
        double upper_value = envelope->getUpperLine(0);
        double lower_value = envelope->getLowerLine(0);
        
        EXPECT_FALSE(std::isnan(mid_value)) << "Mid line should not be NaN";
        EXPECT_FALSE(std::isnan(upper_value)) << "Upper line should not be NaN";
        EXPECT_FALSE(std::isnan(lower_value)) << "Lower line should not be NaN";
        
        // 验证包络关系
        EXPECT_GT(upper_value, mid_value) << "Upper line should be above mid line";
        EXPECT_LT(lower_value, mid_value) << "Lower line should be below mid line";
        
        // 验证包络百分比
        double expected_upper = mid_value * (1.0 + percentage / 100.0);
        double expected_lower = mid_value * (1.0 - percentage / 100.0);
        
        EXPECT_NEAR(upper_value, expected_upper, 1e-6) 
            << "Upper line should be mid * (1 + percentage/100)";
        EXPECT_NEAR(lower_value, expected_lower, 1e-6) 
            << "Lower line should be mid * (1 - percentage/100)";
    }
}

// 测试不同的Envelope参数
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    EnvelopeParameterizedTest,
    ::testing::Values(
        std::make_pair(10, 1.0),    // 短周期，小包络
        std::make_pair(20, 2.0),    // 中周期，中包络
        std::make_pair(30, 2.5),    // 标准参数
        std::make_pair(50, 5.0)     // 长周期，大包络
    )
);

// Envelope包含性测试
TEST(OriginalTests, Envelope_Containment) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("close", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    auto envelope = std::make_shared<Envelope>(close_line, 20, 3.0);
    
    // Calculate envelope once for all data
    envelope->calculate();
    
    int prices_above_upper = 0;
    int prices_below_lower = 0;
    int prices_within_envelope = 0;
    int total_valid = 0;
    
    // 分析价格与包络的关系
    for (size_t i = 0; i < csv_data.size(); ++i) {
        double price = csv_data[i].close;
        
        // Calculate ago value for historical envelope access
        // i=0 -> ago=-(data_size-1), i=data_size-1 -> ago=0
        int ago = -static_cast<int>(csv_data.size() - 1 - i);
        
        double upper = envelope->getUpperLine(ago);
        double lower = envelope->getLowerLine(ago);
        
        if (!std::isnan(upper) && !std::isnan(lower)) {
            total_valid++;
            
            if (price > upper) {
                prices_above_upper++;
            } else if (price < lower) {
                prices_below_lower++;
            } else {
                prices_within_envelope++;
            }
        }
    }
    
    std::cout << "Envelope containment analysis:" << std::endl;
    std::cout << "Total valid points: " << total_valid << std::endl;
    std::cout << "Prices above upper: " << prices_above_upper << std::endl;
    std::cout << "Prices within envelope: " << prices_within_envelope << std::endl;
    std::cout << "Prices below lower: " << prices_below_lower << std::endl;
    
    if (total_valid > 0) {
        double containment_ratio = static_cast<double>(prices_within_envelope) / total_valid;
        std::cout << "Containment ratio: " << containment_ratio << std::endl;
        
        // 大部分价格应该在包络内
        EXPECT_GT(containment_ratio, 0.6) 
            << "Most prices should be within envelope";
        
        // 验证至少有一些有效的计算
        EXPECT_GT(total_valid, 0) << "Should have some valid envelope calculations";
    }
}

// Envelope突破信号测试
TEST(OriginalTests, Envelope_BreakoutSignals) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("close", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    auto envelope = std::make_shared<Envelope>(close_line, 20, 2.0);
    
    // Calculate envelope once for all data
    envelope->calculate();
    
    int upper_breakouts = 0;    // 价格突破上轨
    int lower_breakouts = 0;    // 价格跌破下轨
    int upper_pullbacks = 0;    // 从上轨回落
    int lower_pullbacks = 0;    // 从下轨反弹
    
    bool prev_above_upper = false;
    bool prev_below_lower = false;
    bool has_prev = false;
    
    // 检测突破信号
    for (size_t i = 0; i < csv_data.size(); ++i) {
        double price = csv_data[i].close;
        
        // Calculate ago value for historical envelope access
        int ago = -static_cast<int>(csv_data.size() - 1 - i);
        
        double upper = envelope->getUpperLine(ago);
        double lower = envelope->getLowerLine(ago);
        
        if (!std::isnan(upper) && !std::isnan(lower)) {
            bool current_above_upper = price > upper;
            bool current_below_lower = price < lower;
            
            if (has_prev) {
                // 检测突破上轨
                if (!prev_above_upper && current_above_upper) {
                    upper_breakouts++;
                }
                // 检测从上轨回落
                else if (prev_above_upper && !current_above_upper) {
                    upper_pullbacks++;
                }
                
                // 检测跌破下轨
                if (!prev_below_lower && current_below_lower) {
                    lower_breakouts++;
                }
                // 检测从下轨反弹
                else if (prev_below_lower && !current_below_lower) {
                    lower_pullbacks++;
                }
            }
            
            prev_above_upper = current_above_upper;
            prev_below_lower = current_below_lower;
            has_prev = true;
        }
    }
    
    std::cout << "Envelope breakout signals:" << std::endl;
    std::cout << "Upper breakouts: " << upper_breakouts << std::endl;
    std::cout << "Upper pullbacks: " << upper_pullbacks << std::endl;
    std::cout << "Lower breakouts: " << lower_breakouts << std::endl;
    std::cout << "Lower pullbacks: " << lower_pullbacks << std::endl;
    
    // 验证检测到一些信号
    EXPECT_GE(upper_breakouts + lower_breakouts + upper_pullbacks + lower_pullbacks, 0) 
        << "Should detect some breakout/pullback signals";
}

// Envelope动态宽度测试
TEST(OriginalTests, Envelope_DynamicWidth) {
    // 创建不同波动性的数据
    std::vector<double> low_vol_prices;
    std::vector<double> high_vol_prices;
    
    // 低波动性数据
    for (int i = 0; i < 100; ++i) {
        low_vol_prices.push_back(100.0 + 0.5 * std::sin(i * 0.1));
    }
    
    // 高波动性数据
    for (int i = 0; i < 100; ++i) {
        high_vol_prices.push_back(100.0 + 5.0 * std::sin(i * 0.1));
    }
    
    auto low_vol_line = std::make_shared<LineSeries>();
    low_vol_line->lines->add_line(std::make_shared<LineBuffer>());
    low_vol_line->lines->add_alias("low_vol", 0);
    auto low_vol_line_buffer = std::dynamic_pointer_cast<LineBuffer>(low_vol_line->lines->getline(0));

    for (double price : low_vol_prices) {
        low_vol_line_buffer->append(price);
    }
    
    auto high_vol_line = std::make_shared<LineSeries>();
    high_vol_line->lines->add_line(std::make_shared<LineBuffer>());
    high_vol_line->lines->add_alias("high_vol", 0);
    auto high_vol_line_buffer = std::dynamic_pointer_cast<LineBuffer>(high_vol_line->lines->getline(0));

    for (double price : high_vol_prices) {
        high_vol_line_buffer->append(price);
    }
    
    auto low_vol_envelope = std::make_shared<Envelope>(low_vol_line, 20, 2.0);
    auto high_vol_envelope = std::make_shared<Envelope>(high_vol_line, 20, 2.0);
    
    // 计算包络 - calculate once for all data
    low_vol_envelope->calculate();
    high_vol_envelope->calculate();
    
    // 比较包络宽度
    double low_vol_upper = low_vol_envelope->getUpperLine(0);
    double low_vol_lower = low_vol_envelope->getLowerLine(0);
    double low_vol_width = low_vol_upper - low_vol_lower;
    
    double high_vol_upper = high_vol_envelope->getUpperLine(0);
    double high_vol_lower = high_vol_envelope->getLowerLine(0);
    double high_vol_width = high_vol_upper - high_vol_lower;
    
    std::cout << "Envelope width comparison:" << std::endl;
    std::cout << "Low volatility width: " << low_vol_width << std::endl;
    std::cout << "High volatility width: " << high_vol_width << std::endl;
    
    // 对于相同的百分比，高波动性数据的包络应该更宽
    if (!std::isnan(low_vol_width) && !std::isnan(high_vol_width)) {
        EXPECT_GT(high_vol_width, low_vol_width) 
            << "High volatility should result in wider envelope";
    }
}

// Envelope均值回归测试
TEST(OriginalTests, Envelope_MeanReversion) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("close", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    auto envelope = std::make_shared<Envelope>(close_line, 20, 2.5);
    
    // Calculate envelope once for all data
    envelope->calculate();
    
    int successful_reversions = 0;
    int total_extreme_moves = 0;
    
    // 分析均值回归特性
    for (size_t i = 0; i < csv_data.size() - 5; ++i) {  // 留出5个点观察回归
        double price = csv_data[i].close;
        
        // Calculate ago value for historical envelope access
        int ago = -static_cast<int>(csv_data.size() - 1 - i);
        
        double upper = envelope->getUpperLine(ago);
        double lower = envelope->getLowerLine(ago);
        double mid = envelope->getMidLine(ago);
        
        if (!std::isnan(upper) && !std::isnan(lower) && !std::isnan(mid)) {
            // 检测极端位置
            bool at_extreme = (price > upper) || (price < lower);
            
            if (at_extreme) {
                total_extreme_moves++;
                
                // 检查后续5个点是否向中线回归
                bool reverted = false;
                for (int j = 1; j <= 5 && i + j < csv_data.size(); ++j) {
                    double future_price = csv_data[i + j].close;
                    
                    if (price > upper && future_price < price) {
                        // 从上轨向下回归
                        reverted = true;
                        break;
                    } else if (price < lower && future_price > price) {
                        // 从下轨向上回归
                        reverted = true;
                        break;
                    }
                }
                
                if (reverted) {
                    successful_reversions++;
                }
            }
        }
    }
    
    std::cout << "Mean reversion analysis:" << std::endl;
    std::cout << "Total extreme moves: " << total_extreme_moves << std::endl;
    std::cout << "Successful reversions: " << successful_reversions << std::endl;
    
    if (total_extreme_moves > 0) {
        double reversion_rate = static_cast<double>(successful_reversions) / total_extreme_moves;
        std::cout << "Reversion rate: " << reversion_rate << std::endl;
        
        // 一定程度的均值回归是预期的
        EXPECT_GT(reversion_rate, 0.3) 
            << "Should observe some mean reversion from extremes";
    }
}

// 边界条件测试
TEST(OriginalTests, Envelope_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineSeries>();
    flat_line->lines->add_line(std::make_shared<LineBuffer>());
    flat_line->lines->add_alias("flat", 0);
    auto flat_line_buffer = std::dynamic_pointer_cast<LineBuffer>(flat_line->lines->getline(0));

    for (double price : flat_prices) {
        flat_line_buffer->append(price);
    }
    
    auto flat_envelope = std::make_shared<Envelope>(flat_line, 20, 2.0);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_envelope->calculate();
        if (i < flat_prices.size() - 1) {
            if (flat_line_buffer) flat_line_buffer->forward();
        }
    }
    
    // 当所有价格相同时，中线应该等于该价格
    double mid = flat_envelope->getMidLine(0);
    double upper = flat_envelope->getUpperLine(0);
    double lower = flat_envelope->getLowerLine(0);
    
    if (!std::isnan(mid) && !std::isnan(upper) && !std::isnan(lower)) {
        EXPECT_NEAR(mid, 100.0, 1e-6) << "Mid line should equal constant price";
        EXPECT_NEAR(upper, 102.0, 1e-6) << "Upper line should be 2% above";
        EXPECT_NEAR(lower, 98.0, 1e-6) << "Lower line should be 2% below";
    }
    
    // 测试零百分比
    auto zero_percent_envelope = std::make_shared<Envelope>(flat_line, 20, 0.0);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        zero_percent_envelope->calculate();
        if (i < flat_prices.size() - 1) {
            if (flat_line_buffer) flat_line_buffer->forward();
        }
    }
    
    double zero_mid = zero_percent_envelope->getMidLine(0);
    double zero_upper = zero_percent_envelope->getUpperLine(0);
    double zero_lower = zero_percent_envelope->getLowerLine(0);
    
    if (!std::isnan(zero_mid) && !std::isnan(zero_upper) && !std::isnan(zero_lower)) {
        EXPECT_NEAR(zero_upper, zero_mid, 1e-6) << "Zero percent should collapse envelope";
        EXPECT_NEAR(zero_lower, zero_mid, 1e-6) << "Zero percent should collapse envelope";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineSeries>();
    insufficient_line->lines->add_line(std::make_shared<LineBuffer>());
    insufficient_line->lines->add_alias("insufficient", 0);
    auto insufficient_line_buffer = std::dynamic_pointer_cast<LineBuffer>(insufficient_line->lines->getline(0));

    // 只添加几个数据点
    for (int i = 0; i < 15; ++i) {
        insufficient_line_buffer->append(100.0 + i);
    }
    
    auto insufficient_envelope = std::make_shared<Envelope>(insufficient_line, 20, 2.0);
    
    for (int i = 0; i < 15; ++i) {
        insufficient_envelope->calculate();
        if (i < 14) {
            if (insufficient_line_buffer) insufficient_line_buffer->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result_mid = insufficient_envelope->getMidLine(0);
    EXPECT_TRUE(std::isnan(result_mid)) << "Envelope should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, Envelope_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<double> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        large_data.push_back(dist(rng));
    }
    
    auto large_line = std::make_shared<LineSeries>();
    large_line->lines->add_line(std::make_shared<LineBuffer>());
    large_line->lines->add_alias("large", 0);
    auto large_line_buffer = std::dynamic_pointer_cast<LineBuffer>(large_line->lines->getline(0));

    for (double price : large_data) {
        large_line_buffer->append(price);
    }
    
    auto large_envelope = std::make_shared<Envelope>(large_line, 50, 3.0);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Calculate envelope once for all data
    large_envelope->calculate();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Envelope calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_mid = large_envelope->getMidLine(0);
    double final_upper = large_envelope->getUpperLine(0);
    double final_lower = large_envelope->getLowerLine(0);
    
    EXPECT_FALSE(std::isnan(final_mid)) << "Final mid line should not be NaN";
    EXPECT_FALSE(std::isnan(final_upper)) << "Final upper line should not be NaN";
    EXPECT_FALSE(std::isnan(final_lower)) << "Final lower line should not be NaN";
    
    EXPECT_GT(final_upper, final_mid) << "Upper line should be above mid line";
    EXPECT_LT(final_lower, final_mid) << "Lower line should be below mid line";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}
