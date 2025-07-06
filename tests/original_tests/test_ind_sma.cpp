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

#include "test_common.h"
#include "indicators/sma.h"

using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;

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
    
    // 创建数据线系列
    auto close_line_series = std::make_shared<LineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    close_line_series->lines->add_alias("close", 0);
    
    // 逐步添加数据到线缓冲区
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    if (close_buffer) {
        for (size_t i = 0; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    // 创建SMA指标（30周期）
    auto sma = std::make_shared<SMA>(close_line_series, 30);
    
    // 计算
    sma->calculate();
    
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
        
        close_line_series_ = std::make_shared<LineSeries>();
        close_line_series_->lines->add_line(std::make_shared<LineBuffer>());
        close_line_series_->lines->add_alias("close", 0);
        
        auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series_->lines->getline(0));
        if (close_buffer) {
            for (size_t i = 0; i < csv_data_.size(); ++i) {
                close_buffer->append(csv_data_[i].close);
            }
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineSeries> close_line_series_;
};

TEST_P(SMAParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto sma = std::make_shared<SMA>(close_line_series_, period);
    
    // 计算所有值
    sma->calculate();
    
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
    
    // 创建数据线系列，只使用前5个数据点
    auto close_line_series = std::make_shared<LineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    close_line_series->lines->add_alias("close", 0);
    
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    size_t data_count = std::min(size_t(5), csv_data.size());
    if (close_buffer) {
        for (size_t i = 0; i < data_count; ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    // 测试周期大于数据量的情况
    auto sma = std::make_shared<SMA>(close_line_series, 10);
    
    sma->calculate();
    
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
    
    auto close_line_series = std::make_shared<LineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    close_line_series->lines->add_alias("close", 0);
    
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    if (close_buffer) {
        for (size_t i = 0; i < test_prices.size(); ++i) {
            close_buffer->append(test_prices[i]);
        }
    }
    
    auto sma5 = std::make_shared<SMA>(close_line_series, 5);
    
    // 计算到所有数据点
    sma5->calculate();
    
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