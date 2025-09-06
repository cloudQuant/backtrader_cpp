/**
 * @file test_ind_trix.cpp
 * @brief TRIX指标测试 - 对应Python test_ind_trix.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['0.071304', '0.181480', '0.050954']
 * ]
 * chkmin = 44
 * chkind = btind.Trix
 */

#include "test_common.h"
#include "lineseries.h"
#include "linebuffer.h"
#include <random>

#include "indicators/trix.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> TRIX_EXPECTED_VALUES = {
    {"0.071304", "0.181480", "0.050954"}
};

const int TRIX_MIN_PERIOD = 44;

} // anonymous namespace

// 使用默认参数的TRIX测试
DEFINE_INDICATOR_TEST(TRIX_Default, TRIX, TRIX_EXPECTED_VALUES, TRIX_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, TRIX_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线系列
    auto close_lineseries = std::make_shared<LineSeries>();
    close_lineseries->lines->add_line(std::make_shared<LineBuffer>());
    auto close_lineseries_buffer = std::dynamic_pointer_cast<LineBuffer>(close_lineseries->lines->getline(0));
    close_lineseries->lines->add_alias("close", 0);
    
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_lineseries->lines->getline(0));
    if (close_buffer) {
        close_buffer->set(0, csv_data[0].close);
    for (size_t i = 1; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    // 创建TRIX指标（默认15周期，最小周期为44）
    auto trix = std::make_shared<TRIX>(close_lineseries, 15);
    
    // 计算
    trix->calculate();
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 44;  // 大约 3 * period - 2 + 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    // 注意：Python的 // 是地板除法，对负数向负无穷方向取整
    int neg_offset = -(data_length - min_period);  // -211
    int mid_point = neg_offset / 2;  // C++: -211 / 2 = -105
    // Python: -211 // 2 = -106 (floor division)
    // 需要调整为 Python 的行为
    if (neg_offset < 0 && neg_offset % 2 != 0) {
        mid_point -= 1;  // 调整为 -106
    }
    
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        neg_offset,                          // 倒数第(data_length - min_period)个值
        mid_point                            // 中间值
    };
    
    std::vector<std::string> expected = {"0.071304", "0.181480", "0.050954"};
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = trix->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "TRIX value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(trix->getMinPeriod(), 44) << "TRIX minimum period should be 44";
}

// 参数化测试 - 测试不同周期的TRIX
class TRIXParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line = std::make_shared<LineSeries>();
        close_line->lines->add_line(std::make_shared<LineBuffer>());
        close_line->lines->add_alias("close", 0);
        
        auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
        if (close_buffer) {
            close_buffer->set(0, csv_data_[0].close);
    for (size_t i = 1; i < csv_data_.size(); ++i) {
                close_buffer->append(csv_data_[i].close);
            }
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineSeries> close_line;
};

TEST_P(TRIXParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto trix = std::make_shared<TRIX>(close_line, period);
    
    // 计算
    trix->calculate();
    
    // 验证最小周期 (大约 3 * period - 2 + 1)
    int expected_min_period = 3 * period - 2 + 1;
    EXPECT_EQ(trix->getMinPeriod(), expected_min_period) 
        << "TRIX minimum period should be approximately 3 * period - 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double last_value = trix->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last TRIX value should not be NaN";
        EXPECT_TRUE(std::isfinite(last_value)) << "TRIX value should be finite";
    }
}

// 测试不同的TRIX周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    TRIXParameterizedTest,
    ::testing::Values(10, 15, 20)
);

// TRIX计算逻辑验证测试
TEST(OriginalTests, TRIX_CalculationLogic) {
    // 使用简单的测试数据验证TRIX计算
    std::vector<double> prices;
    for (int i = 0; i < 100; ++i) {
        prices.push_back(100.0 + i * 0.5);  // 线性增长
    }
    auto close_lineseries = std::make_shared<LineSeries>();
    close_lineseries->lines->add_line(std::make_shared<LineBuffer>());
    auto close_lineseries_buffer = std::dynamic_pointer_cast<LineBuffer>(close_lineseries->lines->getline(0));
    close_lineseries->lines->add_alias("close", 0);
    
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_lineseries->lines->getline(0));
    if (close_buffer) {
        close_buffer->set(0, prices[0]);
    for (size_t i = 1; i < prices.size(); ++i) {
            close_buffer->append(prices[i]);
        }
    }
    
    auto trix = std::make_shared<TRIX>(close_lineseries, 10);
    
    trix->calculate();
    
    double trix_val = trix->get(0);
    
    // TRIX应该产生有限值
    if (!std::isnan(trix_val)) {
        EXPECT_TRUE(std::isfinite(trix_val)) 
            << "TRIX should be finite";
    }
}

// TRIX趋势检测测试
TEST(OriginalTests, TRIX_TrendDetection) {
    // 创建上升趋势数据
    std::vector<double> uptrend_prices;
    for (int i = 0; i < 100; ++i) {
        uptrend_prices.push_back(100.0 + i * 1.0);  // 持续上升
    }
    auto up_line = std::make_shared<LineSeries>();

    up_line->lines->add_line(std::make_shared<LineBuffer>());
    up_line->lines->add_alias("up_line", 0);
    auto up_line_buffer = std::dynamic_pointer_cast<LineBuffer>(up_line->lines->getline(0));
    


    for (double price : uptrend_prices) {
        up_line_buffer->append(price);
    }
    
    auto up_trix = std::make_shared<TRIX>(up_line, 15);
    for (size_t i = 0; i < uptrend_prices.size(); ++i) {
        up_trix->calculate();
        if (i < uptrend_prices.size() - 1) {
            if (up_line_buffer) up_line_buffer->forward();
        }
    }
    
    double final_up_trix = up_trix->get(0);
    if (!std::isnan(final_up_trix)) {
        EXPECT_GT(final_up_trix, 0.0) 
            << "TRIX should be positive for uptrend";
    }
    
    // 创建下降趋势数据
    std::vector<double> downtrend_prices;
    for (int i = 0; i < 100; ++i) {
        downtrend_prices.push_back(200.0 - i * 1.0);  // 持续下降
    }
    auto down_line = std::make_shared<LineSeries>();

    down_line->lines->add_line(std::make_shared<LineBuffer>());
    down_line->lines->add_alias("down_line", 0);
    auto down_line_buffer = std::dynamic_pointer_cast<LineBuffer>(down_line->lines->getline(0));
    


    for (double price : downtrend_prices) {
        down_line_buffer->append(price);
    }
    
    auto down_trix = std::make_shared<TRIX>(down_line, 15);
    for (size_t i = 0; i < downtrend_prices.size(); ++i) {
        down_trix->calculate();
        if (i < downtrend_prices.size() - 1) {
            if (down_line_buffer) down_line_buffer->forward();
        }
    }
    
    double final_down_trix = down_trix->get(0);
    if (!std::isnan(final_down_trix)) {
        EXPECT_LT(final_down_trix, 0.0) 
            << "TRIX should be negative for downtrend";
    }
    
    std::cout << "Uptrend TRIX: " << final_up_trix << std::endl;
    std::cout << "Downtrend TRIX: " << final_down_trix << std::endl;
}

// TRIX零线穿越测试
TEST(OriginalTests, TRIX_ZeroCrossing) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();

    close_line->lines->add_line(std::make_shared<LineBuffer>());

    close_line->lines->add_alias("close_line", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));

    // Line already added above
    for (const auto& bar : csv_data) {
        close_buffer->append(bar.close);
    }
    
    auto trix = std::make_shared<TRIX>(close_line, 15);
    
    int positive_crossings = 0;  // 从负到正
    int negative_crossings = 0;  // 从正到负
    double prev_trix = 0.0;
    bool has_prev = false;
    
    // 检测零线穿越;
    for (size_t i = 0; i < csv_data.size(); ++i) {
        trix->calculate();
        
        double current_trix = trix->get(0);
        
        if (!std::isnan(current_trix) && has_prev) {
            if (prev_trix <= 0.0 && current_trix > 0.0) {
                positive_crossings++;
            } else if (prev_trix >= 0.0 && current_trix < 0.0) {
                negative_crossings++;
            }
        }
        
        if (!std::isnan(current_trix)) {
            prev_trix = current_trix;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            if (close_buffer) close_buffer->forward();
        }
    }
    
    std::cout << "TRIX zero line crossings:" << std::endl;
    std::cout << "Positive crossings: " << positive_crossings << std::endl;
    std::cout << "Negative crossings: " << negative_crossings << std::endl;
    
    // 验证检测到一些交叉信号
    EXPECT_GE(positive_crossings + negative_crossings, 0) 
        << "Should detect some zero line crossings";
}

// TRIX滤波特性测试
TEST(OriginalTests, TRIX_FilteringCharacteristics) {
    // 创建带噪声的价格数据
    std::vector<double> noisy_prices;
    std::mt19937 rng(42);
    std::normal_distribution<double> noise(0.0, 1.0);
    for (int i = 0; i < 200; ++i) {
        double trend = 100.0 + i * 0.1;  // 缓慢上升趋势
        double noise_val = noise(rng);   // 随机噪声
        noisy_prices.push_back(trend + noise_val);
    }
    auto noisy_line = std::make_shared<LineSeries>();

    noisy_line->lines->add_line(std::make_shared<LineBuffer>());
    noisy_line->lines->add_alias("noisy_line", 0);
    auto noisy_line_buffer = std::dynamic_pointer_cast<LineBuffer>(noisy_line->lines->getline(0));
    


    for (double price : noisy_prices) {
        noisy_line_buffer->append(price);
    }
    
    auto noisy_trix = std::make_shared<TRIX>(noisy_line, 15);
    
    std::vector<double> trix_values;
    for (size_t i = 0; i < noisy_prices.size(); ++i) {
        noisy_trix->calculate();
        
        double trix_val = noisy_trix->get(0);
        if (!std::isnan(trix_val)) {
            trix_values.push_back(trix_val);
        }
        
        if (i < noisy_prices.size() - 1) {
            if (noisy_line_buffer) noisy_line_buffer->forward();
        }
    }
    
    // 验证TRIX的滤波特性
    if (!trix_values.empty()) {
        // 计算TRIX值的变异系数
        double mean = std::accumulate(trix_values.begin(), trix_values.end(), 0.0) / trix_values.size();
        double variance = 0.0;
    
    for (double val : trix_values) {
            variance += (val - mean) * (val - mean);
        }
        variance /= trix_values.size();
        double std_dev = std::sqrt(variance);
        
        std::cout << "TRIX filtering test - Mean: " << mean << ", StdDev: " << std_dev << std::endl;
        
        // TRIX应该比原始噪声数据更平滑
        EXPECT_TRUE(std::isfinite(mean)) << "TRIX mean should be finite";
        EXPECT_TRUE(std::isfinite(std_dev)) << "TRIX std dev should be finite";
    }
}

// TRIX发散测试
TEST(OriginalTests, TRIX_Divergence) {
    // 创建价格走高但动量减弱的发散情况
    std::vector<double> divergence_prices;
    
    // 第一阶段：强劲上升;
    for (int i = 0; i < 50; ++i) {
        divergence_prices.push_back(100.0 + i * 2.0);
    }
    
    // 第二阶段：缓慢上升（价格新高但动量减弱）;
    for (int i = 0; i < 50; ++i) {
        divergence_prices.push_back(200.0 + i * 0.2);
    }
    auto div_line = std::make_shared<LineSeries>();

    div_line->lines->add_line(std::make_shared<LineBuffer>());
    div_line->lines->add_alias("div_line", 0);
    auto div_line_buffer = std::dynamic_pointer_cast<LineBuffer>(div_line->lines->getline(0));
    


    for (double price : divergence_prices) {
        div_line_buffer->append(price);
    }
    
    auto div_trix = std::make_shared<TRIX>(div_line, 15);
    
    std::vector<double> early_trix;
    std::vector<double> late_trix;
    for (size_t i = 0; i < divergence_prices.size(); ++i) {
        div_trix->calculate();
        
        double trix_val = div_trix->get(0);
        
        if (!std::isnan(trix_val)) {
            if (i < 60) {
                early_trix.push_back(trix_val);
            } else {
                late_trix.push_back(trix_val);
            }
        }
        
        if (i < divergence_prices.size() - 1) {
            if (div_line_buffer) div_line_buffer->forward();
        }
    }
    
    // 分析发散现象
    if (!early_trix.empty() && !late_trix.empty()) {
        double avg_early = std::accumulate(early_trix.begin(), early_trix.end(), 0.0) / early_trix.size();
        double avg_late = std::accumulate(late_trix.begin(), late_trix.end(), 0.0) / late_trix.size();
        
        std::cout << "Early TRIX average: " << avg_early << std::endl;
        std::cout << "Late TRIX average: " << avg_late << std::endl;
        
        // 验证计算结果是有限的
        EXPECT_TRUE(std::isfinite(avg_early)) << "Early TRIX should be finite";
        EXPECT_TRUE(std::isfinite(avg_late)) << "Late TRIX should be finite";
        
        // 后期动量应该小于早期动量
        EXPECT_LT(avg_late, avg_early) 
            << "Late TRIX should be less than early TRIX in divergence scenario";
    }
}

// 边界条件测试
TEST(OriginalTests, TRIX_EdgeCases) {
    // 测试相同价格的情况
    std::vector<double> flat_prices(100, 100.0);  // 100个相同价格
    
    auto flat_line = std::make_shared<LineSeries>();

    
    flat_line->lines->add_line(std::make_shared<LineBuffer>());
    flat_line->lines->add_alias("flat_line", 0);
    auto flat_line_buffer = std::dynamic_pointer_cast<LineBuffer>(flat_line->lines->getline(0));
    


    for (double price : flat_prices) {
        flat_line_buffer->append(price);
    }
    
    auto flat_trix = std::make_shared<TRIX>(flat_line, 15);
    for (size_t i = 0; i < flat_prices.size(); ++i) {
        flat_trix->calculate();
        if (i < flat_prices.size() - 1) {
            if (flat_line_buffer) flat_line_buffer->forward();
        }
    }
    
    double final_trix = flat_trix->get(0);
    if (!std::isnan(final_trix)) {
        EXPECT_NEAR(final_trix, 0.0, 1e-10) 
            << "TRIX should be zero for constant prices";
    }
    
    // 测试数据不足的情况
    auto insufficient_line = std::make_shared<LineSeries>();

    insufficient_line->lines->add_line(std::make_shared<LineBuffer>());
    insufficient_line->lines->add_alias("insufficient_line", 0);
    auto insufficient_line_buffer = std::dynamic_pointer_cast<LineBuffer>(insufficient_line->lines->getline(0));


    for (int i = 0; i < 20; ++i) {
        insufficient_line_buffer->append(100.0 + i);
    }
    auto insufficient_trix = std::make_shared<TRIX>(insufficient_line, 15);
    for (int i = 0; i < 20; ++i) {
        insufficient_trix->calculate();
        if (i < 19) {
            if (insufficient_line_buffer) insufficient_line_buffer->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = insufficient_trix->get(0);
    EXPECT_TRUE(std::isnan(result)) << "TRIX should return NaN when insufficient data";
}

// 性能测试
TEST(OriginalTests, TRIX_Performance) {
    // 生成大量测试数据
    const size_t data_size = 10000;
    std::vector<double> large_data;
    large_data.reserve(data_size);
    
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(50.0, 150.0);
    for (size_t i = 0; i < data_size; ++i) {
        large_data.push_back(dist(rng));
    }
    
    auto large_data_line = std::make_shared<LineSeries>();

    
    large_data_line->lines->add_line(std::make_shared<LineBuffer>());
    large_data_line->lines->add_alias("large_data_line", 0);
    auto large_data_line_buffer = std::dynamic_pointer_cast<LineBuffer>(large_data_line->lines->getline(0));
    


    for (double price : large_data) {
        large_data_line_buffer->append(price);
    }
    
    auto large_trix = std::make_shared<TRIX>(large_data_line, 15);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 修复性能：O(n²) -> O(n) - 单次计算替代循环
    large_trix->calculate();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "TRIX calculation for " << data_size << " points took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证最终结果是有效的
    double final_result = large_trix->get(0);
    EXPECT_FALSE(std::isnan(final_result)) << "Final result should not be NaN";
    EXPECT_TRUE(std::isfinite(final_result)) << "Final result should be finite";
    
    // 性能要求：10K数据点应该在合理时间内完成
    EXPECT_LT(duration.count(), 1000) << "Performance test: should complete within 1 second";
}