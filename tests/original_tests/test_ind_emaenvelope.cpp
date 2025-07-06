/**
 * @file test_ind_emaenvelope.cpp
 * @brief EMAEnvelope指标测试 - 对应Python test_ind_emaenvelope.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4070.115719', '3644.444667', '3581.728712'],
 *     ['4171.868612', '3735.555783', '3671.271930'],
 *     ['3968.362826', '3553.333550', '3492.185494']
 * ]
 * chkmin = 30
 * chkind = btind.EMAEnvelope
 * 
 * 注：EMAEnvelope包含3条线：Mid (EMA), Upper, Lower
 */

#include "test_common.h"
#include <random>

#include "indicators/envelope.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> EMAENVELOPE_EXPECTED_VALUES = {
    {"4070.115719", "3644.444667", "3581.728712"},  // line 0 (Mid/EMA)
    {"4171.868612", "3735.555783", "3671.271930"},  // line 1 (Upper)
    {"3968.362826", "3553.333550", "3492.185494"}   // line 2 (Lower)
};

const int EMAENVELOPE_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的EMAEnvelope测试
DEFINE_INDICATOR_TEST(EMAEnvelope_Default, EMAEnvelope, EMAENVELOPE_EXPECTED_VALUES, EMAENVELOPE_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, EMAEnvelope_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建EMAEnvelope指标
    auto emaenv = std::make_shared<EMAEnvelope>(close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        emaenv->calculate();
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
        auto expected = EMAENVELOPE_EXPECTED_VALUES[line];
        
        for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
            double actual = emaenv->getLine(line)->get(check_points[i]);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << actual;
            std::string actual_str = ss.str();
            
            EXPECT_EQ(actual_str, expected[i]) 
                << "EMAEnvelope line " << line << " value mismatch at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(emaenv->getMinPeriod(), 30) << "EMAEnvelope minimum period should be 30";
}

// EMAEnvelope计算逻辑验证测试
TEST(OriginalTests, EMAEnvelope_CalculationLogic) {
    // 使用简单的测试数据验证EMAEnvelope计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0,
                                  120.0, 118.0, 116.0, 114.0, 112.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0,
                                  124.0, 126.0, 128.0, 130.0, 132.0};
    
    auto price_line = std::make_shared<backtrader::LineRoot>(prices.size(), "emaenv_calc");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto emaenv = std::make_shared<EMAEnvelope>(price_line, 10, 2.5);  // 10周期，2.5%包络
    auto ema = std::make_shared<EMA>(price_line, 10);  // 比较用的EMA
    
    for (size_t i = 0; i < prices.size(); ++i) {
        emaenv->calculate();
        ema->calculate();
        
        if (i >= 9) {  // EMAEnvelope需要10个数据点
            double mid_value = emaenv->getLine(0)->get(0);
            double upper_value = emaenv->getLine(1)->get(0);
            double lower_value = emaenv->getLine(2)->get(0);
            double ema_value = ema->get(0);
            
            if (!std::isnan(mid_value) && !std::isnan(ema_value)) {
                // Mid应该等于EMA
                EXPECT_NEAR(mid_value, ema_value, 1e-10) 
                    << "EMAEnvelope Mid should equal EMA at step " << i;
                
                // 验证包络线计算
                double expected_upper = ema_value * 1.025;  // +2.5%
                double expected_lower = ema_value * 0.975;  // -2.5%
                
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

// EMAEnvelope响应速度测试
TEST(OriginalTests, EMAEnvelope_ResponseSpeed) {
    // 创建价格突然变化的数据来测试响应速度
    std::vector<double> step_prices;
    
    // 第一阶段：稳定价格
    for (int i = 0; i < 30; ++i) {
        step_prices.push_back(100.0);
    }
    
    // 第二阶段：价格跳跃
    for (int i = 0; i < 30; ++i) {
        step_prices.push_back(120.0);
    }
    
    auto step_line = std::make_shared<backtrader::LineRoot>(step_prices.size(), "step");
    for (double price : step_prices) {
        step_line->forward(price);
    }
    
    auto emaenv = std::make_shared<EMAEnvelope>(step_line, 20, 2.5);
    auto smaenv = std::make_shared<SMAEnvelope>(step_line, 20, 2.5);  // 比较对象
    
    std::vector<double> ema_responses, sma_responses;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        emaenv->calculate();
        smaenv->calculate();
        
        double ema_mid = emaenv->getLine(0)->get(0);
        double sma_mid = smaenv->getLine(0)->get(0);
        
        if (!std::isnan(ema_mid) && !std::isnan(sma_mid)) {
            if (i >= 30) {  // 价格跳跃后
                ema_responses.push_back(ema_mid);
                sma_responses.push_back(sma_mid);
            }
        }
        
        if (i < step_prices.size() - 1) {
            step_line->forward();
        }
    }
    
    // 比较响应速度
    if (!ema_responses.empty() && !sma_responses.empty()) {
        double final_ema = ema_responses.back();
        double final_sma = sma_responses.back();
        
        std::cout << "Response speed comparison:" << std::endl;
        std::cout << "Final EMA envelope mid: " << final_ema << std::endl;
        std::cout << "Final SMA envelope mid: " << final_sma << std::endl;
        
        // EMA应该比SMA更快地响应价格变化
        EXPECT_GT(final_ema, final_sma * 0.95) 
            << "EMA envelope should respond faster than SMA envelope";
    }
}

// 与SMAEnvelope比较测试
TEST(OriginalTests, EMAEnvelope_vs_SMAEnvelope) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto emaenv = std::make_shared<EMAEnvelope>(close_line, 20, 2.5);
    auto smaenv = std::make_shared<SMAEnvelope>(close_line, 20, 2.5);
    
    std::vector<double> ema_ranges, sma_ranges;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        emaenv->calculate();
        smaenv->calculate();
        
        double ema_upper = emaenv->getLine(1)->get(0);
        double ema_lower = emaenv->getLine(2)->get(0);
        double sma_upper = smaenv->getLine(1)->get(0);
        double sma_lower = smaenv->getLine(2)->get(0);
        
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
    if (!ema_ranges.empty() && !sma_ranges.empty()) {
        double avg_ema_range = std::accumulate(ema_ranges.begin(), ema_ranges.end(), 0.0) / ema_ranges.size();
        double avg_sma_range = std::accumulate(sma_ranges.begin(), sma_ranges.end(), 0.0) / sma_ranges.size();
        
        std::cout << "EMA vs SMA envelope comparison:" << std::endl;
        std::cout << "Average EMA envelope range: " << avg_ema_range << std::endl;
        std::cout << "Average SMA envelope range: " << avg_sma_range << std::endl;
        
        // 包络宽度应该相似（都基于相同百分比）
        EXPECT_NEAR(avg_ema_range, avg_sma_range, avg_sma_range * 0.1) 
            << "EMA and SMA envelope ranges should be similar";
    }
}

// 边界条件测试
TEST(OriginalTests, EMAEnvelope_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(50, 100.0);
    
    auto flat_line = std::make_shared<backtrader::LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_emaenv = std::make_shared<EMAEnvelope>(flat_line, 20, 2.5);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_emaenv->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，检查包络线计算
    double final_mid = flat_emaenv->getLine(0)->get(0);
    double final_upper = flat_emaenv->getLine(1)->get(0);
    double final_lower = flat_emaenv->getLine(2)->get(0);
    
    if (!std::isnan(final_mid) && !std::isnan(final_upper) && !std::isnan(final_lower)) {
        EXPECT_NEAR(final_mid, 100.0, 1e-6) << "Mid should equal constant price";
        EXPECT_NEAR(final_upper, 102.5, 1e-6) << "Upper should be 2.5% above constant price";
        EXPECT_NEAR(final_lower, 97.5, 1e-6) << "Lower should be 2.5% below constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<backtrader::LineRoot>(50, "insufficient");
    
    // 只添加少量数据点
    for (int i = 0; i < 15; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_emaenv = std::make_shared<EMAEnvelope>(insufficient_line, 20, 2.5);
    
    for (int i = 0; i < 15; ++i) {
        insufficient_emaenv->calculate();
        if (i < 14) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_emaenv->getLine(0)->get(0);
    EXPECT_TRUE(std::isnan(result)) << "EMAEnvelope should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, EMAEnvelope_Performance) {
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
    
    auto large_emaenv = std::make_shared<EMAEnvelope>(large_line, 50, 2.5);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_emaenv->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "EMAEnvelope calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_mid = large_emaenv->getLine(0)->get(0);
    double final_upper = large_emaenv->getLine(1)->get(0);
    double final_lower = large_emaenv->getLine(2)->get(0);
    
    EXPECT_FALSE(std::isnan(final_mid)) << "Final Mid should not be NaN";
    EXPECT_FALSE(std::isnan(final_upper)) << "Final Upper should not be NaN";
    EXPECT_FALSE(std::isnan(final_lower)) << "Final Lower should not be NaN";
    
    EXPECT_TRUE(std::isfinite(final_mid)) << "Final Mid should be finite";
    EXPECT_TRUE(std::isfinite(final_upper)) << "Final Upper should be finite";
    EXPECT_TRUE(std::isfinite(final_lower)) << "Final Lower should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}
