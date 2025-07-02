/**
 * @file test_ind_temaenvelope.cpp
 * @brief TEMAEnvelope指标测试 - 对应Python test_ind_temaenvelope.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4113.721705', '3862.386854', '3832.691054'],
 *     ['4216.564748', '3958.946525', '3928.508331'],
 *     ['4010.878663', '3765.827182', '3736.873778']
 * ]
 * chkmin = 88
 * chkind = btind.TEMAEnvelope
 * 
 * 注：TEMAEnvelope包含3条线：Mid (TEMA), Upper, Lower
 */

#include "test_common.h"
#include "indicators/TEMAEnvelope.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> TEMAENVELOPE_EXPECTED_VALUES = {
    {"4113.721705", "3862.386854", "3832.691054"},  // line 0 (Mid/TEMA)
    {"4216.564748", "3958.946525", "3928.508331"},  // line 1 (Upper)
    {"4010.878663", "3765.827182", "3736.873778"}   // line 2 (Lower)
};

const int TEMAENVELOPE_MIN_PERIOD = 88;

} // anonymous namespace

// 使用默认参数的TEMAEnvelope测试
DEFINE_INDICATOR_TEST(TEMAEnvelope_Default, TEMAEnvelope, TEMAENVELOPE_EXPECTED_VALUES, TEMAENVELOPE_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, TEMAEnvelope_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建TEMAEnvelope指标
    auto temaenv = std::make_shared<TEMAEnvelope>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        temaenv->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 88;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // 验证3条线的值
    for (int line = 0; line < 3; ++line) {
        auto expected = TEMAENVELOPE_EXPECTED_VALUES[line];
        
        for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
            double actual = temaenv->getLine(line)->get(check_points[i]);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << actual;
            std::string actual_str = ss.str();
            
            EXPECT_EQ(actual_str, expected[i]) 
                << "TEMAEnvelope line " << line << " value mismatch at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(temaenv->getMinPeriod(), 88) << "TEMAEnvelope minimum period should be 88";
}

// 参数化测试 - 测试不同参数的TEMAEnvelope
class TEMAEnvelopeParameterizedTest : public ::testing::TestWithParam<std::tuple<int, double>> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line_ = std::make_shared<LineRoot>(csv_data_.size(), "close");
        for (const auto& bar : csv_data_) {
            close_line_->forward(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineRoot> close_line_;
};

TEST_P(TEMAEnvelopeParameterizedTest, DifferentParameters) {
    auto [period, percentage] = GetParam();
    
    auto temaenv = std::make_shared<TEMAEnvelope>(close_line_, period, percentage);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        temaenv->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最后的值
    int expected_min_period = (period - 1) * 3 + 1;  // TEMA的最小周期计算
    EXPECT_EQ(temaenv->getMinPeriod(), expected_min_period) 
        << "TEMAEnvelope minimum period should match TEMA calculation";
    
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_mid = temaenv->getLine(0)->get(0);     // Mid (TEMA)
        double last_upper = temaenv->getLine(1)->get(0);   // Upper
        double last_lower = temaenv->getLine(2)->get(0);   // Lower
        
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
    TEMAEnvelopeParameterizedTest,
    ::testing::Values(
        std::make_tuple(20, 2.5),
        std::make_tuple(30, 2.5),
        std::make_tuple(50, 2.5),
        std::make_tuple(30, 1.0),
        std::make_tuple(30, 5.0)
    )
);

// TEMAEnvelope计算逻辑验证测试
TEST(OriginalTests, TEMAEnvelope_CalculationLogic) {
    // 使用简单的测试数据验证TEMAEnvelope计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0,
                                  120.0, 118.0, 116.0, 114.0, 112.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0};
    
    // 扩展数据以满足TEMA的最小周期要求
    for (int i = 0; i < 70; ++i) {
        prices.push_back(122.0 + i * 0.1);
    }
    
    auto price_line = std::make_shared<LineRoot>(prices.size(), "temaenv_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto temaenv = std::make_shared<TEMAEnvelope>(price_line, 10, 2.5);  // 10周期，2.5%包络
    auto tema = std::make_shared<TEMA>(price_line, 10);  // 比较用的TEMA
    
    for (size_t i = 0; i < prices.size(); ++i) {
        temaenv->calculate();
        tema->calculate();
        
        if (i >= 28) {  // TEMAEnvelope需要29个数据点（10周期的TEMA）
            double mid_value = temaenv->getLine(0)->get(0);
            double upper_value = temaenv->getLine(1)->get(0);
            double lower_value = temaenv->getLine(2)->get(0);
            double tema_value = tema->get(0);
            
            if (!std::isnan(mid_value) && !std::isnan(tema_value)) {
                // Mid应该等于TEMA
                EXPECT_NEAR(mid_value, tema_value, 1e-10) 
                    << "TEMAEnvelope Mid should equal TEMA at step " << i;
                
                // 验证包络线计算
                double expected_upper = tema_value * 1.025;  // +2.5%
                double expected_lower = tema_value * 0.975;  // -2.5%
                
                EXPECT_NEAR(upper_value, expected_upper, 1e-10) 
                    << "Upper envelope calculation mismatch at step " << i;
                EXPECT_NEAR(lower_value, expected_lower, 1e-10) 
                    << "Lower envelope calculation mismatch at step " << i;
                
                // 验证顺序关系
                EXPECT_GT(upper_value, mid_value) 
                    << "Upper should be greater than Mid at step " << i;
                EXPECT_LT(lower_value, mid_value) 
                    << "Lower should be less than Mid at step " << i;
            }
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
}

// TEMAEnvelope响应速度测试
TEST(OriginalTests, TEMAEnvelope_ResponseSpeed) {
    // 创建价格突然变化的数据来测试响应速度
    std::vector<double> step_prices;
    
    // 第一阶段：稳定价格
    for (int i = 0; i < 100; ++i) {
        step_prices.push_back(100.0);
    }
    
    // 第二阶段：价格跳跃
    for (int i = 0; i < 100; ++i) {
        step_prices.push_back(120.0);
    }
    
    auto step_line = std::make_shared<LineRoot>(step_prices.size(), "step");
    for (double price : step_prices) {
        step_line->forward(price);
    }
    
    auto temaenv = std::make_shared<TEMAEnvelope>(step_line, 20, 2.5);
    auto emaenv = std::make_shared<EMAEnvelope>(step_line, 20, 2.5);   // 比较对象
    auto smaenv = std::make_shared<SMAEnvelope>(step_line, 20, 2.5);   // 比较对象
    
    std::vector<double> tema_responses, ema_responses, sma_responses;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        temaenv->calculate();
        emaenv->calculate();
        smaenv->calculate();
        
        double tema_mid = temaenv->getLine(0)->get(0);
        double ema_mid = emaenv->getLine(0)->get(0);
        double sma_mid = smaenv->getLine(0)->get(0);
        
        if (!std::isnan(tema_mid) && !std::isnan(ema_mid) && !std::isnan(sma_mid)) {
            if (i >= 100) {  // 价格跳跃后
                tema_responses.push_back(tema_mid);
                ema_responses.push_back(ema_mid);
                sma_responses.push_back(sma_mid);
            }
        }
        
        if (i < step_prices.size() - 1) {
            step_line->forward();
        }
    }
    
    // 比较响应速度
    if (!tema_responses.empty() && !ema_responses.empty() && !sma_responses.empty()) {
        double final_tema = tema_responses.back();
        double final_ema = ema_responses.back();
        double final_sma = sma_responses.back();
        
        std::cout << "Response speed comparison:" << std::endl;
        std::cout << "Final TEMA envelope mid: " << final_tema << std::endl;
        std::cout << "Final EMA envelope mid: " << final_ema << std::endl;
        std::cout << "Final SMA envelope mid: " << final_sma << std::endl;
        
        // TEMA通常比EMA和SMA响应更快
        EXPECT_GT(final_tema, final_sma * 0.9) 
            << "TEMA envelope should respond to price changes";
    }
}

// 与其他包络线比较测试
TEST(OriginalTests, TEMAEnvelope_vs_OtherEnvelopes) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto temaenv = std::make_shared<TEMAEnvelope>(close_line, 20, 2.5);
    auto emaenv = std::make_shared<EMAEnvelope>(close_line, 20, 2.5);
    auto smaenv = std::make_shared<SMAEnvelope>(close_line, 20, 2.5);
    
    std::vector<double> tema_ranges, ema_ranges, sma_ranges;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        temaenv->calculate();
        emaenv->calculate();
        smaenv->calculate();
        
        double tema_upper = temaenv->getLine(1)->get(0);
        double tema_lower = temaenv->getLine(2)->get(0);
        double ema_upper = emaenv->getLine(1)->get(0);
        double ema_lower = emaenv->getLine(2)->get(0);
        double sma_upper = smaenv->getLine(1)->get(0);
        double sma_lower = smaenv->getLine(2)->get(0);
        
        if (!std::isnan(tema_upper) && !std::isnan(tema_lower)) {
            tema_ranges.push_back(tema_upper - tema_lower);
        }
        
        if (!std::isnan(ema_upper) && !std::isnan(ema_lower)) {
            ema_ranges.push_back(ema_upper - ema_lower);
        }
        
        if (!std::isnan(sma_upper) && !std::isnan(sma_lower)) {
            sma_ranges.push_back(sma_upper - sma_lower);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 比较包络特性
    if (!tema_ranges.empty() && !ema_ranges.empty() && !sma_ranges.empty()) {
        double avg_tema_range = std::accumulate(tema_ranges.begin(), tema_ranges.end(), 0.0) / tema_ranges.size();
        double avg_ema_range = std::accumulate(ema_ranges.begin(), ema_ranges.end(), 0.0) / ema_ranges.size();
        double avg_sma_range = std::accumulate(sma_ranges.begin(), sma_ranges.end(), 0.0) / sma_ranges.size();
        
        std::cout << "Envelope comparison:" << std::endl;
        std::cout << "Average TEMA envelope range: " << avg_tema_range << std::endl;
        std::cout << "Average EMA envelope range: " << avg_ema_range << std::endl;
        std::cout << "Average SMA envelope range: " << avg_sma_range << std::endl;
        
        // 包络宽度应该相似（都基于相同百分比）
        EXPECT_NEAR(avg_tema_range, avg_ema_range, avg_ema_range * 0.1) 
            << "TEMA and EMA envelope ranges should be similar";
        EXPECT_NEAR(avg_ema_range, avg_sma_range, avg_sma_range * 0.1) 
            << "EMA and SMA envelope ranges should be similar";
    }
}

// TEMAEnvelope支撑阻力测试
TEST(OriginalTests, TEMAEnvelope_SupportResistance) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto temaenv = std::make_shared<TEMAEnvelope>(close_line, 20, 2.5);
    
    int upper_touches = 0;    // 价格触及上轨
    int lower_touches = 0;    // 价格触及下轨
    int inside_envelope = 0;  // 价格在包络内
    int upper_breaks = 0;     // 价格突破上轨
    int lower_breaks = 0;     // 价格突破下轨
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        temaenv->calculate();
        
        double current_price = csv_data[i].close;
        double upper = temaenv->getLine(1)->get(0);
        double lower = temaenv->getLine(2)->get(0);
        
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
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
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

// TEMAEnvelope趋势跟踪测试
TEST(OriginalTests, TEMAEnvelope_TrendFollowing) {
    // 创建明确的趋势数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 150; ++i) {
        trend_prices.push_back(100.0 + i * 1.0);  // 线性上升趋势
    }
    
    auto trend_line = std::make_shared<LineRoot>(trend_prices.size(), "trend");
    for (double price : trend_prices) {
        trend_line->forward(price);
    }
    
    auto trend_temaenv = std::make_shared<TEMAEnvelope>(trend_line, 20, 2.5);
    
    std::vector<double> mid_values, upper_values, lower_values;
    
    for (size_t i = 0; i < trend_prices.size(); ++i) {
        trend_temaenv->calculate();
        
        double mid = trend_temaenv->getLine(0)->get(0);
        double upper = trend_temaenv->getLine(1)->get(0);
        double lower = trend_temaenv->getLine(2)->get(0);
        
        if (!std::isnan(mid) && !std::isnan(upper) && !std::isnan(lower)) {
            mid_values.push_back(mid);
            upper_values.push_back(upper);
            lower_values.push_back(lower);
        }
        
        if (i < trend_prices.size() - 1) {
            trend_line->forward();
        }
    }
    
    // 分析趋势跟踪特性
    if (mid_values.size() > 50) {
        double first_mid = mid_values[0];
        double last_mid = mid_values.back();
        double first_upper = upper_values[0];
        double last_upper = upper_values.back();
        double first_lower = lower_values[0];
        double last_lower = lower_values.back();
        
        std::cout << "Trend following analysis:" << std::endl;
        std::cout << "Mid: " << first_mid << " -> " << last_mid << " (change: " << (last_mid - first_mid) << ")" << std::endl;
        std::cout << "Upper: " << first_upper << " -> " << last_upper << " (change: " << (last_upper - first_upper) << ")" << std::endl;
        std::cout << "Lower: " << first_lower << " -> " << last_lower << " (change: " << (last_lower - first_lower) << ")" << std::endl;
        
        // 在上升趋势中，所有线都应该上升
        EXPECT_GT(last_mid, first_mid) << "Mid should rise in uptrend";
        EXPECT_GT(last_upper, first_upper) << "Upper should rise in uptrend";
        EXPECT_GT(last_lower, first_lower) << "Lower should rise in uptrend";
    }
}

// TEMAEnvelope平滑特性测试
TEST(OriginalTests, TEMAEnvelope_SmoothingCharacteristics) {
    // 创建带噪音的数据
    std::vector<double> noisy_prices;
    for (int i = 0; i < 150; ++i) {
        double base = 100.0 + i * 0.2;  // 缓慢上升
        double noise = (i % 2 == 0 ? 5.0 : -5.0);  // 交替噪音
        noisy_prices.push_back(base + noise);
    }
    
    auto noisy_line = std::make_shared<LineRoot>(noisy_prices.size(), "noisy");
    for (double price : noisy_prices) {
        noisy_line->forward(price);
    }
    
    auto temaenv = std::make_shared<TEMAEnvelope>(noisy_line, 20, 2.5);
    auto emaenv = std::make_shared<EMAEnvelope>(noisy_line, 20, 2.5);
    
    std::vector<double> tema_smoothness, ema_smoothness;
    double prev_tema = 0.0, prev_ema = 0.0;
    bool has_prev = false;
    
    for (size_t i = 0; i < noisy_prices.size(); ++i) {
        temaenv->calculate();
        emaenv->calculate();
        
        double tema_mid = temaenv->getLine(0)->get(0);
        double ema_mid = emaenv->getLine(0)->get(0);
        
        if (!std::isnan(tema_mid) && !std::isnan(ema_mid) && has_prev) {
            // 计算变化幅度（平滑性指标）
            tema_smoothness.push_back(std::abs(tema_mid - prev_tema));
            ema_smoothness.push_back(std::abs(ema_mid - prev_ema));
        }
        
        if (!std::isnan(tema_mid) && !std::isnan(ema_mid)) {
            prev_tema = tema_mid;
            prev_ema = ema_mid;
            has_prev = true;
        }
        
        if (i < noisy_prices.size() - 1) {
            noisy_line->forward();
        }
    }
    
    // 比较平滑特性
    if (!tema_smoothness.empty() && !ema_smoothness.empty()) {
        double avg_tema_change = std::accumulate(tema_smoothness.begin(), tema_smoothness.end(), 0.0) / tema_smoothness.size();
        double avg_ema_change = std::accumulate(ema_smoothness.begin(), ema_smoothness.end(), 0.0) / ema_smoothness.size();
        
        std::cout << "Smoothing characteristics:" << std::endl;
        std::cout << "Average TEMA change: " << avg_tema_change << std::endl;
        std::cout << "Average EMA change: " << avg_ema_change << std::endl;
        
        // TEMA通常比EMA更快响应但仍然平滑
        EXPECT_GT(avg_tema_change, avg_ema_change * 0.5) 
            << "TEMA should be responsive to changes";
    }
}

// 边界条件测试
TEST(OriginalTests, TEMAEnvelope_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(150, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_temaenv = std::make_shared<TEMAEnvelope>(flat_line, 20, 2.5);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_temaenv->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，检查包络线计算
    double final_mid = flat_temaenv->getLine(0)->get(0);
    double final_upper = flat_temaenv->getLine(1)->get(0);
    double final_lower = flat_temaenv->getLine(2)->get(0);
    
    if (!std::isnan(final_mid) && !std::isnan(final_upper) && !std::isnan(final_lower)) {
        EXPECT_NEAR(final_mid, 100.0, 1e-6) << "Mid should equal constant price";
        EXPECT_NEAR(final_upper, 102.5, 1e-6) << "Upper should be 2.5% above constant price";
        EXPECT_NEAR(final_lower, 97.5, 1e-6) << "Lower should be 2.5% below constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 50; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_temaenv = std::make_shared<TEMAEnvelope>(insufficient_line, 20, 2.5);
    
    for (int i = 0; i < 50; ++i) {
        insufficient_temaenv->calculate();
        if (i < 49) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_temaenv->getLine(0)->get(0);
    EXPECT_TRUE(std::isnan(result)) << "TEMAEnvelope should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, TEMAEnvelope_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<double> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        large_data.push_back(dist(rng));
    }
    
    auto large_line = std::make_shared<LineRoot>(large_data.size(), "large");
    for (double price : large_data) {
        large_line->forward(price);
    }
    
    auto large_temaenv = std::make_shared<TEMAEnvelope>(large_line, 50, 2.5);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_temaenv->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "TEMAEnvelope calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_mid = large_temaenv->getLine(0)->get(0);
    double final_upper = large_temaenv->getLine(1)->get(0);
    double final_lower = large_temaenv->getLine(2)->get(0);
    
    EXPECT_FALSE(std::isnan(final_mid)) << "Final Mid should not be NaN";
    EXPECT_FALSE(std::isnan(final_upper)) << "Final Upper should not be NaN";
    EXPECT_FALSE(std::isnan(final_lower)) << "Final Lower should not be NaN";
    
    EXPECT_TRUE(std::isfinite(final_mid)) << "Final Mid should be finite";
    EXPECT_TRUE(std::isfinite(final_upper)) << "Final Upper should be finite";
    EXPECT_TRUE(std::isfinite(final_lower)) << "Final Lower should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}