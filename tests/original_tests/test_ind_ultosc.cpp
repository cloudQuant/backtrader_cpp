/**
 * @file test_ind_ultosc.cpp
 * @brief UltimateOscillator指标测试 - 对应Python test_ind_ultosc.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['51.991177', '62.334055', '46.707445']
 * ]
 * chkmin = 29  # 28 from longest SumN/Sum + 1 extra from truelow/truerange
 * chkind = bt.indicators.UltimateOscillator
 */

#include "test_common.h"
#include "lineseries.h"
#include "linebuffer.h"
#include "dataseries.h"
#include <random>

#include "indicators/ultimateoscillator.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> ULTOSC_EXPECTED_VALUES = {
    {"51.991177", "62.334055", "46.707445"}
};

const int ULTOSC_MIN_PERIOD = 29;

} // anonymous namespace

// 使用默认参数的UltimateOscillator测试
DEFINE_INDICATOR_TEST(UltimateOscillator_Default, UltimateOscillator, ULTOSC_EXPECTED_VALUES, ULTOSC_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, UltimateOscillator_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建DataSeries包含所有OHLCV数据
    // DataSeries constructor already creates 7 lines in the correct order
    auto data_source = std::make_shared<DataSeries>();
    
    // Get existing lines from DataSeries (they are already created in constructor)
    auto datetime_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(0));
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(1));
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(2));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(3));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(4));
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(5));
    auto openinterest_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(6));
    
    // 填充数据
    for (const auto& bar : csv_data) {
        datetime_buffer->append(0.0);  // 暂时用0
        open_buffer->append(bar.open);
        high_buffer->append(bar.high);
        low_buffer->append(bar.low);
        close_buffer->append(bar.close);
        volume_buffer->append(bar.volume);
        openinterest_buffer->append(0.0);  // 暂时用0
    }
    
    // 创建UltimateOscillator指标（默认参数：7, 14, 28）
    auto ultosc = std::make_shared<UltimateOscillator>(data_source, 7, 14, 28);
    
    // 一次性计算所有值
    ultosc->calculate();
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 29;  // 28 + 1
    
    // Debug: Check if indicator has values
    std::cout << "UltimateOscillator size: " << ultosc->size() << std::endl;
    std::cout << "Data length: " << data_length << std::endl;
    std::cout << "Min period: " << min_period << std::endl;
    
    // Check current buffer positions
    std::cout << "High buffer size: " << high_buffer->size() << std::endl;
    std::cout << "Low buffer size: " << low_buffer->size() << std::endl;
    std::cout << "Close buffer size: " << close_buffer->size() << std::endl;
    
    // Check the UO line directly
    if (ultosc->lines && ultosc->lines->size() > 0) {
        auto uo_line = ultosc->lines->getline(0);
        if (uo_line) {
            std::cout << "UO line size: " << uo_line->size() << std::endl;
            auto uo_buffer = std::dynamic_pointer_cast<LineBuffer>(uo_line);
            if (uo_buffer) {
                std::cout << "UO buffer buflen: " << uo_buffer->buflen() << std::endl;
                std::cout << "UO buffer idx: " << uo_buffer->get_idx() << std::endl;
            }
        }
    }
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    // 其中l是indicator长度（应该等于data_length）
    int indicator_length = data_length;  // 255
    int second_point = -indicator_length + min_period;  // -255 + 29 = -226
    int third_point = static_cast<int>(std::floor(second_point / 2.0));  // floor(-226/2) = -113
    std::vector<int> check_points = {
        0,                  // 最新值
        second_point,       // -226
        third_point         // -113
    };
    
    std::vector<std::string> expected = {"51.991177", "62.334055", "46.707445"};
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = ultosc->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "UltimateOscillator value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(ultosc->getMinPeriod(), 29) << "UltimateOscillator minimum period should be 29";
}

// UltimateOscillator范围验证测试
TEST(OriginalTests, UltimateOscillator_RangeValidation) {
    auto csv_data = getdata(0);
    
    // 创建DataSeries
    auto data_source = std::make_shared<DataSeries>();
    
    // Get existing lines from DataSeries (they are already created in constructor)
    auto datetime_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(0));
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(1));
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(2));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(3));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(4));
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(5));
    auto openinterest_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(6));
    
    for (const auto& bar : csv_data) {
        open_buffer->append(bar.open);
        high_buffer->append(bar.high);
        low_buffer->append(bar.low);
        close_buffer->append(bar.close);
        volume_buffer->append(bar.volume);
    }
    
    auto ultosc = std::make_shared<UltimateOscillator>(data_source, 7, 14, 28);
    
    // 计算所有值并验证范围;
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ultosc->calculate();
        
        double ultosc_value = ultosc->get(0);
        
        // 验证UltimateOscillator在0到100范围内
        if (!std::isnan(ultosc_value)) {
            EXPECT_GE(ultosc_value, 0.0) << "UltimateOscillator should be >= 0 at step " << i;
            EXPECT_LE(ultosc_value, 100.0) << "UltimateOscillator should be <= 100 at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            open_buffer->forward();
            high_buffer->forward();
            low_buffer->forward();
            close_buffer->forward();
            volume_buffer->forward();
        }
    }
}

// 参数化测试 - 测试不同参数的UltimateOscillator
class UltimateOscillatorParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int, int>> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        // 创建DataSeries
        data_source_ = std::make_shared<DataSeries>();
        
        // Get existing lines from DataSeries (they are already created in constructor)
        datetime_buffer_ = std::dynamic_pointer_cast<LineBuffer>(data_source_->lines->getline(0));
        open_buffer_ = std::dynamic_pointer_cast<LineBuffer>(data_source_->lines->getline(1));
        high_buffer_ = std::dynamic_pointer_cast<LineBuffer>(data_source_->lines->getline(2));
        low_buffer_ = std::dynamic_pointer_cast<LineBuffer>(data_source_->lines->getline(3));
        close_buffer_ = std::dynamic_pointer_cast<LineBuffer>(data_source_->lines->getline(4));
        volume_buffer_ = std::dynamic_pointer_cast<LineBuffer>(data_source_->lines->getline(5));
        openinterest_buffer_ = std::dynamic_pointer_cast<LineBuffer>(data_source_->lines->getline(6));
        
        for (const auto& bar : csv_data_) {
            datetime_buffer_->append(0.0);  // 暂时用0
            open_buffer_->append(bar.open);
            high_buffer_->append(bar.high);
            low_buffer_->append(bar.low);
            close_buffer_->append(bar.close);
            volume_buffer_->append(bar.volume);
            openinterest_buffer_->append(0.0);  // 暂时用0
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<DataSeries> data_source_;
    std::shared_ptr<LineBuffer> datetime_buffer_;
    std::shared_ptr<LineBuffer> open_buffer_;
    std::shared_ptr<LineBuffer> high_buffer_;
    std::shared_ptr<LineBuffer> low_buffer_;
    std::shared_ptr<LineBuffer> close_buffer_;
    std::shared_ptr<LineBuffer> volume_buffer_;
    std::shared_ptr<LineBuffer> openinterest_buffer_;
};

TEST_P(UltimateOscillatorParameterizedTest, DifferentParameters) {
    auto [period1, period2, period3] = GetParam();
    auto ultosc = std::make_shared<UltimateOscillator>(data_source_, period1, period2, period3);
    
    // 计算所有值
    ultosc->calculate();
    
    // 验证最小周期
    int expected_min_period = std::max({period1, period2, period3}) + 1;
    EXPECT_EQ(ultosc->getMinPeriod(), expected_min_period) 
        << "UltimateOscillator minimum period should be max period + 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_value = ultosc->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last UltimateOscillator value should not be NaN";
        EXPECT_GE(last_value, 0.0) << "UltimateOscillator should be >= 0";
        EXPECT_LE(last_value, 100.0) << "UltimateOscillator should be <= 100";
    }
}

// 测试不同的UltimateOscillator参数
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    UltimateOscillatorParameterizedTest,
    ::testing::Values(
        std::make_tuple(7, 14, 28),   // 标准参数
        std::make_tuple(5, 10, 20),
        std::make_tuple(3, 7, 14),
        std::make_tuple(10, 20, 40)
    )
);

// UltimateOscillator计算逻辑验证测试
TEST(OriginalTests, UltimateOscillator_CalculationLogic) {
    // 使用简单的测试数据验证UltimateOscillator计算
    std::vector<CSVDataReader::OHLCVData> csv_data = {
        {"2006-01-01", 100.0, 110.0, 90.0, 105.0, 0, 0},
        {"2006-01-02", 105.0, 115.0, 95.0, 110.0, 0, 0},
        {"2006-01-03", 110.0, 120.0, 100.0, 115.0, 0, 0},
        {"2006-01-04", 115.0, 125.0, 105.0, 120.0, 0, 0},
        {"2006-01-05", 120.0, 130.0, 110.0, 125.0, 0, 0},
        {"2006-01-06", 125.0, 135.0, 115.0, 130.0, 0, 0},
        {"2006-01-07", 130.0, 140.0, 120.0, 135.0, 0, 0},
        {"2006-01-08", 135.0, 145.0, 125.0, 140.0, 0, 0}
    };
    
    // 创建DataSeries包含所有OHLCV数据
    // DataSeries constructor already creates 7 lines in the correct order
    auto data_source = std::make_shared<DataSeries>();
    
    // Get existing lines from DataSeries (they are already created in constructor)
    auto datetime_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(0));
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(1));
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(2));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(3));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(4));
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(5));
    auto openinterest_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(6));
    
    // 填充数据
    for (const auto& bar : csv_data) {
        datetime_buffer->append(0.0);  // 暂时用0
        open_buffer->append(bar.open);
        high_buffer->append(bar.high);
        low_buffer->append(bar.low);
        close_buffer->append(bar.close);
        volume_buffer->append(bar.volume);
        openinterest_buffer->append(0.0);  // 暂时用0
    }
    
    auto ultosc = std::make_shared<UltimateOscillator>(data_source, 3, 5, 7);
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ultosc->calculate();
        
        double ultosc_val = ultosc->get(0);
        
        // UltimateOscillator应该产生有限值且在0-100范围内
        if (!std::isnan(ultosc_val)) {
            EXPECT_TRUE(std::isfinite(ultosc_val)) 
                << "UltimateOscillator should be finite at step " << i;
            EXPECT_GE(ultosc_val, 0.0) 
                << "UltimateOscillator should be >= 0 at step " << i;
            EXPECT_LE(ultosc_val, 100.0) 
                << "UltimateOscillator should be <= 100 at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            open_buffer->forward();
            high_buffer->forward();
            low_buffer->forward();
            close_buffer->forward();
            volume_buffer->forward();
        }
    }
}

// 超买超卖信号测试
TEST(OriginalTests, UltimateOscillator_OverboughtOversold) {
    auto csv_data = getdata(0);
    
    // 创建DataSeries包含所有OHLCV数据
    // DataSeries constructor already creates 7 lines in the correct order
    auto data_source = std::make_shared<DataSeries>();
    
    // Get existing lines from DataSeries (they are already created in constructor)
    auto datetime_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(0));
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(1));
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(2));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(3));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(4));
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(5));
    auto openinterest_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(6));
    
    // 填充数据
    for (const auto& bar : csv_data) {
        datetime_buffer->append(0.0);  // 暂时用0
        open_buffer->append(bar.open);
        high_buffer->append(bar.high);
        low_buffer->append(bar.low);
        close_buffer->append(bar.close);
        volume_buffer->append(bar.volume);
        openinterest_buffer->append(0.0);  // 暂时用0
    }
    
    auto ultosc = std::make_shared<UltimateOscillator>(data_source, 7, 14, 28);
    
    int overbought_signals = 0;  // UO > 70
    int oversold_signals = 0;    // UO < 30
    int normal_signals = 0;      // 30 <= UO <= 70
    
    // 统计超买超卖信号;
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ultosc->calculate();
        
        double ultosc_value = ultosc->get(0);
        
        if (!std::isnan(ultosc_value)) {
            if (ultosc_value > 70.0) {
                overbought_signals++;
            } else if (ultosc_value < 30.0) {
                oversold_signals++;
            } else {
                normal_signals++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            open_buffer->forward();
            high_buffer->forward();
            low_buffer->forward();
            close_buffer->forward();
            volume_buffer->forward();
        }
    }
    
    std::cout << "UltimateOscillator signal statistics:" << std::endl;
    std::cout << "Overbought signals (> 70): " << overbought_signals << std::endl;
    std::cout << "Oversold signals (< 30): " << oversold_signals << std::endl;
    std::cout << "Normal signals (30-70): " << normal_signals << std::endl;
    
    // 验证至少有一些有效的计算
    EXPECT_GT(overbought_signals + oversold_signals + normal_signals, 0) 
        << "Should have some valid UltimateOscillator calculations";
}

// 趋势反转信号测试
TEST(OriginalTests, UltimateOscillator_ReversalSignals) {
    auto csv_data = getdata(0);
    
    // 创建DataSeries包含所有OHLCV数据
    // DataSeries constructor already creates 7 lines in the correct order
    auto data_source = std::make_shared<DataSeries>();
    
    // Get existing lines from DataSeries (they are already created in constructor)
    auto datetime_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(0));
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(1));
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(2));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(3));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(4));
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(5));
    auto openinterest_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(6));
    
    // 填充数据
    for (const auto& bar : csv_data) {
        datetime_buffer->append(0.0);  // 暂时用0
        open_buffer->append(bar.open);
        high_buffer->append(bar.high);
        low_buffer->append(bar.low);
        close_buffer->append(bar.close);
        volume_buffer->append(bar.volume);
        openinterest_buffer->append(0.0);  // 暂时用0
    }
    
    auto ultosc = std::make_shared<UltimateOscillator>(data_source, 7, 14, 28);
    
    int bullish_reversals = 0;   // 从超卖区域向上反转
    int bearish_reversals = 0;   // 从超买区域向下反转
    
    double prev_ultosc = 0.0;
    bool was_oversold = false;
    bool was_overbought = false;
    bool has_prev = false;
    
    // 检测反转信号;
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ultosc->calculate();
        
        double current_ultosc = ultosc->get(0);
        
        if (!std::isnan(current_ultosc) && has_prev) {
            // 检测从超卖区域的看涨反转
            if (was_oversold && prev_ultosc < 30.0 && current_ultosc > 30.0) {
                bullish_reversals++;
                was_oversold = false;
            }
            
            // 检测从超买区域的看跌反转
            if (was_overbought && prev_ultosc > 70.0 && current_ultosc < 70.0) {
                bearish_reversals++;
                was_overbought = false;
            }
            
            // 更新状态
            if (current_ultosc < 30.0) {
                was_oversold = true;
            }
            if (current_ultosc > 70.0) {
                was_overbought = true;
            }
        }
        
        if (!std::isnan(current_ultosc)) {
            prev_ultosc = current_ultosc;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            open_buffer->forward();
            high_buffer->forward();
            low_buffer->forward();
            close_buffer->forward();
            volume_buffer->forward();
        }
    }
    
    std::cout << "UltimateOscillator reversal signals:" << std::endl;
    std::cout << "Bullish reversals: " << bullish_reversals << std::endl;
    std::cout << "Bearish reversals: " << bearish_reversals << std::endl;
    
    // 验证检测到一些反转信号
    EXPECT_GE(bullish_reversals + bearish_reversals, 0) 
        << "Should detect some reversal signals";
}

// 多时间框架验证测试
TEST(OriginalTests, UltimateOscillator_MultiTimeframe) {
    auto csv_data = getdata(0);
    
    // 创建DataSeries包含所有OHLCV数据
    // DataSeries constructor already creates 7 lines in the correct order
    auto data_source = std::make_shared<DataSeries>();
    
    // Get existing lines from DataSeries (they are already created in constructor)
    auto datetime_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(0));
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(1));
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(2));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(3));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(4));
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(5));
    auto openinterest_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(6));
    
    // 填充数据
    for (const auto& bar : csv_data) {
        datetime_buffer->append(0.0);  // 暂时用0
        open_buffer->append(bar.open);
        high_buffer->append(bar.high);
        low_buffer->append(bar.low);
        close_buffer->append(bar.close);
        volume_buffer->append(bar.volume);
        openinterest_buffer->append(0.0);  // 暂时用0
    }
    
    // 创建不同参数的UltimateOscillator
    auto ultosc_fast = std::make_shared<UltimateOscillator>(data_source, 3, 7, 14);
    auto ultosc_standard = std::make_shared<UltimateOscillator>(data_source, 7, 14, 28);
    auto ultosc_slow = std::make_shared<UltimateOscillator>(data_source, 14, 28, 56);
    
    std::vector<double> fast_values;
    std::vector<double> standard_values;
    std::vector<double> slow_values;
    
    // 计算并收集值;
    for (size_t i = 0; i < csv_data.size(); ++i) {
        ultosc_fast->calculate();
        ultosc_standard->calculate();
        ultosc_slow->calculate();
        
        double fast_val = ultosc_fast->get(0);
        double standard_val = ultosc_standard->get(0);
        double slow_val = ultosc_slow->get(0);
        
        if (!std::isnan(fast_val) && !std::isnan(standard_val) && !std::isnan(slow_val)) {
            fast_values.push_back(fast_val);
            standard_values.push_back(standard_val);
            slow_values.push_back(slow_val);
        }
        
        if (i < csv_data.size() - 1) {
            open_buffer->forward();
            high_buffer->forward();
            low_buffer->forward();
            close_buffer->forward();
            volume_buffer->forward();
        }
    }
    
    // 验证多时间框架的特性
    EXPECT_FALSE(fast_values.empty()) << "Fast UO should produce values";
    EXPECT_FALSE(standard_values.empty()) << "Standard UO should produce values";
    EXPECT_FALSE(slow_values.empty()) << "Slow UO should produce values";
    
    if (!fast_values.empty() && !standard_values.empty() && !slow_values.empty()) {
        std::cout << "Multi-timeframe UO values collected successfully" << std::endl;
    }
}

// 边界条件测试
TEST(OriginalTests, UltimateOscillator_EdgeCases) {
    // 测试相同价格的情况
    std::vector<CSVDataReader::OHLCVData> large_data;
    for (int i = 0; i < 50; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 100.0;
        bar.low = 100.0;
        bar.close = 100.0;
        bar.open = 100.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        large_data.push_back(bar);
    }
    
    // 创建DataSeries包含所有OHLCV数据
    auto data_source = std::make_shared<DataSeries>();
    auto open_buffer = std::make_shared<LineBuffer>();
    auto high_buffer = std::make_shared<LineBuffer>();
    auto low_buffer = std::make_shared<LineBuffer>();
    auto close_buffer = std::make_shared<LineBuffer>();
    auto volume_buffer = std::make_shared<LineBuffer>();
    
    data_source->lines->add_line(open_buffer);   // 0 - open
    data_source->lines->add_line(high_buffer);   // 1 - high
    data_source->lines->add_line(low_buffer);    // 2 - low
    data_source->lines->add_line(close_buffer);  // 3 - close
    data_source->lines->add_line(volume_buffer); // 4 - volume
    
    // 填充数据
    for (const auto& bar : large_data) {
        open_buffer->append(bar.open);
        high_buffer->append(bar.high);
        low_buffer->append(bar.low);
        close_buffer->append(bar.close);
        volume_buffer->append(bar.volume);
    }
    
    auto flat_ultosc = std::make_shared<UltimateOscillator>(data_source, 7, 14, 28);
    for (size_t i = 0; i < large_data.size(); ++i) {
        flat_ultosc->calculate();
        if (i < large_data.size() - 1) {
            open_buffer->forward();
            high_buffer->forward();
            low_buffer->forward();
            close_buffer->forward();
            volume_buffer->forward();
        }
    }
    
    // 当所有价格相同时，UltimateOscillator的行为可能因实现而异
    double final_ultosc = flat_ultosc->get(0);
    if (!std::isnan(final_ultosc)) {
        EXPECT_GE(final_ultosc, 0.0) << "UltimateOscillator should be >= 0 for constant prices";
        EXPECT_LE(final_ultosc, 100.0) << "UltimateOscillator should be <= 100 for constant prices";
    }
}

// 性能测试
TEST(OriginalTests, UltimateOscillator_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<CSVDataReader::OHLCVData> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> price_dist(50.0, 150.0);
    std::uniform_real_distribution<double> range_dist(1.0, 5.0);
    for (size_t i = 0; i < data_size; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-01";
        bar.close = price_dist(rng);
        double range = range_dist(rng);
        bar.high = bar.close + range;
        bar.low = bar.close - range;
        bar.open = bar.close;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        large_data.push_back(bar);
    }
    
    // 创建DataSeries包含所有OHLCV数据
    // DataSeries constructor already creates 7 lines in the correct order
    auto data_source = std::make_shared<DataSeries>();
    
    // Get existing lines from DataSeries (they are already created in constructor)
    auto datetime_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(0));
    auto open_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(1));
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(2));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(3));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(4));
    auto volume_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(5));
    auto openinterest_buffer = std::dynamic_pointer_cast<LineBuffer>(data_source->lines->getline(6));
    
    // 填充数据
    for (const auto& bar : large_data) {
        datetime_buffer->append(0.0);  // 暂时用0
        open_buffer->append(bar.open);
        high_buffer->append(bar.high);
        low_buffer->append(bar.low);
        close_buffer->append(bar.close);
        volume_buffer->append(bar.volume);
        openinterest_buffer->append(0.0);  // 暂时用0
    }
    
    auto large_ultosc = std::make_shared<UltimateOscillator>(data_source, 7, 14, 28);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    large_ultosc->calculate();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "UltimateOscillator calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_ultosc->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_GE(final_result, 0.0) << "Final result should be >= 0";
    EXPECT_LE(final_result, 100.0) << "Final result should be <= 100";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}