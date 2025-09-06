/**
 * @file test_ind_rsi.cpp
 * @brief RSI指标测试 - 对应Python test_ind_rsi.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['57.644284', '41.630968', '53.352553'],
 * ]
 * chkmin = 15
 * chkind = btind.RSI
 */

#include "test_common.h"
#include "lineseries.h"
#include <cmath>
#include "indicators/rsi.h"


using namespace backtrader::tests::original;
using namespace backtrader;
using namespace backtrader::indicators;
using LineSeries = backtrader::LineSeries;
using LineBuffer = backtrader::LineBuffer;

namespace {

const std::vector<std::vector<std::string>> RSI_EXPECTED_VALUES = {
    {"57.644284", "41.630968", "53.352553"}
};

const int RSI_MIN_PERIOD = 15;

} // anonymous namespace

// 使用默认参数的RSI测试
DEFINE_INDICATOR_TEST(RSI_Default, RSI, RSI_EXPECTED_VALUES, RSI_MIN_PERIOD)

// 手动测试函数，用于详细验证
TEST(OriginalTests, RSI_Manual) {
    std::cout << "Starting RSI_Manual test" << std::endl;
    
    // 加载测试数据
    auto csv_data = getdata(0);
    ASSERT_FALSE(csv_data.empty());
    
    std::cout << "CSV data loaded: " << csv_data.size() << " records" << std::endl;
    
    // 使用SimpleTestDataSeries (与框架测试相同的模式)
    auto data_series = std::make_shared<SimpleTestDataSeries>(csv_data);
    
    // 创建RSI指标（默认14周期，最小周期为15）- 使用框架测试相同的模式
    std::shared_ptr<RSI> rsi;
    try {
        // First try LineSeries like framework test
        auto lineseries_ptr = std::static_pointer_cast<LineSeries>(data_series);
        rsi = std::make_shared<RSI>(lineseries_ptr, 14);
    } catch (...) {
        // Fall back to DataSeries
        auto dataseries_ptr = std::static_pointer_cast<DataSeries>(data_series);
        rsi = std::make_shared<RSI>(dataseries_ptr, 14);
    }
    
    // 设置data和datas (框架测试的关键步骤)
    rsi->data = data_series;
    rsi->datas.clear();
    rsi->datas.push_back(data_series);
    
    // Debug: Print data setup info
    std::cout << "Test setup - Data size: " << csv_data.size() << std::endl;
    std::cout << "Test setup - Data series size: " << data_series->size() << std::endl;
    
    // 计算
    rsi->calculate();
    
    // Debug: Print RSI info
    std::cout << "Data size: " << csv_data.size() << std::endl;
    std::cout << "Data series size: " << data_series->size() << std::endl;
    std::cout << "RSI line size: " << rsi->size() << std::endl;
    if (rsi->size() > 0) {
        std::cout << "Last few RSI values: ";
        for (size_t i = 0; i < std::min(size_t(5), rsi->size()); ++i) {
            int ago = -static_cast<int>(i);
            double value = rsi->get(ago);
            std::cout << "[" << i << "]=" << value << "(ago=" << ago << ") ";
        }
        std::cout << std::endl;
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 15;  // RSI的最小周期是period + 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    // Python floor division: (-240) // 2 = -120 (floor towards negative infinity)
    int middle_checkpoint = static_cast<int>(std::floor(-(data_length - min_period) / 2.0));
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        middle_checkpoint                     // 中间值 (使用floor division匹配Python)
    };
    
    std::vector<std::string> expected = {"57.644284", "41.630968", "53.352553"};
    
    // Use get() method with correct ago values
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        int python_ago = check_points[i];
        double actual = rsi->get(python_ago);
        
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        std::cout << "Check point " << i << " (ago=" << check_points[i] << "): "
                  << "expected " << expected[i] << ", got " << actual_str;
        
        // Debug: show RSI line size and values at specific indices
        if (i == 0) {
            auto rsi_line = rsi->lines->getline(0);
            std::cout << ", RSI size=" << rsi_line->size();
            // Show last few values and first few valid values
            std::cout << "\n   Last values: ";
            for (int j = rsi_line->size() - 5; j < rsi_line->size(); j++) {
                if (j >= 0) {
                    double val = (*rsi_line)[j];
                    std::cout << "[" << j << "]=" << val << " ";
                }
            }
            std::cout << "\n   First valid values: ";
            int shown = 0;
            for (int j = 0; j < std::min(20, (int)rsi_line->size()) && shown < 5; j++) {
                double val = (*rsi_line)[j];
                if (!std::isnan(val)) {
                    std::cout << "[" << j << "]=" << val << " ";
                    shown++;
                }
            }
            // Show values at the expected indices
            std::cout << "\n   Values at test indices:";
            std::cout << " [14]=" << (*rsi_line)[14];
            std::cout << " [15]=" << (*rsi_line)[15];
            std::cout << " [240]=" << (*rsi_line)[240];
            std::cout << " [254]=" << (*rsi_line)[254];
            std::cout << " [255]=" << (*rsi_line)[255] << " (should be last)";
        }
        std::cout << std::endl;
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "RSI value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(rsi->getMinPeriod(), 15) << "RSI minimum period should be 15";
}

// RSI范围验证测试
TEST(OriginalTests, RSI_RangeValidation) {
    auto csv_data = getdata(0);
    
    // 创建数据线系列
    auto close_line_series = std::make_shared<LineSeries>();
    close_line_series->lines->add_line(std::make_shared<LineBuffer>());
    close_line_series->lines->add_alias("close", 0);
    
    // 逐步添加数据到线缓冲区  
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series->lines->getline(0));
    if (close_buffer) {
        close_buffer->set(0, csv_data[0].close);
        for (size_t i = 1; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    auto rsi = std::make_shared<RSI>(close_line_series, 14);
    
    // 计算
    rsi->calculate();
    
    // 检查最终值是否在有效范围内
    double current_rsi = rsi->get(0);
    if (!std::isnan(current_rsi)) {
        EXPECT_GE(current_rsi, 0.0) << "RSI should be >= 0";
        EXPECT_LE(current_rsi, 100.0) << "RSI should be <= 100";
    }
}

// 参数化测试 - 测试不同周期的RSI
class RSIParameterizedTest : public ::testing::TestWithParam<int> {
protected:
    void SetUp() override {
        csv_data_ = getdata(0);
        ASSERT_FALSE(csv_data_.empty());
        
        // 创建数据线系列
        close_line_series_ = std::make_shared<LineSeries>();
        close_line_series_->lines->add_line(std::make_shared<LineBuffer>());
        close_line_series_->lines->add_alias("close", 0);
        
        // 逐步添加数据到线缓冲区  
        auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line_series_->lines->getline(0));
        if (close_buffer) {
            close_buffer->set(0, csv_data_[0].close);
            for (size_t i = 1; i < csv_data_.size(); ++i) {
                close_buffer->append(csv_data_[i].close);
            }
        }
    }
    
    std::vector<CSVDataReader::OHLCVData> csv_data_;
    std::shared_ptr<LineSeries> close_line_series_;
};

TEST_P(RSIParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto rsi = std::make_shared<RSI>(close_line_series_, period);
    
    // 计算
    rsi->calculate();
    
    // 验证最小周期 (period + 1)
    EXPECT_EQ(rsi->getMinPeriod(), period + 1) 
        << "RSI minimum period should be period + 1";
    
    // 验证最后的值在有效范围内
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        double last_value = rsi->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last RSI value should not be NaN";
        EXPECT_GE(last_value, 0.0) << "RSI should be >= 0";
        EXPECT_LE(last_value, 100.0) << "RSI should be <= 100";
    }
}

// 测试不同的RSI周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    RSIParameterizedTest,
    ::testing::Values(7, 14, 21, 28)
);

// 超买超卖测试
TEST(OriginalTests, RSI_OverboughtOversold) {
    auto csv_data = getdata(0);
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("close", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    if (close_buffer) {
        close_buffer->set(0, csv_data[0].close);
        for (size_t i = 1; i < csv_data.size(); ++i) {
            close_buffer->append(csv_data[i].close);
        }
    }
    
    auto rsi = std::make_shared<RSI>(close_line, 14);
    
    // 计算所有值
    rsi->calculate();
    
    bool found_overbought = false;
    bool found_oversold = false;
    
    // 检查是否有超买超卖情况
    for (size_t i = 0; i < csv_data.size(); ++i) {
        double current_rsi = rsi->get(static_cast<int>(csv_data.size() - i - 1));
        if (!std::isnan(current_rsi)) {
            if (current_rsi > 70.0) {
                found_overbought = true;
            }
            if (current_rsi < 30.0) {
                found_oversold = true;
            }
            
            // 测试RSI的超买超卖状态函数 (仅对最新值)
            if (i == 0) {
                double status = rsi->getOverboughtOversoldStatus();
                EXPECT_TRUE(status == -1.0 || status == 0.0 || status == 1.0)
                    << "Overbought/Oversold status should be -1, 0, or 1";
            }
        }
    }
    
    // 注意：不强制要求找到超买超卖，因为这取决于具体的测试数据
    std::cout << "Found overbought: " << found_overbought 
              << ", Found oversold: " << found_oversold << std::endl;
}

// 边界条件测试
TEST(OriginalTests, RSI_EdgeCases) {
    // 测试相同价格序列
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("constant", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    // 添加相同的价格
    if (close_buffer) {
        close_buffer->set(0, 100.0);
        for (int i = 1; i < 50; ++i) {
            close_buffer->append(100.0);
        }
    }
    
    auto rsi = std::make_shared<RSI>(close_line, 14);
    
    // 计算所有值
    rsi->calculate();
    
    // 当价格不变时，RSI应该是50.0或NaN
    double result = rsi->get(0);
    if (!std::isnan(result)) {
        EXPECT_NEAR(result, 50.0, 1e-6) 
            << "RSI should be 50 when prices are constant";
    }
}

// 计算验证测试 - 验证RSI计算逻辑
TEST(OriginalTests, RSI_CalculationLogic) {
    // 使用一个带有小幅波动的上升价格序列
    std::vector<double> prices = {
        100.0, 101.0, 100.5, 102.0, 103.0, 102.5, 104.0, 105.0,
        104.5, 106.0, 107.0, 106.5, 108.0, 109.0, 108.5, 110.0
    };
    
    auto close_line = std::make_shared<LineSeries>();
    close_line->lines->add_line(std::make_shared<LineBuffer>());
    close_line->lines->add_alias("ascending", 0);
    auto close_buffer = std::dynamic_pointer_cast<LineBuffer>(close_line->lines->getline(0));
    if (close_buffer) {
        close_buffer->set(0, prices[0]);
        for (size_t i = 1; i < prices.size(); ++i) {
            close_buffer->append(prices[i]);
        }
    }
    
    auto rsi = std::make_shared<RSI>(close_line, 14);
    
    // 计算所有值 (data is already pre-filled)
    rsi->calculate();
    
    // 对于持续上升的价格序列，RSI应该接近100
    double final_rsi = rsi->get(0);
    EXPECT_FALSE(std::isnan(final_rsi)) << "RSI should not be NaN";
    EXPECT_GT(final_rsi, 50.0) << "RSI should be > 50 for ascending prices";
    
    // 但不应该完全等于100（除非是极端情况）
    EXPECT_LT(final_rsi, 100.0) << "RSI should be < 100 for gradual price increase";
}
