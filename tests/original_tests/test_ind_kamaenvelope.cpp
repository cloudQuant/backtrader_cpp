/**
 * @file test_ind_kamaenvelope.cpp
 * @brief KAMAEnvelope指标测试 - 对应Python test_ind_kamaenvelope.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4063.463000', '3644.444667', '3554.693333'],
 *     ['4165.049575', '3735.555783', '3643.560667'],
 *     ['3961.876425', '3553.333550', '3465.826000']
 * ]
 * chkmin = 30
 * chkind = btind.SMAEnvelope  // 注：原文件可能有错误，应该是KAMAEnvelope
 * 
 * 注：KAMAEnvelope包含3条线：Mid (KAMA), Upper, Lower
 */

#include "test_common.h"
#include "indicators/KAMAEnvelope.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> KAMAENVELOPE_EXPECTED_VALUES = {
    {"4063.463000", "3644.444667", "3554.693333"},  // line 0 (Mid/KAMA)
    {"4165.049575", "3735.555783", "3643.560667"},  // line 1 (Upper)
    {"3961.876425", "3553.333550", "3465.826000"}   // line 2 (Lower)
};

const int KAMAENVELOPE_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的KAMAEnvelope测试
DEFINE_INDICATOR_TEST(KAMAEnvelope_Default, KAMAEnvelope, KAMAENVELOPE_EXPECTED_VALUES, KAMAENVELOPE_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, KAMAEnvelope_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建KAMAEnvelope指标
    auto kamaenv = std::make_shared<KAMAEnvelope>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        kamaenv->calculate();
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
        auto expected = KAMAENVELOPE_EXPECTED_VALUES[line];
        
        for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
            double actual = kamaenv->getLine(line)->get(check_points[i]);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << actual;
            std::string actual_str = ss.str();
            
            EXPECT_EQ(actual_str, expected[i]) 
                << "KAMAEnvelope line " << line << " value mismatch at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(kamaenv->getMinPeriod(), 30) << "KAMAEnvelope minimum period should be 30";
}

// 参数化测试 - 测试不同参数的KAMAEnvelope
class KAMAEnvelopeParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int, int, double>> {
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

TEST_P(KAMAEnvelopeParameterizedTest, DifferentParameters) {
    auto [period, fast, slow, percentage] = GetParam();
    
    auto kamaenv = std::make_shared<KAMAEnvelope>(close_line_, period, fast, slow, percentage);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        kamaenv->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        double last_mid = kamaenv->getLine(0)->get(0);     // Mid (KAMA)
        double last_upper = kamaenv->getLine(1)->get(0);   // Upper
        double last_lower = kamaenv->getLine(2)->get(0);   // Lower
        
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
    KAMAEnvelopeParameterizedTest,
    ::testing::Values(
        std::make_tuple(14, 2, 30, 2.5),
        std::make_tuple(20, 2, 30, 2.5),
        std::make_tuple(30, 2, 30, 2.5),
        std::make_tuple(20, 3, 20, 2.5),
        std::make_tuple(20, 2, 30, 1.0),
        std::make_tuple(20, 2, 30, 5.0)
    )
);

// KAMAEnvelope计算逻辑验证测试
TEST(OriginalTests, KAMAEnvelope_CalculationLogic) {
    // 使用简单的测试数据验证KAMAEnvelope计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0,
                                  120.0, 118.0, 116.0, 114.0, 112.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0,
                                  124.0, 126.0, 128.0, 130.0, 132.0, 134.0, 136.0, 138.0, 140.0, 142.0};
    
    auto price_line = std::make_shared<LineRoot>(prices.size(), "kamaenv_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto kamaenv = std::make_shared<KAMAEnvelope>(price_line, 14, 2, 30, 2.5);  // 14周期，2.5%包络
    auto kama = std::make_shared<KAMA>(price_line, 14, 2, 30);  // 比较用的KAMA
    
    for (size_t i = 0; i < prices.size(); ++i) {
        kamaenv->calculate();
        kama->calculate();
        
        if (i >= 14) {  // KAMAEnvelope需要15个数据点
            double mid_value = kamaenv->getLine(0)->get(0);
            double upper_value = kamaenv->getLine(1)->get(0);
            double lower_value = kamaenv->getLine(2)->get(0);
            double kama_value = kama->get(0);
            
            if (!std::isnan(mid_value) && !std::isnan(kama_value)) {
                // Mid应该等于KAMA
                EXPECT_NEAR(mid_value, kama_value, 1e-10) 
                    << "KAMAEnvelope Mid should equal KAMA at step " << i;
                
                // 验证包络线计算
                double expected_upper = kama_value * 1.025;  // +2.5%
                double expected_lower = kama_value * 0.975;  // -2.5%
                
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

// KAMAEnvelope自适应特性测试
TEST(OriginalTests, KAMAEnvelope_AdaptiveCharacteristics) {
    // 创建波动性变化的数据
    std::vector<double> varying_vol_prices;
    
    // 第一阶段：低波动性
    for (int i = 0; i < 50; ++i) {
        double base = 100.0 + i * 0.1;
        double noise = std::sin(i * 0.1) * 0.5;  // 小幅波动
        varying_vol_prices.push_back(base + noise);
    }
    
    // 第二阶段：高波动性
    for (int i = 0; i < 50; ++i) {
        double base = 105.0 + i * 0.1;
        double noise = std::sin(i * 0.5) * 3.0;  // 大幅波动
        varying_vol_prices.push_back(base + noise);
    }
    
    auto varying_line = std::make_shared<LineRoot>(varying_vol_prices.size(), "varying");
    for (double price : varying_vol_prices) {
        varying_line->forward(price);
    }
    
    auto kamaenv = std::make_shared<KAMAEnvelope>(varying_line, 20, 2, 30, 2.5);
    auto emaenv = std::make_shared<EMAEnvelope>(varying_line, 20, 2.5);  // 比较对象
    
    std::vector<double> low_vol_kama_ranges, high_vol_kama_ranges;
    std::vector<double> low_vol_ema_ranges, high_vol_ema_ranges;
    
    for (size_t i = 0; i < varying_vol_prices.size(); ++i) {
        kamaenv->calculate();
        emaenv->calculate();
        
        double kama_upper = kamaenv->getLine(1)->get(0);
        double kama_lower = kamaenv->getLine(2)->get(0);
        double ema_upper = emaenv->getLine(1)->get(0);
        double ema_lower = emaenv->getLine(2)->get(0);
        
        if (!std::isnan(kama_upper) && !std::isnan(kama_lower) && 
            !std::isnan(ema_upper) && !std::isnan(ema_lower)) {
            
            if (i < 50) {  // 低波动阶段
                low_vol_kama_ranges.push_back(kama_upper - kama_lower);
                low_vol_ema_ranges.push_back(ema_upper - ema_lower);
            } else {  // 高波动阶段
                high_vol_kama_ranges.push_back(kama_upper - kama_lower);
                high_vol_ema_ranges.push_back(ema_upper - ema_lower);
            }
        }
        
        if (i < varying_vol_prices.size() - 1) {
            varying_line->forward();
        }
    }
    
    // 分析自适应特性
    if (!low_vol_kama_ranges.empty() && !high_vol_kama_ranges.empty()) {
        double avg_low_kama = std::accumulate(low_vol_kama_ranges.begin(), low_vol_kama_ranges.end(), 0.0) / low_vol_kama_ranges.size();
        double avg_high_kama = std::accumulate(high_vol_kama_ranges.begin(), high_vol_kama_ranges.end(), 0.0) / high_vol_kama_ranges.size();
        
        std::cout << "KAMA envelope adaptive characteristics:" << std::endl;
        std::cout << "Low volatility average range: " << avg_low_kama << std::endl;
        std::cout << "High volatility average range: " << avg_high_kama << std::endl;
        
        // KAMA包络应该在不同波动环境下保持相对稳定（基于百分比）
        EXPECT_GT(avg_low_kama, 0.0) << "KAMA envelope should have positive range in low volatility";
        EXPECT_GT(avg_high_kama, 0.0) << "KAMA envelope should have positive range in high volatility";
    }
}

// KAMAEnvelope响应速度测试
TEST(OriginalTests, KAMAEnvelope_ResponseSpeed) {
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
    
    auto kamaenv = std::make_shared<KAMAEnvelope>(step_line, 20, 2, 30, 2.5);
    auto emaenv = std::make_shared<EMAEnvelope>(step_line, 20, 2.5);   // 比较对象
    auto smaenv = std::make_shared<SMAEnvelope>(step_line, 20, 2.5);   // 比较对象
    
    std::vector<double> kama_responses, ema_responses, sma_responses;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        kamaenv->calculate();
        emaenv->calculate();
        smaenv->calculate();
        
        double kama_mid = kamaenv->getLine(0)->get(0);
        double ema_mid = emaenv->getLine(0)->get(0);
        double sma_mid = smaenv->getLine(0)->get(0);
        
        if (!std::isnan(kama_mid) && !std::isnan(ema_mid) && !std::isnan(sma_mid)) {
            if (i >= 50) {  // 价格跳跃后
                kama_responses.push_back(kama_mid);
                ema_responses.push_back(ema_mid);
                sma_responses.push_back(sma_mid);
            }
        }
        
        if (i < step_prices.size() - 1) {
            step_line->forward();
        }
    }
    
    // 比较响应速度
    if (!kama_responses.empty() && !ema_responses.empty() && !sma_responses.empty()) {
        double final_kama = kama_responses.back();
        double final_ema = ema_responses.back();
        double final_sma = sma_responses.back();
        
        std::cout << "Response speed comparison:" << std::endl;
        std::cout << "Final KAMA envelope mid: " << final_kama << std::endl;
        std::cout << "Final EMA envelope mid: " << final_ema << std::endl;
        std::cout << "Final SMA envelope mid: " << final_sma << std::endl;
        
        // KAMA在价格跳跃时应该快速响应
        EXPECT_GT(final_kama, final_sma * 0.9) 
            << "KAMA envelope should respond to price jumps";
    }
}

// KAMAEnvelope支撑阻力测试
TEST(OriginalTests, KAMAEnvelope_SupportResistance) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto kamaenv = std::make_shared<KAMAEnvelope>(close_line, 20, 2, 30, 2.5);
    
    int upper_touches = 0;    // 价格触及上轨
    int lower_touches = 0;    // 价格触及下轨
    int inside_envelope = 0;  // 价格在包络内
    int upper_breaks = 0;     // 价格突破上轨
    int lower_breaks = 0;     // 价格突破下轨
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        kamaenv->calculate();
        
        double current_price = csv_data[i].close;
        double upper = kamaenv->getLine(1)->get(0);
        double lower = kamaenv->getLine(2)->get(0);
        
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

// 与其他包络线比较测试
TEST(OriginalTests, KAMAEnvelope_vs_OtherEnvelopes) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto kamaenv = std::make_shared<KAMAEnvelope>(close_line, 20, 2, 30, 2.5);
    auto emaenv = std::make_shared<EMAEnvelope>(close_line, 20, 2.5);
    auto smaenv = std::make_shared<SMAEnvelope>(close_line, 20, 2.5);
    
    std::vector<double> kama_ranges, ema_ranges, sma_ranges;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        kamaenv->calculate();
        emaenv->calculate();
        smaenv->calculate();
        
        double kama_upper = kamaenv->getLine(1)->get(0);
        double kama_lower = kamaenv->getLine(2)->get(0);
        double ema_upper = emaenv->getLine(1)->get(0);
        double ema_lower = emaenv->getLine(2)->get(0);
        double sma_upper = smaenv->getLine(1)->get(0);
        double sma_lower = smaenv->getLine(2)->get(0);
        
        if (!std::isnan(kama_upper) && !std::isnan(kama_lower)) {
            kama_ranges.push_back(kama_upper - kama_lower);
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
    if (!kama_ranges.empty() && !ema_ranges.empty() && !sma_ranges.empty()) {
        double avg_kama_range = std::accumulate(kama_ranges.begin(), kama_ranges.end(), 0.0) / kama_ranges.size();
        double avg_ema_range = std::accumulate(ema_ranges.begin(), ema_ranges.end(), 0.0) / ema_ranges.size();
        double avg_sma_range = std::accumulate(sma_ranges.begin(), sma_ranges.end(), 0.0) / sma_ranges.size();
        
        std::cout << "Envelope comparison:" << std::endl;
        std::cout << "Average KAMA envelope range: " << avg_kama_range << std::endl;
        std::cout << "Average EMA envelope range: " << avg_ema_range << std::endl;
        std::cout << "Average SMA envelope range: " << avg_sma_range << std::endl;
        
        // 包络宽度应该相似（都基于相同百分比）
        EXPECT_NEAR(avg_kama_range, avg_ema_range, avg_ema_range * 0.1) 
            << "KAMA and EMA envelope ranges should be similar";
        EXPECT_NEAR(avg_ema_range, avg_sma_range, avg_sma_range * 0.1) 
            << "EMA and SMA envelope ranges should be similar";
    }
}

// 边界条件测试
TEST(OriginalTests, KAMAEnvelope_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_kamaenv = std::make_shared<KAMAEnvelope>(flat_line, 20, 2, 30, 2.5);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_kamaenv->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，检查包络线计算
    double final_mid = flat_kamaenv->getLine(0)->get(0);
    double final_upper = flat_kamaenv->getLine(1)->get(0);
    double final_lower = flat_kamaenv->getLine(2)->get(0);
    
    if (!std::isnan(final_mid) && !std::isnan(final_upper) && !std::isnan(final_lower)) {
        EXPECT_NEAR(final_mid, 100.0, 1e-6) << "Mid should equal constant price";
        EXPECT_NEAR(final_upper, 102.5, 1e-6) << "Upper should be 2.5% above constant price";
        EXPECT_NEAR(final_lower, 97.5, 1e-6) << "Lower should be 2.5% below constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 20; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_kamaenv = std::make_shared<KAMAEnvelope>(insufficient_line, 30, 2, 30, 2.5);
    
    for (int i = 0; i < 20; ++i) {
        insufficient_kamaenv->calculate();
        if (i < 19) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_kamaenv->getLine(0)->get(0);
    EXPECT_TRUE(std::isnan(result)) << "KAMAEnvelope should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, KAMAEnvelope_Performance) {
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
    
    auto large_kamaenv = std::make_shared<KAMAEnvelope>(large_line, 50, 2, 30, 2.5);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_kamaenv->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "KAMAEnvelope calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_mid = large_kamaenv->getLine(0)->get(0);
    double final_upper = large_kamaenv->getLine(1)->get(0);
    double final_lower = large_kamaenv->getLine(2)->get(0);
    
    EXPECT_FALSE(std::isnan(final_mid)) << "Final Mid should not be NaN";
    EXPECT_FALSE(std::isnan(final_upper)) << "Final Upper should not be NaN";
    EXPECT_FALSE(std::isnan(final_lower)) << "Final Lower should not be NaN";
    
    EXPECT_TRUE(std::isfinite(final_mid)) << "Final Mid should be finite";
    EXPECT_TRUE(std::isfinite(final_upper)) << "Final Upper should be finite";
    EXPECT_TRUE(std::isfinite(final_lower)) << "Final Lower should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}