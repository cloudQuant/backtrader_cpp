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

#include "indicators/stochastic.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

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
    
    // 创建数据线
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    // 创建Stochastic指标（默认参数：period=14, period_dfast=3）
    auto stochastic = std::make_shared<Stochastic>(close_line, high_line, low_line, 14, 3);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        stochastic->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 18;  // Stochastic最小周期
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // 验证%K线
    std::vector<std::string> expected_k = {"88.667626", "21.409626", "63.796187"};
    for (size_t i = 0; i < check_points.size() && i < expected_k.size(); ++i) {
        double actual = stochastic->getPercentK(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
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
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto stochastic = std::make_shared<Stochastic>(close_line, high_line, low_line, 14, 3);
    
    // 计算所有值并验证范围
    for (size_t i = 0; i < csv_data.size(); ++i) {
        stochastic->calculate();
        
        double percent_k = stochastic->getPercentK(0);
        double percent_d = stochastic->getPercentD(0);
        
        // 验证%K在0-100范围内
        if (!std::isnan(percent_k)) {
            EXPECT_GE(percent_k, 0.0) << "Stochastic %K should be >= 0 at step " << i;
            EXPECT_LE(percent_k, 100.0) << "Stochastic %K should be <= 100 at step " << i;
        }
        
        // 验证%D在0-100范围内
        if (!std::isnan(percent_d)) {
            EXPECT_GE(percent_d, 0.0) << "Stochastic %D should be >= 0 at step " << i;
            EXPECT_LE(percent_d, 100.0) << "Stochastic %D should be <= 100 at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}

// 参数化测试 - 测试不同参数的Stochastic
class StochasticParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int>> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        high_line_ = std::make_shared<backtrader::LineRoot>(csv_data_.size(), "high");
        low_line_ = std::make_shared<backtrader::LineRoot>(csv_data_.size(), "low");
        close_line_ = std::make_shared<backtrader::LineRoot>(csv_data_.size(), "close");
        
        for (const auto& bar : csv_data_) {
            high_line_->forward(bar.high);
            low_line_->forward(bar.low);
            close_line_->forward(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<backtrader::LineRoot> high_line_;
    std::shared_ptr<backtrader::LineRoot> low_line_;
    std::shared_ptr<backtrader::LineRoot> close_line_;
};

TEST_P(StochasticParameterizedTest, DifferentParameters) {
    auto [period, period_dfast] = GetParam();
    auto stochastic = std::make_shared<Stochastic>(close_line_, high_line_, low_line_, period, period_dfast);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        stochastic->calculate();
        if (i < csv_data_.size() - 1) {
            high_line_->forward();
            low_line_->forward();
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    int expected_min_period = period + period_dfast + 1;
    EXPECT_EQ(stochastic->getMinPeriod(), expected_min_period) 
        << "Stochastic minimum period should be period + period_dfast + 1";
    
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
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto stochastic = std::make_shared<Stochastic>(close_line, high_line, low_line, 14, 3);
    
    int overbought_count = 0;
    int oversold_count = 0;
    int normal_count = 0;
    
    // 统计超买超卖情况
    for (size_t i = 0; i < csv_data.size(); ++i) {
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
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
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
    auto high_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto stochastic = std::make_shared<Stochastic>(close_line, high_line, low_line, 14, 3);
    
    std::vector<double> k_values;
    std::vector<double> d_values;
    
    // 收集所有有效值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        stochastic->calculate();
        
        double k_value = stochastic->getPercentK(0);
        double d_value = stochastic->getPercentD(0);
        
        if (!std::isnan(k_value) && !std::isnan(d_value)) {
            k_values.push_back(k_value);
            d_values.push_back(d_value);
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
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
    std::vector<CSVDataReader::OHLCVData> flat_data;
    for (int i = 0; i < 30; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 100.0;
        bar.low = 100.0;
        bar.close = 100.0;
        bar.open = 100.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        flat_data.push_back(bar);
    }
    
    auto high_line = std::make_shared<backtrader::LineRoot>(flat_data.size(), "flat_high");
    auto low_line = std::make_shared<backtrader::LineRoot>(flat_data.size(), "flat_low");
    auto close_line = std::make_shared<backtrader::LineRoot>(flat_data.size(), "flat_close");
    
    for (const auto& bar : flat_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto stochastic = std::make_shared<Stochastic>(close_line, high_line, low_line, 14, 3);
    
    for (size_t i = 0; i < flat_data.size(); ++i) {
        stochastic->calculate();
        if (i < flat_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
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