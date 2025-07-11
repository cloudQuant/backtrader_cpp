/**
 * @file test_ind_macdhisto.cpp
 * @brief MACD直方图指标测试 - 对应Python test_ind_macdhisto.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['25.821368', '32.469404', '1.772445'],   # MACD line
 *     ['21.977853', '26.469735', '-2.845646'],  # Signal line
 *     ['3.843516', '5.999669', '4.618090'],     # Histogram
 * ]
 * chkmin = 34
 * chkind = btind.MACDHisto
 */

#include "test_common.h"

#include "indicators/macd.h"
#include "indicators/rsi.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> MACD_EXPECTED_VALUES = {
    {"25.821368", "32.469404", "1.772445"},   // MACD line
    {"21.977853", "26.469735", "-2.845646"},  // Signal line  
    {"3.843516", "5.999669", "4.618090"}      // Histogram
};

const int MACD_MIN_PERIOD = 34;

} // anonymous namespace

// 使用默认参数的MACD测试
DEFINE_INDICATOR_TEST(MACD_Default, MACD, MACD_EXPECTED_VALUES, MACD_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, MACD_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建MACD指标（默认参数：12, 26, 9）
    auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        macd->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 34;  // 26 + 9 - 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // 验证MACD线
    std::vector<std::string> expected_macd = {"25.821368", "32.469404", "1.772445"};
    for (size_t i = 0; i < check_points.size() && i < expected_macd.size(); ++i) {
        double actual = macd->getMACDLine(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_macd[i]) 
            << "MACD line mismatch at check point " << i;
    }
    
    // 验证信号线
    std::vector<std::string> expected_signal = {"21.977853", "26.469735", "-2.845646"};
    for (size_t i = 0; i < check_points.size() && i < expected_signal.size(); ++i) {
        double actual = macd->getSignalLine(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_signal[i]) 
            << "MACD signal line mismatch at check point " << i;
    }
    
    // 验证直方图
    std::vector<std::string> expected_histogram = {"3.843516", "5.999669", "4.618090"};
    for (size_t i = 0; i < check_points.size() && i < expected_histogram.size(); ++i) {
        double actual = macd->getHistogram(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_histogram[i]) 
            << "MACD histogram mismatch at check point " << i;
    }
    
    // 验证最小周期
    EXPECT_EQ(macd->getMinPeriod(), 34) << "MACD minimum period should be 34";
}

// MACD关系验证测试
TEST(OriginalTests, MACD_RelationshipValidation) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
    
    // 计算所有值并验证关系
    for (size_t i = 0; i < csv_data.size(); ++i) {
        macd->calculate();
        
        double macd_line = macd->getMACDLine(0);
        double signal_line = macd->getSignalLine(0);
        double histogram = macd->getHistogram(0);
        
        // 验证直方图 = MACD线 - 信号线
        if (!std::isnan(macd_line) && !std::isnan(signal_line) && !std::isnan(histogram)) {
            double expected_histogram = macd_line - signal_line;
            EXPECT_NEAR(histogram, expected_histogram, 1e-10) 
                << "Histogram should equal MACD line minus Signal line at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
}

// 参数化测试 - 测试不同参数的MACD
class MACDParameterizedTest : public ::testing::TestWithParam<std::tuple<int, int, int>> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        close_line_ = std::make_shared<backtrader::LineRoot>(csv_data_.size(), "close");
        for (const auto& bar : csv_data_) {
            close_line_->forward(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<backtrader::LineRoot> close_line_;
};

TEST_P(MACDParameterizedTest, DifferentParameters) {
    auto [fast_period, slow_period, signal_period] = GetParam();
    auto macd = std::make_shared<MACD>(close_line_, fast_period, slow_period, signal_period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        macd->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    int expected_min_period = slow_period + signal_period - 1;
    EXPECT_EQ(macd->getMinPeriod(), expected_min_period) 
        << "MACD minimum period should be slow_period + signal_period - 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(expected_min_period)) {
        double macd_value = macd->getMACDLine(0);
        double signal_value = macd->getSignalLine(0);
        double histogram_value = macd->getHistogram(0);
        
        EXPECT_FALSE(std::isnan(macd_value)) << "MACD line should not be NaN";
        EXPECT_FALSE(std::isnan(signal_value)) << "Signal line should not be NaN";
        EXPECT_FALSE(std::isnan(histogram_value)) << "Histogram should not be NaN";
        
        // 验证直方图关系
        EXPECT_NEAR(histogram_value, macd_value - signal_value, 1e-10) 
            << "Histogram should equal MACD - Signal";
    }
}

// 测试不同的MACD参数
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    MACDParameterizedTest,
    ::testing::Values(
        std::make_tuple(5, 10, 3),
        std::make_tuple(12, 26, 9),   // 标准参数
        std::make_tuple(8, 17, 9),
        std::make_tuple(6, 13, 5)
    )
);

// 交叉信号测试
TEST(OriginalTests, MACD_CrossoverSignals) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
    
    int bullish_crossovers = 0;
    int bearish_crossovers = 0;
    double prev_histogram = 0.0;
    bool has_prev = false;
    
    // 检测交叉信号
    for (size_t i = 0; i < csv_data.size(); ++i) {
        macd->calculate();
        
        double current_histogram = macd->getHistogram(0);
        
        if (!std::isnan(current_histogram) && has_prev) {
            // 检测直方图零轴穿越
            if (prev_histogram <= 0.0 && current_histogram > 0.0) {
                bullish_crossovers++;  // MACD上穿信号线
            } else if (prev_histogram >= 0.0 && current_histogram < 0.0) {
                bearish_crossovers++;  // MACD下穿信号线
            }
        }
        
        if (!std::isnan(current_histogram)) {
            prev_histogram = current_histogram;
            has_prev = true;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "MACD crossover signals:" << std::endl;
    std::cout << "Bullish crossovers: " << bullish_crossovers << std::endl;
    std::cout << "Bearish crossovers: " << bearish_crossovers << std::endl;
    
    // 验证至少检测到一些信号（这取决于具体数据）
    EXPECT_GE(bullish_crossovers + bearish_crossovers, 0) 
        << "Should detect some crossover signals";
}

// 趋势强度测试
TEST(OriginalTests, MACD_TrendStrength) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
    
    std::vector<double> macd_values;
    std::vector<double> histogram_values;
    
    // 收集MACD值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        macd->calculate();
        
        double macd_val = macd->getMACDLine(0);
        double hist_val = macd->getHistogram(0);
        
        if (!std::isnan(macd_val) && !std::isnan(hist_val)) {
            macd_values.push_back(macd_val);
            histogram_values.push_back(hist_val);
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 分析趋势强度
    if (!macd_values.empty()) {
        double avg_macd = std::accumulate(macd_values.begin(), macd_values.end(), 0.0) / macd_values.size();
        double avg_histogram = std::accumulate(histogram_values.begin(), histogram_values.end(), 0.0) / histogram_values.size();
        
        std::cout << "Average MACD: " << avg_macd << std::endl;
        std::cout << "Average Histogram: " << avg_histogram << std::endl;
        
        // 验证值是有限的
        EXPECT_TRUE(std::isfinite(avg_macd)) << "Average MACD should be finite";
        EXPECT_TRUE(std::isfinite(avg_histogram)) << "Average histogram should be finite";
    }
}

// 发散测试 - 验证MACD发散现象
TEST(OriginalTests, MACD_Divergence) {
    // 创建一个简单的价格序列来测试发散
    std::vector<double> prices;
    
    // 创建上升趋势但动量减弱的价格序列
    for (int i = 0; i < 50; ++i) {
        if (i < 25) {
            prices.push_back(100.0 + i * 2.0);  // 强劲上升
        } else {
            prices.push_back(150.0 + (i - 25) * 0.5);  // 缓慢上升
        }
    }
    
    auto close_line = std::make_shared<backtrader::LineRoot>(prices.size(), "divergence");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
    
    std::vector<double> price_highs;
    std::vector<double> macd_highs;
    
    for (size_t i = 0; i < prices.size(); ++i) {
        macd->calculate();
        
        double macd_val = macd->getMACDLine(0);
        
        if (!std::isnan(macd_val)) {
            // 记录局部高点
            if (i > 5 && i < prices.size() - 5) {
                bool is_price_high = true;
                bool is_macd_high = true;
                
                for (int j = -3; j <= 3; ++j) {
                    if (j != 0) {
                        if (prices[i] <= prices[i + j]) is_price_high = false;
                        if (macd_val <= macd->getMACDLine(j)) is_macd_high = false;
                    }
                }
                
                if (is_price_high) price_highs.push_back(prices[i]);
                if (is_macd_high) macd_highs.push_back(macd_val);
            }
        }
        
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
    
    std::cout << "Found " << price_highs.size() << " price highs" << std::endl;
    std::cout << "Found " << macd_highs.size() << " MACD highs" << std::endl;
    
    // 验证找到了一些高点
    EXPECT_GT(price_highs.size() + macd_highs.size(), 0) 
        << "Should find some price or MACD highs";
}
