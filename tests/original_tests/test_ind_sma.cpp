/**
 * @file test_ind_sma.cpp
 * @brief SMA指标测试 - 对应Python test_ind_sma.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['4063.463000', '3644.444667', '3554.693333'],
 * ]
 * chkmin = 30
 * chkind = btind.SMA
 */

#include "test_common_simple.h"
#include "indicators/SMA.h"

using namespace backtrader::tests::original;
using namespace backtrader;

namespace {

const std::vector<std::vector<std::string>> SMA_EXPECTED_VALUES = {
    {"4063.463000", "3644.444667", "3554.693333"}
};

const int SMA_MIN_PERIOD = 30;

} // anonymous namespace

// 使用默认参数的SMA测试
DEFINE_INDICATOR_TEST(SMA_Default, SMA, SMA_EXPECTED_VALUES, SMA_MIN_PERIOD)

// 手动测试函数，用于调试
TEST(OriginalTests, SMA_Manual) {
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    // 创建数据线
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    // 创建SMA指标（30周期）
    auto sma = std::make_shared<SMA>(close_line, 30);
    
    // 逐步添加数据并计算
    for (size_t i = 0; i < csv_data.size(); ++i) {
        close_line->forward(csv_data[i].close);
        sma->calculate();
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 30;
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    // Note: Python floor division (//) for negative numbers differs from C++ /
    // Python: (-225) // 2 = -113, C++: (-225) / 2 = -112.5 -> -112
    int middle_checkpoint = static_cast<int>(std::floor(static_cast<double>(-(data_length - min_period)) / 2.0));
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        middle_checkpoint                     // 中间值 (使用floor division匹配Python)
    };
    
    std::vector<std::string> expected = {"4063.463000", "3644.444667", "3554.693333"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = sma->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "SMA value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(sma->getMinPeriod(), 30) << "SMA minimum period should be 30";
}

// 参数化测试 - 测试不同周期的SMA
class SMAParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(SMAParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto sma = std::make_shared<SMA>(close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        sma->calculate();
        if (i < csv_data_.size() - 1) {
            close_line_->forward();
        }
    }
    
    // 验证基本属性
    EXPECT_EQ(sma->getMinPeriod(), period) << "SMA minimum period should match parameter";
    
    // 在有足够数据的情况下，验证最后的值不是NaN
    if (csv_data_.size() >= static_cast<size_t>(period)) {
        double last_value = sma->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last SMA value should not be NaN";
        EXPECT_GT(last_value, 0) << "SMA value should be positive for this test data";
    }
}

// 测试不同的SMA周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    SMAParameterizedTest,
    ::testing::Values(5, 10, 20, 30, 50, 100)
);

// 边界条件测试
TEST(OriginalTests, SMA_EdgeCases) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    // 只添加几个数据点
    for (size_t i = 0; i < std::min(size_t(5), csv_data.size()); ++i) {
        close_line->forward(csv_data[i].close);
    }
    
    // 测试周期大于数据量的情况
    auto sma = std::make_shared<SMA>(close_line, 10);
    
    for (int i = 0; i < 5; ++i) {
        sma->calculate();
        if (i < 4) {
            close_line->forward();
        }
    }
    
    // 数据不足时应该返回NaN
    double result = sma->get(0);
    EXPECT_TRUE(std::isnan(result)) << "SMA should return NaN when insufficient data";
}

// 精度测试 - 验证与手动计算的一致性
TEST(OriginalTests, SMA_PrecisionTest) {
    // 使用固定的小数据集进行精度验证
    std::vector<double> test_prices = {
        3578.73, 3604.33, 3544.31, 3526.75, 3571.43,
        3610.23, 3633.44, 3669.98, 3687.24, 3704.55
    };
    
    auto close_line = std::make_shared<LineRoot>(test_prices.size(), "precision_test");
    for (double price : test_prices) {
        close_line->forward(price);
    }
    
    auto sma5 = std::make_shared<SMA>(close_line, 5);
    
    // 计算到第5个数据点
    for (size_t i = 0; i < test_prices.size(); ++i) {
        sma5->calculate();
        if (i < test_prices.size() - 1) {
            close_line->forward();
        }
    }
    
    // 手动计算最后5个值的平均值
    double expected = 0.0;
    for (size_t i = test_prices.size() - 5; i < test_prices.size(); ++i) {
        expected += test_prices[i];
    }
    expected /= 5.0;
    
    double actual = sma5->get(0);
    EXPECT_NEAR(actual, expected, 1e-10) 
        << "SMA calculation should match manual calculation";
}