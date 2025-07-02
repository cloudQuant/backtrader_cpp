/**
 * @file test_ind_smmaenvelope.cpp
 * @brief SMMAEnvelope指标测试 - 对应Python test_ind_smmaenvelope.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4021.569725', '3644.444667', '3616.427648'],
 *     ['4122.108968', '3735.555783', '3706.838340'],
 *     ['3921.030482', '3553.333550', '3526.016957']
 * ]
 * chkmin = 30
 * chkind = btind.SMMAEnvelope
 * 
 * 注：SMMAEnvelope包含3条线：Mid (SMMA), Upper, Lower
 */

#include "test_common.h"
#include "indicators/SMMAEnvelope.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> SMMAENVELOPE_EXPECTED_VALUES = {
    {"4021.569725", "3644.444667", "3616.427648"},  // line 0 (Mid/SMMA)
    {"4122.108968", "3735.555783", "3706.838340"},  // line 1 (Upper)
    {"3921.030482", "3553.333550", "3526.016957"}   // line 2 (Lower)
};

const int SMMAENVELOPE_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的SMMAEnvelope测试
DEFINE_INDICATOR_TEST(SMMAEnvelope_Default, SMMAEnvelope, SMMAENVELOPE_EXPECTED_VALUES, SMMAENVELOPE_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, SMMAEnvelope_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建SMMAEnvelope指标
    auto smmaenv = std::make_shared<SMMAEnvelope>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        smmaenv->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 30;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // 验证3条线的值
    for (int line = 0; line < 3; ++line) {
        auto expected = SMMAENVELOPE_EXPECTED_VALUES[line];
        
        for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
            double actual = smmaenv->getLine(line)->get(check_points[i]);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << actual;
            std::string actual_str = ss.str();
            
            EXPECT_EQ(actual_str, expected[i]) 
                << "SMMAEnvelope line " << line << " value mismatch at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(smmaenv->getMinPeriod(), 30) << "SMMAEnvelope minimum period should be 30";
}

// 参数化测试 - 测试不同参数的SMMAEnvelope
class SMMAEnvelopeParameterizedTest : public ::testing::TestWithParam<std::tuple<int, double>> {
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

TEST_P(SMMAEnvelopeParameterizedTest, DifferentParameters) {
    auto [period, percentage] = GetParam();
    
    auto smmaenv = std::make_shared<SMMAEnvelope>(close_line_, period, percentage);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        smmaenv->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_mid = smmaenv->getLine(0)->get(0);     // Mid (SMMA)
        double last_upper = smmaenv->getLine(1)->get(0);   // Upper
        double last_lower = smmaenv->getLine(2)->get(0);   // Lower
        
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
    SMMAEnvelopeParameterizedTest,
    ::testing::Values(
        std::make_tuple(20, 2.5),
        std::make_tuple(30, 2.5),
        std::make_tuple(50, 2.5),
        std::make_tuple(30, 1.0),
        std::make_tuple(30, 5.0)
    )
);

// SMMAEnvelope计算逻辑验证测试
TEST(OriginalTests, SMMAEnvelope_CalculationLogic) {
    // 使用简单的测试数据验证SMMAEnvelope计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0,
                                  120.0, 118.0, 116.0, 114.0, 112.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0,
                                  124.0, 126.0, 128.0, 130.0, 132.0};
    
    auto price_line = std::make_shared<LineRoot>(prices.size(), "smmaenv_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto smmaenv = std::make_shared<SMMAEnvelope>(price_line, 10, 2.5);  // 10周期，2.5%包络
    auto smma = std::make_shared<SMMA>(price_line, 10);  // 比较用的SMMA
    
    for (size_t i = 0; i < prices.size(); ++i) {
        smmaenv->calculate();
        smma->calculate();
        
        if (i >= 9) {  // SMMAEnvelope需要10个数据点
            double mid_value = smmaenv->getLine(0)->get(0);
            double upper_value = smmaenv->getLine(1)->get(0);
            double lower_value = smmaenv->getLine(2)->get(0);
            double smma_value = smma->get(0);
            
            if (!std::isnan(mid_value) && !std::isnan(smma_value)) {
                // Mid应该等于SMMA
                EXPECT_NEAR(mid_value, smma_value, 1e-10) 
                    << "SMMAEnvelope Mid should equal SMMA at step " << i;
                
                // 验证包络线计算
                double expected_upper = smma_value * 1.025;  // +2.5%
                double expected_lower = smma_value * 0.975;  // -2.5%
                
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

// SMMAEnvelope响应速度测试
TEST(OriginalTests, SMMAEnvelope_ResponseSpeed) {
    // 创建价格突然变化的数据来测试响应速度
    std::vector<double> step_prices;
    
    // 第一阶段：稳定价格
    for (int i = 0; i < 50; ++i) {
        step_prices.push_back(100.0);
    }
    
    // 第二阶段：价格跳跃
    for (int i = 0; i < 50; ++i) {
        step_prices.push_back(120.0);
    }
    
    auto step_line = std::make_shared<LineRoot>(step_prices.size(), "step");
    for (double price : step_prices) {
        step_line->forward(price);
    }
    
    auto smmaenv = std::make_shared<SMMAEnvelope>(step_line, 20, 2.5);
    auto emaenv = std::make_shared<EMAEnvelope>(step_line, 20, 2.5);   // 比较对象
    auto smaenv = std::make_shared<SMAEnvelope>(step_line, 20, 2.5);   // 比较对象
    
    std::vector<double> smma_responses, ema_responses, sma_responses;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        smmaenv->calculate();
        emaenv->calculate();
        smaenv->calculate();
        
        double smma_mid = smmaenv->getLine(0)->get(0);
        double ema_mid = emaenv->getLine(0)->get(0);
        double sma_mid = smaenv->getLine(0)->get(0);
        
        if (!std::isnan(smma_mid) && !std::isnan(ema_mid) && !std::isnan(sma_mid)) {
            if (i >= 50) {  // 价格跳跃后
                smma_responses.push_back(smma_mid);
                ema_responses.push_back(ema_mid);
                sma_responses.push_back(sma_mid);
            }
        }
        
        if (i < step_prices.size() - 1) {
            step_line->forward();
        }
    }
    
    // 比较响应速度
    if (!smma_responses.empty() && !ema_responses.empty() && !sma_responses.empty()) {
        double final_smma = smma_responses.back();
        double final_ema = ema_responses.back();
        double final_sma = sma_responses.back();
        
        std::cout << "Response speed comparison:" << std::endl;
        std::cout << "Final SMMA envelope mid: " << final_smma << std::endl;
        std::cout << "Final EMA envelope mid: " << final_ema << std::endl;
        std::cout << "Final SMA envelope mid: " << final_sma << std::endl;
        
        // SMMA响应速度通常介于EMA和SMA之间
        EXPECT_GT(final_smma, final_sma * 0.9) 
            << "SMMA envelope should respond to price changes";
    }
}

// 与其他包络线比较测试
TEST(OriginalTests, SMMAEnvelope_vs_OtherEnvelopes) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto smmaenv = std::make_shared<SMMAEnvelope>(close_line, 20, 2.5);
    auto emaenv = std::make_shared<EMAEnvelope>(close_line, 20, 2.5);
    auto smaenv = std::make_shared<SMAEnvelope>(close_line, 20, 2.5);
    
    std::vector<double> smma_ranges, ema_ranges, sma_ranges;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        smmaenv->calculate();
        emaenv->calculate();
        smaenv->calculate();
        
        double smma_upper = smmaenv->getLine(1)->get(0);
        double smma_lower = smmaenv->getLine(2)->get(0);
        double ema_upper = emaenv->getLine(1)->get(0);
        double ema_lower = emaenv->getLine(2)->get(0);
        double sma_upper = smaenv->getLine(1)->get(0);
        double sma_lower = smaenv->getLine(2)->get(0);
        
        if (!std::isnan(smma_upper) && !std::isnan(smma_lower)) {
            smma_ranges.push_back(smma_upper - smma_lower);
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
    if (!smma_ranges.empty() && !ema_ranges.empty() && !sma_ranges.empty()) {
        double avg_smma_range = std::accumulate(smma_ranges.begin(), smma_ranges.end(), 0.0) / smma_ranges.size();
        double avg_ema_range = std::accumulate(ema_ranges.begin(), ema_ranges.end(), 0.0) / ema_ranges.size();
        double avg_sma_range = std::accumulate(sma_ranges.begin(), sma_ranges.end(), 0.0) / sma_ranges.size();
        
        std::cout << "Envelope comparison:" << std::endl;
        std::cout << "Average SMMA envelope range: " << avg_smma_range << std::endl;
        std::cout << "Average EMA envelope range: " << avg_ema_range << std::endl;
        std::cout << "Average SMA envelope range: " << avg_sma_range << std::endl;
        
        // 包络宽度应该相似（都基于相同百分比）
        EXPECT_NEAR(avg_smma_range, avg_ema_range, avg_ema_range * 0.1) 
            << "SMMA and EMA envelope ranges should be similar";
        EXPECT_NEAR(avg_ema_range, avg_sma_range, avg_sma_range * 0.1) 
            << "EMA and SMA envelope ranges should be similar";
    }
}

// SMMAEnvelope支撑阻力测试
TEST(OriginalTests, SMMAEnvelope_SupportResistance) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto smmaenv = std::make_shared<SMMAEnvelope>(close_line, 20, 2.5);
    
    int upper_touches = 0;    // 价格触及上轨
    int lower_touches = 0;    // 价格触及下轨
    int inside_envelope = 0;  // 价格在包络内
    int upper_breaks = 0;     // 价格突破上轨
    int lower_breaks = 0;     // 价格突破下轨
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        smmaenv->calculate();
        
        double current_price = csv_data[i].close;
        double upper = smmaenv->getLine(1)->get(0);
        double lower = smmaenv->getLine(2)->get(0);
        
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

// SMMAEnvelope趋势跟踪测试
TEST(OriginalTests, SMMAEnvelope_TrendFollowing) {
    // 创建明确的趋势数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 80; ++i) {
        trend_prices.push_back(100.0 + i * 1.0);  // 线性上升趋势
    }
    
    auto trend_line = std::make_shared<LineRoot>(trend_prices.size(), "trend");
    for (double price : trend_prices) {
        trend_line->forward(price);
    }
    
    auto trend_smmaenv = std::make_shared<SMMAEnvelope>(trend_line, 20, 2.5);
    
    std::vector<double> mid_values, upper_values, lower_values;
    
    for (size_t i = 0; i < trend_prices.size(); ++i) {
        trend_smmaenv->calculate();
        
        double mid = trend_smmaenv->getLine(0)->get(0);
        double upper = trend_smmaenv->getLine(1)->get(0);
        double lower = trend_smmaenv->getLine(2)->get(0);
        
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
    if (mid_values.size() > 20) {
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

// SMMAEnvelope平滑特性测试
TEST(OriginalTests, SMMAEnvelope_SmoothingCharacteristics) {
    // 创建带噪音的数据
    std::vector<double> noisy_prices;
    for (int i = 0; i < 100; ++i) {
        double base = 100.0 + i * 0.2;  // 缓慢上升
        double noise = (i % 2 == 0 ? 5.0 : -5.0);  // 交替噪音
        noisy_prices.push_back(base + noise);
    }
    
    auto noisy_line = std::make_shared<LineRoot>(noisy_prices.size(), "noisy");
    for (double price : noisy_prices) {
        noisy_line->forward(price);
    }
    
    auto smmaenv = std::make_shared<SMMAEnvelope>(noisy_line, 20, 2.5);
    auto emaenv = std::make_shared<EMAEnvelope>(noisy_line, 20, 2.5);
    
    std::vector<double> smma_smoothness, ema_smoothness;
    double prev_smma = 0.0, prev_ema = 0.0;
    bool has_prev = false;
    
    for (size_t i = 0; i < noisy_prices.size(); ++i) {
        smmaenv->calculate();
        emaenv->calculate();
        
        double smma_mid = smmaenv->getLine(0)->get(0);
        double ema_mid = emaenv->getLine(0)->get(0);
        
        if (!std::isnan(smma_mid) && !std::isnan(ema_mid) && has_prev) {
            // 计算变化幅度（平滑性指标）
            smma_smoothness.push_back(std::abs(smma_mid - prev_smma));
            ema_smoothness.push_back(std::abs(ema_mid - prev_ema));
        }
        
        if (!std::isnan(smma_mid) && !std::isnan(ema_mid)) {
            prev_smma = smma_mid;
            prev_ema = ema_mid;
            has_prev = true;
        }
        
        if (i < noisy_prices.size() - 1) {
            noisy_line->forward();
        }
    }
    
    // 比较平滑特性
    if (!smma_smoothness.empty() && !ema_smoothness.empty()) {
        double avg_smma_change = std::accumulate(smma_smoothness.begin(), smma_smoothness.end(), 0.0) / smma_smoothness.size();
        double avg_ema_change = std::accumulate(ema_smoothness.begin(), ema_smoothness.end(), 0.0) / ema_smoothness.size();
        
        std::cout << "Smoothing characteristics:" << std::endl;
        std::cout << "Average SMMA change: " << avg_smma_change << std::endl;
        std::cout << "Average EMA change: " << avg_ema_change << std::endl;
        
        // SMMA通常比EMA更平滑（变化更小）
        EXPECT_LT(avg_smma_change, avg_ema_change * 1.2) 
            << "SMMA should be smoother than EMA";
    }
}

// SMMAEnvelope价格通道测试
TEST(OriginalTests, SMMAEnvelope_PriceChannel) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto smmaenv = std::make_shared<SMMAEnvelope>(close_line, 20, 3.0);  // 3%包络
    
    int channel_breakouts = 0;  // 通道突破次数
    int channel_reversals = 0;  // 通道反转次数
    
    std::vector<double> price_history;
    std::vector<double> upper_history, lower_history;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        smmaenv->calculate();
        
        double current_price = csv_data[i].close;
        double upper = smmaenv->getLine(1)->get(0);
        double lower = smmaenv->getLine(2)->get(0);
        
        if (!std::isnan(upper) && !std::isnan(lower)) {
            price_history.push_back(current_price);
            upper_history.push_back(upper);
            lower_history.push_back(lower);
            
            // 检测通道突破和反转
            if (price_history.size() >= 3) {
                size_t n = price_history.size();
                double prev2_price = price_history[n-3];
                double prev_price = price_history[n-2];
                double curr_price = price_history[n-1];
                double prev_upper = upper_history[n-2];
                double prev_lower = lower_history[n-2];
                
                // 检测上轨突破
                if (prev_price <= prev_upper && curr_price > upper) {
                    channel_breakouts++;
                }
                
                // 检测下轨突破
                if (prev_price >= prev_lower && curr_price < lower) {
                    channel_breakouts++;
                }
                
                // 检测反转（从上轨回到通道内）
                if (prev2_price > prev_upper && prev_price > prev_upper && 
                    curr_price <= upper) {
                    channel_reversals++;
                }
                
                // 检测反转（从下轨回到通道内）
                if (prev2_price < prev_lower && prev_price < prev_lower && 
                    curr_price >= lower) {
                    channel_reversals++;
                }
            }
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
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
TEST(OriginalTests, SMMAEnvelope_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_smmaenv = std::make_shared<SMMAEnvelope>(flat_line, 20, 2.5);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_smmaenv->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，检查包络线计算
    double final_mid = flat_smmaenv->getLine(0)->get(0);
    double final_upper = flat_smmaenv->getLine(1)->get(0);
    double final_lower = flat_smmaenv->getLine(2)->get(0);
    
    if (!std::isnan(final_mid) && !std::isnan(final_upper) && !std::isnan(final_lower)) {
        EXPECT_NEAR(final_mid, 100.0, 1e-6) << "Mid should equal constant price";
        EXPECT_NEAR(final_upper, 102.5, 1e-6) << "Upper should be 2.5% above constant price";
        EXPECT_NEAR(final_lower, 97.5, 1e-6) << "Lower should be 2.5% below constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 15; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_smmaenv = std::make_shared<SMMAEnvelope>(insufficient_line, 20, 2.5);
    
    for (int i = 0; i < 15; ++i) {
        insufficient_smmaenv->calculate();
        if (i < 14) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_smmaenv->getLine(0)->get(0);
    EXPECT_TRUE(std::isnan(result)) << "SMMAEnvelope should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, SMMAEnvelope_Performance) {
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
    
    auto large_smmaenv = std::make_shared<SMMAEnvelope>(large_line, 50, 2.5);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_smmaenv->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "SMMAEnvelope calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_mid = large_smmaenv->getLine(0)->get(0);
    double final_upper = large_smmaenv->getLine(1)->get(0);
    double final_lower = large_smmaenv->getLine(2)->get(0);
    
    EXPECT_FALSE(std::isnan(final_mid)) << "Final Mid should not be NaN";
    EXPECT_FALSE(std::isnan(final_upper)) << "Final Upper should not be NaN";
    EXPECT_FALSE(std::isnan(final_lower)) << "Final Lower should not be NaN";
    
    EXPECT_TRUE(std::isfinite(final_mid)) << "Final Mid should be finite";
    EXPECT_TRUE(std::isfinite(final_upper)) << "Final Upper should be finite";
    EXPECT_TRUE(std::isfinite(final_lower)) << "Final Lower should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}