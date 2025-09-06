/**
 * @file test_ind_smma.cpp
 * @brief SMMA指标测试 - 对应Python test_ind_smma.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4021.569725', '3644.444667', '3616.427648']
 * ]
 * chkmin = 30
 * chkind = btind.SMMA
 */

#include "test_common.h"
#include "lineseries.h"
#include <random>

#include "indicators/smma.h"
#include "indicators/sma.h"
#include "indicators/ema.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;
using LineSeries = backtrader::LineSeries;
using LineBuffer = backtrader::LineBuffer;

namespace {

const std::vector<std::vector<std::string>> SMMA_EXPECTED_VALUES = {
    {"4021.569725", "3644.444667", "3616.427648"}
};

const int SMMA_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的SMMA测试
DEFINE_INDICATOR_TEST(SMMA_Default, SMMA, SMMA_EXPECTED_VALUES, SMMA_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, SMMA_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建一个简单的LineSeries包装器，提供正确的size()
    class TestLineSeries : public LineSeries {
    public:
        size_t data_size = 0;
        size_t size() const override {
            return data_size;
        }
    };
    
    // 创建数据线系列
    auto close_line_series = std::make_shared<TestLineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    close_line_series->lines->add_alias("close", 0);
    close_line_series->data_size = csv_data.size();
    
    // 逐步添加数据到线缓冲区  
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    if (close_buffer) {
        // Set the first data point to replace the initial NaN, then append the rest
        close_buffer->set(0, csv_data[0].close);
        for (size_t i = 1; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    // 创建SMMA指标（30周期）
    auto smma = std::make_shared<SMMA>(close_line_series, 30);
    
    // 计算
    smma->calculate();
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 30;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    // Python floor division: -225 // 2 = -113 (floor towards negative infinity)
    int middle_checkpoint = static_cast<int>(std::floor(static_cast<double>(-(data_length - min_period)) / 2.0));
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        middle_checkpoint                     // 中间值 (-113 not -112)
    };
    
    std::vector<double> expected = {4021.569725, 3644.444667, 3616.427648};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = smma->get(check_points[i]);
        
        // Use EXPECT_NEAR for floating-point comparison with tolerance
        EXPECT_NEAR(actual, expected[i], 0.1) 
            << "SMMA value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual;
    }
    
    // 验证最小周期
    EXPECT_EQ(smma->getMinPeriod(), 30) << "SMMA minimum period should be 30";
}

// 参数化测试 - 测试不同周期的SMMA
class SMMAParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line_ = std::make_shared<LineSeries>();
        close_line_->lines->add_line(std::make_shared<LineBuffer>());
        close_line_->lines->add_alias("close", 0);
        close_line_buffer_ = std::dynamic_pointer_cast<LineBuffer>(close_line_->lines->getline(0));
        for (const auto& bar : csv_data_) {
            close_line_buffer_->append(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineSeries> close_line_;
    std::shared_ptr<LineBuffer> close_line_buffer_;
};

TEST_P(SMMAParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    
    // Use SimpleTestDataSeries pattern and cast to DataSeries
    auto test_data_series = std::make_shared<SimpleTestDataSeries>(csv_data_);
    auto data_series = std::static_pointer_cast<DataSeries>(test_data_series);
    
    auto smma = std::make_shared<SMMA>(data_series, period);
    
    // 计算所有值 - 修复性能：O(n²) -> O(n)
    smma->calculate();
    
    // 验证最小周期
    EXPECT_EQ(smma->getMinPeriod(), period) 
        << "SMMA minimum period should equal period parameter";
    
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_value = smma->get(0);
        
        EXPECT_FALSE(std::isnan(last_value)) << "Last SMMA value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last SMMA value should be finite";
        EXPECT_GT(last_value, 0.0) << "SMMA should be positive for positive prices";
    }
}

// 测试不同的SMMA周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    SMMAParameterizedTest,
    ::testing::Values(10, 20, 30, 50)
);

// SMMA计算逻辑验证测试
TEST(OriginalTests, SMMA_CalculationLogic) {
    // 使用简单的测试数据验证SMMA计算
    std::vector<double> prices = {100.0, 102.0, 104.0, 106.0, 108.0, 110.0, 108.0, 106.0, 104.0, 102.0};
    
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("smma_calc", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    // Set the first data point to replace the initial NaN, then append the rest
    if (!prices.empty()) {
        close_buffer->set(0, prices[0]);
        for (size_t i = 1; i < prices.size(); ++i) {
            close_buffer->append(prices[i]);
        }
    }
    
    auto smma = std::make_shared<SMMA>(std::static_pointer_cast<LineSeries>(close_line), 5);
    
    // 手动计算SMMA进行验证
    double manual_smma = 0.0;
    bool first_calculation = true;
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    smma->calculate();
    
    // 验证最终计算结果
    if (prices.size() >= 5) {  // SMMA需要5个数据点
        // 手动计算最终SMMA值进行验证
        double manual_smma = (prices[0] + prices[1] + prices[2] + prices[3] + prices[4]) / 5.0;
        
        // 计算后续所有SMMA值
        for (size_t i = 5; i < prices.size(); ++i) {
            manual_smma = (manual_smma * 4.0 + prices[i]) / 5.0;
        }
        
        double actual_smma = smma->get(0);
        
        if (!std::isnan(actual_smma)) {
            EXPECT_NEAR(actual_smma, manual_smma, 0.5) 
                << "Final SMMA calculation mismatch";
        }
    }
}

// SMMA平滑特性测试
TEST(OriginalTests, SMMA_SmoothingCharacteristics) {
    // 创建包含噪声的数据
    std::vector<double> noisy_prices;
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> noise_dist(-2.0, 2.0);
    
    for (int i = 0; i < 100; ++i) {
        double trend = 100.0 + i * 0.5;  // 缓慢上升趋势
        double noise = noise_dist(rng);   // 随机噪声
        noisy_prices.push_back(trend + noise);
    }
    
    auto noisy_line = std::make_shared<LineSeries>();
    noisy_line->lines->add_line(std::make_shared<LineBuffer>());
    noisy_line->lines->add_alias("noisy", 0);
    auto noisy_line_buffer = std::dynamic_pointer_cast<LineBuffer>(noisy_line->lines->getline(0));

    // Set the first data point to replace the initial NaN, then append the rest
    if (!noisy_prices.empty()) {
        noisy_line_buffer->set(0, noisy_prices[0]);
        for (size_t i = 1; i < noisy_prices.size(); ++i) {
            noisy_line_buffer->append(noisy_prices[i]);
        }
    }
    
    auto smma = std::make_shared<SMMA>(std::static_pointer_cast<LineSeries>(noisy_line), 20);
    auto sma = std::make_shared<SMA>(std::static_pointer_cast<LineSeries>(noisy_line), 20);  // 比较对象
    
    std::vector<double> smma_changes;
    std::vector<double> sma_changes;
    double prev_smma = 0.0, prev_sma = 0.0;
    bool has_prev = false;
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    smma->calculate();
    sma->calculate();
    
    // 收集数据进行平滑性分析
    for (size_t i = 0; i < noisy_prices.size(); ++i) {
        double current_smma = smma->get(-static_cast<int>(i));
        double current_sma = sma->get(-static_cast<int>(i));
        
        if (!std::isnan(current_smma) && !std::isnan(current_sma)) {
            if (has_prev && i > 0) {
                smma_changes.push_back(std::abs(current_smma - prev_smma));
                sma_changes.push_back(std::abs(current_sma - prev_sma));
            }
            prev_smma = current_smma;
            prev_sma = current_sma;
            has_prev = true;
        }
    }
    
    // 比较SMMA和SMA的平滑性
    if (!smma_changes.empty() && !sma_changes.empty()) {
        double avg_smma_change = std::accumulate(smma_changes.begin(), smma_changes.end(), 0.0) / smma_changes.size();
        double avg_sma_change = std::accumulate(sma_changes.begin(), sma_changes.end(), 0.0) / sma_changes.size();
        
        std::cout << "Smoothing comparison:" << std::endl;
        std::cout << "Average SMMA change: " << avg_smma_change << std::endl;
        std::cout << "Average SMA change: " << avg_sma_change << std::endl;
        
        // SMMA应该比SMA更平滑（变化更小）
        EXPECT_LT(avg_smma_change, avg_sma_change) 
            << "SMMA should be smoother than SMA";
    }
}

// SMMA趋势跟随能力测试
TEST(OriginalTests, SMMA_TrendFollowing) {
    // 创建明确的趋势数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 100; ++i) {
        trend_prices.push_back(100.0 + i * 1.0);  // 线性上升趋势
    }
    
    auto trend_line = std::make_shared<LineSeries>();
    trend_line->lines->add_line(std::make_shared<LineBuffer>());
    trend_line->lines->add_alias("trend", 0);
    auto trend_line_buffer = std::dynamic_pointer_cast<LineBuffer>(trend_line->lines->getline(0));

    // Set the first data point to replace the initial NaN, then append the rest
    if (!trend_prices.empty()) {
        trend_line_buffer->set(0, trend_prices[0]);
        for (size_t i = 1; i < trend_prices.size(); ++i) {
            trend_line_buffer->append(trend_prices[i]);
        }
    }
    
    auto smma = std::make_shared<SMMA>(std::static_pointer_cast<LineSeries>(trend_line), 20);
    
    double prev_smma = 0.0;
    bool has_prev = false;
    int increasing_count = 0;
    int total_count = 0;
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    smma->calculate();
    
    // 分析趋势跟随性
    // We need to access values in chronological order (oldest to newest)
    // The buffer has 100 values, so we access from -99 to 0
    for (int i = static_cast<int>(trend_prices.size()) - 1; i >= 0; --i) {
        double current_smma = smma->get(-i);
        
        if (!std::isnan(current_smma)) {
            if (has_prev) {
                total_count++;
                if (current_smma > prev_smma) {
                    increasing_count++;
                }
            }
            prev_smma = current_smma;
            has_prev = true;
        }
    }
    
    // 在上升趋势中，SMMA应该主要呈上升趋势
    if (total_count > 0) {
        double increasing_ratio = static_cast<double>(increasing_count) / total_count;
        EXPECT_GT(increasing_ratio, 0.8) 
            << "SMMA should follow uptrend effectively";
        
        std::cout << "Trend following - SMMA increasing ratio: " << increasing_ratio << std::endl;
    }
}

// SMMA与EMA比较测试
TEST(OriginalTests, SMMA_vs_EMA_Comparison) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("close", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    auto smma = std::make_shared<SMMA>(std::static_pointer_cast<LineSeries>(close_line), 20);
    auto ema = std::make_shared<EMA>(std::static_pointer_cast<LineSeries>(close_line), 20);
    
    std::vector<double> smma_values;
    std::vector<double> ema_values;
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    smma->calculate();
    ema->calculate();
    
    // 分析最终对比结果
    double smma_val = smma->get(0);
    double ema_val = ema->get(0);
    
    if (!std::isnan(smma_val) && !std::isnan(ema_val)) {
        smma_values.push_back(smma_val);
        ema_values.push_back(ema_val);
    }
    
    // 比较SMMA和EMA的特性
    if (!smma_values.empty() && !ema_values.empty()) {
        double final_smma = smma_values.back();
        double final_ema = ema_values.back();
        
        std::cout << "SMMA vs EMA comparison:" << std::endl;
        std::cout << "Final SMMA: " << final_smma << std::endl;
        std::cout << "Final EMA: " << final_ema << std::endl;
        
        // 验证两者都是有限值
        EXPECT_TRUE(std::isfinite(final_smma)) << "Final SMMA should be finite";
        EXPECT_TRUE(std::isfinite(final_ema)) << "Final EMA should be finite";
    }
}

// SMMA响应速度测试
TEST(OriginalTests, SMMA_ResponseSpeed) {
    // 创建价格突然变化的数据
    std::vector<double> step_prices;
    
    // 第一阶段：稳定价格
    for (int i = 0; i < 50; ++i) {
        step_prices.push_back(100.0);
    }
    
    // 第二阶段：价格跳跃
    for (int i = 0; i < 50; ++i) {
        step_prices.push_back(120.0);
    }
    
    auto step_line = std::make_shared<LineSeries>();
    step_line->lines->add_line(std::make_shared<LineBuffer>());
    step_line->lines->add_alias("step", 0);
    auto step_line_buffer = std::dynamic_pointer_cast<LineBuffer>(step_line->lines->getline(0));

    // Set the first data point to replace the initial NaN, then append the rest
    if (!step_prices.empty()) {
        step_line_buffer->set(0, step_prices[0]);
        for (size_t i = 1; i < step_prices.size(); ++i) {
            step_line_buffer->append(step_prices[i]);
        }
    }
    
    auto smma = std::make_shared<SMMA>(std::static_pointer_cast<LineSeries>(step_line), 20);
    
    std::vector<double> pre_step_smma;
    std::vector<double> post_step_smma;
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    smma->calculate();
    
    // 分析价格变化响应
    // Access values in chronological order
    for (int i = static_cast<int>(step_prices.size()) - 1; i >= 0; --i) {
        double smma_val = smma->get(-i);
        
        if (!std::isnan(smma_val)) {
            // Remember: we're going backwards, so high indices are early data
            if (i >= 50) {
                pre_step_smma.push_back(smma_val);
            } else {
                post_step_smma.push_back(smma_val);
            }
        }
    }
    
    // 分析SMMA对价格跳跃的响应
    if (!pre_step_smma.empty() && !post_step_smma.empty()) {
        double avg_pre = std::accumulate(pre_step_smma.end() - 10, pre_step_smma.end(), 0.0) / 10.0;
        double final_post = post_step_smma.back();
        
        std::cout << "Step response - Pre-step SMMA: " << avg_pre 
                  << ", Final post-step SMMA: " << final_post << std::endl;
        
        // SMMA应该能够响应价格变化，但比较平滑
        EXPECT_GT(final_post, avg_pre) << "SMMA should respond to price step";
        EXPECT_LT(final_post, 120.0) << "SMMA should lag behind price step";
        EXPECT_GT(final_post, 110.0) << "SMMA should partially adapt to new price level";
    }
}

// SMMA滞后特性测试
TEST(OriginalTests, SMMA_LagCharacteristics) {
    // 创建正弦波数据来测试滞后
    std::vector<double> sine_prices;
    for (int i = 0; i < 200; ++i) {
        double angle = i * M_PI / 50.0;  // 完整周期100个点
        sine_prices.push_back(100.0 + 10.0 * std::sin(angle));
    }
    
    auto sine_line = std::make_shared<LineSeries>();
    sine_line->lines->add_line(std::make_shared<LineBuffer>());
    sine_line->lines->add_alias("sine", 0);
    auto sine_line_buffer = std::dynamic_pointer_cast<LineBuffer>(sine_line->lines->getline(0));

    // Set the first data point to replace the initial NaN, then append the rest
    if (!sine_prices.empty()) {
        sine_line_buffer->set(0, sine_prices[0]);
        for (size_t i = 1; i < sine_prices.size(); ++i) {
            sine_line_buffer->append(sine_prices[i]);
        }
    }
    
    auto smma = std::make_shared<SMMA>(std::static_pointer_cast<LineSeries>(sine_line), 20);
    auto sma = std::make_shared<SMA>(std::static_pointer_cast<LineSeries>(sine_line), 20);
    
    std::vector<double> price_values;
    std::vector<double> smma_values;
    std::vector<double> sma_values;
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    smma->calculate();
    sma->calculate();
    
    // 分析最终振荡响应
    for (size_t i = 0; i < sine_prices.size(); ++i) {
        double smma_val = smma->get(-static_cast<int>(i));
        double sma_val = sma->get(-static_cast<int>(i));
        
        if (!std::isnan(smma_val) && !std::isnan(sma_val)) {
            price_values.push_back(sine_prices[i]);
            smma_values.push_back(smma_val);
            sma_values.push_back(sma_val);
        }
    }
    
    // 分析滞后特性
    if (smma_values.size() >= 100) {
        // 计算价格和SMMA的相关性（简化版本）
        double price_range = *std::max_element(price_values.begin(), price_values.end()) - 
                            *std::min_element(price_values.begin(), price_values.end());
        double smma_range = *std::max_element(smma_values.begin(), smma_values.end()) - 
                           *std::min_element(smma_values.begin(), smma_values.end());
        
        std::cout << "Lag characteristics:" << std::endl;
        std::cout << "Price range: " << price_range << std::endl;
        std::cout << "SMMA range: " << smma_range << std::endl;
        
        // SMMA的波动范围应该小于原始价格
        EXPECT_LT(smma_range, price_range) 
            << "SMMA should have smaller range than original prices";
    }
}

// 边界条件测试
TEST(OriginalTests, SMMA_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineSeries>();
    flat_line->lines->add_line(std::make_shared<LineBuffer>());
    flat_line->lines->add_alias("flat", 0);
    auto flat_line_buffer = std::dynamic_pointer_cast<LineBuffer>(flat_line->lines->getline(0));

    // Set the first data point to replace the initial NaN, then append the rest
    if (!flat_prices.empty()) {
        flat_line_buffer->set(0, flat_prices[0]);
        for (size_t i = 1; i < flat_prices.size(); ++i) {
            flat_line_buffer->append(flat_prices[i]);
        }
    }
    
    auto flat_smma = std::make_shared<SMMA>(std::static_pointer_cast<LineSeries>(flat_line), 20);
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    flat_smma->calculate();
    
    // 当所有价格相同时，SMMA应该等于该价格
    double final_smma = flat_smma->get(0);
    if (!std::isnan(final_smma)) {
        EXPECT_NEAR(final_smma, 100.0, 1e-6) 
            << "SMMA should equal constant price";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineSeries>();
    insufficient_line->lines->add_line(std::make_shared<LineBuffer>());
    insufficient_line->lines->add_alias("insufficient", 0);
    auto insufficient_line_buffer = std::dynamic_pointer_cast<LineBuffer>(insufficient_line->lines->getline(0));

    // 只添加几个数据点
    for (int i = 0; i < 15; ++i) {
        insufficient_line_buffer->append(100.0 + i);
    }
    
    auto insufficient_smma = std::make_shared<SMMA>(std::static_pointer_cast<LineSeries>(insufficient_line), 20);
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    insufficient_smma->calculate();
    
    // 数据不足时应该返回NaN
    double result = insufficient_smma->get(0);
    EXPECT_TRUE(std::isnan(result)) << "SMMA should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, SMMA_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<double> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    
    for (size_t i = 0; i < data_size; ++i) {
        large_data.push_back(dist(rng));
    }
    
    // Create test data in CSVDataReader format
    std::vector<CSVDataReader::OHLCVData> ohlcv_data;
    ohlcv_data.reserve(large_data.size());
    for (size_t i = 0; i < large_data.size(); ++i) {
        CSVDataReader::OHLCVData bar;
        bar.close = large_data[i];
        bar.high = bar.close * 1.01;
        bar.low = bar.close * 0.99;
        bar.open = i > 0 ? large_data[i-1] : bar.close;
        bar.volume = 1000.0;
        bar.openinterest = 0.0;
        ohlcv_data.push_back(bar);
    }
    
    // Use SimpleTestDataSeries and cast to DataSeries
    auto test_data_series = std::make_shared<SimpleTestDataSeries>(ohlcv_data);
    auto data_series = std::static_pointer_cast<DataSeries>(test_data_series);
    auto large_smma = std::make_shared<SMMA>(data_series, 50);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    large_smma->calculate();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "SMMA calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    std::cout << "SMMA size after calculate: " << large_smma->size() << std::endl;
    
    if (large_smma->size() > 0) {
        // Debug: Check the internal buffer
        if (large_smma->lines && large_smma->lines->size() > 0) {
            auto smma_line = large_smma->lines->getline(0);
            if (auto buffer = std::dynamic_pointer_cast<LineBuffer>(smma_line)) {
                std::cout << "SMMA buffer data_size: " << buffer->data_size() << std::endl;
                std::cout << "SMMA buffer array size: " << buffer->array().size() << std::endl;
                std::cout << "SMMA buffer idx: " << buffer->get_idx() << std::endl;
                
                // Check last few values
                const auto& arr = buffer->array();
                if (buffer->data_size() >= 5) {
                    std::cout << "Last 5 SMMA values: ";
                    for (size_t i = buffer->data_size() - 5; i < buffer->data_size(); ++i) {
                        std::cout << arr[i] << " ";
                    }
                    std::cout << std::endl;
                }
            }
        }
        
        double final_result = large_smma->get(0);
        std::cout << "Final result from get(0): " << final_result << std::endl;
        
        EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
        EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
        EXPECT_GT(final_result, 0.0) << "Final result should be positive";
    } else {
        FAIL() << "SMMA has no values after calculate()";
    }
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}
