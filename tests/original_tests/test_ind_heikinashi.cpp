/**
 * @file test_ind_heikinashi.cpp
 * @brief HeikinAshi指标测试 - 对应Python test_ind_heikinashi.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4119.466107', '3591.732500', '3578.625259'],
 *     ['4142.010000', '3638.420000', '3662.920000'],
 *     ['4119.466107', '3591.732500', '3578.625259'],
 *     ['4128.002500', '3614.670000', '3653.455000']
 * ]
 * chkmin = 2
 * chkind = bt.ind.HeikinAshi
 * 
 * 注：HeikinAshi创建平滑的蜡烛图，有4条线：open, high, low, close
 */

#include "test_common.h"
#include <random>

#include "indicators/heikinashi.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> HEIKINASHI_EXPECTED_VALUES = {
    {"4119.466107", "3591.732500", "3578.625259"},  // line 0 (close)
    {"4142.010000", "3638.420000", "3662.920000"},  // line 1 (open)
    {"4119.466107", "3591.732500", "3578.625259"},  // line 2 (high)
    {"4128.002500", "3614.670000", "3653.455000"}   // line 3 (low)
};

const int HEIKINASHI_MIN_PERIOD = 2;

} // anonymous namespace

// 使用默认参数的HeikinAshi测试
DEFINE_INDICATOR_TEST(HeikinAshi_Default, HeikinAshi, HEIKINASHI_EXPECTED_VALUES, HEIKINASHI_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, HeikinAshi_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建OHLC数据线
    auto open_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "open");
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        open_line->forward(bar.open);
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    // 创建HeikinAshi指标
    auto heikinashi = std::make_shared<HeikinAshi>(open_line, high_line, low_line, close_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        heikinashi->calculate();
        if (i < csv_data.size() - 1) {
            open_line->forward();
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 2;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // 验证4条线的值
    for (int line = 0; line < 4; ++line) {
        auto expected = HEIKINASHI_EXPECTED_VALUES[line];
        
        for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
            double actual = heikinashi->getLine(line)->get(check_points[i]);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << actual;
            std::string actual_str = ss.str();
            
            EXPECT_EQ(actual_str, expected[i]) 
                << "HeikinAshi line " << line << " value mismatch at check point " << i 
                << " (ago=" << check_points[i] << "): "
                << "expected " << expected[i] << ", got " << actual_str;
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(heikinashi->getMinPeriod(), 2) << "HeikinAshi minimum period should be 2";
}

// HeikinAshi计算逻辑验证测试
TEST(OriginalTests, HeikinAshi_CalculationLogic) {
    // 使用简单的测试数据验证HeikinAshi计算
    std::vector<std::vector<double>> ohlc_data = {
        {100.0, 105.0, 95.0, 102.0},   // O, H, L, C
        {102.0, 108.0, 101.0, 106.0},
        {106.0, 110.0, 104.0, 107.0},
        {107.0, 112.0, 105.0, 111.0},
        {111.0, 115.0, 109.0, 113.0}
    };
    
    auto open_line = std::make_shared<backtrader::LineRoot>(ohlc_data.size(), "open");
    auto high_line = std::make_shared<backtrader::LineRoot>(ohlc_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(ohlc_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(ohlc_data.size(), "close");
    
    for (const auto& bar : ohlc_data) {
        open_line->forward(bar[0]);
        high_line->forward(bar[1]);
        low_line->forward(bar[2]);
        close_line->forward(bar[3]);
    }
    
    auto heikinashi = std::make_shared<HeikinAshi>(open_line, high_line, low_line, close_line);
    
    // 手动计算HeikinAshi值进行验证
    std::vector<std::vector<double>> expected_ha;
    
    for (size_t i = 0; i < ohlc_data.size(); ++i) {
        heikinashi->calculate();
        
        if (i >= 1) {  // HeikinAshi需要至少2个数据点
            double o = ohlc_data[i][0];
            double h = ohlc_data[i][1];
            double l = ohlc_data[i][2];
            double c = ohlc_data[i][3];
            
            // HeikinAshi计算公式:
            // HA_Close = (O + H + L + C) / 4
            // HA_Open = (prev_HA_Open + prev_HA_Close) / 2 (第一个点用原始O和C)
            // HA_High = max(H, HA_Open, HA_Close)
            // HA_Low = min(L, HA_Open, HA_Close)
            
            double ha_close = (o + h + l + c) / 4.0;
            double ha_open;
            
            if (i == 1) {
                // 第一个HeikinAshi点
                ha_open = (ohlc_data[0][0] + ohlc_data[0][3]) / 2.0;
            } else {
                // 后续点使用前一个HA的开盘和收盘
                double prev_ha_open = heikinashi->getLine(1)->get(1);  // 前一个HA开盘
                double prev_ha_close = heikinashi->getLine(0)->get(1); // 前一个HA收盘
                ha_open = (prev_ha_open + prev_ha_close) / 2.0;
            }
            
            double ha_high = std::max({h, ha_open, ha_close});
            double ha_low = std::min({l, ha_open, ha_close});
            
            // 验证计算结果
            double actual_close = heikinashi->getLine(0)->get(0);  // HA Close
            double actual_open = heikinashi->getLine(1)->get(0);   // HA Open
            double actual_high = heikinashi->getLine(2)->get(0);   // HA High
            double actual_low = heikinashi->getLine(3)->get(0);    // HA Low
            
            if (!std::isnan(actual_close) && !std::isnan(actual_open) && 
                !std::isnan(actual_high) && !std::isnan(actual_low)) {
                
                EXPECT_NEAR(actual_close, ha_close, 1e-10) 
                    << "HA Close calculation mismatch at step " << i;
                EXPECT_NEAR(actual_open, ha_open, 1e-10) 
                    << "HA Open calculation mismatch at step " << i;
                EXPECT_NEAR(actual_high, ha_high, 1e-10) 
                    << "HA High calculation mismatch at step " << i;
                EXPECT_NEAR(actual_low, ha_low, 1e-10) 
                    << "HA Low calculation mismatch at step " << i;
            }
        }
        
        if (i < ohlc_data.size() - 1) {
            open_line->forward();
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}

// HeikinAshi平滑特性测试
TEST(OriginalTests, HeikinAshi_SmoothingCharacteristics) {
    auto csv_data = getdata(0);
    
    auto open_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "open");
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        open_line->forward(bar.open);
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto heikinashi = std::make_shared<HeikinAshi>(open_line, high_line, low_line, close_line);
    
    std::vector<double> original_volatility;
    std::vector<double> ha_volatility;
    
    // 计算原始数据和HeikinAshi的波动性
    for (size_t i = 0; i < csv_data.size(); ++i) {
        heikinashi->calculate();
        
        if (i >= 1) {
            // 原始数据的波动性（高低差）
            double original_range = csv_data[i].high - csv_data[i].low;
            original_volatility.push_back(original_range);
            
            // HeikinAshi的波动性
            double ha_high = heikinashi->getLine(2)->get(0);
            double ha_low = heikinashi->getLine(3)->get(0);
            
            if (!std::isnan(ha_high) && !std::isnan(ha_low)) {
                double ha_range = ha_high - ha_low;
                ha_volatility.push_back(ha_range);
            }
        }
        
        if (i < csv_data.size() - 1) {
            open_line->forward();
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 比较平均波动性
    if (!original_volatility.empty() && !ha_volatility.empty()) {
        double avg_original = std::accumulate(original_volatility.begin(), original_volatility.end(), 0.0) / original_volatility.size();
        double avg_ha = std::accumulate(ha_volatility.begin(), ha_volatility.end(), 0.0) / ha_volatility.size();
        
        std::cout << "Smoothing characteristics:" << std::endl;
        std::cout << "Average original range: " << avg_original << std::endl;
        std::cout << "Average HeikinAshi range: " << avg_ha << std::endl;
        
        // HeikinAshi通常会平滑价格，但不一定总是减少波动性
        EXPECT_GT(avg_original, 0.0) << "Original volatility should be positive";
        EXPECT_GT(avg_ha, 0.0) << "HeikinAshi volatility should be positive";
    }
}

// HeikinAshi趋势识别测试
TEST(OriginalTests, HeikinAshi_TrendIdentification) {
    // 创建明确的上升趋势数据
    std::vector<std::vector<double>> uptrend_data;
    for (int i = 0; i < 20; ++i) {
        double base = 100.0 + i * 2.0;
        uptrend_data.push_back({
            base - 1.0,      // open
            base + 2.0,      // high
            base - 2.0,      // low
            base + 1.0       // close
        });
    }
    
    auto open_line = std::make_shared<backtrader::LineRoot>(uptrend_data.size(), "open");
    auto high_line = std::make_shared<backtrader::LineRoot>(uptrend_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(uptrend_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(uptrend_data.size(), "close");
    
    for (const auto& bar : uptrend_data) {
        open_line->forward(bar[0]);
        high_line->forward(bar[1]);
        low_line->forward(bar[2]);
        close_line->forward(bar[3]);
    }
    
    auto heikinashi = std::make_shared<HeikinAshi>(open_line, high_line, low_line, close_line);
    
    int bullish_candles = 0;  // HA_Close > HA_Open
    int bearish_candles = 0;  // HA_Close < HA_Open
    int total_candles = 0;
    
    for (size_t i = 0; i < uptrend_data.size(); ++i) {
        heikinashi->calculate();
        
        double ha_close = heikinashi->getLine(0)->get(0);
        double ha_open = heikinashi->getLine(1)->get(0);
        
        if (!std::isnan(ha_close) && !std::isnan(ha_open)) {
            total_candles++;
            if (ha_close > ha_open) {
                bullish_candles++;
            } else if (ha_close < ha_open) {
                bearish_candles++;
            }
        }
        
        if (i < uptrend_data.size() - 1) {
            open_line->forward();
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    std::cout << "Trend identification:" << std::endl;
    std::cout << "Total candles: " << total_candles << std::endl;
    std::cout << "Bullish candles: " << bullish_candles << std::endl;
    std::cout << "Bearish candles: " << bearish_candles << std::endl;
    
    // 在明确的上升趋势中，应该有更多的看涨蜡烛
    if (total_candles > 0) {
        double bullish_ratio = static_cast<double>(bullish_candles) / total_candles;
        EXPECT_GT(bullish_ratio, 0.5) << "In uptrend, should have more bullish HeikinAshi candles";
    }
}

// HeikinAshi连续性测试
TEST(OriginalTests, HeikinAshi_Continuity) {
    auto csv_data = getdata(0);
    
    auto open_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "open");
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        open_line->forward(bar.open);
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto heikinashi = std::make_shared<HeikinAshi>(open_line, high_line, low_line, close_line);
    
    // 验证OHLC关系的连续性
    for (size_t i = 0; i < csv_data.size(); ++i) {
        heikinashi->calculate();
        
        double ha_open = heikinashi->getLine(1)->get(0);
        double ha_high = heikinashi->getLine(2)->get(0);
        double ha_low = heikinashi->getLine(3)->get(0);
        double ha_close = heikinashi->getLine(0)->get(0);
        
        if (!std::isnan(ha_open) && !std::isnan(ha_high) && 
            !std::isnan(ha_low) && !std::isnan(ha_close)) {
            
            // HeikinAshi应该满足基本的OHLC关系
            EXPECT_GE(ha_high, ha_open) << "HA High should be >= HA Open at step " << i;
            EXPECT_GE(ha_high, ha_close) << "HA High should be >= HA Close at step " << i;
            EXPECT_LE(ha_low, ha_open) << "HA Low should be <= HA Open at step " << i;
            EXPECT_LE(ha_low, ha_close) << "HA Low should be <= HA Close at step " << i;
            
            // 验证所有值都是有限的
            EXPECT_TRUE(std::isfinite(ha_open)) << "HA Open should be finite at step " << i;
            EXPECT_TRUE(std::isfinite(ha_high)) << "HA High should be finite at step " << i;
            EXPECT_TRUE(std::isfinite(ha_low)) << "HA Low should be finite at step " << i;
            EXPECT_TRUE(std::isfinite(ha_close)) << "HA Close should be finite at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            open_line->forward();
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}

// HeikinAshi与原始数据比较测试
TEST(OriginalTests, HeikinAshi_OriginalDataComparison) {
    auto csv_data = getdata(0);
    
    auto open_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "open");
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        open_line->forward(bar.open);
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto heikinashi = std::make_shared<HeikinAshi>(open_line, high_line, low_line, close_line);
    
    std::vector<double> original_closes;
    std::vector<double> ha_closes;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        heikinashi->calculate();
        
        double ha_close = heikinashi->getLine(0)->get(0);
        
        if (!std::isnan(ha_close)) {
            original_closes.push_back(csv_data[i].close);
            ha_closes.push_back(ha_close);
        }
        
        if (i < csv_data.size() - 1) {
            open_line->forward();
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 比较原始收盘价和HeikinAshi收盘价的特性
    if (!original_closes.empty() && !ha_closes.empty()) {
        double original_avg = std::accumulate(original_closes.begin(), original_closes.end(), 0.0) / original_closes.size();
        double ha_avg = std::accumulate(ha_closes.begin(), ha_closes.end(), 0.0) / ha_closes.size();
        
        std::cout << "Original vs HeikinAshi comparison:" << std::endl;
        std::cout << "Original average close: " << original_avg << std::endl;
        std::cout << "HeikinAshi average close: " << ha_avg << std::endl;
        
        // 两者的平均值应该相近（HeikinAshi是平滑版本）
        double diff_ratio = std::abs(original_avg - ha_avg) / original_avg;
        EXPECT_LT(diff_ratio, 0.1) << "HeikinAshi and original averages should be similar";
    }
}

// 边界条件测试
TEST(OriginalTests, HeikinAshi_EdgeCases) {
    // 测试相同OHLC的情况
    std::vector<std::vector<double>> flat_data(10, {100.0, 100.0, 100.0, 100.0});
    
    auto open_line = std::make_shared<backtrader::LineRoot>(flat_data.size(), "open");
    auto high_line = std::make_shared<backtrader::LineRoot>(flat_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(flat_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(flat_data.size(), "close");
    
    for (const auto& bar : flat_data) {
        open_line->forward(bar[0]);
        high_line->forward(bar[1]);
        low_line->forward(bar[2]);
        close_line->forward(bar[3]);
    }
    
    auto flat_heikinashi = std::make_shared<HeikinAshi>(open_line, high_line, low_line, close_line);
    
    for (size_t i = 0; i < flat_data.size(); ++i) {
        flat_heikinashi->calculate();
        if (i < flat_data.size() - 1) {
            open_line->forward();
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 当所有OHLC相同时，HeikinAshi应该收敛到相同的值
    double ha_open = flat_heikinashi->getLine(1)->get(0);
    double ha_high = flat_heikinashi->getLine(2)->get(0);
    double ha_low = flat_heikinashi->getLine(3)->get(0);
    double ha_close = flat_heikinashi->getLine(0)->get(0);
    
    if (!std::isnan(ha_open) && !std::isnan(ha_high) && 
        !std::isnan(ha_low) && !std::isnan(ha_close)) {
        
        EXPECT_NEAR(ha_open, 100.0, 1e-6) << "HA Open should converge to constant price";
        EXPECT_NEAR(ha_high, 100.0, 1e-6) << "HA High should converge to constant price";
        EXPECT_NEAR(ha_low, 100.0, 1e-6) << "HA Low should converge to constant price";
        EXPECT_NEAR(ha_close, 100.0, 1e-6) << "HA Close should converge to constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_open = std::make_shared<backtrader::LineRoot>(10, "insufficient_open");
    auto insufficient_high = std::make_shared<backtrader::LineRoot>(10, "insufficient_high");
    auto insufficient_low = std::make_shared<backtrader::LineRoot>(10, "insufficient_low");
    auto insufficient_close = std::make_shared<backtrader::LineRoot>(10, "insufficient_close");
    
    // 只添加一个数据点
    insufficient_open->forward(100.0);
    insufficient_high->forward(105.0);
    insufficient_low->forward(95.0);
    insufficient_close->forward(102.0);
    
    auto insufficient_heikinashi = std::make_shared<HeikinAshi>(
        insufficient_open, insufficient_high, insufficient_low, insufficient_close);
    
    insufficient_heikinashi->calculate();
    
    // 数据不足时应该返回NaN
    double result = insufficient_heikinashi->getLine(0)->get(0);
    EXPECT_TRUE(std::isnan(result)) << "HeikinAshi should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, HeikinAshi_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<std::vector<double>> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        double base = dist(rng);
        large_data.push_back({
            base,                    // open
            base + dist(rng) * 0.1,  // high
            base - dist(rng) * 0.1,  // low
            base + (dist(rng) - 100.0) * 0.05  // close
        });
    }
    
    auto large_open = std::make_shared<backtrader::LineRoot>(large_data.size(), "large_open");
    auto large_high = std::make_shared<backtrader::LineRoot>(large_data.size(), "large_high");
    auto large_low = std::make_shared<backtrader::LineRoot>(large_data.size(), "large_low");
    auto large_close = std::make_shared<backtrader::LineRoot>(large_data.size(), "large_close");
    
    for (const auto& bar : large_data) {
        large_open->forward(bar[0]);
        large_high->forward(bar[1]);
        large_low->forward(bar[2]);
        large_close->forward(bar[3]);
    }
    
    auto large_heikinashi = std::make_shared<HeikinAshi>(large_open, large_high, large_low, large_close);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_heikinashi->calculate();
        if (i < large_data.size() - 1) {
            large_open->forward();
            large_high->forward();
            large_low->forward();
            large_close->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "HeikinAshi calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_close = large_heikinashi->getLine(0)->get(0);
    double final_open = large_heikinashi->getLine(1)->get(0);
    double final_high = large_heikinashi->getLine(2)->get(0);
    double final_low = large_heikinashi->getLine(3)->get(0);
    
    EXPECT_FALSE(std::isnan(final_close)) << "Final HA Close should not be NaN";
    EXPECT_FALSE(std::isnan(final_open)) << "Final HA Open should not be NaN";
    EXPECT_FALSE(std::isnan(final_high)) << "Final HA High should not be NaN";
    EXPECT_FALSE(std::isnan(final_low)) << "Final HA Low should not be NaN";
    
    EXPECT_TRUE(std::isfinite(final_close)) << "Final HA Close should be finite";
    EXPECT_TRUE(std::isfinite(final_open)) << "Final HA Open should be finite";
    EXPECT_TRUE(std::isfinite(final_high)) << "Final HA High should be finite";
    EXPECT_TRUE(std::isfinite(final_low)) << "Final HA Low should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}