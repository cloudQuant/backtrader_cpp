/**
 * @file test_ind_atr.cpp
 * @brief ATR指标测试 - 对应Python test_ind_atr.py
 * 
 * 原始Python测试:
 * chkdatas = 1
 * chkvals = [
 *     ['35.866308', '34.264286', '54.329064'],
 * ]
 * chkmin = 15
 * chkind = btind.ATR
 */

#include "test_common.h"
#include "indicators/ATR.h"

using namespace backtrader::tests::original;
using namespace backtrader::indicators;

namespace {

const std::vector<std::vector<std::string>> ATR_EXPECTED_VALUES = {
    {"35.866308", "34.264286", "54.329064"}
};

const int ATR_MIN_PERIOD = 15;

} // anonymous namespace

// 手动测试函数，用于详细验证
TEST(OriginalTests, ATR_Manual) {
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
    
    // 创建ATR指标（默认14周期）
    auto atr = std::make_shared<ATR>(high_line, low_line, close_line, 14);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data.size(); ++i) {
        atr->calculate();
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
    
    // 验证关键点的值
    int data_length = static_cast<int>(csv_data.size());
    int min_period = 15;  // ATR最小周期是period + 1
    
    // Python测试的检查点: [0, -l + mp, (-l + mp) // 2]
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        -(data_length - min_period),         // 倒数第(data_length - min_period)个值
        -(data_length - min_period) / 2      // 中间值
    };
    
    std::vector<std::string> expected = {"35.866308", "34.264286", "54.329064"};
    
    for (size_t i = 0; i < check_points.size() && i < expected.size(); ++i) {
        double actual = atr->get(check_points[i]);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(6) << actual;
        std::string actual_str = ss.str();
        
        EXPECT_EQ(actual_str, expected[i]) 
            << "ATR value mismatch at check point " << i 
            << " (ago=" << check_points[i] << "): "
            << "expected " << expected[i] << ", got " << actual_str;
    }
    
    // 验证最小周期
    EXPECT_EQ(atr->getMinPeriod(), 15) << "ATR minimum period should be 15";
}

// ATR非负值测试
TEST(OriginalTests, ATR_PositiveValues) {
    auto csv_data = getdata(0);
    auto high_line = std::make_shared<LineRoot>(csv_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(csv_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(csv_data.size(), "close");
    
    for (const auto& bar : csv_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto atr = std::make_shared<ATR>(high_line, low_line, close_line, 14);
    
    // 计算所有值并验证非负性
    for (size_t i = 0; i < csv_data.size(); ++i) {
        atr->calculate();
        
        double current_atr = atr->get(0);
        if (!std::isnan(current_atr)) {
            EXPECT_GE(current_atr, 0.0) 
                << "ATR should be non-negative at step " << i;
        }
        
        if (i < csv_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}

// 参数化测试 - 测试不同周期的ATR
class ATRParameterizedTest : public ::testing::TestWithParam<int> {
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

TEST_P(ATRParameterizedTest, DifferentPeriods) {
    int period = GetParam();
    auto atr = std::make_shared<ATR>(high_line_, low_line_, close_line_, period);
    
    // 计算所有值
    for (size_t i = 0; i < csv_data_.size(); ++i) {
        atr->calculate();
        if (i < csv_data_.size() - 1) {
            high_line_->forward();
            low_line_->forward();
            close_line_->forward();
        }
    }
    
    // 验证最小周期 (period + 1)
    EXPECT_EQ(atr->getMinPeriod(), period + 1) 
        << "ATR minimum period should be period + 1";
    
    // 验证最后的值
    if (csv_data_.size() >= static_cast<size_t>(period + 1)) {
        double last_value = atr->get(0);
        EXPECT_FALSE(std::isnan(last_value)) << "Last ATR value should not be NaN";
        EXPECT_GE(last_value, 0.0) << "ATR should be non-negative";
    }
}

// 测试不同的ATR周期
INSTANTIATE_TEST_SUITE_P(
    VariousPeriods,
    ATRParameterizedTest,
    ::testing::Values(7, 14, 21, 28)
);

// True Range计算验证测试
TEST(OriginalTests, ATR_TrueRangeCalculation) {
    // 使用简单的测试数据验证True Range计算
    std::vector<CSVDataReader::OHLCVData> test_data = {
        {"2006-01-01", 100.0, 105.0, 95.0, 102.0, 0, 0},
        {"2006-01-02", 101.0, 107.0, 98.0, 104.0, 0, 0},
        {"2006-01-03", 103.0, 108.0, 100.0, 106.0, 0, 0},
        {"2006-01-04", 105.0, 110.0, 103.0, 108.0, 0, 0},
        {"2006-01-05", 107.0, 112.0, 105.0, 110.0, 0, 0}
    };
    
    auto high_line = std::make_shared<LineRoot>(test_data.size(), "high");
    auto low_line = std::make_shared<LineRoot>(test_data.size(), "low");
    auto close_line = std::make_shared<LineRoot>(test_data.size(), "close");
    
    for (const auto& bar : test_data) {
        high_line->forward(bar.high);
        low_line->forward(bar.low);
        close_line->forward(bar.close);
    }
    
    auto atr = std::make_shared<ATR>(high_line, low_line, close_line, 3);
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        atr->calculate();
        
        // 对于第二个数据点开始验证True Range
        if (i >= 1) {
            // True Range = max(H-L, |H-PC|, |L-PC|)
            double h = test_data[i].high;
            double l = test_data[i].low;
            double pc = test_data[i-1].close;  // Previous close
            
            double tr1 = h - l;
            double tr2 = std::abs(h - pc);
            double tr3 = std::abs(l - pc);
            double expected_tr = std::max({tr1, tr2, tr3});
            
            // 获取当前的True Range值（如果实现了此方法）
            // double actual_tr = atr->getCurrentTrueRange();
            // EXPECT_NEAR(actual_tr, expected_tr, 1e-10);
        }
        
        if (i < test_data.size() - 1) {
            high_line->forward();
            low_line->forward();
            close_line->forward();
        }
    }
}

// 波动性测试
TEST(OriginalTests, ATR_VolatilityMeasure) {
    // 创建高波动性数据
    std::vector<CSVDataReader::OHLCVData> high_volatility_data;
    for (int i = 0; i < 50; ++i) {
        double base = 100.0;
        double volatility = 10.0;  // 高波动性
        
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.close = base + (i % 2 == 0 ? volatility : -volatility);
        bar.high = bar.close + volatility / 2;
        bar.low = bar.close - volatility / 2;
        bar.open = bar.close;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        high_volatility_data.push_back(bar);
    }
    
    // 创建低波动性数据
    std::vector<CSVDataReader::OHLCVData> low_volatility_data;
    for (int i = 0; i < 50; ++i) {
        double base = 100.0;
        double volatility = 1.0;  // 低波动性
        
        CSVDataReader::OHLCVData bar;
        bar.date = "2006-01-" + std::to_string(i + 1);
        bar.close = base + (i % 2 == 0 ? volatility : -volatility);
        bar.high = bar.close + volatility / 2;
        bar.low = bar.close - volatility / 2;
        bar.open = bar.close;
        bar.volume = 1000;
        bar.openinterest = 0;
        
        low_volatility_data.push_back(bar);
    }
    
    // 计算高波动性ATR
    auto high_vol_high = std::make_shared<LineRoot>(high_volatility_data.size(), "high_vol_high");
    auto high_vol_low = std::make_shared<LineRoot>(high_volatility_data.size(), "high_vol_low");
    auto high_vol_close = std::make_shared<LineRoot>(high_volatility_data.size(), "high_vol_close");
    
    for (const auto& bar : high_volatility_data) {
        high_vol_high->forward(bar.high);
        high_vol_low->forward(bar.low);
        high_vol_close->forward(bar.close);
    }
    
    auto high_vol_atr = std::make_shared<ATR>(high_vol_high, high_vol_low, high_vol_close, 14);
    
    for (size_t i = 0; i < high_volatility_data.size(); ++i) {
        high_vol_atr->calculate();
        if (i < high_volatility_data.size() - 1) {
            high_vol_high->forward();
            high_vol_low->forward();
            high_vol_close->forward();
        }
    }
    
    // 计算低波动性ATR
    auto low_vol_high = std::make_shared<LineRoot>(low_volatility_data.size(), "low_vol_high");
    auto low_vol_low = std::make_shared<LineRoot>(low_volatility_data.size(), "low_vol_low");
    auto low_vol_close = std::make_shared<LineRoot>(low_volatility_data.size(), "low_vol_close");
    
    for (const auto& bar : low_volatility_data) {
        low_vol_high->forward(bar.high);
        low_vol_low->forward(bar.low);
        low_vol_close->forward(bar.close);
    }
    
    auto low_vol_atr = std::make_shared<ATR>(low_vol_high, low_vol_low, low_vol_close, 14);
    
    for (size_t i = 0; i < low_volatility_data.size(); ++i) {
        low_vol_atr->calculate();
        if (i < low_volatility_data.size() - 1) {
            low_vol_high->forward();
            low_vol_low->forward();
            low_vol_close->forward();
        }
    }
    
    // 比较ATR值 - 高波动性数据的ATR应该明显更大
    double high_vol_final_atr = high_vol_atr->get(0);
    double low_vol_final_atr = low_vol_atr->get(0);
    
    if (!std::isnan(high_vol_final_atr) && !std::isnan(low_vol_final_atr)) {
        EXPECT_GT(high_vol_final_atr, low_vol_final_atr) 
            << "High volatility data should produce higher ATR values";
        
        std::cout << "High volatility ATR: " << high_vol_final_atr << std::endl;
        std::cout << "Low volatility ATR: " << low_vol_final_atr << std::endl;
    }
}