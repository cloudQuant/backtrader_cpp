/**
 * @file test_ind_bbands.cpp
 * @brief 布林带指标测试 - 对应Python test_ind_bbands.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4065.884000', '3621.185000', '3582.895500'],  # middle (SMA)
 *     ['4190.782310', '3712.008864', '3709.453081'],  # upper band
 *     ['3940.985690', '3530.361136', '3456.337919'],  # lower band
 * ]
 * chkmin = 20
 * chkind = btind.BBands
 */

#include "test_common.h"

#include "indicators/bollinger.h"

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> BBANDS_EXPECTED_VALUES = {
    {"4065.884000", "3621.185000", "3582.895500"},  // middle band (SMA)
    {"4190.782310", "3712.008864", "3709.453081"},  // upper band
    {"3940.985690", "3530.361136", "3456.337919"}   // lower band
};

const int BBANDS_MIN_PERIOD = 20;

} // anonymous namespace

// 使用默认参数的BBands测试
DEFINE_INDICATOR_TEST(BBands_Default, BollingerBands, BBANDS_EXPECTED_VALUES, BBANDS_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, BBands_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    // 创建布林带指标（默认参数：period=20, devfactor=2.0）
    auto bbands = std::make_shared<BollingerBands>(close_line, 20, 2.0);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        bbands->calculate();
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 20;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    // 验证中轨（SMA）
    std::vector<std::string> expected_middle = {"4065.884000", "3621.185000", "3582.895500"};
    for (size_t i = 0; i < check_points.size() && i < expected_middle.size(); ++i) {
        double actual = bbands->getMiddleBand(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_middle[i]) 
            << "BBands middle band mismatch at check point " << i;
    }
    
    // 验证上轨
    std::vector<std::string> expected_upper = {"4190.782310", "3712.008864", "3709.453081"};
    for (size_t i = 0; i < check_points.size() && i < expected_upper.size(); ++i) {
        double actual = bbands->getUpperBand(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_upper[i]) 
            << "BBands upper band mismatch at check point " << i;
    }
    
    // 验证下轨
    std::vector<std::string> expected_lower = {"3940.985690", "3530.361136", "3456.337919"};
    for (size_t i = 0; i < check_points.size() && i < expected_lower.size(); ++i) {
        double actual = bbands->getLowerBand(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected_lower[i]) 
            << "BBands lower band mismatch at check point " << i;
    }
    
    // 验证最小周期
    EXPECT_EQ(bbands->getMinPeriod(), 20) << "BBands minimum period should be 20";
}

// 布林带关系验证测试
TEST(OriginalTests, BBands_BandRelationships) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto bbands = std::make_shared<BollingerBands>(close_line, 20, 2.0);
    
    // 计算所有值并验证带的关系
    for (size_t i = 0; i < csv_data.size(); ++i) {
        bbands->calculate();
        
        double upper = bbands->getUpperBand(0);
        double middle = bbands->getMiddleBand(0);
        double lower = bbands->getLowerBand(0);
        
        if (!std::isnan(upper) && !std::isnan(middle) && !std::isnan(lower)) {
            EXPECT_GT(upper, middle) 
                << "Upper band should be greater than middle band at step " << i;
            EXPECT_GT(middle, lower) 
                << "Middle band should be greater than lower band at step " << i;
            EXPECT_GT(upper, lower) 
                << "Upper band should be greater than lower band at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
}

// 布林带宽度和百分比B测试
TEST(OriginalTests, BBands_WidthAndPercentB) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto bbands = std::make_shared<BollingerBands>(close_line, 20, 2.0);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        bbands->calculate();
        
        // 测试带宽
        double bandwidth = bbands->getBandwidth();
        if (!std::isnan(bandwidth)) {
            EXPECT_GT(bandwidth, 0.0) 
                << "Bandwidth should be positive at step " << i;
        }
        
        // 测试百分比B
        double percent_b = bbands->getPercentB();
        // 百分比B可以在任何范围，但应该是有限值
        if (!std::isnan(percent_b)) {
            EXPECT_TRUE(std::isfinite(percent_b)) 
                << "Percent B should be finite at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
}

// 参数化测试 - 测试不同参数的布林带
class BollingerBandsParameterizedTest : public ::testing::TestWithParam<std::tuple<int, double>> {
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

TEST_P(BollingerBandsParameterizedTest, DifferentParameters) {
    auto [period, devfactor] = GetParam();
    auto bbands = std::make_shared<BollingerBands>(close_line_, period, devfactor);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        bbands->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(bbands->getMinPeriod(), period) 
        << "BBands minimum period should match period parameter";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double upper = bbands->getUpperBand(0);
        double middle = bbands->getMiddleBand(0);
        double lower = bbands->getLowerBand(0);
        
        EXPECT_FALSE(std::isnan(middle)) << "Middle band should not be NaN";
        EXPECT_FALSE(std::isnan(upper)) << "Upper band should not be NaN";
        EXPECT_FALSE(std::isnan(lower)) << "Lower band should not be NaN";
        
        EXPECT_GT(upper, middle) << "Upper band should be > middle band";
        EXPECT_GT(middle, lower) << "Middle band should be > lower band";
    }
}

// 测试不同的布林带参数
INSTANTIATE_TEST_SUITE_P(
    VariousParameters,
    BollingerBandsParameterizedTest,
    ::testing::Values(
        std::make_tuple(10, 1.5),
        std::make_tuple(20, 2.0),
        std::make_tuple(30, 2.5),
        std::make_tuple(50, 1.0)
    )
);

// 价格位置测试
TEST(OriginalTests, BBands_PricePosition) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<backtrader::LineRoot>(csv_data.size(), "close");
    for (const auto& bar : csv_data) {
        close_line->forward(bar.close);
    }
    
    auto bbands = std::make_shared<BollingerBands>(close_line, 20, 2.0);
    
    int inside_bands = 0;
    int above_upper = 0;
    int below_lower = 0;
    int total_valid = 0;
    
    // 统计价格相对于布林带的位置
    for (size_t i = 0; i < csv_data.size(); ++i) {
        bbands->calculate();
        
        double price = csv_data[i].close;
        double upper = bbands->getUpperBand(0);
        double middle = bbands->getMiddleBand(0);
        double lower = bbands->getLowerBand(0);
        
        if (!std::isnan(upper) && !std::isnan(middle) && !std::isnan(lower)) {
            total_valid++;
            
            if (price > upper) {
                above_upper++;
            } else if (price < lower) {
                below_lower++;
            } else {
                inside_bands++;
            }
        }
        
        if (i < csv_data.size() - 1) {
            close_line->forward();
        }
    }
    
    if (total_valid > 0) {
        double inside_ratio = static_cast<double>(inside_bands) / total_valid;
        
        // 根据布林带理论，大约95%的价格应该在2倍标准差内
        // 但实际数据可能有所不同，我们只检查大致合理性
        EXPECT_GT(inside_ratio, 0.8) 
            << "Most prices should be inside the bands. Inside ratio: " << inside_ratio;
        
        std::cout << "Price position statistics:" << std::endl;
        std::cout << "Inside bands: " << inside_bands << " (" << inside_ratio * 100 << "%)" << std::endl;
        std::cout << "Above upper: " << above_upper << std::endl;
        std::cout << "Below lower: " << below_lower << std::endl;
    }
}

// 标准差验证测试
TEST(OriginalTests, BBands_StandardDeviation) {
    // 使用一个简单的数据集来验证标准差计算
    std::vector<double> prices = {
        100.0, 101.0, 99.0, 102.0, 98.0,
        103.0, 97.0, 104.0, 96.0, 105.0
    };
    
    auto close_line = std::make_shared<backtrader::LineRoot>(prices.size(), "stdev_test");
    for (double price : prices) {
        close_line->forward(price);
    }
    
    auto bbands = std::make_shared<BollingerBands>(close_line, 10, 2.0);
    
    for (size_t i = 0; i < prices.size(); ++i) {
        bbands->calculate();
        if (i < prices.size() - 1) {
            close_line->forward();
        }
    }
    
    // 验证带的对称性
    double upper = bbands->getUpperBand(0);
    double middle = bbands->getMiddleBand(0);
    double lower = bbands->getLowerBand(0);
    
    if (!std::isnan(upper) && !std::isnan(middle) && !std::isnan(lower)) {
        double upper_distance = upper - middle;
        double lower_distance = middle - lower;
        
        // 上下带到中轨的距离应该相等（对称性）
        EXPECT_NEAR(upper_distance, lower_distance, 1e-10) 
            << "Upper and lower bands should be symmetric around middle band";
    }
}