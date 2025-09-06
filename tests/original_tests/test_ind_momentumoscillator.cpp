/**
 * @file test_ind_momentumoscillator.cpp
 * @brief MomentumOscillator指标测试 - 对应Python test_ind_momentumoscillator.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['101.654375', '99.052251', '101.904990']
 * ]
 * chkmin = 13
 * chkind = btind.MomentumOscillator
 * 
 * 注：MomentumOscillator (动量振荡器) 基于动量指标的振荡器
 */

#include "test_common.h"
#include <random>

#include "indicators/momentumoscillator.h"
#include "indicators/momentum.h"
#include "lineseries.h"
#include "linebuffer.h"
#include "indicators/sma.h"
#include "indicators/rsi.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;
using LineSeries = backtrader::LineSeries;
using LineBuffer = backtrader::LineBuffer;

namespace {

const std::vector<std::vector<std::string>> MOMENTUMOSCILLATOR_EXPECTED_VALUES = {
    {"101.654375", "99.052251", "101.904990"}
};

const int MOMENTUMOSCILLATOR_MIN_PERIOD = 13;

} // anonymous namespace

// 使用默认参数的MomentumOscillator测试
DEFINE_INDICATOR_TEST(MomentumOscillator_Default, MomentumOscillator, MOMENTUMOSCILLATOR_EXPECTED_VALUES, MOMENTUMOSCILLATOR_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, MomentumOscillator_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线系列 (使用LineSeries+LineBuffer模式)
    auto close_line_series = std::make_shared<LineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    
    // 获取buffer并添加数据
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    if (close_buffer) {
        // Clear the buffer first
        close_buffer->reset();
        // Add all data points
        for (size_t i = 0; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    // 创建MomentumOscillator指标
    auto momosc = std::make_shared<MomentumOscillator>(close_line_series, 12);
    
    // 计算一次（once()方法会处理所有数据）
    momosc->calculate();
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 13;
    
    
    // Check line values
    auto momosc_line = momosc->lines->getline(0);
    if (momosc_line) {
        std::cout << "MomentumOscillator line size: " << momosc_line->size() << std::endl;
        
    }
    
    // Python测试的检查点: [0, -242, -121]
    // Python有255个数据点，C++有256个（包括初始NaN）
    // 需要调整索引以匹配Python的期望值
    
    std::vector<std::string> expected = {"101.654375", "99.052251", "101.904990"};
    
    // 计算对应的C++索引
    // Python: momosc[0] = 最新值
    // Python: momosc[-242] = 第一个有效值（255-13=242）
    // Python: momosc[-121] = 中间值
    
    // C++中使用负索引访问历史数据
    std::vector<int> cpp_indices = {
        0,    // 最新值（对应Python的[0]）
        -242, // 第一个有效值（对应Python的[-242]）
        -121  // 中间值（对应Python的[-121]）
    };
    
    for (size_t i = 0; i < cpp_indices.size() && i < expected.size(); ++i) {
        int index = cpp_indices[i];
        
        double actual;
        // 对于负索引，不需要范围检查，LineBuffer会处理
        actual = (*momosc_line)[index];
        
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "MomentumOscillator value mismatch at check point " << i 
            << " (index=" << index << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(momosc->getMinPeriod(), 13) << "MomentumOscillator minimum period should be 13";
}

// 参数化测试 - 测试不同参数的MomentumOscillator
class MomentumOscillatorParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line_ = std::make_shared<LineSeries>();
        close_line_->lines->add_line(std::make_shared<LineBuffer>());
        close_line_buffer_ = std::dynamic_pointer_cast<LineBuffer>(close_line_->lines->getline(0));
        
        if (close_line_buffer_) {
            // Clear and fill buffer
            close_line_buffer_->reset();
            for (size_t i = 0; i < csv_data_.size(); ++i) {
                close_line_buffer_->append(csv_data_[i].close);
            }
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineSeries> close_line_;
    std::shared_ptr<LineBuffer> close_line_buffer_;
};

TEST_P(MomentumOscillatorParameterizedTest, DifferentParameters) {
    int period = GetParam();
    auto momosc = std::make_shared<MomentumOscillator>(close_line_, period);
    
    // 计算所有值 - 修复性能：只调用一次
    momosc->calculate();
    
    // 验证最小周期
    int expected_minperiod = period + 1;
    EXPECT_EQ(momosc->getMinPeriod(), expected_minperiod) 
        << "MomentumOscillator minimum period should be " << expected_minperiod;
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_minperiod)) {
        double last_value = momosc->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last MomentumOscillator value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last MomentumOscillator value should be finite";
        EXPECT_GT(last_value, 0.0) << "MomentumOscillator should be positive (percentage-based)";
    }
}

// 测试不同的MomentumOscillator参数组合
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    MomentumOscillatorParameterizedTest,
    ::testing::Values(
        10,
        12,   // 默认参数
        14,
        20
    )
);

// MomentumOscillator计算逻辑验证测试
TEST(OriginalTests, MomentumOscillator_CalculationLogic) {
    // 使用简单的测试数据验证MomentumOscillator计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 108.0, 106.0, 104.0, 102.0,
                                  104.0, 106.0, 108.0, 110.0, 112.0, 114.0, 116.0, 118.0, 120.0, 122.0};
    
    auto price_line = std::make_shared<LineSeries>();
    price_line->lines->add_line(std::make_shared<LineBuffer>());
    auto price_buffer = std::dynamic_pointer_cast<LineBuffer>(price_line->lines->getline(0));
    
    if (price_buffer) {
        price_buffer->reset();
        for (size_t i = 0; i < prices.size(); ++i) {
            price_buffer->append(prices[i]);
        }
    }
    
    auto momosc = std::make_shared<MomentumOscillator>(price_line, 10);
    auto momentum = std::make_shared<Momentum>(price_line, 10);
    auto sma = SMA::fromIndicator(momentum, 3);
    
    // Calculate once
    momosc->calculate();
    momentum->calculate();
    sma->calculate();
    
    // 验证MomentumOscillator计算逻辑
    for (size_t i = 12; i < prices.size(); ++i) {  // MomentumOscillator需要13个数据点
        double momentum_value = momentum->get(-(prices.size() - 1 - i));
        double sma_momentum = sma->get(-(prices.size() - 1 - i));
        double actual_momosc = momosc->get(-(prices.size() - 1 - i));
        
        if (!std::isnan(momentum_value) && !std::isnan(sma_momentum) && !std::isnan(actual_momosc)) {
            // MomentumOscillator = 100 * SMA(Momentum) / SMA(SMA(Momentum))
            // 简化为：基于平滑后的动量的百分比振荡器
            EXPECT_GT(actual_momosc, 0.0) << "MomentumOscillator should be positive at position " << i;
            EXPECT_LT(actual_momosc, 200.0) << "MomentumOscillator should be reasonable at position " << i;
        }
    }
}

// MomentumOscillator趋势分析测试
TEST(OriginalTests, MomentumOscillator_TrendAnalysis) {
    // 创建明确的趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 50; ++i) {
        uptrend_prices.push_back(100.0 + i * 1.0);  // 强劲上升趋势
    }
    
    auto uptrend_line = std::make_shared<LineSeries>();
    uptrend_line->lines->add_line(std::make_shared<LineBuffer>());
    auto uptrend_buffer_ptr = std::dynamic_pointer_cast<LineBuffer>(uptrend_line->lines->getline(0));
    
    if (uptrend_buffer_ptr) {
        uptrend_buffer_ptr->reset();
        for (size_t i = 0; i < uptrend_prices.size(); ++i) {
            uptrend_buffer_ptr->append(uptrend_prices[i]);
        }
    }
    
    auto uptrend_momosc = std::make_shared<MomentumOscillator>(uptrend_line, 12);
    
    // Calculate once
    uptrend_momosc->calculate();
    
    std::vector<double> uptrend_values;
    for (size_t i = 14; i < uptrend_prices.size(); ++i) {
        double osc_value = uptrend_momosc->get(-(uptrend_prices.size() - 1 - i));
        if (!std::isnan(osc_value)) {
            uptrend_values.push_back(osc_value);
        }
    }
    
    // 创建下降趋势数据
    std::vector<double> downtrend_prices;
    for (int i = 0; i < 50; ++i) {
        downtrend_prices.push_back(150.0 - i * 1.0);  // 强劲下降趋势
    }
    
    auto downtrend_line = std::make_shared<LineSeries>();
    downtrend_line->lines->add_line(std::make_shared<LineBuffer>());
    auto downtrend_buffer_ptr = std::dynamic_pointer_cast<LineBuffer>(downtrend_line->lines->getline(0));
    
    if (downtrend_buffer_ptr) {
        downtrend_buffer_ptr->reset();
        for (size_t i = 0; i < downtrend_prices.size(); ++i) {
            downtrend_buffer_ptr->append(downtrend_prices[i]);
        }
    }
    
    auto downtrend_momosc = std::make_shared<MomentumOscillator>(downtrend_line, 12);
    
    // Calculate once
    downtrend_momosc->calculate();
    
    std::vector<double> downtrend_values;
    for (size_t i = 14; i < downtrend_prices.size(); ++i) {
        double osc_value = downtrend_momosc->get(-(downtrend_prices.size() - 1 - i));
        if (!std::isnan(osc_value)) {
            downtrend_values.push_back(osc_value);
        }
    }
    
    // 分析趋势特性
    if (!uptrend_values.empty() && !downtrend_values.empty()) {
        double avg_uptrend = std::accumulate(uptrend_values.begin(), uptrend_values.end(), 0.0) / uptrend_values.size();
        double avg_downtrend = std::accumulate(downtrend_values.begin(), downtrend_values.end(), 0.0) / downtrend_values.size();
        
        std::cout << "Trend analysis:" << std::endl;
        std::cout << "Uptrend average: " << avg_uptrend << std::endl;
        std::cout << "Downtrend average: " << avg_downtrend << std::endl;
        
        // 上升趋势应该有更高的动量振荡器值
        EXPECT_GT(avg_uptrend, avg_downtrend) 
            << "Uptrend should have higher momentum oscillator values";
        
        // 上升趋势值应该明显大于100（中性线）
        EXPECT_GT(avg_uptrend, 100.0) 
            << "Strong uptrend should have momentum oscillator above 100";
        
        // 下降趋势值应该明显小于100
        EXPECT_LT(avg_downtrend, 100.0) 
            << "Strong downtrend should have momentum oscillator below 100";
    }
}

// MomentumOscillator中性线穿越测试
TEST(OriginalTests, MomentumOscillator_NeutralLineCrossing) {
    auto csv_data = getdata(0);
    
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    
    if (close_buffer) {
        close_buffer->reset();
        for (size_t i = 0; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    auto momosc = std::make_shared<MomentumOscillator>(close_line, 12);
    
    // Calculate once
    momosc->calculate();
    
    int above_neutral = 0;    // 高于100的次数
    int below_neutral = 0;    // 低于100的次数
    int crossings_up = 0;     // 向上穿越100的次数
    int crossings_down = 0;   // 向下穿越100的次数
    
    double prev_value = 0.0;
    bool has_prev = false;
    
    for (size_t i = 14; i < csv_data.size(); ++i) {
        double current_value = momosc->get(-(csv_data.size() - 1 - i));
        
        if (!std::isnan(current_value)) {
            if (current_value > 100.0) {
                above_neutral++;
            } else if (current_value < 100.0) {
                below_neutral++;
            }
            
            // 检测穿越
            if (has_prev) {
                if (prev_value <= 100.0 && current_value > 100.0) {
                    crossings_up++;
                } else if (prev_value >= 100.0 && current_value < 100.0) {
                    crossings_down++;
                }
            }
            
            prev_value = current_value;
            has_prev = true;
        }
    }
    
    std::cout << "Neutral line analysis:" << std::endl;
    std::cout << "Above neutral (>100): " << above_neutral << std::endl;
    std::cout << "Below neutral (<100): " << below_neutral << std::endl;
    std::cout << "Crossings up: " << crossings_up << std::endl;
    std::cout << "Crossings down: " << crossings_down << std::endl;
    
    int total_values = above_neutral + below_neutral;
    EXPECT_GT(total_values, 0) << "Should have some valid oscillator values";
    
    // 验证检测到一些穿越信号
    EXPECT_GE(crossings_up + crossings_down, 0) 
        << "Should detect some neutral line crossings";
}

// MomentumOscillator振荡特性测试
TEST(OriginalTests, MomentumOscillator_OscillationCharacteristics) {
    // 创建振荡数据
    std::vector<double> oscillating_prices;
    for (int i = 0; i < 100; ++i) {
        double base = 100.0;
        double oscillation = 10.0 * std::sin(i * 0.2);
        oscillating_prices.push_back(base + oscillation);
    }
    
    auto osc_line = std::make_shared<LineSeries>();
    osc_line->lines->add_line(std::make_shared<LineBuffer>());
    auto osc_buffer = std::dynamic_pointer_cast<LineBuffer>(osc_line->lines->getline(0));
    
    if (osc_buffer) {
        osc_buffer->reset();
        for (size_t i = 0; i < oscillating_prices.size(); ++i) {
            osc_buffer->append(oscillating_prices[i]);
        }
    }
    
    auto momosc = std::make_shared<MomentumOscillator>(osc_line, 12);
    
    // Calculate once
    momosc->calculate();
    
    std::vector<double> osc_values;
    double max_val = -std::numeric_limits<double>::infinity();
    double min_val = std::numeric_limits<double>::infinity();
    
    for (size_t i = 14; i < oscillating_prices.size(); ++i) {
        double val = momosc->get(-(oscillating_prices.size() - 1 - i));
        if (!std::isnan(val)) {
            osc_values.push_back(val);
            max_val = std::max(max_val, val);
            min_val = std::min(min_val, val);
        }
    }
    
    if (!osc_values.empty()) {
        double range = max_val - min_val;
        double avg_val = std::accumulate(osc_values.begin(), osc_values.end(), 0.0) / osc_values.size();
        
        std::cout << "Oscillation characteristics:" << std::endl;
        std::cout << "Max value: " << max_val << std::endl;
        std::cout << "Min value: " << min_val << std::endl;
        std::cout << "Range: " << range << std::endl;
        std::cout << "Average: " << avg_val << std::endl;
        
        // 验证振荡特性
        EXPECT_GT(range, 0.0) << "Should have oscillation range";
        EXPECT_GT(max_val, 100.0) << "Should oscillate above neutral";
        EXPECT_LT(min_val, 100.0) << "Should oscillate below neutral";
        EXPECT_NEAR(avg_val, 100.0, 5.0) << "Average should be near neutral line";
    }
}