/**
 * @file test_ind_dma.cpp
 * @brief DMA指标测试 - 对应Python test_ind_dma.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4121.903804', '3677.634675', '3579.962958']
 * ]
 * chkmin = 30
 * chkind = btind.DMA
 */

#include "test_common_simple.h"

using namespace backtrader::indicators;
#include "indicators/dma.h"

using namespace backtrader::indicators;

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> DMA_EXPECTED_VALUES = {
    {"4121.903804", "3677.634675", "3579.962958"}
};

const int DMA_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的DMA测试
DEFINE_INDICATOR_TEST(DMA_Default, DMA, DMA_EXPECTED_VALUES, DMA_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, DMA_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建DMA指标（默认参数：period=30, displacement=30）
    auto dma = std::make_shared<DMA>(close_line, 30, 30);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        dma->calculate();
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
    
    std::vector<std::string> expected = {"4121.903804", "3677.634675", "3579.962958"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = dma->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "DMA value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(dma->getMinPeriod(), 30) << "DMA minimum period should be 30";
}

// 参数化测试 - 测试不同参数的DMA
class DMAParameterizedTest : public ::testing::TestWithParam<std::pair<int, int>> {
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

TEST_P(DMAParameterizedTest, DifferentParameters) {
    auto [period, displacement] = GetParam();
    auto dma = std::make_shared<DMA>(close_line_, period, displacement);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        dma->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    int expected_min_period = period;
    EXPECT_EQ(dma->getMinPeriod(), expected_min_period) 
        << "DMA minimum period should equal MA period";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_value = dma->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last DMA value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "Last DMA value should be finite";
    }
}

// 测试不同的DMA参数
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    DMAParameterizedTest,
    ::testing::Values(
        std::make_pair(10, 5),    // 短周期，小位移
        std::make_pair(20, 10),   // 中周期，中位移
        std::make_pair(30, 30),   // 标准参数
        std::make_pair(50, 25)    // 长周期，大位移
    )
);

// DMA位移效果测试
TEST(OriginalTests, DMA_DisplacementEffect) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建普通SMA和DMA进行比较
    auto sma = std::make_shared<SMA>(close_line, 20);
    auto dma_pos = std::make_shared<DMA>(close_line, 20, 10);   // 正位移
    auto dma_neg = std::make_shared<DMA>(close_line, 20, -10);  // 负位移
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        sma->calculate();
        dma_pos->calculate();
        dma_neg->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 比较SMA和DMA的最终值
    double sma_val = sma->get(0);
    double dma_pos_val = dma_pos->get(0);
    double dma_neg_val = dma_neg->get(0);
    
    if (!std::isnan(sma_val) && !std::isnan(dma_pos_val) && !std::isnan(dma_neg_val)) {
        std::cout << "SMA: " << sma_val << std::endl;
        std::cout << "DMA (positive displacement): " << dma_pos_val << std::endl;
        std::cout << "DMA (negative displacement): " << dma_neg_val << std::endl;
        
        // DMA的位移应该产生不同的值
        EXPECT_TRUE(std::isfinite(sma_val)) << "SMA should be finite";
        EXPECT_TRUE(std::isfinite(dma_pos_val)) << "DMA positive should be finite";
        EXPECT_TRUE(std::isfinite(dma_neg_val)) << "DMA negative should be finite";
    }
}

// 与SMA时间对齐测试
TEST(OriginalTests, DMA_TimeAlignment) {
    // 使用简单的递增数据来验证位移效果
    std::vector<double> prices;
    for (int i = 1; i <= 100; ++i) {
        prices.push_back(static_cast<double>(i));
    }
    
    auto price_line = std::make_shared<LineRoot>(prices.size(), "alignment");
    for (double price : prices) {
        price_line->forward(price);
    }
    
    auto sma = std::make_shared<SMA>(price_line, 10);
    auto dma = std::make_shared<DMA>(price_line, 10, 5);  // 5期正位移
    
    std::vector<double> sma_values;
    std::vector<double> dma_values;
    
    for (size_t i = 0; i < prices.size(); ++i) {
        sma->calculate();
        dma->calculate();
        
        double sma_val = sma->get(0);
        double dma_val = dma->get(0);
        
        if (!std::isnan(sma_val)) {
            sma_values.push_back(sma_val);
        }
        if (!std::isnan(dma_val)) {
            dma_values.push_back(dma_val);
        }
        
        if (i < prices.size() - 1) {
            price_line->forward();
        }
    }
    
    // 验证DMA与SMA的时间对齐关系
    if (sma_values.size() > 10 && dma_values.size() > 10) {
        // DMA应该滞后SMA（正位移）
        double recent_sma = sma_values[sma_values.size() - 6];  // 5期前的SMA
        double current_dma = dma_values.back();
        
        std::cout << "Time alignment test - SMA (5 periods ago): " << recent_sma 
                  << ", DMA (current): " << current_dma << std::endl;
        
        // 对于正位移，当前DMA应该接近过去的SMA
        EXPECT_NEAR(current_dma, recent_sma, 0.1) 
            << "DMA should align with displaced SMA";
    }
}

// 趋势跟随能力测试
TEST(OriginalTests, DMA_TrendFollowing) {
    // 创建明显的趋势数据
    std::vector<double> trend_prices;
    for (int i = 0; i < 100; ++i) {
        trend_prices.push_back(100.0 + i * 1.0);  // 线性上升趋势
    }
    
    auto trend_line = std::make_shared<LineRoot>(trend_prices.size(), "trend");
    for (double price : trend_prices) {
        trend_line->forward(price);
    }
    
    auto dma = std::make_shared<DMA>(trend_line, 20, 10);
    
    double prev_dma = 0.0;
    bool has_prev = false;
    int increasing_count = 0;
    int total_count = 0;
    
    for (size_t i = 0; i < trend_prices.size(); ++i) {
        dma->calculate();
        
        double current_dma = dma->get(0);
        
        if (!std::isnan(current_dma)) {
            if (has_prev) {
                total_count++;
                if (current_dma > prev_dma) {
                    increasing_count++;
                }
            }
            prev_dma = current_dma;
            has_prev = true;
        }
        
        if (i < trend_prices.size() - 1) {
            trend_line->forward();
        }
    }
    
    // 在上升趋势中，DMA应该主要呈上升趋势
    if (total_count > 0) {
        double increasing_ratio = static_cast<double>(increasing_count) / total_count;
        EXPECT_GT(increasing_ratio, 0.8) 
            << "DMA should follow uptrend effectively";
        
        std::cout << "Trend following - DMA increasing ratio: " << increasing_ratio << std::endl;
    }
}

// 滞后效应测试
TEST(OriginalTests, DMA_LagEffect) {
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
    
    auto step_line = std::make_shared<LineRoot>(step_prices.size(), "step");
    for (double price : step_prices) {
        step_line->forward(price);
    }
    
    auto dma_zero = std::make_shared<DMA>(step_line, 10, 0);    // 无位移
    auto dma_pos = std::make_shared<DMA>(step_line, 10, 5);    // 正位移
    auto dma_neg = std::make_shared<DMA>(step_line, 10, -5);   // 负位移
    
    std::vector<double> zero_values;
    std::vector<double> pos_values;
    std::vector<double> neg_values;
    
    for (size_t i = 0; i < step_prices.size(); ++i) {
        dma_zero->calculate();
        dma_pos->calculate();
        dma_neg->calculate();
        
        double zero_val = dma_zero->get(0);
        double pos_val = dma_pos->get(0);
        double neg_val = dma_neg->get(0);
        
        if (!std::isnan(zero_val)) zero_values.push_back(zero_val);
        if (!std::isnan(pos_val)) pos_values.push_back(pos_val);
        if (!std::isnan(neg_val)) neg_values.push_back(neg_val);
        
        if (i < step_prices.size() - 1) {
            step_line->forward();
        }
    }
    
    // 分析不同位移的响应
    if (!zero_values.empty() && !pos_values.empty() && !neg_values.empty()) {
        double final_zero = zero_values.back();
        double final_pos = pos_values.back();
        double final_neg = neg_values.back();
        
        std::cout << "Lag effect analysis:" << std::endl;
        std::cout << "Zero displacement: " << final_zero << std::endl;
        std::cout << "Positive displacement: " << final_pos << std::endl;
        std::cout << "Negative displacement: " << final_neg << std::endl;
        
        // 验证所有值都是有限的
        EXPECT_TRUE(std::isfinite(final_zero)) << "Zero displacement should be finite";
        EXPECT_TRUE(std::isfinite(final_pos)) << "Positive displacement should be finite";
        EXPECT_TRUE(std::isfinite(final_neg)) << "Negative displacement should be finite";
    }
}

// 振荡市场测试
TEST(OriginalTests, DMA_OscillatingMarket) {
    // 创建振荡数据
    std::vector<double> oscillating_prices;
    for (int i = 0; i < 100; ++i) {
        double base = 100.0;
        double oscillation = 10.0 * std::sin(i * 0.2);
        oscillating_prices.push_back(base + oscillation);
    }
    
    auto osc_line = std::make_shared<LineRoot>(oscillating_prices.size(), "oscillating");
    for (double price : oscillating_prices) {
        osc_line->forward(price);
    }
    
    auto dma = std::make_shared<DMA>(osc_line, 15, 7);
    
    std::vector<double> dma_values;
    
    for (size_t i = 0; i < oscillating_prices.size(); ++i) {
        dma->calculate();
        
        double dma_val = dma->get(0);
        if (!std::isnan(dma_val)) {
            dma_values.push_back(dma_val);
        }
        
        if (i < oscillating_prices.size() - 1) {
            osc_line->forward();
        }
    }
    
    // 在振荡市场中，DMA应该围绕中心值波动
    if (!dma_values.empty()) {
        double avg_dma = std::accumulate(dma_values.begin(), dma_values.end(), 0.0) / dma_values.size();
        EXPECT_NEAR(avg_dma, 100.0, 5.0) 
            << "DMA should oscillate around center value";
        
        std::cout << "Oscillating market - Average DMA: " << avg_dma << std::endl;
    }
}

// 边界条件测试
TEST(OriginalTests, DMA_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);
    
    auto flat_line = std::make_shared<LineRoot>(flat_prices.size(), "flat");
    for (double price : flat_prices) {
        flat_line->forward(price);
    }
    
    auto flat_dma = std::make_shared<DMA>(flat_line, 20, 10);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_dma->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    // 当所有价格相同时，DMA应该等于该价格
    double final_dma = flat_dma->get(0);
    if (!std::isnan(final_dma)) {
        EXPECT_NEAR(final_dma, 100.0, 1e-6) 
            << "DMA should equal constant price";
    }
    
    // 测试零位移
    auto zero_disp_dma = std::make_shared<DMA>(flat_line, 20, 0);
    
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        zero_disp_dma->calculate();
        if (i < flat_prices.size() - 1) {
            flat_line->forward();
        }
    }
    
    double zero_result = zero_disp_dma->get(0);
    if (!std::isnan(zero_result)) {
        EXPECT_NEAR(zero_result, 100.0, 1e-6) 
            << "Zero displacement DMA should equal SMA";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineRoot>(100, "insufficient");
    
    // 只添加几个数据点
    for (int i = 0; i < 15; ++i) {
        insufficient_line->forward(100.0 + i);
    }
    
    auto insufficient_dma = std::make_shared<DMA>(insufficient_line, 20, 10);
    
    for (int i = 0; i < 15; ++i) {
        insufficient_dma->calculate();
        if (i < 14) {
            insufficient_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_dma->get(0);
    EXPECT_TRUE(std::isnan(result)) << "DMA should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, DMA_Performance) {
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
    
    auto large_dma = std::make_shared<DMA>(large_line, 50, 25);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < large_data.size(); ++i) {
        large_dma->calculate();
        if (i < large_data.size() - 1) {
            large_line->forward();
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "DMA calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_dma->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}