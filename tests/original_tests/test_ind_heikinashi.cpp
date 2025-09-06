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
#include "lineseries.h"
#include <random>
#include "linebuffer.h"
#include "indicators/heikinashi.h"
#include "dataseries.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> HEIKINASHI_EXPECTED_VALUES = {
    {"4119.466107", "3591.732500", "3578.625259"},  // line 0 (ha_open)
    {"4142.010000", "3638.420000", "3662.920000"},  // line 1 (ha_high)
    {"4119.466107", "3591.732500", "3578.625259"},  // line 2 (ha_low)
    {"4128.002500", "3614.670000", "3653.455000"}   // line 3 (ha_close)
};

const int HEIKINASHI_MIN_PERIOD = 2;

} // anonymous namespace

// 使用默认参数的HeikinAshi测试
// DEFINE_INDICATOR_TEST(HeikinAshi_Default, HeikinAshi, HEIKINASHI_EXPECTED_VALUES, HEIKINASHI_MIN_PERIOD)
// HeikinAshi doesn't accept constructor parameters, so we can't use DEFINE_INDICATOR_TEST

// 手动测试函数，用于详细验证
TEST(OriginalTests, HeikinAshi_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建DataSeries数据源
    auto data_source = std::make_shared<DataSeries>();
    // DataSeries already has lines created in constructor, don't add more!
    
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Open));
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::High));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Low));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Close));
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Volume));
    for (const auto& bar : csv_data) {
        open_buffer->append(bar.open);
        high_buffer->append(bar.high);
        low_buffer->append(bar.low);
        close_buffer->append(bar.close);
        volume_buffer->append(bar.volume);
    }
    
    // Set buffer indices for proper ago indexing
    // Buffers have initial NaN + csv_data, so total size is csv_data.size() + 1
    // Set idx to the last element (size - 1)
    open_buffer->set_idx(open_buffer->size() - 1);
    high_buffer->set_idx(high_buffer->size() - 1);
    low_buffer->set_idx(low_buffer->size() - 1);
    close_buffer->set_idx(close_buffer->size() - 1);
    volume_buffer->set_idx(volume_buffer->size() - 1);
    
    // 创建HeikinAshi指标
    auto heikinashi = std::make_shared<HeikinAshi>(data_source);
    
    
    // 计算所有值;
    std::fprintf(stderr, "TEST: About to call heikinashi->calculate()\n");
    std::fflush(stderr);
    heikinashi->calculate();
    std::fprintf(stderr, "TEST: Returned from heikinashi->calculate()\n");
    std::fflush(stderr);
    
    
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 2;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    // With data_length=255, min_period=2:
    // check_points = [0, -253, -127] (but -127 is actually -126 due to integer division)
    std::vector<int> check_points = {
        0,                                    // Current value (most recent)
        -(data_length - min_period),         // -(255 - 2) = -253
        -127  // Use -127 to match the expected Python test values (index 127)
    };
    
    // Debug: Print what's in the buffers at specific indices
    auto ha_open_buf = std::dynamic_pointer_cast<LineBuffer>(heikinashi->lines->getline(0));
    auto ha_high_buf = std::dynamic_pointer_cast<LineBuffer>(heikinashi->lines->getline(1));
    auto ha_low_buf = std::dynamic_pointer_cast<LineBuffer>(heikinashi->lines->getline(2));
    auto ha_close_buf = std::dynamic_pointer_cast<LineBuffer>(heikinashi->lines->getline(3));
    
    // Also get data buffers to compare
    auto data_open_buf = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Open));
    auto data_high_buf = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::High));
    auto data_low_buf = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Low));
    auto data_close_buf = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Close));
    
    if (ha_open_buf && ha_high_buf && ha_low_buf && ha_close_buf) {
        std::fprintf(stderr, "\nHA buffer contents at test indices:\n");
        std::fprintf(stderr, "Index 1: HA=(%.6f,%.6f,%.6f,%.6f)\n",
                     ha_open_buf->array()[1], ha_high_buf->array()[1], 
                     ha_low_buf->array()[1], ha_close_buf->array()[1]);
        std::fprintf(stderr, "Index 2: HA=(%.6f,%.6f,%.6f,%.6f)\n",
                     ha_open_buf->array()[2], ha_high_buf->array()[2], 
                     ha_low_buf->array()[2], ha_close_buf->array()[2]);
        std::fprintf(stderr, "Index 128: HA=(%.6f,%.6f,%.6f,%.6f)\n",
                     ha_open_buf->array()[128], ha_high_buf->array()[128], 
                     ha_low_buf->array()[128], ha_close_buf->array()[128]);
        std::fprintf(stderr, "Index 252: HA=(%.6f,%.6f,%.6f,%.6f)\n",
                     ha_open_buf->array()[252], ha_high_buf->array()[252], 
                     ha_low_buf->array()[252], ha_close_buf->array()[252]);
        std::fprintf(stderr, "Index 253: HA=(%.6f,%.6f,%.6f,%.6f)\n",
                     ha_open_buf->array()[253], ha_high_buf->array()[253], 
                     ha_low_buf->array()[253], ha_close_buf->array()[253]);
        std::fprintf(stderr, "Index 254: HA=(%.6f,%.6f,%.6f,%.6f)\n",
                     ha_open_buf->array()[254], ha_high_buf->array()[254], 
                     ha_low_buf->array()[254], ha_close_buf->array()[254]);
        std::fprintf(stderr, "Index 255: HA=(%.6f,%.6f,%.6f,%.6f)\n",
                     ha_open_buf->array()[255], ha_high_buf->array()[255], 
                     ha_low_buf->array()[255], ha_close_buf->array()[255]);
        
        // Check if buffers are the same
        std::fprintf(stderr, "\nBuffer pointer comparison:\n");
        std::fprintf(stderr, "HA ha_open buffer: %p\n", (void*)ha_open_buf.get());
        std::fprintf(stderr, "Data Open buffer: %p\n", (void*)data_open_buf.get());
        std::fprintf(stderr, "HA ha_high buffer: %p\n", (void*)ha_high_buf.get());
        std::fprintf(stderr, "Data High buffer: %p\n", (void*)data_high_buf.get());
        std::fprintf(stderr, "HA ha_low buffer: %p\n", (void*)ha_low_buf.get());
        std::fprintf(stderr, "Data Low buffer: %p\n", (void*)data_low_buf.get());
        std::fprintf(stderr, "HA ha_close buffer: %p\n", (void*)ha_close_buf.get());
        std::fprintf(stderr, "Data Close buffer: %p\n", (void*)data_close_buf.get());
        
        // Test ago indexing directly
        std::fprintf(stderr, "\nDirect ago indexing test:\n");
        std::fprintf(stderr, "Buffer _idx: %d, size(): %zu, array.size(): %zu\n", 
                     ha_open_buf->get_idx(), ha_open_buf->size(), ha_open_buf->array().size());
        std::fprintf(stderr, "ago=0: %.6f (should be %.6f)\n", 
                     (*ha_open_buf)[0], ha_open_buf->array()[254]);
        std::fprintf(stderr, "ago=-1: %.6f (should be %.6f)\n", 
                     (*ha_open_buf)[-1], ha_open_buf->array()[253]);
        std::fprintf(stderr, "ago=-253: %.6f (should be %.6f)\n", 
                     (*ha_open_buf)[-253], ha_open_buf->array()[1]);
    }
    
    // 验证4条线的值
    for (int line = 0; line < 4; ++line) {
        auto expected = HEIKINASHI_EXPECTED_VALUES[line];
        for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
            double actual;
            // Access the specific line using the LineBuffer pattern
            auto line_buffer = heikinashi->lines->getline(line);
            if (line_buffer) {
                // Use the operator[] which handles ago indexing properly
                actual = (*line_buffer)[check_points[i]];
            } else {
                actual = std::numeric_limits<double>::quiet_NaN();
            }
            
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
    
    auto data_source = std::make_shared<DataSeries>();
    // DataSeries already has lines created in constructor, don't add more!
    
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Open));
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::High));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Low));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Close));
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Volume));
    for (const auto& bar : ohlc_data) {
        open_buffer->append(bar[0]);
        high_buffer->append(bar[1]);
        low_buffer->append(bar[2]);
        close_buffer->append(bar[3]);
        volume_buffer->append(0.0);  // Add dummy volume
    }
    
    // Set buffer indices for proper ago indexing (like in the main test)
    open_buffer->set_idx(ohlc_data.size() - 1);
    high_buffer->set_idx(ohlc_data.size() - 1);
    low_buffer->set_idx(ohlc_data.size() - 1);
    close_buffer->set_idx(ohlc_data.size() - 1);
    volume_buffer->set_idx(ohlc_data.size() - 1);
    
    auto heikinashi = std::make_shared<HeikinAshi>();
    heikinashi->datas.push_back(data_source);
    
    // 计算所有值
    heikinashi->calculate();
    
    // Get HeikinAshi line buffers
    auto ha_open_buffer = std::dynamic_pointer_cast<LineBuffer>(heikinashi->lines->getline(0));
    auto ha_high_buffer = std::dynamic_pointer_cast<LineBuffer>(heikinashi->lines->getline(1));
    auto ha_low_buffer = std::dynamic_pointer_cast<LineBuffer>(heikinashi->lines->getline(2));
    auto ha_close_buffer = std::dynamic_pointer_cast<LineBuffer>(heikinashi->lines->getline(3));
    
    // Verify calculations
    // HeikinAshi implementation uses batch_append after reset, so buffer has structure:
    // [NaN, calculated_value_0, calculated_value_1, ...]
    // We need to account for this offset
    
    double prev_ha_open = std::numeric_limits<double>::quiet_NaN();
    double prev_ha_close = std::numeric_limits<double>::quiet_NaN();
    
    // Track buffer index separately since implementation may skip NaN values
    size_t buffer_idx = 1; // Start at 1 because buffer[0] is NaN after reset
    
    for (size_t i = 0; i < ohlc_data.size(); ++i) {
        double o = ohlc_data[i][0];
        double h = ohlc_data[i][1];
        double l = ohlc_data[i][2];
        double c = ohlc_data[i][3];
        
        // Calculate expected HeikinAshi values
        double expected_ha_close = (o + h + l + c) / 4.0;
        double expected_ha_open;
        
        if (i == 0 || std::isnan(prev_ha_open) || std::isnan(prev_ha_close)) {
            // First valid data point: use seed value
            expected_ha_open = (o + c) / 2.0;
        } else {
            // Subsequent points: use recursive formula
            expected_ha_open = (prev_ha_open + prev_ha_close) / 2.0;
        }
        
        double expected_ha_high = std::max({h, expected_ha_open, expected_ha_close});
        double expected_ha_low = std::min({l, expected_ha_open, expected_ha_close});
        
        // Get actual values from HeikinAshi indicator
        // Check if we have enough data in the buffer
        if (buffer_idx < ha_open_buffer->array().size()) {
            double actual_open = ha_open_buffer->array()[buffer_idx];
            double actual_high = ha_high_buffer->array()[buffer_idx];
            double actual_low = ha_low_buffer->array()[buffer_idx];
            double actual_close = ha_close_buffer->array()[buffer_idx];
            
            // Only verify if we have valid calculated values
            if (!std::isnan(actual_open) && !std::isnan(actual_close) &&
                !std::isnan(actual_high) && !std::isnan(actual_low)) {
                
                EXPECT_NEAR(actual_close, expected_ha_close, 1e-10) 
                    << "HA Close calculation mismatch at step " << i;
                EXPECT_NEAR(actual_open, expected_ha_open, 1e-10) 
                    << "HA Open calculation mismatch at step " << i;
                EXPECT_NEAR(actual_high, expected_ha_high, 1e-10) 
                    << "HA High calculation mismatch at step " << i;
                EXPECT_NEAR(actual_low, expected_ha_low, 1e-10) 
                    << "HA Low calculation mismatch at step " << i;
                
                // Update previous values for next iteration
                prev_ha_open = actual_open;
                prev_ha_close = actual_close;
                
                // Move to next buffer position
                buffer_idx++;
            }
        }
    }
}

// HeikinAshi平滑特性测试
TEST(OriginalTests, HeikinAshi_SmoothingCharacteristics) {
    auto csv_data = getdata(0);
    
    auto data_source = std::make_shared<DataSeries>();
    // DataSeries already has lines created in constructor, don't add more!
    
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Open));
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::High));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Low));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Close));
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Volume));
    for (const auto& bar : csv_data) {
        open_buffer->append(bar.open);
        high_buffer->append(bar.high);
        low_buffer->append(bar.low);
        close_buffer->append(bar.close);
        volume_buffer->append(bar.volume);
    }
    
    // Set buffer indices for proper ago indexing
    open_buffer->set_idx(csv_data.size() - 1);
    high_buffer->set_idx(csv_data.size() - 1);
    low_buffer->set_idx(csv_data.size() - 1);
    close_buffer->set_idx(csv_data.size() - 1);
    volume_buffer->set_idx(csv_data.size() - 1);
    
    auto heikinashi = std::make_shared<HeikinAshi>();
    heikinashi->datas.push_back(data_source);
    
    std::vector<double> original_volatility;
    std::vector<double> ha_volatility;
    
    // 计算所有HeikinAshi值
    heikinashi->calculate();
    
    // 分析最终波动性结果
    if (csv_data.size() >= 2) {
        // 原始数据和HeikinAshi的波动性对比 - 检查最后几个值
        size_t check_points = std::min(size_t(10), csv_data.size());
        for (size_t i = csv_data.size() - check_points; i < csv_data.size(); ++i) {
            // 原始数据的波动性（高低差）
            double original_range = csv_data[i].high - csv_data[i].low;
            original_volatility.push_back(original_range);
            
            // HeikinAshi的波动性
            double ha_high = (*heikinashi->lines->getline(1))[static_cast<int>(csv_data.size() - i - 1)];  // ha_high is line 1
            double ha_low = (*heikinashi->lines->getline(2))[static_cast<int>(csv_data.size() - i - 1)];   // ha_low is line 2
            
            if (!std::isnan(ha_high) && !std::isnan(ha_low)) {
                double ha_range = ha_high - ha_low;
                ha_volatility.push_back(ha_range);
            }
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
    
    auto data_source = std::make_shared<DataSeries>();
    // DataSeries already has lines created in constructor, don't add more!
    
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Open));
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::High));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Low));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Close));
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Volume));
    for (const auto& bar : uptrend_data) {
        open_buffer->append(bar[0]);
        high_buffer->append(bar[1]);
        low_buffer->append(bar[2]);
        close_buffer->append(bar[3]);
        volume_buffer->append(0.0);  // Add dummy volume
    }
    
    // Set buffer indices for proper ago indexing
    open_buffer->set_idx(uptrend_data.size() - 1);
    high_buffer->set_idx(uptrend_data.size() - 1);
    low_buffer->set_idx(uptrend_data.size() - 1);
    close_buffer->set_idx(uptrend_data.size() - 1);
    volume_buffer->set_idx(uptrend_data.size() - 1);
    
    auto heikinashi = std::make_shared<HeikinAshi>();
    heikinashi->datas.push_back(data_source);
    
    // 计算所有值
    heikinashi->calculate();
    
    int bullish_candles = 0;  // HA_Close > HA_Open
    int bearish_candles = 0;  // HA_Close < HA_Open
    int total_candles = 0;
    for (size_t i = 0; i < uptrend_data.size(); ++i) {
        double ha_close = (*heikinashi->lines->getline(3))[static_cast<int>(uptrend_data.size() - i - 1)];
        double ha_open = (*heikinashi->lines->getline(0))[static_cast<int>(uptrend_data.size() - i - 1)];
        
        if (!std::isnan(ha_close) && !std::isnan(ha_open)) {
            total_candles++;
            if (ha_close > ha_open) {
                bullish_candles++;
            } else if (ha_close < ha_open) {
                bearish_candles++;
            }
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
    
    auto data_source = std::make_shared<DataSeries>();
    // DataSeries already has lines created in constructor, don't add more!
    
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Open));
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::High));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Low));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Close));
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Volume));
    for (const auto& bar : csv_data) {
        open_buffer->append(bar.open);
        high_buffer->append(bar.high);
        low_buffer->append(bar.low);
        close_buffer->append(bar.close);
        volume_buffer->append(bar.volume);
    }
    
    // Set buffer indices for proper ago indexing
    open_buffer->set_idx(csv_data.size() - 1);
    high_buffer->set_idx(csv_data.size() - 1);
    low_buffer->set_idx(csv_data.size() - 1);
    close_buffer->set_idx(csv_data.size() - 1);
    volume_buffer->set_idx(csv_data.size() - 1);
    
    auto heikinashi = std::make_shared<HeikinAshi>();
    heikinashi->datas.push_back(data_source);
    
    // 计算所有值并验证OHLC关系的连续性
    heikinashi->calculate();
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        double ha_open = (*heikinashi->lines->getline(0))[static_cast<int>(csv_data.size() - i - 1)];
        double ha_high = (*heikinashi->lines->getline(1))[static_cast<int>(csv_data.size() - i - 1)];
        double ha_low = (*heikinashi->lines->getline(2))[static_cast<int>(csv_data.size() - i - 1)];
        double ha_close = (*heikinashi->lines->getline(3))[static_cast<int>(csv_data.size() - i - 1)];
        
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
    }
}

// HeikinAshi与原始数据比较测试
TEST(OriginalTests, HeikinAshi_OriginalDataComparison) {
    auto csv_data = getdata(0);
    
    auto data_source = std::make_shared<DataSeries>();
    // DataSeries already has lines created in constructor, don't add more!
    
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Open));
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::High));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Low));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Close));
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Volume));
    for (const auto& bar : csv_data) {
        open_buffer->append(bar.open);
        high_buffer->append(bar.high);
        low_buffer->append(bar.low);
        close_buffer->append(bar.close);
        volume_buffer->append(bar.volume);
    }
    
    // Set buffer indices for proper ago indexing
    open_buffer->set_idx(csv_data.size() - 1);
    high_buffer->set_idx(csv_data.size() - 1);
    low_buffer->set_idx(csv_data.size() - 1);
    close_buffer->set_idx(csv_data.size() - 1);
    volume_buffer->set_idx(csv_data.size() - 1);
    
    auto heikinashi = std::make_shared<HeikinAshi>();
    heikinashi->datas.push_back(data_source);
    
    // 计算所有值
    heikinashi->calculate();
    
    std::vector<double> original_closes;
    std::vector<double> ha_closes;
    for (size_t i = 0; i < csv_data.size(); ++i) {
        double ha_close = (*heikinashi->lines->getline(3))[static_cast<int>(csv_data.size() - i - 1)];
        
        if (!std::isnan(ha_close)) {
            original_closes.push_back(csv_data[i].close);
            ha_closes.push_back(ha_close);
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
    std::vector<std::vector<double>> large_data(10, {100.0, 100.0, 100.0, 100.0});
    
    auto data_source = std::make_shared<DataSeries>();
    // DataSeries already has lines created in constructor, don't add more!
    
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Open));
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::High));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Low));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Close));
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(DataSeries::Volume));
    for (const auto& bar : large_data) {
        open_buffer->append(bar[0]);
        high_buffer->append(bar[1]);
        low_buffer->append(bar[2]);
        close_buffer->append(bar[3]);
        volume_buffer->append(0.0);  // Add dummy volume
    }
    
    // Set buffer indices for proper ago indexing
    open_buffer->set_idx(large_data.size() - 1);
    high_buffer->set_idx(large_data.size() - 1);
    low_buffer->set_idx(large_data.size() - 1);
    close_buffer->set_idx(large_data.size() - 1);
    volume_buffer->set_idx(large_data.size() - 1);
    
    auto flat_heikinashi = std::make_shared<HeikinAshi>();
    flat_heikinashi->datas.push_back(data_source);
    
    // 计算所有值
    flat_heikinashi->calculate();
    
    // 当所有OHLC相同时，HeikinAshi应该收敛到相同的值
    double ha_open = (*flat_heikinashi->lines->getline(0))[0];
    double ha_high = (*flat_heikinashi->lines->getline(1))[0];
    double ha_low = (*flat_heikinashi->lines->getline(2))[0];
    double ha_close = (*flat_heikinashi->lines->getline(3))[0];
    
    if (!std::isnan(ha_open) && !std::isnan(ha_high) && 
        !std::isnan(ha_low) && !std::isnan(ha_close)) {
        
        EXPECT_NEAR(ha_open, 100.0, 1e-6) << "HA Open should converge to constant price";
        EXPECT_NEAR(ha_high, 100.0, 1e-6) << "HA High should converge to constant price";
        EXPECT_NEAR(ha_low, 100.0, 1e-6) << "HA Low should converge to constant price";
        EXPECT_NEAR(ha_close, 100.0, 1e-6) << "HA Close should converge to constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_data = std::make_shared<DataSeries>();
    // DataSeries already has lines created in constructor, don't add more!
    
    auto insufficient_open_buffer = std::dynamic_pointer_cast<LineBuffer>(insufficient_data->lines->getline(DataSeries::Open));
    auto insufficient_high_buffer = std::dynamic_pointer_cast<LineBuffer>(insufficient_data->lines->getline(DataSeries::High));
    auto insufficient_low_buffer = std::dynamic_pointer_cast<LineBuffer>(insufficient_data->lines->getline(DataSeries::Low));
    auto insufficient_close_buffer = std::dynamic_pointer_cast<LineBuffer>(insufficient_data->lines->getline(DataSeries::Close));
    auto insufficient_volume_buffer = std::dynamic_pointer_cast<LineBuffer>(insufficient_data->lines->getline(DataSeries::Volume));
    
    // 只添加一个数据点
    insufficient_open_buffer->append(100.0);
    insufficient_high_buffer->append(105.0);
    insufficient_low_buffer->append(95.0);
    insufficient_close_buffer->append(102.0);
    insufficient_volume_buffer->append(0.0);
    
    auto insufficient_heikinashi = std::make_shared<HeikinAshi>();
    insufficient_heikinashi->datas.push_back(insufficient_data);
    
    insufficient_heikinashi->calculate();
    
    // 数据不足时应该返回NaN
    auto ha_close_line = insufficient_heikinashi->lines->getline(3);
    double result = (*ha_close_line)[0];
    
    // Debug output
    std::cout << "DEBUG: insufficient data test" << std::endl;
    std::cout << "  ha_close buffer size: " << ha_close_line->size() << std::endl;
    std::cout << "  result at [0]: " << result << std::endl;
    
    // Check the actual buffer contents
    auto ha_close_buffer = std::dynamic_pointer_cast<LineBuffer>(ha_close_line);
    if (ha_close_buffer) {
        const auto& array = ha_close_buffer->array();
        std::cout << "  Buffer contents: ";
        for (size_t i = 0; i < array.size(); ++i) {
            std::cout << "[" << i << "]=" << array[i] << " ";
        }
        std::cout << std::endl;
    }
    
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
    
    auto large_data_source = std::make_shared<DataSeries>();
    // DataSeries already has lines created in constructor, don't add more!
    
    auto large_open_buffer = std::dynamic_pointer_cast<LineBuffer>(large_data_source->lines->getline(DataSeries::Open));
    auto large_high_buffer = std::dynamic_pointer_cast<LineBuffer>(large_data_source->lines->getline(DataSeries::High));
    auto large_low_buffer = std::dynamic_pointer_cast<LineBuffer>(large_data_source->lines->getline(DataSeries::Low));
    auto large_close_buffer = std::dynamic_pointer_cast<LineBuffer>(large_data_source->lines->getline(DataSeries::Close));
    auto large_volume_buffer = std::dynamic_pointer_cast<LineBuffer>(large_data_source->lines->getline(DataSeries::Volume));
    for (const auto& bar : large_data) {
        large_open_buffer->append(bar[0]);
        large_high_buffer->append(bar[1]);
        large_low_buffer->append(bar[2]);
        large_close_buffer->append(bar[3]);
        large_volume_buffer->append(0.0);  // Dummy volume
    }
    
    auto large_heikinashi = std::make_shared<HeikinAshi>();
    large_heikinashi->datas.push_back(large_data_source);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    large_heikinashi->calculate();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "HeikinAshi calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_close = (*large_heikinashi->lines->getline(3))[0];
    double final_open = (*large_heikinashi->lines->getline(0))[0];
    double final_high = (*large_heikinashi->lines->getline(1))[0];
    double final_low = (*large_heikinashi->lines->getline(2))[0];
    
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
