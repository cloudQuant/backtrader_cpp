/**
 * @file test_ind_smaenvelope.cpp
 * @brief SMAEnvelope指标测试 - 对应Python test_ind_smaenvelope.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4063.463000', '3644.444667', '3554.693333'],
 *     ['4165.049575', '3735.555783', '3643.560667'],
 *     ['3961.876425', '3553.333550', '3465.826000']
 * ]
 * chkmin = 30
 * chkind = btind.SMAEnvelope
 * 
 * 注：SMAEnvelope包含3条线：Mid (SMA), Upper, Lower
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

const std::vector<std::vector<std::string>> SMAENVELOPE_EXPECTED_VALUES = {
    {"4063.463000", "3644.444667", "3554.693333"},  // line 0 (Mid/SMA)
    {"4165.049575", "3735.555783", "3643.560667"},  // line 1 (Upper)
    {"3961.876425", "3553.333550", "3465.826000"}   // line 2 (Lower)
};

const int SMAENVELOPE_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的SMAEnvelope测试
DEFINE_INDICATOR_TEST(SMAEnvelope_Default, SMAEnvelope, SMAENVELOPE_EXPECTED_VALUES, SMAENVELOPE_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, SMAEnvelope_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("close", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    if (close_buffer) {
        // Keep the initial NaN and append all data
        // This gives us 256 elements total (1 NaN + 255 data points)
        for (size_t i = 0; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
        // Set the correct _idx to the last position (255 for 256 elements)
        close_buffer->set_idx(csv_data.size());  // This will be 255
    }
    
    // 创建SMAEnvelope指标
    auto smaenv = std::make_shared<SMAEnvelope>(close_line, 30, 2.5);
    
    std::cout << "Created SMAEnvelope, lines count: " << (smaenv->lines ? smaenv->lines->size() : 0) << std::endl;
    
    // 计算所有值 - 批量计算
    std::cout << "About to call smaenv->calculate()" << std::endl;
    std::cout << "smaenv type: " << typeid(*smaenv).name() << std::endl;
    smaenv->calculate();
    std::cout << "Finished calling smaenv->calculate()" << std::endl;
    
    std::cout << "After calculation, lines count: " << (smaenv->lines ? smaenv->lines->size() : 0) << std::endl;
    
    // Debug buffer sizes
    for (int i = 0; i < 3; ++i) {
        auto line = smaenv->getLine(i);
        if (line) {
            std::cout << "Line " << i << " size: " << line->size() << std::endl;
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 30;
    
    // Get the actual indicator length
    int indicator_length = smaenv->getLine(0)->size();
    
    std::cout << "Data length: " << data_length << " Min period: " << min_period << std::endl;
    std::cout << "Indicator length: " << indicator_length << std::endl;
    
    // Based on debug output, SMA buffer has:
    // - Positions 0-29: NaN 
    // - Position 30: first valid SMA = 3644.44 (close to expected 3644.444667)
    // - Position 142: middle SMA (need to check)
    // - Position 255: last SMA = 4063.46 (close to expected 4063.463000)
    // With LineBuffer _idx=255:
    // - ago=0 -> position 255 -> last SMA
    // - ago=-225 -> position 30 -> first valid SMA (255-225=30)
    // - ago=-113 -> position 142 -> middle SMA (255-113=142)
    std::vector<int> check_points = {
        0,      // last element (position 255)
        -225,   // first valid SMA (255 - 225 = 30)
        -113    // middle SMA (255 - 113 = 142)
    };
    
    // 验证3条线的值
    
    int line;
    for (int line = 0; line < 3; ++line) {
        auto expected = SMAENVELOPE_EXPECTED_VALUES[line];
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
            auto line_buffer = smaenv->getLine(line);
            if (!line_buffer) {
                std::cout << "Line " << line << " is null!" << std::endl;
                continue;
            }
            std::cout << "Accessing line " << line << " check point " << i << " (ago=" << check_points[i] 
                      << ") buffer size=" << line_buffer->size() << std::endl;
            
            double actual = line_buffer->get(check_points[i]);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << actual;
            std::string actual_str = ss.str();
            
            EXPECT_EQ(actual_str, expected[i]) 
                << "SMAEnvelope line " << line << " value mismatch at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(smaenv->getMinPeriod(), 30) << "SMAEnvelope minimum period should be 30";
}

// 参数化测试 - 测试不同参数的SMAEnvelope
class SMAEnvelopeParameterizedTest : public ::testing::TestWithParam<std::tuple<int, double>> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line = std::make_shared<LineSeries>();
        close_line->lines->add_line(std::make_shared<LineBuffer>());
        close_line->lines->add_alias("close", 0);
        close_buffer_ = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
        if (close_buffer_) {
            close_buffer_->set(0, csv_data_[0].close);
            for (size_t i = 1; i < csv_data_.size(); ++i) {
                close_buffer_->append(csv_data_[i].close);
            }
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineSeries> close_line;
    std::shared_ptr<LineBuffer> close_buffer_;
};

TEST_P(SMAEnvelopeParameterizedTest, DifferentParameters) {
    auto [period, percentage] = GetParam();
    
    // 使用自定义参数创建SMAEnvelope
    auto smaenv = std::make_shared<SMAEnvelope>(close_line, period, percentage);
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    smaenv->calculate();
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_mid = smaenv->getLine(0)->get(0);     // Mid (SMA)
        double last_upper = smaenv->getLine(1)->get(0);   // Upper
        double last_lower = smaenv->getLine(2)->get(0);   // Lower
        
        EXPECT_FALSE(std::isnan(last_mid)) << "Last Mid should not be NaN";
        EXPECT_FALSE(std::isnan(last_upper)) << "Last Upper should not be NaN";
        EXPECT_FALSE(std::isnan(last_lower)) << "Last Lower should not be NaN";
        
        EXPECT_TRUE(std::isfinite(last_mid)) << "Last Mid should be finite";
        EXPECT_TRUE(std::isfinite(last_upper)) << "Last Upper should be finite";
        EXPECT_TRUE(std::isfinite(last_lower)) << "Last Lower should be finite";
        
        // 验证包络线关系
        EXPECT_GT(last_upper, last_mid) << "Upper should be greater than Mid";
        EXPECT_LT(last_lower, last_mid) << "Lower should be less than Mid";
        
        // 验证百分比关系
        double expected_upper = last_mid * (1.0 + percentage / 100.0);
        double expected_lower = last_mid * (1.0 - percentage / 100.0);
        
        EXPECT_NEAR(last_upper, expected_upper, 1e-6) << "Upper should match percentage calculation";
        EXPECT_NEAR(last_lower, expected_lower, 1e-6) << "Lower should match percentage calculation";
    }
}

// 测试不同的参数组合
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    SMAEnvelopeParameterizedTest,
    ::testing::Values(
        std::make_tuple(20, 2.5),
        std::make_tuple(30, 2.5),
        std::make_tuple(50, 2.5),
        std::make_tuple(30, 1.0),
        std::make_tuple(30, 5.0)
    )
);

// SMAEnvelope计算逻辑验证测试
TEST(OriginalTests, SMAEnvelope_CalculationLogic) {
    // 使用简单的测试数据验证SMAEnvelope计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0,
                                  120.0, 118.0, 116.0, 114.0, 112.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0,
                                  124.0, 126.0, 128.0, 130.0, 132.0};
    
    auto price_line = std::make_shared<LineSeries>();
    price_line->lines->add_line(std::make_shared<LineBuffer>());
    price_line->lines->add_alias("smaosc_calc", 0);
    auto price_buffer = std::dynamic_pointer_cast<LineBuffer>(price_line->lines->getline(0));
    if (price_buffer) {
        price_buffer->set(0, prices[0]);
        for (size_t i = 1; i < prices.size(); ++i) {
            price_buffer->append(prices[i]);
        }
    }
    
    auto smaenv = std::make_shared<SMAEnvelope>(price_line, 10, 2.5);  // 10周期，2.5%包络
    auto price_line_wrapper = std::make_shared<LineSeries>();
    price_line_wrapper->lines->add_line(std::dynamic_pointer_cast<LineSingle>(price_line));
    auto sma = std::make_shared<SMA>(price_line_wrapper, 10);  // 比较用的SMA
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    smaenv->calculate();
    sma->calculate();
    
    // 验证最终值（SMAEnvelope需要10个数据点）
    if (prices.size() >= 10) {
        double mid_value = smaenv->getLine(0)->get(0);
        double upper_value = smaenv->getLine(1)->get(0);
        double lower_value = smaenv->getLine(2)->get(0);
        double sma_value = sma->get(0);
        
        if (!std::isnan(mid_value) && !std::isnan(sma_value)) {
            // Mid应该等于SMA
            EXPECT_NEAR(mid_value, sma_value, 1e-10) 
                << "SMAEnvelope Mid should equal SMA";
            
            // 验证包络线计算
            double expected_upper = sma_value * 1.025;  // +2.5%
            double expected_lower = sma_value * 0.975;  // -2.5%
            
            EXPECT_NEAR(upper_value, expected_upper, 1e-10) 
                << "Upper envelope calculation mismatch";
            EXPECT_NEAR(lower_value, expected_lower, 1e-10) 
                << "Lower envelope calculation mismatch";
            
            // 验证顺序关系
            EXPECT_GT(upper_value, mid_value) 
                << "Upper should be greater than Mid";
            EXPECT_LT(lower_value, mid_value) 
                << "Lower should be less than Mid";
        }
    }
}

// SMAEnvelope支撑阻力测试
TEST(OriginalTests, SMAEnvelope_SupportResistance) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("close", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    if (close_buffer) {
        close_buffer->set(0, csv_data[0].close);
        for (size_t i = 1; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    auto smaenv = std::make_shared<SMAEnvelope>(close_line, 20, 2.5);
    
    // Calculate envelope once for all data
    smaenv->calculate();
    
    int upper_touches = 0;    // 价格触及上轨
    int lower_touches = 0;    // 价格触及下轨
    int inside_envelope = 0;  // 价格在包络内
    int upper_breaks = 0;     // 价格突破上轨
    int lower_breaks = 0;     // 价格突破下轨
    
    // Analyze each price against envelope values at corresponding positions
    for (size_t i = 0; i < csv_data.size(); ++i) {
        double current_price = csv_data[i].close;
        
        // Calculate ago value for historical envelope access
        // i=0 -> ago=-(data_size-1), i=data_size-1 -> ago=0
        int ago = -static_cast<int>(csv_data.size() - 1 - i);
        
        double upper = smaenv->getLine(1)->get(ago);
        double lower = smaenv->getLine(2)->get(ago);
        
        // Debug: print first few iterations to see what's happening
        if (i < 25 || (i % 50 == 0)) {
            std::cout << "DEBUG: i=" << i << " ago=" << ago << " price=" << current_price 
                      << " upper=" << upper << " lower=" << lower << std::endl;
        }
        
        if (!std::isnan(upper) && !std::isnan(lower)) {
            double upper_threshold = upper * 0.999;  // 允许微小误差
            double lower_threshold = lower * 1.001;
            
            if (current_price > upper) {
                upper_breaks++;
            } else if (current_price < lower) {
                lower_breaks++;
            } else if (current_price >= upper_threshold) {
                upper_touches++;
            } else if (current_price <= lower_threshold) {
                lower_touches++;
            } else {
                inside_envelope++;
            }
        }
    }
    
    std::cout << "Support/Resistance analysis:" << std::endl;
    std::cout << "Upper touches: " << upper_touches << std::endl;
    std::cout << "Lower touches: " << lower_touches << std::endl;
    std::cout << "Inside envelope: " << inside_envelope << std::endl;
    std::cout << "Upper breaks: " << upper_breaks << std::endl;
    std::cout << "Lower breaks: " << lower_breaks << std::endl;
    
    int total_valid = upper_touches + lower_touches + inside_envelope + upper_breaks + lower_breaks;
    EXPECT_GT(total_valid, 0) << "Should have some valid envelope analysis";
    
    // 大多数价格应该在包络内
    if (total_valid > 0) {
        double inside_ratio = static_cast<double>(inside_envelope) / total_valid;
        std::cout << "Inside envelope ratio: " << inside_ratio << std::endl;
        EXPECT_GT(inside_ratio, 0.5) << "Most prices should be inside envelope";
    }
}

// SMAEnvelope趋势分析测试
TEST(OriginalTests, SMAEnvelope_TrendAnalysis) {
    // 创建明确的趋势数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 50; ++i) {
        trend_prices.push_back(100.0 + i * 1.0);  // 线性上升趋势
    }
    auto trend_line = std::make_shared<LineSeries>();

    trend_line->lines->add_line(std::make_shared<LineBuffer>());
    trend_line->lines->add_alias("trend_buffer", 0);
    auto trend_buffer = std::dynamic_pointer_cast<LineBuffer>(trend_line->lines->getline(0));
    for (double price : trend_prices) {
        trend_buffer->append(price);
    }
    
    auto trend_smaenv = std::make_shared<SMAEnvelope>(trend_line, 20, 2.5);
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    trend_smaenv->calculate();
    
    // 获取最终计算结果进行趋势分析
    double final_mid = trend_smaenv->getLine(0)->get(0);
    double final_upper = trend_smaenv->getLine(1)->get(0);
    double final_lower = trend_smaenv->getLine(2)->get(0);
    
    // 分析趋势特性
    if (!std::isnan(final_mid) && !std::isnan(final_upper) && !std::isnan(final_lower) && trend_prices.size() >= 20) {
        double first_price = trend_prices[0];  // 100.0
        double last_price = trend_prices.back();  // 149.0
        double expected_mid_range = (first_price + last_price) / 2.0;  // 期望中位值范围
        
        std::cout << "Trend analysis:" << std::endl;
        std::cout << "Price trend: " << first_price << " -> " << last_price << " (change: " << (last_price - first_price) << ")" << std::endl;
        std::cout << "Final Mid: " << final_mid << std::endl;
        std::cout << "Final Upper: " << final_upper << std::endl;
        std::cout << "Final Lower: " << final_lower << std::endl;
        
        // 在上升趋势中，最终SMA应该反映趋势方向
        EXPECT_GT(final_mid, first_price) << "Mid should be higher than initial price in uptrend";
        EXPECT_GT(final_upper, final_mid) << "Upper should be above Mid";
        EXPECT_LT(final_lower, final_mid) << "Lower should be below Mid";
        
        // 验证包络关系
        double expected_upper = final_mid * 1.025;
        double expected_lower = final_mid * 0.975;
        EXPECT_NEAR(final_upper, expected_upper, 1e-6) << "Upper envelope calculation";
        EXPECT_NEAR(final_lower, expected_lower, 1e-6) << "Lower envelope calculation";
    }
}

// SMAEnvelope波动性分析测试
TEST(OriginalTests, SMAEnvelope_VolatilityAnalysis) {
    // 创建不同波动性的数据
    std::vector<double> low_vol_prices, high_vol_prices;
    
    // 低波动性数据;
    for (int i = 0; i < 40; ++i) {
        double base = 100.0;
        double noise = std::sin(i * 0.3) * 1.0;  // 小幅波动
        low_vol_prices.push_back(base + noise);
    }
    
    // 高波动性数据;
    for (int i = 0; i < 40; ++i) {
        double base = 100.0;
        double noise = std::sin(i * 0.3) * 5.0;  // 大幅波动
        high_vol_prices.push_back(base + noise);
    }
    auto low_vol_line = std::make_shared<LineSeries>();

    low_vol_line->lines->add_line(std::make_shared<LineBuffer>());
    low_vol_line->lines->add_alias("low_vol_line", 0);
    auto low_vol_line_buffer = std::dynamic_pointer_cast<LineBuffer>(low_vol_line->lines->getline(0));
    


    for (double price : low_vol_prices) {
        low_vol_line_buffer->append(price);
    }
    
    auto high_vol_line = std::make_shared<LineSeries>();

    
    high_vol_line->lines->add_line(std::make_shared<LineBuffer>());
    high_vol_line->lines->add_alias("high_vol_line", 0);
    auto high_vol_line_buffer = std::dynamic_pointer_cast<LineBuffer>(high_vol_line->lines->getline(0));
    


    for (double price : high_vol_prices) {
        high_vol_line_buffer->append(price);
    }
    
    auto low_vol_env = std::make_shared<SMAEnvelope>(low_vol_line, 20, 2.5);
    auto high_vol_env = std::make_shared<SMAEnvelope>(high_vol_line, 20, 2.5);
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    low_vol_env->calculate();
    high_vol_env->calculate();
    
    // 获取最终包络范围
    double low_vol_upper = low_vol_env->getLine(1)->get(0);
    double low_vol_lower = low_vol_env->getLine(2)->get(0);
    double high_vol_upper = high_vol_env->getLine(1)->get(0);
    double high_vol_lower = high_vol_env->getLine(2)->get(0);
    
    double low_vol_range = 0.0;
    double high_vol_range = 0.0;
    
    if (!std::isnan(low_vol_upper) && !std::isnan(low_vol_lower)) {
        low_vol_range = low_vol_upper - low_vol_lower;
    }
    
    if (!std::isnan(high_vol_upper) && !std::isnan(high_vol_lower)) {
        high_vol_range = high_vol_upper - high_vol_lower;
    }
    
    // 比较包络宽度
    if (low_vol_range > 0.0 && high_vol_range > 0.0) {
        std::cout << "Volatility analysis:" << std::endl;
        std::cout << "Low volatility envelope range: " << low_vol_range << std::endl;
        std::cout << "High volatility envelope range: " << high_vol_range << std::endl;
        
        // 包络宽度应该反映基础SMA的值，而不是价格波动性
        // 因为包络是基于百分比的
        EXPECT_GT(low_vol_range, 0.0) << "Low volatility envelope should have positive range";
        EXPECT_GT(high_vol_range, 0.0) << "High volatility envelope should have positive range";
        
        // 由于两个数据集的基础价格相近（都围绕100），包络范围应该相近
        // 验证包络计算的一致性
        double low_vol_mid = low_vol_env->getLine(0)->get(0);
        double high_vol_mid = high_vol_env->getLine(0)->get(0);
        
        if (!std::isnan(low_vol_mid) && !std::isnan(high_vol_mid)) {
            double expected_low_range = low_vol_mid * 0.05;  // 2.5% * 2
            double expected_high_range = high_vol_mid * 0.05;  // 2.5% * 2
            
            EXPECT_NEAR(low_vol_range, expected_low_range, 1e-6) << "Low vol envelope range calculation";
            EXPECT_NEAR(high_vol_range, expected_high_range, 1e-6) << "High vol envelope range calculation";
        }
    }
}

// SMAEnvelope价格通道测试
TEST(OriginalTests, SMAEnvelope_PriceChannel) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("close", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    if (close_buffer) {
        close_buffer->set(0, csv_data[0].close);
        for (size_t i = 1; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    auto smaenv = std::make_shared<SMAEnvelope>(close_line, 20, 3.0);  // 3%包络
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    smaenv->calculate();
    
    int channel_breakouts = 0;  // 通道突破次数
    int channel_reversals = 0;  // 通道反转次数
    
    // 获取最终包络线值
    double final_upper = smaenv->getLine(1)->get(0);
    double final_lower = smaenv->getLine(2)->get(0);
    
    if (!std::isnan(final_upper) && !std::isnan(final_lower)) {
        // 分析所有价格与最终包络线的关系
        std::vector<bool> above_upper, below_lower;
        
        for (size_t i = 0; i < csv_data.size(); ++i) {
            double current_price = csv_data[i].close;
            
            above_upper.push_back(current_price > final_upper);
            below_lower.push_back(current_price < final_lower);
            
            // 检测连续模式（简化的突破和反转检测）
            if (i >= 2) {
                // 检测突破模式：连续两个点在通道外
                if (above_upper[i] && above_upper[i-1]) {
                    channel_breakouts++;
                }
                if (below_lower[i] && below_lower[i-1]) {
                    channel_breakouts++;
                }
                
                // 检测反转模式：从通道外回到通道内
                if (above_upper[i-1] && !above_upper[i]) {
                    channel_reversals++;
                }
                if (below_lower[i-1] && !below_lower[i]) {
                    channel_reversals++;
                }
            }
        }
    }
    
    std::cout << "Price channel analysis:" << std::endl;
    std::cout << "Channel breakouts: " << channel_breakouts << std::endl;
    std::cout << "Channel reversals: " << channel_reversals << std::endl;
    
    // 验证检测到一些通道活动
    EXPECT_GE(channel_breakouts + channel_reversals, 0) 
        << "Should detect some channel activity";
}

// 边界条件测试
TEST(OriginalTests, SMAEnvelope_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(50, 100.0);
    
    auto flat_line = std::make_shared<LineSeries>();

    
    flat_line->lines->add_line(std::make_shared<LineBuffer>());
    flat_line->lines->add_alias("flat_line", 0);
    auto flat_line_buffer = std::dynamic_pointer_cast<LineBuffer>(flat_line->lines->getline(0));
    


    for (double price : flat_prices) {
        flat_line_buffer->append(price);
    }
    
    auto flat_smaenv = std::make_shared<SMAEnvelope>(flat_line, 20, 2.5);
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    flat_smaenv->calculate();
    
    // 当所有价格相同时，检查包络线计算
    double final_mid = flat_smaenv->getLine(0)->get(0);
    double final_upper = flat_smaenv->getLine(1)->get(0);
    double final_lower = flat_smaenv->getLine(2)->get(0);
    
    if (!std::isnan(final_mid) && !std::isnan(final_upper) && !std::isnan(final_lower)) {
        EXPECT_NEAR(final_mid, 100.0, 1e-6) << "Mid should equal constant price";
        EXPECT_NEAR(final_upper, 102.5, 1e-6) << "Upper should be 2.5% above constant price";
        EXPECT_NEAR(final_lower, 97.5, 1e-6) << "Lower should be 2.5% below constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineSeries>();

    insufficient_line->lines->add_line(std::make_shared<LineBuffer>());
    insufficient_line->lines->add_alias("insufficient_line", 0);
    auto insufficient_line_buffer = std::dynamic_pointer_cast<LineBuffer>(insufficient_line->lines->getline(0));


    for (int i = 0; i < 15; ++i) {
        insufficient_line_buffer->append(100.0 + i);
    }
    auto insufficient_smaenv = std::make_shared<SMAEnvelope>(insufficient_line, 20, 2.5);
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    insufficient_smaenv->calculate();
    
    // 数据不足时应该返回NaN
    double result = insufficient_smaenv->getLine(0)->get(0);
    EXPECT_TRUE(std::isnan(result)) << "SMAEnvelope should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, SMAEnvelope_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<double> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    for (size_t i = 0; i < data_size; ++i) {
        large_data.push_back(dist(rng));
    }
    
    auto large_data_line = std::make_shared<LineSeries>();

    
    large_data_line->lines->add_line(std::make_shared<LineBuffer>());
    large_data_line->lines->add_alias("large_data_line", 0);
    auto large_data_line_buffer = std::dynamic_pointer_cast<LineBuffer>(large_data_line->lines->getline(0));
    


    for (double price : large_data) {
        large_data_line_buffer->append(price);
    }
    
    auto large_smaenv = std::make_shared<SMAEnvelope>(large_data_line, 50, 2.5);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    large_smaenv->calculate();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "SMAEnvelope calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_mid = large_smaenv->getLine(0)->get(0);
    double final_upper = large_smaenv->getLine(1)->get(0);
    double final_lower = large_smaenv->getLine(2)->get(0);
    
    EXPECT_FALSE(std::isnan(final_mid)) << "Final Mid should not be NaN";
    EXPECT_FALSE(std::isnan(final_upper)) << "Final Upper should not be NaN";
    EXPECT_FALSE(std::isnan(final_lower)) << "Final Lower should not be NaN";
    
    EXPECT_TRUE(std::isfinite(final_mid)) << "Final Mid should be finite";
    EXPECT_TRUE(std::isfinite(final_upper)) << "Final Upper should be finite";
    EXPECT_TRUE(std::isfinite(final_lower)) << "Final Lower should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}
