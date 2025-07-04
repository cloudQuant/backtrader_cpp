/**
 * @file test_ind_williamsr.cpp
 * @brief Williams %R指标测试 - 对应Python test_ind_williamsr.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['-16.458733', '-68.298609', '-28.602854'],
 * ]
 * chkmin = 14
 * chkind = btind.WilliamsR
 */

#include "test_common_simple.h"

#include "indicators/williamsr.h"


using namespace backtrader::tests::original;

namespace {

const std::vector<std::vector<std::string>> WILLIAMSR_EXPECTED_VALUES = {
    {"-16.458733", "-68.298609", "-28.602854"}
};

const int WILLIAMSR_MIN_PERIOD = 14;

} // anonymous namespace

// 使用默认参数的Williams %R测试
DEFINE_INDICATOR_TEST(WilliamsR_Default, WilliamsR, WILLIAMSR_EXPECTED_VALUES, WILLIAMSR_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, WilliamsR_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    // 创建Williams %R指标（默认14周期）
    auto williamsr = std::make_shared<WilliamsR>(close_line, high_line, low_line, 14);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        williamsr->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 14;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"-16.458733", "-68.298609", "-28.602854"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = williamsr->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "Williams %R value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(williamsr->getMinPeriod(), 14) << "Williams %R minimum period should be 14";
}

// Williams %R范围验证测试
TEST(OriginalTests, WilliamsR_RangeValidation) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto williamsr = std::make_shared<WilliamsR>(close_line, high_line, low_line, 14);
    
    // 计算所有值并验证范围
    for (size_t i = 0; i < csv_data.size(); ++i) {
        williamsr->calculate();
        
        double wr_value = williamsr->get(0);
        
        // 验证Williams %R在-100到0范围内
        if (!std::isnan(wr_value)) {
            EXPECT_GE(wr_value, -100.0) << "Williams %R should be >= -100 at step " << i;
            EXPECT_LE(wr_value, 0.0) << "Williams %R should be <= 0 at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}

// 参数化测试 - 测试不同周期的Williams %R
class WilliamsRParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        high_line_ = std::make_shared<LineRoot>(csv_data_.size(), "high");
        low_line_ = std::make_shared<LineRoot>(csv_data_.size(), "low");
        close_line_ = std::make_shared<LineRoot>(csv_data_.size(), "close");
        
        for (const auto& bar : csv_data_) {
            high_line_->forward(bar.high);
            low_line_->forward(bar.low);
            close_line_->forward(bar.close);
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineRoot> high_line_;
    std::shared_ptr<LineRoot> low_line_;
    std::shared_ptr<LineRoot> close_line_;
};

TEST_P(WilliamsRParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto williamsr = std::make_shared<WilliamsR>(close_line_, high_line_, low_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        williamsr->calculate();
        if (i < csv_data_.size() - 1) {
            high_line_->forward();
            low_line_->forward();
            close_line_->forward();
        }
    }
    
    // 验证最小周期
    EXPECT_EQ(williamsr->getMinPeriod(), period) 
        << "Williams %R minimum period should match parameter";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_value = williamsr->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last Williams %R value should not be NaN";
        EXPECT_GE(last_value, -100.0) << "Williams %R should be >= -100";
        EXPECT_LE(last_value, 0.0) << "Williams %R should be <= 0";
    }
}

// 测试不同的Williams %R周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    WilliamsRParameterizedTest,
    ::testing::Values(7, 14, 21, 28)
);

// 超买超卖测试
TEST(OriginalTests, WilliamsR_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto williamsr = std::make_shared<WilliamsR>(close_line, high_line, low_line, 14);
    
    int overbought_count = 0;  // %R > -20
    int oversold_count = 0;    // %R < -80
    int normal_count = 0;
    
    // 统计超买超卖情况
    for (size_t i = 0; i < csv_data.size(); ++i) {
        williamsr->calculate();
        
        double wr_value = williamsr->get(0);
        
        if (!std::isnan(wr_value)) {
            if (wr_value > -20.0) {
                overbought_count++;
            } else if (wr_value < -80.0) {
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
    
    std::cout << "Williams %R statistics:" << std::endl;
    std::cout << "Overbought periods (> -20): " << overbought_count << std::endl;
    std::cout << "Oversold periods (< -80): " << oversold_count << std::endl;
    std::cout << "Normal periods: " << normal_count << std::endl;
    
    // 验证至少有一些有效的计算
    EXPECT_GT(normal_count + overbought_count + oversold_count, 0) 
        << "Should have some valid Williams %R calculations";
}

// 计算逻辑验证测试
TEST(OriginalTests, WilliamsR_CalculationLogic) {
    // 使用简单的测试数据验证Williams %R计算
    std::vector<CSVDataReader::OHLCVData> test_data = {
        {"2006-01-01", 100.0, 110.0, 90.0, 105.0, 0, 0},
        {"2006-01-02", 105.0, 115.0, 95.0, 110.0, 0, 0},
        {"2006-01-03", 110.0, 120.0, 100.0, 115.0, 0, 0},
        {"2006-01-04", 115.0, 125.0, 105.0, 120.0, 0, 0},
        {"2006-01-05", 120.0, 130.0, 110.0, 125.0, 0, 0}
    };
    
    auto high_line = std::make_shared<LineRoot>(test_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(test_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(test_data.size(), "close");
    
    for (const auto& bar : test_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto williamsr = std::make_shared<WilliamsR>(close_line, high_line, low_line, 3);
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        williamsr->calculate();
        
        // 手动计算Williams %R进行验证
        if (i >= 2) {  // 需要至少3个数据点
            double highest_high = std::max({test_data[i].high, test_data[i-1].high, test_data[i-2].high});
            double lowest_low = std::min({test_data[i].low, test_data[i-1].low, test_data[i-2].low});
            double current_close = test_data[i].close;
            
            double expected_wr = ((highest_high - current_close) / (highest_high - lowest_low)) * -100.0;
            double actual_wr = williamsr->get(0);
            
            if (!std::isnan(actual_wr)) {
                EXPECT_NEAR(actual_wr, expected_wr, 1e-10) 
                    << "Williams %R calculation mismatch at step " << i;
            }
        }
        
        if (i < test_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}

// 边界条件测试
TEST(OriginalTests, WilliamsR_EdgeCases) {
    // 测试价格在区间顶部的情况（Williams %R应该接近0）
    std::vector<CSVDataReader::OHLCVData> top_data;
    for (int i = 0; i < 20; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 120.0;
        bar.low = 100.0;
        bar.close = 119.0;  // 接近最高价
        bar.open = 110.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        top_data.push_back(bar);
    }
    
    auto high_line = std::make_shared<LineRoot>(top_data.size(), "top_high");
    auto low_line = std::make_shared<LineRoot>(top_data.size(), "top_low");
    auto close_line = std::make_shared<LineRoot>(top_data.size(), "top_close");
    
    for (const auto& bar : top_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto williamsr_top = std::make_shared<WilliamsR>(close_line, high_line, low_line, 14);
    
    for (size_t i = 0; i < top_data.size(); ++i) {
        williamsr_top->calculate();
        if (i < top_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    double final_wr = williamsr_top->get(0);
    if (!std::isnan(final_wr)) {
        EXPECT_GT(final_wr, -10.0) << "Williams %R should be close to 0 when price is near high";
    }
    
    // 测试价格在区间底部的情况（Williams %R应该接近-100）
    std::vector<CSVDataReader::OHLCVData> bottom_data;
    for (int i = 0; i < 20; ++i) {
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.high = 120.0;
        bar.low = 100.0;
        bar.close = 101.0;  // 接近最低价
        bar.open = 110.0;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        bottom_data.push_back(bar);
    }
    
    auto high_line_bot = std::make_shared<LineRoot>(bottom_data.size(), "bot_high");
    auto low_line_bot = std::make_shared<LineRoot>(bottom_data.size(), "bot_low");
    auto close_line_bot = std::make_shared<LineRoot>(bottom_data.size(), "bot_close");
    
    for (const auto& bar : bottom_data) {
        high_line_bot->forward(bar.high);
        low_line_bot->forward(bar.low);
        close_line_bot->forward(bar.close);
    }
    
    auto williamsr_bot = std::make_shared<WilliamsR>(close_line_bot, high_line_bot, low_line_bot, 14);
    
    for (size_t i = 0; i < bottom_data.size(); ++i) {
        williamsr_bot->calculate();
        if (i < bottom_data.size() - 1) {
            high_line_bot->forward();
            low_line_bot->forward();
            close_line_bot->forward();
        }
    }
    
    double final_wr_bot = williamsr_bot->get(0);
    if (!std::isnan(final_wr_bot)) {
        EXPECT_LT(final_wr_bot, -90.0) << "Williams %R should be close to -100 when price is near low";
    }
}

// 与Stochastic的关系测试
TEST(OriginalTests, WilliamsR_vs_Stochastic) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto williamsr = std::make_shared<WilliamsR>(close_line, high_line, low_line, 14);
    auto stochastic = std::make_shared<Stochastic>(close_line, high_line, low_line, 14, 1);  // %K only
    
    for (size_t i = 0; i < csv_data.size(); ++i) {
        williamsr->calculate();
        stochastic->calculate();
        
        double wr_value = williamsr->get(0);
        double stoch_k = stochastic->getPercentK(0);
        
        // Williams %R = Stochastic %K - 100
        if (!std::isnan(wr_value) && !std::isnan(stoch_k)) {
            double expected_wr = stoch_k - 100.0;
            EXPECT_NEAR(wr_value, expected_wr, 1e-10) 
                << "Williams %R should equal Stochastic %K minus 100 at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}