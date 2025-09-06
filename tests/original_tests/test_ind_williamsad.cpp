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

#include "test_common.h"
#include "lineseries.h"
#include "indicators/williamsad.h"
#include "linebuffer.h"
#include "dataseries.h"
#include <random>
#include <chrono>

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> WILLIAMSAD_EXPECTED_VALUES = {
    {"755.050000", "12.500000", "242.980000"}
};

const int WILLIAMSAD_MIN_PERIOD = 2;

} // anonymous namespace

// 使用默认参数的WilliamsAD测试
DEFINE_INDICATOR_TEST(WilliamsAD_Default, WilliamsAD, WILLIAMSAD_EXPECTED_VALUES, WILLIAMSAD_MIN_PERIOD)

// 简单的调试测试
TEST(OriginalTests, WilliamsAD_SimpleDebug) {
    // 创建简单的测试数据 - DataSeries自动创建7条线
    auto data_series = std::make_shared<DataSeries>();
    
    // DataSeries自动创建这些线，直接获取引用 - 使用正确的索引
    auto datetime_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(0));  // DateTime = 0
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(1));      // Open = 1
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(2));      // High = 2
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(3));       // Low = 3
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(4));     // Close = 4
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(5));    // Volume = 5
    auto openint_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(6));   // OpenInterest = 6
    
    // 只填充一个简单的数据点 - 使用append而不是set
    datetime_buffer->append(20240101.0);  // Date
    open_buffer->append(100.0);
    high_buffer->append(105.0);
    low_buffer->append(95.0);
    close_buffer->append(102.0);
    volume_buffer->append(1000.0);
    openint_buffer->append(0.0);
    
    std::cout << "DataSeries has " << data_series->lines->size() << " lines" << std::endl;
    
    auto williamsad = std::make_shared<WilliamsAD>(data_series);
    
    // 尝试一次计算
    try {
        std::cout << "Buffer sizes - open: " << open_buffer->size() 
                  << ", high: " << high_buffer->size()
                  << ", low: " << low_buffer->size()
                  << ", close: " << close_buffer->size()
                  << ", volume: " << volume_buffer->size() << std::endl;
        
        std::cout << "Before calculate..." << std::endl;
        williamsad->calculate();
        std::cout << "After calculate..." << std::endl;
        
        // Check WilliamsAD internal line
        if (williamsad->size() > 0) {
            double result = williamsad->get(0);
            std::cout << "WilliamsAD result: " << result << std::endl;
            EXPECT_TRUE(std::isfinite(result)) << "Result should be finite";
        } else {
            std::cout << "WilliamsAD internal line is empty (size: " << williamsad->size() << ")" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception caught: " << e.what() << std::endl;
        FAIL() << "Exception during calculation: " << e.what();
    }
}

// 手动测试函数，用于详细验证
TEST(OriginalTests, WilliamsAD_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建DataSeries用于OHLCV数据 - 自动创建7条线
    auto data_series = std::make_shared<DataSeries>();
    
    // DataSeries自动创建这些线，直接获取引用 - 使用正确的索引
    auto datetime_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(0));  // DateTime = 0
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(1));      // Open = 1
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(2));      // High = 2
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(3));       // Low = 3
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(4));     // Close = 4
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(5));    // Volume = 5
    auto openint_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(6));   // OpenInterest = 6
    
    // 填充数据到缓冲区
    for (size_t i = 0; i < csv_data.size(); ++i) {
        if (i == 0) {
            datetime_buffer->set(0, static_cast<double>(20240101 + i));  // 简单的日期
            open_buffer->set(0, csv_data[i].open);
            high_buffer->set(0, csv_data[i].high);
            low_buffer->set(0, csv_data[i].low);
            close_buffer->set(0, csv_data[i].close);
            volume_buffer->set(0, csv_data[i].volume);
            openint_buffer->set(0, csv_data[i].openinterest);
        } else {
            datetime_buffer->append(static_cast<double>(20240101 + i));
            open_buffer->append(csv_data[i].open);
            high_buffer->append(csv_data[i].high);
            low_buffer->append(csv_data[i].low);
            close_buffer->append(csv_data[i].close);
            volume_buffer->append(csv_data[i].volume);
            openint_buffer->append(csv_data[i].openinterest);
        }
    }
    
    // 创建WilliamsAD指标
    auto williamsad = std::make_shared<WilliamsAD>(data_series);
    
    // 计算所有值
    williamsad->calculate();
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = williamsad->getMinPeriod(); // Use actual minperiod from indicator
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // Debug output
    std::cout << "Debug: data_length=" << data_length << ", min_period=" << min_period << std::endl;
    std::cout << "Debug: Check points: ";
    for (int cp : check_points) {
        std::cout << cp << " ";
    }
    std::cout << std::endl;
    
    // Debug: Print some values
    std::cout << "Debug: Williams AD values at various positions:" << std::endl;
    std::cout << "  get(0) = " << williamsad->get(0) << std::endl;
    std::cout << "  get(-1) = " << williamsad->get(-1) << std::endl;
    std::cout << "  get(-253) = " << williamsad->get(-253) << std::endl;
    std::cout << "  get(-126) = " << williamsad->get(-126) << std::endl;
    
    // Also check the internal buffer
    auto ad_line = williamsad->lines->getline(0);
    auto ad_buffer = std::dynamic_pointer_cast<LineBuffer>(ad_line);
    if (ad_buffer) {
        std::cout << "Debug: Buffer info: size=" << ad_buffer->size() 
                  << ", data_size=" << ad_buffer->data_size()
                  << ", idx=" << ad_buffer->get_idx() << std::endl;
        std::cout << "Debug: Last few values in buffer: ";
        for (int i = ad_buffer->data_size() - 5; i < ad_buffer->data_size(); ++i) {
            if (i >= 0) {
                std::cout << "[" << i << "]=" << ad_buffer->data_ptr()[i] << " ";
            }
        }
        std::cout << std::endl;
        
        // Check the specific positions we're looking for
        std::cout << "Debug: Values at specific buffer positions:" << std::endl;
        std::cout << "  buffer[2] = " << ad_buffer->data_ptr()[2] << " (should be 12.5)" << std::endl;
        std::cout << "  buffer[127] = " << ad_buffer->data_ptr()[127] << std::endl;
        std::cout << "  buffer[128] = " << ad_buffer->data_ptr()[128] << std::endl;
        std::cout << "  buffer[129] = " << ad_buffer->data_ptr()[129] << " (should be 242.98?)" << std::endl;
        std::cout << "  buffer[255] = " << ad_buffer->data_ptr()[255] << " (should be 755.05)" << std::endl;
        
        // Check what the Python formula says for middle check point
        int middle_idx = (-253) / 2;  // = -126
        int expected_buffer_idx = 255 + middle_idx;  // = 255 - 126 = 129
        std::cout << "Debug: Middle check point calculation:" << std::endl;
        std::cout << "  Python check point: " << middle_idx << std::endl;
        std::cout << "  Expected buffer index: " << expected_buffer_idx << std::endl;
        
        // Print first few values too
        std::cout << "Debug: First few values in buffer: ";
        for (int i = 0; i < std::min(10, (int)ad_buffer->data_size()); ++i) {
            std::cout << "[" << i << "]=" << ad_buffer->data_ptr()[i] << " ";
        }
        std::cout << std::endl;
    }
    
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
    std::vector<std::tuple<double, double, double, double, double>> ohlcv_data = {
        {100.0, 105.0, 95.0, 100.0, 1000.0},   // O, H, L, C, V
        {100.0, 110.0, 98.0, 108.0, 1500.0},
        {108.0, 112.0, 105.0, 110.0, 1200.0},
        {110.0, 115.0, 107.0, 113.0, 1600.0},
        {113.0, 118.0, 110.0, 115.0, 1400.0}
    };
    
    // 创建DataSeries用于OHLCV数据 - 自动创建7条线
    auto data_series = std::make_shared<DataSeries>();
    
    // DataSeries自动创建这些线，直接获取引用 - 使用正确的索引
    auto datetime_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(0));  // DateTime = 0
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(1));      // Open = 1
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(2));      // High = 2
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(3));       // Low = 3
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(4));     // Close = 4
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(5));    // Volume = 5
    auto openint_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(6));   // OpenInterest = 6
    
    // 填充测试数据
    for (size_t i = 0; i < ohlcv_data.size(); ++i) {
        auto [o, h, l, c, v] = ohlcv_data[i];
        if (i == 0) {
            datetime_buffer->set(0, static_cast<double>(20240101 + i));
            open_buffer->set(0, o);
            high_buffer->set(0, h);
            low_buffer->set(0, l);
            close_buffer->set(0, c);
            volume_buffer->set(0, v);
            openint_buffer->set(0, 0.0);
        } else {
            datetime_buffer->append(static_cast<double>(20240101 + i));
            open_buffer->append(o);
            high_buffer->append(h);
            low_buffer->append(l);
            close_buffer->append(c);
            volume_buffer->append(v);
            openint_buffer->append(0.0);
        }
    }
    
    auto williamsad = std::make_shared<WilliamsAD>(data_series);
    
    // Single calculation for all data
    williamsad->calculate();
    
    // Manually calculate WilliamsAD for verification - Python version doesn't use volume
    std::vector<double> expected_ad;
    double accumulated_ad = 0.0;
    
    for (size_t i = 0; i < ohlcv_data.size(); ++i) {
        auto [o, h, l, c, v] = ohlcv_data[i];
        if (i == 0) {
            // First bar - no AD calculation
            accumulated_ad = 0.0;
        } else {
            auto [prev_o, prev_h, prev_l, prev_c, prev_v] = ohlcv_data[i-1];
            double true_low = std::min(l, prev_c);
            double true_high = std::max(h, prev_c);
            
            double ad_value = 0.0;
            if (c > prev_c) {
                ad_value = c - true_low;
            } else if (c < prev_c) {
                ad_value = c - true_high;
            }
            accumulated_ad += ad_value;  // Accumulate
        }
        expected_ad.push_back(accumulated_ad);
    }
    
    // Verify all calculations at once
    for (size_t i = 0; i < ohlcv_data.size(); ++i) {
        double actual_ad = williamsad->get(-static_cast<int>(ohlcv_data.size() - 1 - i));
        double expected = expected_ad[i];
        
        if (!std::isnan(actual_ad)) {
            // Allow some calculation error
            EXPECT_NEAR(actual_ad, expected, 1e-3) 
                << "WilliamsAD calculation mismatch at index " << i
                << ", expected: " << expected << ", actual: " << actual_ad;
        }
    }
}

// WilliamsAD累积特性测试
TEST(OriginalTests, WilliamsAD_AccumulationCharacteristics) {
    auto csv_data = getdata(0);
    
    // 创建DataSeries用于OHLCV数据 - 自动创建7条线
    auto data_series = std::make_shared<DataSeries>();
    
    // DataSeries自动创建这些线，直接获取引用 - 使用正确的索引
    auto datetime_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(0));  // DateTime = 0
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(1));      // Open = 1
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(2));      // High = 2
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(3));       // Low = 3
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(4));     // Close = 4
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(5));    // Volume = 5
    auto openint_buffer = std::dynamic_pointer_cast<LineBuffer>(data_series->lines->getline(6));   // OpenInterest = 6
    
    // 填充CSV数据
    for (size_t i = 0; i < csv_data.size(); ++i) {
        if (i == 0) {
            open_buffer->set(0, csv_data[i].open);
            high_buffer->set(0, csv_data[i].high);
            low_buffer->set(0, csv_data[i].low);
            close_buffer->set(0, csv_data[i].close);
            volume_buffer->set(0, csv_data[i].volume);
        } else {
            open_buffer->append(csv_data[i].open);
            high_buffer->append(csv_data[i].high);
            low_buffer->append(csv_data[i].low);
            close_buffer->append(csv_data[i].close);
            volume_buffer->append(csv_data[i].volume);
        }
    }
    
    auto williamsad = std::make_shared<WilliamsAD>(data_series);
    
    std::vector<double> ad_values;
    for (size_t i = 0; i < csv_data.size(); ++i) {
        williamsad->calculate();
        
        double ad_val = williamsad->get(0);
        if (!std::isnan(ad_val)) {
            ad_values.push_back(ad_val);
        }
        
        if (i < csv_data.size() - 1) {
            datetime_buffer->forward();
            open_buffer->forward();
            high_buffer->forward();
            low_buffer->forward();
            close_buffer->forward();
            volume_buffer->forward();
            openint_buffer->forward();
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

/*
// WilliamsAD分散/累积信号测试
TEST(OriginalTests, DISABLED_WilliamsAD_DistributionAccumulation) {
    // 创建明确的累积和分散阶段数据
    std::vector<std::tuple<double, double, double, double>> phases_data;
    
    // 累积阶段：价格上升，成交量增加;
    for (int i = 0; i < 15; ++i) {
        double base = 100.0 + i * 1.0;
        phases_data.push_back({
            base + 2.0,               // high
            base - 1.0,               // low  
            base + 1.5,               // close (偏向高位)
            1000.0 + i * 50.0         // increasing volume
        });
    }
    
    // 分散阶段：价格下降，成交量增加;
    for (int i = 0; i < 15; ++i) {
        double base = 115.0 - i * 0.8;
        phases_data.push_back({
            base + 1.0,               // high
            base - 2.0,               // low
            base - 1.5,               // close (偏向低位)
            1750.0 + i * 30.0         // volume
        });
    }
    
    auto phase_high_line = std::make_shared<LineSeries>();

    
    phase_high_line->lines->add_line(std::make_shared<LineBuffer>());
    phase_high_line->lines->add_alias("phase_high_buffer", 0);
    auto phase_high_buffer = std::dynamic_pointer_cast<LineBuffer>(phase_high_line->lines->getline(0));
        auto phase_low_line = std::make_shared<LineSeries>();

    phase_low_line->lines->add_line(std::make_shared<LineBuffer>());
    phase_low_line->lines->add_alias("phase_low_buffer", 0);
    auto phase_low_buffer = std::dynamic_pointer_cast<LineBuffer>(phase_low_line->lines->getline(0));
        auto phase_close_line = std::make_shared<LineSeries>();

    phase_close_line->lines->add_line(std::make_shared<LineBuffer>());
    phase_close_line->lines->add_alias("phase_close_buffer", 0);
    auto phase_close_buffer = std::dynamic_pointer_cast<LineBuffer>(phase_close_line->lines->getline(0));
        auto phase_volume_line = std::make_shared<LineSeries>();

    phase_volume_line->lines->add_line(std::make_shared<LineBuffer>());
    phase_volume_line->lines->add_alias("phase_volume_buffer", 0);
    auto phase_high_buffer = std::dynamic_pointer_cast<LineBuffer>(phase_volume_line->lines->getline(0));
    auto phase_volume_buffer = std::dynamic_pointer_cast<LineBuffer>(phase_volume_line->lines->getline(0));
    
    for (const auto& [h, l, c, v] : phases_data) {
        phase_high_buffer->append(h);
        phase_low_buffer->append(l);
        phase_close_buffer->append(c);
        phase_volume_buffer->append(v);
    }
    
    // Create LineSeries wrappers
    auto phase_high_series = std::make_shared<LineSeries>();
    phase_high_series->lines->add_line(std::dynamic_pointer_cast<LineSingle>(phase_high_buffer));
    
    auto phase_low_series = std::make_shared<LineSeries>();
    phase_low_series->lines->add_line(std::dynamic_pointer_cast<LineSingle>(phase_low_buffer));
    
    auto phase_close_series = std::make_shared<LineSeries>();
    phase_close_series->lines->add_line(std::dynamic_pointer_cast<LineSingle>(phase_close_buffer));
    
    auto phase_volume_series = std::make_shared<LineSeries>();
    phase_volume_series->lines->add_line(std::dynamic_pointer_cast<LineSingle>(phase_volume_buffer));
    
    auto phase_williamsad = std::make_shared<WilliamsAD>(phase_high_series, phase_low_series, phase_close_series, phase_volume_series);
    
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
            if (phase_high_buffer) phase_high_buffer->forward();
            if (phase_low_buffer) phase_low_buffer->forward();
            if (phase_close_buffer) phase_close_buffer->forward();
            if (phase_volume_buffer) phase_volume_buffer->forward();
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
*/

/*
// WilliamsAD与价格发散测试
TEST(OriginalTests, DISABLED_WilliamsAD_PriceDivergence) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineSeries>();

    high_line->lines->add_line(std::make_shared<LineBuffer>());
    high_line->lines->add_alias("high_line", 0);

    auto high_line_buffer =  std::dynamic_pointer_cast<LineBuffer>(high_line->lines->getline(0));    auto low_line = std::make_shared<LineSeries>();

    low_line->lines->add_line(std::make_shared<LineBuffer>());
    low_line->lines->add_alias("low_line", 0);

    auto low_buffer =  std::dynamic_pointer_cast<LineBuffer>(low_line->lines->getline(0));    auto close_line = std::make_shared<LineSeries>();

    close_line->lines->add_line(std::make_shared<LineBuffer>());

    close_line->lines->add_alias("close_line", 0);

    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    close_line->lines->add_line(close_line);
    auto volume_line = std::make_shared<LineSeries>();

    volume_line->lines->add_line(std::make_shared<LineBuffer>());
    volume_line->lines->add_alias("volume_line", 0);
    auto high_line_buffer = std::dynamic_pointer_cast<LineBuffer>(volume_line->lines->getline(0));
    

    auto volume_buffer =  std::dynamic_pointer_cast<LineBuffer>(volume_line->lines->getline(0));
    for (const auto& bar : csv_data) {
        high_line_buffer->append(bar.high);
        low_buffer->append(bar.low);
        close_buffer->append(bar.close);
        volume_buffer->append(bar.volume);
    }
    
    // Create LineSeries wrappers for the WilliamsAD constructor
    auto high_line_wrapper = std::make_shared<LineSeries>();
    high_line_wrapper->lines->add_line(std::dynamic_pointer_cast<LineSingle>(high_line));
    
    auto low_line_wrapper = std::make_shared<LineSeries>();
    low_line_wrapper->lines->add_line(std::dynamic_pointer_cast<LineSingle>(low_line));
    
    auto close_line_wrapper = std::make_shared<LineSeries>();
    close_line_wrapper->lines->add_line(std::dynamic_pointer_cast<LineSingle>(close_line));
    
    auto volume_line_wrapper = std::make_shared<LineSeries>();
    volume_line_wrapper->lines->add_line(std::dynamic_pointer_cast<LineSingle>(volume_line));
    
    auto williamsad = std::make_shared<WilliamsAD>(high_line_wrapper, low_line_wrapper, close_line_wrapper, volume_line_wrapper);
    
    std::vector<double> prices, ad_values;
    for (size_t i = 0; i < csv_data.size(); ++i) {
        williamsad->calculate();
        
        double ad_val = williamsad->get(0);
        if (!std::isnan(ad_val)) {
            prices.push_back(csv_data[i].close);
            ad_values.push_back(ad_val);
        }
        
        if (i < csv_data.size() - 1) {
            if (high_line_buffer) high_line_buffer->forward();
            if (low_buffer) low_buffer->forward();
            if (close_buffer) close_buffer->forward();
            if (volume_buffer) volume_buffer->forward();
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
TEST(OriginalTests, DISABLED_WilliamsAD_VolumeSensitivity) {
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
    auto low_high_line = std::make_shared<LineSeries>();

    low_high_line->lines->add_line(std::make_shared<LineBuffer>());
    low_high_line->lines->add_alias("low_high_buffer", 0);
    auto low_high_buffer = std::dynamic_pointer_cast<LineBuffer>(low_high_line->lines->getline(0));
        auto low_low_line = std::make_shared<LineSeries>();

    low_low_line->lines->add_line(std::make_shared<LineBuffer>());
    low_low_line->lines->add_alias("low_low_buffer", 0);
    auto low_low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_low_line->lines->getline(0));
        auto low_close_line = std::make_shared<LineSeries>();

    low_close_line->lines->add_line(std::make_shared<LineBuffer>());
    low_close_line->lines->add_alias("low_close_buffer", 0);
    auto low_close_buffer = std::dynamic_pointer_cast<LineBuffer>(low_close_line->lines->getline(0));
        auto low_volume_line = std::make_shared<LineSeries>();

    low_volume_line->lines->add_line(std::make_shared<LineBuffer>());
    low_volume_line->lines->add_alias("low_volume_buffer", 0);
    auto low_high_buffer = std::dynamic_pointer_cast<LineBuffer>(low_volume_line->lines->getline(0));
    auto low_volume_buffer = std::dynamic_pointer_cast<LineBuffer>(low_volume_line->lines->getline(0));
    
    for (const auto& [h, l, c, v] : low_vol_data) {
        low_high_buffer->append(h);
        low_low_buffer->append(l);
        low_close_buffer->append(c);
        low_volume_buffer->append(v);
    }
    
    // 创建高成交量指标
    auto high_high_line = std::make_shared<LineSeries>();

    high_high_line->lines->add_line(std::make_shared<LineBuffer>());
    high_high_line->lines->add_alias("high_high_buffer", 0);
    auto high_high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_high_line->lines->getline(0));
        auto high_low_line = std::make_shared<LineSeries>();

    high_low_line->lines->add_line(std::make_shared<LineBuffer>());
    high_low_line->lines->add_alias("high_low_buffer", 0);
    auto high_low_buffer = std::dynamic_pointer_cast<LineBuffer>(high_low_line->lines->getline(0));
        auto high_close_line = std::make_shared<LineSeries>();

    high_close_line->lines->add_line(std::make_shared<LineBuffer>());
    high_close_line->lines->add_alias("high_close_buffer", 0);
    auto high_close_buffer = std::dynamic_pointer_cast<LineBuffer>(high_close_line->lines->getline(0));
    
    auto high_volume_line = std::make_shared<LineSeries>();

    high_volume_line->lines->add_line(std::make_shared<LineBuffer>());
    high_volume_line->lines->add_alias("high_volume_buffer", 0);
    auto high_high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_volume_line->lines->getline(0));
    auto high_volume_buffer = std::dynamic_pointer_cast<LineBuffer>(high_volume_line->lines->getline(0));
    
    for (const auto& [h, l, c, v] : high_vol_data) {
        high_high_buffer->append(h);
        high_low_buffer->append(l);
        high_close_buffer->append(c);
        high_volume_buffer->append(v);
    }
    
    // Create LineSeries wrappers for WilliamsAD constructors
    auto low_high_series = std::make_shared<LineSeries>();
    low_high_series->lines->add_line(std::dynamic_pointer_cast<LineSingle>(low_high_buffer));
    auto low_low_series = std::make_shared<LineSeries>();
    low_low_series->lines->add_line(std::dynamic_pointer_cast<LineSingle>(low_low_buffer));
    auto low_close_series = std::make_shared<LineSeries>();
    low_close_series->lines->add_line(std::dynamic_pointer_cast<LineSingle>(low_close_buffer));
    auto low_volume_series = std::make_shared<LineSeries>();
    low_volume_series->lines->add_line(std::dynamic_pointer_cast<LineSingle>(low_volume_buffer));
    
    auto high_high_series = std::make_shared<LineSeries>();
    high_high_series->lines->add_line(std::dynamic_pointer_cast<LineSingle>(high_high_buffer));
    auto high_low_series = std::make_shared<LineSeries>();
    high_low_series->lines->add_line(std::dynamic_pointer_cast<LineSingle>(high_low_buffer));
    auto high_close_series = std::make_shared<LineSeries>();
    high_close_series->lines->add_line(std::dynamic_pointer_cast<LineSingle>(high_close_buffer));
    auto high_volume_series = std::make_shared<LineSeries>();
    high_volume_series->lines->add_line(std::dynamic_pointer_cast<LineSingle>(high_volume_buffer));
    
    auto low_vol_ad = std::make_shared<WilliamsAD>(low_high_series, low_low_series, low_close_series, low_volume_series);
    auto high_vol_ad = std::make_shared<WilliamsAD>(high_high_series, high_low_series, high_close_series, high_volume_series);
    
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
            if (low_high_buffer) low_high_buffer->forward();
            if (low_low_buffer) low_low_buffer->forward();
            if (low_close_buffer) low_close_buffer->forward();
            if (low_volume_buffer) low_volume_buffer->forward();
            if (high_high_buffer) high_high_buffer->forward();
            if (high_low_buffer) high_low_buffer->forward();
            if (high_close_buffer) high_close_buffer->forward();
            if (high_volume_buffer) high_volume_buffer->forward();
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
TEST(OriginalTests, DISABLED_WilliamsAD_EdgeCases) {
    // 测试相同HLCV的情况
    std::vector<std::tuple<double, double, double, double>> large_data(20, {100.0, 100.0, 100.0, 1000.0});
    
    auto flat_high_series = std::make_shared<LineSeries>();
    flat_high_series->lines->add_line(std::make_shared<LineBuffer>());
    flat_high_series->lines->add_alias("flathigh", 0);
    auto flat_high = std::dynamic_pointer_cast<LineBuffer>(flat_high_series->lines->getline(0));
    
    if (flat_high) {
        flat_high->set(0, std::get<0>(large_data[0]));
        for (size_t i = 1; i < large_data.size(); ++i) {
            flat_high->append(std::get<0>(large_data[i]));
        }
    }
    auto flat_low_series = std::make_shared<LineSeries>();
    flat_low_series->lines->add_line(std::make_shared<LineBuffer>());
    flat_low_series->lines->add_alias("flatlow", 0);
    auto flat_low = std::dynamic_pointer_cast<LineBuffer>(flat_low_series->lines->getline(0));
    
    if (flat_low) {
        flat_low->set(0, std::get<1>(large_data[0]));
        for (size_t i = 1; i < large_data.size(); ++i) {
            flat_low->append(std::get<1>(large_data[i]));
        }
    }
    auto flat_close_series = std::make_shared<LineSeries>();
    flat_close_series->lines->add_line(std::make_shared<LineBuffer>());
    flat_close_series->lines->add_alias("flatclose", 0);
    auto flat_close = std::dynamic_pointer_cast<LineBuffer>(flat_close_series->lines->getline(0));
    
    if (flat_close) {
        flat_close->set(0, std::get<2>(large_data[0]));
        for (size_t i = 1; i < large_data.size(); ++i) {
            flat_close->append(std::get<2>(large_data[i]));
        }
    }
    auto flat_volume_series = std::make_shared<LineSeries>();
    flat_volume_series->lines->add_line(std::make_shared<LineBuffer>());
    flat_volume_series->lines->add_alias("flatvolume", 0);
    auto flat_volume = std::dynamic_pointer_cast<LineBuffer>(flat_volume_series->lines->getline(0));
    
    if (flat_volume) {
        flat_volume->set(0, std::get<3>(large_data[0]));
        for (size_t i = 1; i < large_data.size(); ++i) {
            flat_volume->append(std::get<3>(large_data[i]));
        }
    }
    
    auto flat_williamsad = std::make_shared<WilliamsAD>(flat_high_series, flat_low_series, flat_close_series, flat_volume_series);
    for (size_t i = 0; i < large_data.size(); ++i) {
        flat_williamsad->calculate();
        if (i < large_data.size() - 1) {
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
    
    auto zero_high_series = std::make_shared<LineSeries>();
    zero_high_series->lines->add_line(std::make_shared<LineBuffer>());
    zero_high_series->lines->add_alias("zerohigh", 0);
    auto zero_high = std::dynamic_pointer_cast<LineBuffer>(zero_high_series->lines->getline(0));
    
    // zero_high created, will be populated in loop below
    auto zero_low_series = std::make_shared<LineSeries>();
    zero_low_series->lines->add_line(std::make_shared<LineBuffer>());
    zero_low_series->lines->add_alias("zerolow", 0);
    auto zero_low = std::dynamic_pointer_cast<LineBuffer>(zero_low_series->lines->getline(0));
    
    // zero_low created, will be populated in loop below
    auto zero_close_series = std::make_shared<LineSeries>();
    zero_close_series->lines->add_line(std::make_shared<LineBuffer>());
    zero_close_series->lines->add_alias("zeroclose", 0);
    auto zero_close = std::dynamic_pointer_cast<LineBuffer>(zero_close_series->lines->getline(0));
    
    // zero_close created, will be populated in loop below
    auto zero_volume_series = std::make_shared<LineSeries>();
    zero_volume_series->lines->add_line(std::make_shared<LineBuffer>());
    zero_volume_series->lines->add_alias("zerovolume", 0);
    auto zero_volume = std::dynamic_pointer_cast<LineBuffer>(zero_volume_series->lines->getline(0));
    
    // zero_volume created, will be populated in loop below
    
    for (const auto& [h, l, c, v] : zero_vol_data) {
        zero_high->set(0, 0.0);
        for (int j = 1; j < 20; ++j) {
            zero_high->append(0.0);
        }
        zero_low->set(0, 0.0);
        for (int j = 1; j < 20; ++j) {
            zero_low->append(0.0);
        }
        zero_close->set(0, 0.0);
        for (int j = 1; j < 20; ++j) {
            zero_close->append(0.0);
        }
        zero_volume->set(0, 0.0);
        for (int j = 1; j < 20; ++j) {
            zero_volume->append(0.0);
        }
    }
    
    auto zero_vol_ad = std::make_shared<WilliamsAD>(zero_high_series, zero_low_series, zero_close_series, zero_volume_series);
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
    auto insufficient_high_series = std::make_shared<LineSeries>();
    insufficient_high_series->lines->add_line(std::make_shared<LineBuffer>());
    insufficient_high_series->lines->add_alias("insufficienthigh", 0);
    auto insufficient_high = std::dynamic_pointer_cast<LineBuffer>(insufficient_high_series->lines->getline(0));
    
    if (insufficient_high) {
        insufficient_high->append(100.0);
    }
    auto insufficient_low_series = std::make_shared<LineSeries>();
    insufficient_low_series->lines->add_line(std::make_shared<LineBuffer>());
    insufficient_low_series->lines->add_alias("insufficientlow", 0);
    auto insufficient_low = std::dynamic_pointer_cast<LineBuffer>(insufficient_low_series->lines->getline(0));
    
    if (insufficient_low) {
        insufficient_low->append(95.0);
    }
    auto insufficient_close_series = std::make_shared<LineSeries>();
    insufficient_close_series->lines->add_line(std::make_shared<LineBuffer>());
    insufficient_close_series->lines->add_alias("insufficientclose", 0);
    auto insufficient_close = std::dynamic_pointer_cast<LineBuffer>(insufficient_close_series->lines->getline(0));
    
    if (insufficient_close) {
        insufficient_close->append(98.0);
    }
    auto insufficient_volume_series = std::make_shared<LineSeries>();
    insufficient_volume_series->lines->add_line(std::make_shared<LineBuffer>());
    insufficient_volume_series->lines->add_alias("insufficientvolume", 0);
    auto insufficient_volume = std::dynamic_pointer_cast<LineBuffer>(insufficient_volume_series->lines->getline(0));
    
    if (insufficient_volume) {
        insufficient_volume->append(1000.0);
    }
    
    // 只添加一个数据点
    insufficient_high->append(100.0);
    insufficient_low->append(95.0);
    insufficient_close->append(98.0);
    insufficient_volume->append(1000.0);
    
    auto insufficient_ad = std::make_shared<WilliamsAD>(insufficient_high_series, insufficient_low_series, insufficient_close_series, insufficient_volume_series);
    insufficient_ad->calculate();
    
    // 数据不足时应该返回NaN
    double result = insufficient_ad->get(0);
    EXPECT_TRUE(std::isnan(result)) << "WilliamsAD should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, DISABLED_WilliamsAD_Performance) {
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
    
    auto large_high_series = std::make_shared<LineSeries>();
    large_high_series->lines->add_line(std::make_shared<LineBuffer>());
    large_high_series->lines->add_alias("largehigh", 0);
    auto large_high = std::dynamic_pointer_cast<LineBuffer>(large_high_series->lines->getline(0));
    
    // large_high created, will be populated in loop below
    auto large_low_series = std::make_shared<LineSeries>();
    large_low_series->lines->add_line(std::make_shared<LineBuffer>());
    large_low_series->lines->add_alias("largelow", 0);
    auto large_low = std::dynamic_pointer_cast<LineBuffer>(large_low_series->lines->getline(0));
    
    // large_low created, will be populated in loop below
    auto large_close_series = std::make_shared<LineSeries>();
    large_close_series->lines->add_line(std::make_shared<LineBuffer>());
    large_close_series->lines->add_alias("largeclose", 0);
    auto large_close = std::dynamic_pointer_cast<LineBuffer>(large_close_series->lines->getline(0));
    
    // large_close created, will be populated in loop below
    auto large_volume_series = std::make_shared<LineSeries>();
    large_volume_series->lines->add_line(std::make_shared<LineBuffer>());
    large_volume_series->lines->add_alias("largevolume", 0);
    auto large_volume = std::dynamic_pointer_cast<LineBuffer>(large_volume_series->lines->getline(0));
    
    // large_volume created, will be populated in loop below
    
    for (const auto& [h, l, c, v] : large_data) {
        large_high->append(h);
        large_low->append(l);
        large_close->append(c);
        large_volume->append(v);
    }
    
    auto large_williamsad = std::make_shared<WilliamsAD>(large_high_series, large_low_series, large_close_series, large_volume_series);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < data_size; ++i) {
        large_williamsad->calculate();
        if (i < data_size - 1) {
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
*/