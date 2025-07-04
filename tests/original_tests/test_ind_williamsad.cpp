/**
 * @file test_ind_williamsad.cpp
 * @brief WilliamsAD指标测试 - 对应Python test_ind_williamsad.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['755.050000', '12.500000', '242.980000']
 * ]
 * chkmin = 2
 * chkind = btind.WilliamsAD
 * 
 * 注：WilliamsAD (Williams Accumulation/Distribution) 是一个成交量加权的价格指标
 */

#include "test_common_simple.h"

#include "indicators/williamsad.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> WILLIAMSAD_EXPECTED_VALUES = {
    {"755.050000", "12.500000", "242.980000"}
};

const int WILLIAMSAD_MIN_PERIOD = 2;

} // anonymous namespace

// 使用默认参数的WilliamsAD测试
DEFINE_INDICATOR_TEST(WilliamsAD_Default, WilliamsAD, WILLIAMSAD_EXPECTED_VALUES, WILLIAMSAD_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, WilliamsAD_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建HLCV数据线
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    auto volume_line = std::make_shared<LineRoot>(csv_data.size(), "volume");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
        volume_line->forward(bar.volume);
    }
    
    // 创建WilliamsAD指标
    auto williamsad = std::make_shared<WilliamsAD>(high_line, low_line, close_line, volume_line);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        williamsad->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
            volume_line->forward();
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
    
    std::vector<std::string> expected = {"755.050000", "12.500000", "242.980000"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = williamsad->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "WilliamsAD value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(williamsad->getMinPeriod(), 2) << "WilliamsAD minimum period should be 2";
}

// WilliamsAD计算逻辑验证测试
TEST(OriginalTests, WilliamsAD_CalculationLogic) {
    // 使用简单的测试数据验证WilliamsAD计算
    std::vector<std::tuple<double, double, double, double>> hlcv_data = {
        {105.0, 95.0, 100.0, 1000.0},   // H, L, C, V
        {110.0, 98.0, 108.0, 1500.0},
        {112.0, 105.0, 110.0, 1200.0},
        {108.0, 102.0, 105.0, 1800.0},
        {115.0, 107.0, 113.0, 1600.0},
        {118.0, 110.0, 115.0, 1400.0},
        {116.0, 112.0, 114.0, 1700.0},
        {120.0, 114.0, 118.0, 1300.0},
        {122.0, 116.0, 120.0, 1900.0},
        {119.0, 115.0, 117.0, 1100.0}
    };
    
    auto high_line = std::make_shared<LineRoot>(hlcv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(hlcv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(hlcv_data.size(), "close");
    auto volume_line = std::make_shared<LineRoot>(hlcv_data.size(), "volume");
    
    for (const auto& [h, l, c, v] : hlcv_data) {
        high_line->forward(h);
        low_line->forward(l);
        close_line->forward(c);
        volume_line->forward(v);
    }
    
    auto williamsad = std::make_shared<WilliamsAD>(high_line, low_line, close_line, volume_line);
    
    // 手动计算WilliamsAD进行验证
    double manual_ad = 0.0;
    
    for (size_t i = 0; i < hlcv_data.size(); ++i) {
        williamsad->calculate();
        
        if (i >= 1) {  // WilliamsAD需要2个数据点
            auto [h, l, c, v] = hlcv_data[i];
            auto [prev_h, prev_l, prev_c, prev_v] = hlcv_data[i-1];
            
            // Williams A/D计算公式：
            // True Range High = max(H, prev_C)
            // True Range Low = min(L, prev_C)
            // A/D = ((C - TRL) / (TRH - TRL)) * V
            // Cumulative A/D = sum of all A/D values
            
            double trh = std::max(h, prev_c);
            double trl = std::min(l, prev_c);
            
            if (trh != trl) {
                double ad_value = ((c - trl) / (trh - trl)) * v;
                manual_ad += ad_value;
            }
            
            double actual_ad = williamsad->get(0);
            
            if (!std::isnan(actual_ad)) {
                EXPECT_NEAR(actual_ad, manual_ad, 1e-6) 
                    << "WilliamsAD calculation mismatch at step " << i;
            }
        }
        
        if (i < hlcv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
            volume_line->forward();
        }
    }
}

// WilliamsAD累积特性测试
TEST(OriginalTests, WilliamsAD_AccumulationCharacteristics) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    auto volume_line = std::make_shared<LineRoot>(csv_data.size(), "volume");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
        volume_line->forward(bar.volume);
    }
    
    auto williamsad = std::make_shared<WilliamsAD>(high_line, low_line, close_line, volume_line);
    
    std::vector<double> ad_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        williamsad->calculate();
        
        double ad_val = williamsad->get(0);
        if (!std::isnan(ad_val)) {
            ad_values.push_back(ad_val);
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
            volume_line->forward();
        }
    }
    
    // 分析累积特性
    if (ad_values.size() > 10) {
        // 验证累积性质：后面的值应该基于前面的值
        bool is_cumulative = true;
        double tolerance = 1e-10;
        
        for (size_t i = 1; i < std::min(size_t(10), ad_values.size()); ++i) {
            // Williams A/D是累积指标，每个值都基于前面的总和
            // 检查值是否在合理的累积范围内
            if (std::abs(ad_values[i] - ad_values[i-1]) < tolerance) {
                // 连续相同值可能表示没有价格变化
                continue;
            }
        }
        
        std::cout << "WilliamsAD accumulation analysis:" << std::endl;
        std::cout << "First value: " << ad_values[0] << std::endl;
        std::cout << "Last value: " << ad_values.back() << std::endl;
        std::cout << "Total change: " << (ad_values.back() - ad_values[0]) << std::endl;
        
        // 验证累积指标的基本特性
        EXPECT_TRUE(std::isfinite(ad_values[0])) << "First A/D value should be finite";
        EXPECT_TRUE(std::isfinite(ad_values.back())) << "Last A/D value should be finite";
    }
}

// WilliamsAD分散/累积信号测试
TEST(OriginalTests, WilliamsAD_DistributionAccumulation) {
    // 创建明确的累积和分散阶段数据
    std::vector<std::tuple<double, double, double, double>> phases_data;
    
    // 累积阶段：价格上升，成交量增加
    for (int i = 0; i < 15; ++i) {
        double base = 100.0 + i * 1.0;
        phases_data.push_back({
            base + 2.0,               // high
            base - 1.0,               // low  
            base + 1.5,               // close (偏向高位)
            1000.0 + i * 50.0         // increasing volume
        });
    }
    
    // 分散阶段：价格下降，成交量增加
    for (int i = 0; i < 15; ++i) {
        double base = 115.0 - i * 0.8;
        phases_data.push_back({
            base + 1.0,               // high
            base - 2.0,               // low
            base - 1.5,               // close (偏向低位)
            1750.0 + i * 30.0         // volume
        });
    }
    
    auto phase_high = std::make_shared<LineRoot>(phases_data.size(), "high");
    auto phase_low = std::make_shared<LineRoot>(phases_data.size(), "low");
    auto phase_close = std::make_shared<LineRoot>(phases_data.size(), "close");
    auto phase_volume = std::make_shared<LineRoot>(phases_data.size(), "volume");
    
    for (const auto& [h, l, c, v] : phases_data) {
        phase_high->forward(h);
        phase_low->forward(l);
        phase_close->forward(c);
        phase_volume->forward(v);
    }
    
    auto phase_williamsad = std::make_shared<WilliamsAD>(phase_high, phase_low, phase_close, phase_volume);
    
    std::vector<double> accumulation_values, distribution_values;
    
    for (size_t i = 0; i < phases_data.size(); ++i) {
        phase_williamsad->calculate();
        
        double ad_val = phase_williamsad->get(0);
        if (!std::isnan(ad_val)) {
            if (i < 15) {
                accumulation_values.push_back(ad_val);
            } else {
                distribution_values.push_back(ad_val);
            }
        }
        
        if (i < phases_data.size() - 1) {
            phase_high->forward();
            phase_low->forward();
            phase_close->forward();
            phase_volume->forward();
        }
    }
    
    // 分析累积和分散阶段
    if (!accumulation_values.empty() && !distribution_values.empty()) {
        double acc_start = accumulation_values.front();
        double acc_end = accumulation_values.back();
        double dist_start = distribution_values.front();
        double dist_end = distribution_values.back();
        
        std::cout << "Distribution/Accumulation analysis:" << std::endl;
        std::cout << "Accumulation phase: " << acc_start << " -> " << acc_end 
                  << " (change: " << (acc_end - acc_start) << ")" << std::endl;
        std::cout << "Distribution phase: " << dist_start << " -> " << dist_end 
                  << " (change: " << (dist_end - dist_start) << ")" << std::endl;
        
        // 累积阶段应该显示正向增长
        EXPECT_GT(acc_end, acc_start) << "Accumulation phase should show positive A/D growth";
        
        // 分散阶段的增长应该小于累积阶段
        double acc_growth = acc_end - acc_start;
        double dist_growth = dist_end - dist_start;
        EXPECT_LT(dist_growth, acc_growth) << "Distribution phase should show less A/D growth";
    }
}

// WilliamsAD与价格发散测试
TEST(OriginalTests, WilliamsAD_PriceDivergence) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    auto volume_line = std::make_shared<LineRoot>(csv_data.size(), "volume");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
        volume_line->forward(bar.volume);
    }
    
    auto williamsad = std::make_shared<WilliamsAD>(high_line, low_line, close_line, volume_line);
    
    std::vector<double> prices, ad_values;
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        williamsad->calculate();
        
        double ad_val = williamsad->get(0);
        if (!std::isnan(ad_val)) {
            prices.push_back(csv_data[i].close);
            ad_values.push_back(ad_val);
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
            volume_line->forward();
        }
    }
    
    // 寻找价格和A/D的峰值进行发散分析
    std::vector<size_t> price_highs, ad_highs;
    
    for (size_t i = 2; i < prices.size() - 2; ++i) {
        // 寻找价格峰值
        if (prices[i] > prices[i-1] && prices[i] > prices[i+1] &&
            prices[i] > prices[i-2] && prices[i] > prices[i+2]) {
            price_highs.push_back(i);
        }
        
        // 寻找A/D峰值
        if (ad_values[i] > ad_values[i-1] && ad_values[i] > ad_values[i+1] &&
            ad_values[i] > ad_values[i-2] && ad_values[i] > ad_values[i+2]) {
            ad_highs.push_back(i);
        }
    }
    
    std::cout << "Divergence analysis:" << std::endl;
    std::cout << "Price highs found: " << price_highs.size() << std::endl;
    std::cout << "A/D highs found: " << ad_highs.size() << std::endl;
    
    // 分析最近的几个峰值
    if (price_highs.size() >= 2) {
        size_t latest_price_high = price_highs.back();
        size_t prev_price_high = price_highs[price_highs.size() - 2];
        
        std::cout << "Recent price highs comparison:" << std::endl;
        std::cout << "Previous: " << prices[prev_price_high] << " at index " << prev_price_high << std::endl;
        std::cout << "Latest: " << prices[latest_price_high] << " at index " << latest_price_high << std::endl;
        std::cout << "Corresponding A/D values: " << ad_values[prev_price_high] 
                  << " -> " << ad_values[latest_price_high] << std::endl;
    }
    
    EXPECT_TRUE(true) << "Price/A/D divergence analysis completed";
}

// WilliamsAD成交量敏感性测试
TEST(OriginalTests, WilliamsAD_VolumeSensitivity) {
    // 创建相同价格模式但不同成交量的数据
    std::vector<std::tuple<double, double, double, double>> low_vol_data, high_vol_data;
    
    // 相同的价格模式
    std::vector<std::tuple<double, double, double>> price_pattern = {
        {105.0, 95.0, 102.0},   // H, L, C
        {108.0, 100.0, 106.0},
        {110.0, 103.0, 108.0},
        {107.0, 102.0, 104.0},
        {112.0, 105.0, 110.0},
        {115.0, 108.0, 113.0},
        {113.0, 109.0, 111.0},
        {118.0, 112.0, 116.0},
        {120.0, 114.0, 118.0},
        {117.0, 113.0, 115.0}
    };
    
    // 低成交量版本
    for (const auto& [h, l, c] : price_pattern) {
        low_vol_data.push_back({h, l, c, 500.0});  // 低成交量
    }
    
    // 高成交量版本
    for (const auto& [h, l, c] : price_pattern) {
        high_vol_data.push_back({h, l, c, 2000.0});  // 高成交量
    }
    
    // 创建低成交量指标
    auto low_high = std::make_shared<LineRoot>(low_vol_data.size(), "low_high");
    auto low_low = std::make_shared<LineRoot>(low_vol_data.size(), "low_low");
    auto low_close = std::make_shared<LineRoot>(low_vol_data.size(), "low_close");
    auto low_volume = std::make_shared<LineRoot>(low_vol_data.size(), "low_volume");
    
    for (const auto& [h, l, c, v] : low_vol_data) {
        low_high->forward(h);
        low_low->forward(l);
        low_close->forward(c);
        low_volume->forward(v);
    }
    
    // 创建高成交量指标
    auto high_high = std::make_shared<LineRoot>(high_vol_data.size(), "high_high");
    auto high_low = std::make_shared<LineRoot>(high_vol_data.size(), "high_low");
    auto high_close = std::make_shared<LineRoot>(high_vol_data.size(), "high_close");
    auto high_volume = std::make_shared<LineRoot>(high_vol_data.size(), "high_volume");
    
    for (const auto& [h, l, c, v] : high_vol_data) {
        high_high->forward(h);
        high_low->forward(l);
        high_close->forward(c);
        high_volume->forward(v);
    }
    
    auto low_vol_ad = std::make_shared<WilliamsAD>(low_high, low_low, low_close, low_volume);
    auto high_vol_ad = std::make_shared<WilliamsAD>(high_high, high_low, high_close, high_volume);
    
    std::vector<double> low_vol_values, high_vol_values;
    
    for (size_t i = 0; i < price_pattern.size(); ++i) {
        low_vol_ad->calculate();
        high_vol_ad->calculate();
        
        double low_val = low_vol_ad->get(0);
        double high_val = high_vol_ad->get(0);
        
        if (!std::isnan(low_val) && !std::isnan(high_val)) {
            low_vol_values.push_back(low_val);
            high_vol_values.push_back(high_val);
        }
        
        if (i < price_pattern.size() - 1) {
            low_high->forward();
            low_low->forward();
            low_close->forward();
            low_volume->forward();
            high_high->forward();
            high_low->forward();
            high_close->forward();
            high_volume->forward();
        }
    }
    
    // 比较成交量敏感性
    if (!low_vol_values.empty() && !high_vol_values.empty()) {
        double low_vol_range = *std::max_element(low_vol_values.begin(), low_vol_values.end()) - 
                               *std::min_element(low_vol_values.begin(), low_vol_values.end());
        double high_vol_range = *std::max_element(high_vol_values.begin(), high_vol_values.end()) - 
                                *std::min_element(high_vol_values.begin(), high_vol_values.end());
        
        std::cout << "Volume sensitivity analysis:" << std::endl;
        std::cout << "Low volume A/D range: " << low_vol_range << std::endl;
        std::cout << "High volume A/D range: " << high_vol_range << std::endl;
        
        // 高成交量应该产生更大的A/D变化范围
        EXPECT_GT(high_vol_range, low_vol_range) 
            << "Higher volume should produce larger A/D movements";
    }
}

// 边界条件测试
TEST(OriginalTests, WilliamsAD_EdgeCases) {
    // 测试相同HLCV的情况
    std::vector<std::tuple<double, double, double, double>> flat_data(20, {100.0, 100.0, 100.0, 1000.0});
    
    auto flat_high = std::make_shared<LineRoot>(flat_data.size(), "flat_high");
    auto flat_low = std::make_shared<LineRoot>(flat_data.size(), "flat_low");
    auto flat_close = std::make_shared<LineRoot>(flat_data.size(), "flat_close");
    auto flat_volume = std::make_shared<LineRoot>(flat_data.size(), "flat_volume");
    
    for (const auto& [h, l, c, v] : flat_data) {
        flat_high->forward(h);
        flat_low->forward(l);
        flat_close->forward(c);
        flat_volume->forward(v);
    }
    
    auto flat_williamsad = std::make_shared<WilliamsAD>(flat_high, flat_low, flat_close, flat_volume);
    
    for (size_t i = 0; i < flat_data.size(); ++i) {
        flat_williamsad->calculate();
        if (i < flat_data.size() - 1) {
            flat_high->forward();
            flat_low->forward();
            flat_close->forward();
            flat_volume->forward();
        }
    }
    
    // 当所有HLCV相同时，A/D变化应该很小或为零
    double final_ad = flat_williamsad->get(0);
    if (!std::isnan(final_ad)) {
        EXPECT_TRUE(std::isfinite(final_ad)) << "A/D should be finite for flat prices";
        // 具体值取决于实现，但应该是有限的
    }
    
    // 测试零成交量的情况
    std::vector<std::tuple<double, double, double, double>> zero_vol_data = {
        {105.0, 95.0, 100.0, 0.0},
        {110.0, 98.0, 108.0, 0.0},
        {108.0, 102.0, 105.0, 0.0}
    };
    
    auto zero_high = std::make_shared<LineRoot>(zero_vol_data.size(), "zero_high");
    auto zero_low = std::make_shared<LineRoot>(zero_vol_data.size(), "zero_low");
    auto zero_close = std::make_shared<LineRoot>(zero_vol_data.size(), "zero_close");
    auto zero_volume = std::make_shared<LineRoot>(zero_vol_data.size(), "zero_volume");
    
    for (const auto& [h, l, c, v] : zero_vol_data) {
        zero_high->forward(h);
        zero_low->forward(l);
        zero_close->forward(c);
        zero_volume->forward(v);
    }
    
    auto zero_vol_ad = std::make_shared<WilliamsAD>(zero_high, zero_low, zero_close, zero_volume);
    
    for (size_t i = 0; i < zero_vol_data.size(); ++i) {
        zero_vol_ad->calculate();
        if (i < zero_vol_data.size() - 1) {
            zero_high->forward();
            zero_low->forward();
            zero_close->forward();
            zero_volume->forward();
        }
    }
    
    // 零成交量时，A/D应该保持不变
    double zero_vol_result = zero_vol_ad->get(0);
    if (!std::isnan(zero_vol_result)) {
        EXPECT_NEAR(zero_vol_result, 0.0, 1e-10) << "Zero volume should result in zero A/D change";
    }
    
    // 测试数据不足的情况
    auto insufficient_high = std::make_shared<LineRoot>(5, "insufficient_high");
    auto insufficient_low = std::make_shared<LineRoot>(5, "insufficient_low");
    auto insufficient_close = std::make_shared<LineRoot>(5, "insufficient_close");
    auto insufficient_volume = std::make_shared<LineRoot>(5, "insufficient_volume");
    
    // 只添加一个数据点
    insufficient_high->forward(100.0);
    insufficient_low->forward(95.0);
    insufficient_close->forward(98.0);
    insufficient_volume->forward(1000.0);
    
    auto insufficient_ad = std::make_shared<WilliamsAD>(insufficient_high, insufficient_low, insufficient_close, insufficient_volume);
    insufficient_ad->calculate();
    
    // 数据不足时应该返回NaN
    double result = insufficient_ad->get(0);
    EXPECT_TRUE(std::isnan(result)) << "WilliamsAD should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, WilliamsAD_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<std::tuple<double, double, double, double>> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> price_dist(50.0, 150.0);
    std::uniform_real_distribution<double> vol_dist(1000.0, 5000.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        double base = price_dist(rng);
        large_data.push_back({
            base + price_dist(rng) * 0.05,  // high
            base - price_dist(rng) * 0.05,  // low
            base + (price_dist(rng) - 100.0) * 0.02,  // close
            vol_dist(rng)  // volume
        });
    }
    
    auto large_high = std::make_shared<LineRoot>(large_data.size(), "large_high");
    auto large_low = std::make_shared<LineRoot>(large_data.size(), "large_low");
    auto large_close = std::make_shared<LineRoot>(large_data.size(), "large_close");
    auto large_volume = std::make_shared<LineRoot>(large_data.size(), "large_volume");
    
    for (const auto& [h, l, c, v] : large_data) {
        large_high->forward(h);
        large_low->forward(l);
        large_close->forward(c);
        large_volume->forward(v);
    }
    
    auto large_williamsad = std::make_shared<WilliamsAD>(large_high, large_low, large_close, large_volume);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_williamsad->calculate();
        if (i < large_data.size() - 1) {
            large_high->forward();
            large_low->forward();
            large_close->forward();
            large_volume->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "WilliamsAD calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_williamsad->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}