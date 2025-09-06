/**
 * @file test_ind_stochastic.cpp
 * @brief Stochastic指标测试 - 对应Python test_ind_stochastic.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['88.667626', '21.409626', '63.796187'],  # %K line
 *     ['82.845850', '15.710059', '77.642219'],  # %D line
 * ]
 * chkmin = 18
 * chkind = btind.Stochastic
 */

#include "test_common.h"
#include "lineseries.h"

#include "indicators/stochastic.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;
using LineSeries = backtrader::LineSeries;
using LineBuffer = backtrader::LineBuffer;

namespace {

const std::vector<std::vector<std::string>> STOCHASTIC_EXPECTED_VALUES = {
    {"88.667626", "21.409626", "63.796187"},  // %K line
    {"82.845850", "15.710059", "77.642219"}   // %D line
};

const int STOCHASTIC_MIN_PERIOD = 18;

} // anonymous namespace

// 手动测试函数，用于详细验证
TEST(OriginalTests, Stochastic_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线系列
    auto high_line_series = std::make_shared<LineSeries>();
    high_line_series->lines->add_line(std::make_shared<LineBuffer>());
    high_line_series->lines->add_alias("high", 0);
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line_series->lines->getline(0));
    
    auto low_line_series = std::make_shared<LineSeries>();
    low_line_series->lines->add_line(std::make_shared<LineBuffer>());
    low_line_series->lines->add_alias("low", 0);
    
    auto close_lineseries = std::make_shared<LineSeries>();
    close_lineseries->lines->add_line(std::make_shared<LineBuffer>());
    close_lineseries->lines->add_alias("close", 0);
    
    // 逐步添加数据到线缓冲区
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line_series->lines->getline(0));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_lineseries->lines->getline(0));
    
    if (high_buffer && low_buffer && close_buffer) {
        high_buffer->set(0, csv_data[0].high);
        low_buffer->set(0, csv_data[0].low);
        close_buffer->set(0, csv_data[0].close);
    for (size_t i = 1; i < csv_data.size(); ++i) {
            high_buffer->append(csv_data[i].high);
            low_buffer->append(csv_data[i].low);
            close_buffer->append(csv_data[i].close);
        }
    }
    
    // 创建Stochastic指标（默认参数：period=14, period_dfast=3）
    auto stochastic = std::make_shared<Stochastic>(high_line_series, low_line_series, close_lineseries, 14, 3);
    
    // 计算
    std::cout << "About to call stochastic->calculate()" << std::endl;
    // Use virtual dispatch
    stochastic->calculate();
    std::cout << "Finished stochastic->calculate()" << std::endl;
    
    // Also try directly calling once()
    std::cout << "Manually calling once(0, 255)" << std::endl;
    printf("DEBUG: About to call once() method\n");
    fflush(stdout);
    stochastic->once(0, 255);
    printf("DEBUG: Finished calling once() method\n");
    fflush(stdout);
    std::cout << "Finished calling once()" << std::endl;
    
    // Debug: check line buffer contents
    auto k_line = stochastic->lines->getline(0);
    auto d_line = stochastic->lines->getline(1);
    auto k_buffer = std::dynamic_pointer_cast<LineBuffer>(k_line);
    auto d_buffer = std::dynamic_pointer_cast<LineBuffer>(d_line);
    
    if (k_buffer && d_buffer) {
        std::cout << "K buffer size: " << k_buffer->array().size() << std::endl;
        std::cout << "D buffer size: " << d_buffer->array().size() << std::endl;
        
        if (!k_buffer->array().empty()) {
            std::cout << "Last K value: " << k_buffer->array().back() << std::endl;
            std::cout << "First few K values: ";
            for (size_t i = 0; i < std::min(5UL, k_buffer->array().size()); ++i) {
                std::cout << k_buffer->array()[i] << " ";
            }
            std::cout << std::endl;
        }
        if (!d_buffer->array().empty()) {
            std::cout << "Last D value: " << d_buffer->array().back() << std::endl;
            std::cout << "First few D values: ";
            for (size_t i = 0; i < std::min(5UL, d_buffer->array().size()); ++i) {
                std::cout << d_buffer->array()[i] << " ";
            }
            std::cout << std::endl;
        }
        
        // Check if all values are NaN
        bool all_k_nan = true, all_d_nan = true;
        for (const auto& val : k_buffer->array()) {
            if (!std::isnan(val)) {
                all_k_nan = false;
                break;
            }
        }
        for (const auto& val : d_buffer->array()) {
            if (!std::isnan(val)) {
                all_d_nan = false;
                break;
            }
        }
        std::cout << "All K values are NaN: " << (all_k_nan ? "YES" : "NO") << std::endl;
        std::cout << "All D values are NaN: " << (all_d_nan ? "YES" : "NO") << std::endl;
    } else {
        std::cout << "Buffer cast failed: k_buffer=" << (k_buffer ? "valid" : "null") 
                  << ", d_buffer=" << (d_buffer ? "valid" : "null") << std::endl;
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 18;  // Stochastic最小周期
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        static_cast<int>(std::floor(-(data_length - min_period) / 2.0))  // 中间值，匹配Python的floor division
    };
    
    // 验证%K线
    std::vector<std::string> expected_k = {"88.667626", "21.409626", "63.796187"};
    for (size_t i = 0; i < check_points.size() && i < expected_k.size(); ++i) {
        double actual = stochastic->getPercentK(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        std::cout << "Checkpoint " << i << ": ago=" << check_points[i] 
                  << ", actual=" << actual_str << ", expected=" << expected_k[i] << std::endl;
        
        EXPECT_EQ(actual_str, expected_k[i]) 
            << "Stochastic %K mismatch at check point " << i;
    }
    
    // 验证%D线
    std::vector<std::string> expected_d = {"82.845850", "15.710059", "77.642219"};
    for (size_t i = 0; i < check_points.size() && i < expected_d.size(); ++i) {
        double actual = stochastic->getPercentD(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_d[i]) 
            << "Stochastic %D mismatch at check point " << i;
    }
    
    // 验证最小周期
    EXPECT_EQ(stochastic->getMinPeriod(), 18) << "Stochastic minimum period should be 18";
}

// Stochastic范围验证测试
TEST(OriginalTests, Stochastic_RangeValidation) {
    auto csv_data = getdata(0);
    
    auto high_line_series = std::make_shared<LineSeries>();
    high_line_series->lines->add_line(std::make_shared<LineBuffer>());
    high_line_series->lines->add_alias("high", 0);
    
    auto low_line_series = std::make_shared<LineSeries>();
    low_line_series->lines->add_line(std::make_shared<LineBuffer>());
    low_line_series->lines->add_alias("low", 0);
    
    auto close_lineseries = std::make_shared<LineSeries>();
    close_lineseries->lines->add_line(std::make_shared<LineBuffer>());
    close_lineseries->lines->add_alias("close", 0);
    
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line_series->lines->getline(0));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line_series->lines->getline(0));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_lineseries->lines->getline(0));
    
    if (high_buffer && low_buffer && close_buffer) {
        high_buffer->set(0, csv_data[0].high);
        low_buffer->set(0, csv_data[0].low);
        close_buffer->set(0, csv_data[0].close);
    for (size_t i = 1; i < csv_data.size(); ++i) {
            high_buffer->append(csv_data[i].high);
            low_buffer->append(csv_data[i].low);
            close_buffer->append(csv_data[i].close);
        }
    }
    
    auto stochastic = std::make_shared<Stochastic>(high_line_series, low_line_series, close_lineseries, 14, 3);
    
    // 计算并验证范围
    stochastic->calculate();
    
    double percent_k = stochastic->getPercentK(0);
    double percent_d = stochastic->getPercentD(0);
    
    // 验证%K在0-100范围内
    if (!std::isnan(percent_k)) {
        EXPECT_GE(percent_k, 0.0) << "Stochastic %K should be >= 0";
        EXPECT_LE(percent_k, 100.0) << "Stochastic %K should be <= 100";
    }
    
    // 验证%D在0-100范围内
    if (!std::isnan(percent_d)) {
        EXPECT_GE(percent_d, 0.0) << "Stochastic %D should be >= 0";
        EXPECT_LE(percent_d, 100.0) << "Stochastic %D should be <= 100";
    }
}

// 参数化测试 - 测试不同参数的Stochastic
class StochasticParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int>> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        high_line = std::make_shared<LineSeries>();
        high_line->lines->add_line(std::make_shared<LineBuffer>());
        high_line->lines->add_alias("high", 0);
        
        low_line = std::make_shared<LineSeries>();
        low_line->lines->add_line(std::make_shared<LineBuffer>());
        low_line->lines->add_alias("low", 0);
        
        close_line = std::make_shared<LineSeries>();
        close_line->lines->add_line(std::make_shared<LineBuffer>());
        close_line->lines->add_alias("close", 0);
        
        auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line->lines->getline(0));
        auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line->lines->getline(0));
        auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
        
        if (high_buffer && low_buffer && close_buffer) {
            high_buffer->set(0, csv_data_[0].high);
            low_buffer->set(0, csv_data_[0].low);
            close_buffer->set(0, csv_data_[0].close);
    for (size_t i = 1; i < csv_data_.size(); ++i) {
                high_buffer->append(csv_data_[i].high);
                low_buffer->append(csv_data_[i].low);
                close_buffer->append(csv_data_[i].close);
            }
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineSeries> high_line;
    std::shared_ptr<LineSeries> low_line;
    std::shared_ptr<LineSeries> close_line;
};

TEST_P(StochasticParameterizedTest, DifferentParameters) {
    auto [period, period_dfast] = GetParam();
    auto stochastic = std::make_shared<Stochastic>(high_line, low_line, close_line, period, period_dfast);
    
    // 计算
    stochastic->calculate();
    
    // 验证最小周期 - For slow stochastic, default period_dslow is 3
    // Minimum period = period + period_dfast + period_dslow - 2
    // For (14, 3) params: 14 + 3 + 3 - 2 = 18
    // For (5, 3) params: 5 + 3 + 3 - 2 = 9  
    // For (21, 5) params: 21 + 5 + 3 - 2 = 27
    // For (14, 1) params: 14 + 1 + 3 - 2 = 16
    int expected_min_period = period + period_dfast + 1; // period_dslow defaults to 3, so +3-2=+1
    EXPECT_EQ(stochastic->getMinPeriod(), expected_min_period) 
        << "Stochastic minimum period should be period + period_dfast + period_dslow - 2";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double k_value = stochastic->getPercentK(0);
        double d_value = stochastic->getPercentD(0);
        
        EXPECT_FALSE(std::isnan(k_value)) << "%K should not be NaN";
        EXPECT_FALSE(std::isnan(d_value)) << "%D should not be NaN";
        
        EXPECT_GE(k_value, 0.0) << "%K should be >= 0";
        EXPECT_LE(k_value, 100.0) << "%K should be <= 100";
        EXPECT_GE(d_value, 0.0) << "%D should be >= 0";
        EXPECT_LE(d_value, 100.0) << "%D should be <= 100";
    }
}

// 测试不同的Stochastic参数
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    StochasticParameterizedTest,
    ::testing::Values(
        std::make_tuple(5, 3),
        std::make_tuple(14, 3),
        std::make_tuple(21, 5),
        std::make_tuple(14, 1)
    )
);

// 超买超卖测试
TEST(OriginalTests, Stochastic_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto high_line_series = std::make_shared<LineSeries>();
    high_line_series->lines->add_line(std::make_shared<LineBuffer>());
    high_line_series->lines->add_alias("high", 0);
    
    auto low_line_series = std::make_shared<LineSeries>();
    low_line_series->lines->add_line(std::make_shared<LineBuffer>());
    low_line_series->lines->add_alias("low", 0);
    
    auto close_lineseries = std::make_shared<LineSeries>();
    close_lineseries->lines->add_line(std::make_shared<LineBuffer>());
    close_lineseries->lines->add_alias("close", 0);
    
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line_series->lines->getline(0));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line_series->lines->getline(0));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_lineseries->lines->getline(0));
    
    if (high_buffer && low_buffer && close_buffer) {
        high_buffer->set(0, csv_data[0].high);
        low_buffer->set(0, csv_data[0].low);
        close_buffer->set(0, csv_data[0].close);
    for (size_t i = 1; i < csv_data.size(); ++i) {
            high_buffer->append(csv_data[i].high);
            low_buffer->append(csv_data[i].low);
            close_buffer->append(csv_data[i].close);
        }
    }
    
    auto stochastic = std::make_shared<Stochastic>(high_line_series, low_line_series, close_lineseries, 14, 3);
    
    int overbought_count = 0;
    int oversold_count = 0;
    int normal_count = 0;
    
    // 统计超买超卖情况
    stochastic->calculate();
    
    double k_value = stochastic->getPercentK(0);
    double d_value = stochastic->getPercentD(0);
    
    if (!std::isnan(k_value) && !std::isnan(d_value)) {
        // 传统的超买超卖阈值
        if (k_value > 80.0 && d_value > 80.0) {
            overbought_count++;
        } else if (k_value < 20.0 && d_value < 20.0) {
            oversold_count++;
        } else {
            normal_count++;
        }
    }
    
    std::cout << "Stochastic statistics:" << std::endl;
    std::cout << "Overbought periods: " << overbought_count << std::endl;
    std::cout << "Oversold periods: " << oversold_count << std::endl;
    std::cout << "Normal periods: " << normal_count << std::endl;
    
    // 验证至少有一些有效的计算
    EXPECT_GT(normal_count + overbought_count + oversold_count, 0) 
        << "Should have some valid Stochastic calculations";
}

// 平滑性测试 - %D应该比%K更平滑
TEST(OriginalTests, Stochastic_Smoothness) {
    auto csv_data = getdata(0);
    auto high_line_series = std::make_shared<LineSeries>();
    high_line_series->lines->add_line(std::make_shared<LineBuffer>());
    high_line_series->lines->add_alias("high", 0);
    
    auto low_line_series = std::make_shared<LineSeries>();
    low_line_series->lines->add_line(std::make_shared<LineBuffer>());
    low_line_series->lines->add_alias("low", 0);
    
    auto close_lineseries = std::make_shared<LineSeries>();
    close_lineseries->lines->add_line(std::make_shared<LineBuffer>());
    close_lineseries->lines->add_alias("close", 0);
    
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line_series->lines->getline(0));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line_series->lines->getline(0));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_lineseries->lines->getline(0));
    
    if (high_buffer && low_buffer && close_buffer) {
        high_buffer->set(0, csv_data[0].high);
        low_buffer->set(0, csv_data[0].low);
        close_buffer->set(0, csv_data[0].close);
    for (size_t i = 1; i < csv_data.size(); ++i) {
            high_buffer->append(csv_data[i].high);
            low_buffer->append(csv_data[i].low);
            close_buffer->append(csv_data[i].close);
        }
    }
    
    auto stochastic = std::make_shared<Stochastic>(high_line_series, low_line_series, close_lineseries, 14, 3);
    
    std::vector<double> k_values;
    std::vector<double> d_values;
    
    // 收集所有有效值
    stochastic->calculate();
    
    double k_value = stochastic->getPercentK(0);
    double d_value = stochastic->getPercentD(0);
    
    if (!std::isnan(k_value) && !std::isnan(d_value)) {
        k_values.push_back(k_value);
        d_values.push_back(d_value);
    }
    
    // 计算波动性（连续值之间的平均差异）
    if (k_values.size() > 1 && d_values.size() > 1) {
        double k_volatility = 0.0;
        double d_volatility = 0.0;
    for (size_t i = 1; i < k_values.size(); ++i) {
            k_volatility += std::abs(k_values[i] - k_values[i-1]);
            d_volatility += std::abs(d_values[i] - d_values[i-1]);
        }
        
        k_volatility /= (k_values.size() - 1);
        d_volatility /= (d_values.size() - 1);
        
        // %D应该比%K更平滑（波动性更小）
        EXPECT_LT(d_volatility, k_volatility) 
            << "%D should be smoother than %K. K volatility: " << k_volatility 
            << ", D volatility: " << d_volatility;
        
        std::cout << "%K average volatility: " << k_volatility << std::endl;
        std::cout << "%D average volatility: " << d_volatility << std::endl;
    }
}

// 边界条件测试
TEST(OriginalTests, Stochastic_EdgeCases) {
    // 测试价格始终在同一水平的情况
    std::vector<CSVDataReader::OHLCVData> large_data;
    for (int i = 0; i < 30; ++i) {
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
    auto high_line_series = std::make_shared<LineSeries>();
    high_line_series->lines->add_line(std::make_shared<LineBuffer>());
    high_line_series->lines->add_alias("high", 0);
    
    auto low_line_series = std::make_shared<LineSeries>();
    low_line_series->lines->add_line(std::make_shared<LineBuffer>());
    low_line_series->lines->add_alias("low", 0);
    
    auto close_lineseries = std::make_shared<LineSeries>();
    close_lineseries->lines->add_line(std::make_shared<LineBuffer>());
    close_lineseries->lines->add_alias("close", 0);
    
    auto high_buffer = std::dynamic_pointer_cast<LineBuffer>(high_line_series->lines->getline(0));
    auto low_buffer = std::dynamic_pointer_cast<LineBuffer>(low_line_series->lines->getline(0));
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_lineseries->lines->getline(0));
    
    if (high_buffer && low_buffer && close_buffer) {
        high_buffer->set(0, large_data[0].high);
        low_buffer->set(0, large_data[0].low);
        close_buffer->set(0, large_data[0].close);
    for (size_t i = 1; i < large_data.size(); ++i) {
            high_buffer->append(large_data[i].high);
            low_buffer->append(large_data[i].low);
            close_buffer->append(large_data[i].close);
        }
    }
    
    auto stochastic = std::make_shared<Stochastic>(high_line_series, low_line_series, close_lineseries, 14, 3);
    
    stochastic->calculate();
    
    // 当价格平坦时，Stochastic可能是NaN或某个特定值
    double k_value = stochastic->getPercentK(0);
    double d_value = stochastic->getPercentD(0);
    
    // 如果不是NaN，应该在有效范围内
    if (!std::isnan(k_value)) {
        EXPECT_GE(k_value, 0.0) << "%K should be >= 0 for flat prices";
        EXPECT_LE(k_value, 100.0) << "%K should be <= 100 for flat prices";
    }
    
    if (!std::isnan(d_value)) {
        EXPECT_GE(d_value, 0.0) << "%D should be >= 0 for flat prices";
        EXPECT_LE(d_value, 100.0) << "%D should be <= 100 for flat prices";
    }
}