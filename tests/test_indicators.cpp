#include "test_framework.h"
#include "indicators/SMA.h"
#include "indicators/RSI.h"
#include "indicators/MACD.h"
#include "indicators/BollingerBands.h"
#include "indicators/Stochastic.h"
#include "indicators/ATR.h"
#include "indicators/Ichimoku.h"
#include "indicators/CCI.h"
#include "core/LineRoot.h"

using namespace backtrader;
using namespace backtrader::testing;

/**
 * @brief SMA指标测试类
 */
class SMATest : public BacktraderTestBase {
protected:
    void SetUp() override {
        BacktraderTestBase::SetUp();
        
        // 创建测试数据
        test_data_ = {10.0, 12.0, 13.0, 15.0, 16.0, 14.0, 13.0, 12.0, 11.0, 10.0};
        
        // 创建数据线
        data_line_ = std::make_shared<LineRoot>(1000, "test_data");
        for (double value : test_data_) {
            data_line_->forward(value);
        }
        
        // 创建SMA指标
        sma_ = std::make_shared<SMA>(data_line_, 5);
    }
    
    std::vector<double> test_data_;
    std::shared_ptr<LineRoot> data_line_;
    std::shared_ptr<SMA> sma_;
};

TEST_F(SMATest, BasicCalculation) {
    // 计算SMA值
    for (size_t i = 0; i < test_data_.size(); ++i) {
        sma_->calculate();
        if (i < test_data_.size() - 1) {
            data_line_->forward();
        }
    }
    
    // 验证最后5个值的平均值
    double expected = (16.0 + 14.0 + 13.0 + 12.0 + 11.0) / 5.0;
    double actual = sma_->get(0);
    
    EXPECT_DOUBLE_NEAR(expected, actual, 1e-10);
}

TEST_F(SMATest, InsufficientData) {
    // 测试数据不足时的行为
    auto small_data = std::make_shared<LineRoot>(1000, "small_data");
    small_data->forward(10.0);
    small_data->forward(12.0);
    
    auto small_sma = std::make_shared<SMA>(small_data, 5);
    small_sma->calculate();
    
    // 数据不足时应该返回NaN
    EXPECT_TRUE(std::isnan(small_sma->get(0)));
}

TEST_F(SMATest, EdgeCases) {
    // 测试边界条件
    auto edge_data = std::make_shared<LineRoot>(1000, "edge_data");
    
    // 测试相同值
    for (int i = 0; i < 10; ++i) {
        edge_data->forward(100.0);
    }
    
    auto edge_sma = std::make_shared<SMA>(edge_data, 5);
    
    for (int i = 0; i < 6; ++i) {
        edge_sma->calculate();
        if (i < 5) edge_data->forward();
    }
    
    EXPECT_DOUBLE_NEAR(100.0, edge_sma->get(0), 1e-10);
}

TEST_F(SMATest, Performance) {
    // 性能测试
    auto large_data = TestDataGenerator::generateRandomOHLCV(10000);
    auto perf_data = std::make_shared<LineRoot>(large_data[3].size(), "perf_data");
    
    for (double value : large_data[3]) {
        perf_data->forward(value);
    }
    
    auto perf_sma = std::make_shared<SMA>(perf_data, 20);
    
    BENCHMARK_START();
    
    for (size_t i = 0; i < large_data[3].size(); ++i) {
        perf_sma->calculate();
        if (i < large_data[3].size() - 1) {
            perf_data->forward();
        }
    }
    
    BENCHMARK_END(100.0); // 应该在100ms内完成
}

/**
 * @brief RSI指标测试类
 */
class RSITest : public BacktraderTestBase {
protected:
    void SetUp() override {
        BacktraderTestBase::SetUp();
        
        // 创建波动测试数据
        test_data_ = {44.0, 44.25, 44.5, 43.75, 44.5, 44.0, 44.25, 44.75, 44.5, 44.0,
                     43.5, 44.0, 44.25, 45.0, 45.5, 45.25, 46.0, 46.5, 46.25, 47.0};
        
        data_line_ = std::make_shared<LineRoot>(1000, "rsi_data");
        for (double value : test_data_) {
            data_line_->forward(value);
        }
        
        rsi_ = std::make_shared<RSI>(data_line_, 14);
    }
    
    std::vector<double> test_data_;
    std::shared_ptr<LineRoot> data_line_;
    std::shared_ptr<RSI> rsi_;
};

TEST_F(RSITest, RSIRange) {
    // 计算RSI
    for (size_t i = 0; i < test_data_.size(); ++i) {
        rsi_->calculate();
        if (i < test_data_.size() - 1) {
            data_line_->forward();
        }
    }
    
    double rsi_value = rsi_->get(0);
    
    // RSI应该在0-100范围内
    EXPECT_GE(rsi_value, 0.0);
    EXPECT_LE(rsi_value, 100.0);
    EXPECT_NO_NAN(rsi_value);
}

TEST_F(RSITest, OverboughtOversold) {
    // 测试超买超卖检测
    for (size_t i = 0; i < test_data_.size(); ++i) {
        rsi_->calculate();
        if (i < test_data_.size() - 1) {
            data_line_->forward();
        }
    }
    
    double overbought_status = rsi_->getOverboughtOversoldStatus();
    
    // 应该返回-1, 0, 或1
    EXPECT_TRUE(overbought_status == -1.0 || overbought_status == 0.0 || overbought_status == 1.0);
}

/**
 * @brief MACD指标测试类
 */
class MACDTest : public BacktraderTestBase {
protected:
    void SetUp() override {
        BacktraderTestBase::SetUp();
        
        auto trend_data = TestDataGenerator::generateTrendOHLCV(100, 0.5, 100.0, 0.02);
        
        data_line_ = std::make_shared<LineRoot>(1000, "macd_data");
        for (double value : trend_data[3]) {
            data_line_->forward(value);
        }
        
        macd_ = std::make_shared<MACD>(data_line_, 12, 26, 9);
    }
    
    std::shared_ptr<LineRoot> data_line_;
    std::shared_ptr<MACD> macd_;
};

TEST_F(MACDTest, MultiLineOutput) {
    // 计算MACD
    for (int i = 0; i < 50; ++i) {
        macd_->calculate();
        if (i < 49) {
            data_line_->forward();
        }
    }
    
    // 验证MACD线
    double macd_line = macd_->getMACDLine(0);
    double signal_line = macd_->getSignalLine(0);
    double histogram = macd_->getHistogram(0);
    
    EXPECT_NO_NAN(macd_line);
    EXPECT_NO_NAN(signal_line);
    EXPECT_NO_NAN(histogram);
    
    // 验证直方图计算
    EXPECT_DOUBLE_NEAR(histogram, macd_line - signal_line, 1e-10);
}

TEST_F(MACDTest, CrossoverSignals) {
    // 计算足够的点以获得交叉信号
    for (int i = 0; i < 50; ++i) {
        macd_->calculate();
        if (i < 49) {
            data_line_->forward();
        }
    }
    
    double crossover = macd_->getCrossoverSignal();
    
    // 交叉信号应该是-1, 0, 或1
    EXPECT_TRUE(crossover == -1.0 || crossover == 0.0 || crossover == 1.0);
}

/**
 * @brief 布林带测试类
 */
class BollingerBandsTest : public BacktraderTestBase {
protected:
    void SetUp() override {
        BacktraderTestBase::SetUp();
        
        auto sine_data = TestDataGenerator::generateSineWaveOHLCV(50, 10.0, 0.2, 100.0, 0.01);
        
        data_line_ = std::make_shared<LineRoot>(1000, "bb_data");
        for (double value : sine_data[3]) {
            data_line_->forward(value);
        }
        
        bb_ = std::make_shared<BollingerBands>(data_line_, 20, 2.0);
    }
    
    std::shared_ptr<LineRoot> data_line_;
    std::shared_ptr<BollingerBands> bb_;
};

TEST_F(BollingerBandsTest, BandRelationships) {
    // 计算布林带
    for (int i = 0; i < 30; ++i) {
        bb_->calculate();
        if (i < 29) {
            data_line_->forward();
        }
    }
    
    double upper = bb_->getUpperBand(0);
    double middle = bb_->getMiddleBand(0);
    double lower = bb_->getLowerBand(0);
    double current_price = data_line_->get(0);
    
    // 验证带的关系
    EXPECT_GT(upper, middle);
    EXPECT_GT(middle, lower);
    EXPECT_NO_NAN(upper);
    EXPECT_NO_NAN(middle);
    EXPECT_NO_NAN(lower);
}

TEST_F(BollingerBandsTest, BandWidthCalculation) {
    for (int i = 0; i < 30; ++i) {
        bb_->calculate();
        if (i < 29) {
            data_line_->forward();
        }
    }
    
    double bandwidth = bb_->getBandwidth();
    double percent_b = bb_->getPercentB();
    
    EXPECT_GT(bandwidth, 0.0);
    EXPECT_NO_NAN(bandwidth);
    EXPECT_NO_NAN(percent_b);
}

/**
 * @brief Ichimoku测试类
 */
class IchimokuTest : public BacktraderTestBase {
protected:
    void SetUp() override {
        BacktraderTestBase::SetUp();
        
        auto trend_data = TestDataGenerator::generateTrendOHLCV(100, 0.3, 100.0, 0.02);
        
        close_line_ = std::make_shared<LineRoot>(1000, "close");
        high_line_ = std::make_shared<LineRoot>(1000, "high");
        low_line_ = std::make_shared<LineRoot>(1000, "low");
        
        for (size_t i = 0; i < trend_data[0].size(); ++i) {
            high_line_->forward(trend_data[1][i]);
            low_line_->forward(trend_data[2][i]);
            close_line_->forward(trend_data[3][i]);
        }
        
        ichimoku_ = std::make_shared<Ichimoku>(close_line_, high_line_, low_line_, 9, 26, 52, 26);
    }
    
    std::shared_ptr<LineRoot> close_line_;
    std::shared_ptr<LineRoot> high_line_;
    std::shared_ptr<LineRoot> low_line_;
    std::shared_ptr<Ichimoku> ichimoku_;
};

TEST_F(IchimokuTest, FiveLineCalculation) {
    // 计算Ichimoku
    for (int i = 0; i < 60; ++i) {
        ichimoku_->calculate();
        if (i < 59) {
            close_line_->forward();
            high_line_->forward();
            low_line_->forward();
        }
    }
    
    double tenkan = ichimoku_->getTenkanSen(0);
    double kijun = ichimoku_->getKijunSen(0);
    double senkou_a = ichimoku_->getSenkouSpanA(0);
    double senkou_b = ichimoku_->getSenkouSpanB(0);
    double chikou = ichimoku_->getChikouSpan(0);
    
    // 验证所有线都有值
    EXPECT_NO_NAN(tenkan);
    EXPECT_NO_NAN(kijun);
    EXPECT_NO_NAN(chikou);
    
    // 先行线可能为NaN（因为位移）
    // EXPECT_NO_NAN(senkou_a);
    // EXPECT_NO_NAN(senkou_b);
}

TEST_F(IchimokuTest, CloudAnalysis) {
    for (int i = 0; i < 60; ++i) {
        ichimoku_->calculate();
        if (i < 59) {
            close_line_->forward();
            high_line_->forward();
            low_line_->forward();
        }
    }
    
    double cloud_direction = ichimoku_->getCloudDirection(0);
    double cloud_thickness = ichimoku_->getCloudThickness(0);
    
    // 云图方向应该是-1, 0, 或1
    EXPECT_TRUE(cloud_direction == -1.0 || cloud_direction == 0.0 || cloud_direction == 1.0);
    
    // 云图厚度应该是非负数
    if (!std::isnan(cloud_thickness)) {
        EXPECT_GE(cloud_thickness, 0.0);
    }
}

/**
 * @brief CCI测试类
 */
class CCITest : public BacktraderTestBase {
protected:
    void SetUp() override {
        BacktraderTestBase::SetUp();
        
        auto sine_data = TestDataGenerator::generateSineWaveOHLCV(60, 15.0, 0.15, 100.0, 0.02);
        
        high_line_ = std::make_shared<LineRoot>(1000, "high");
        low_line_ = std::make_shared<LineRoot>(1000, "low");
        close_line_ = std::make_shared<LineRoot>(1000, "close");
        
        for (size_t i = 0; i < sine_data[0].size(); ++i) {
            high_line_->forward(sine_data[1][i]);
            low_line_->forward(sine_data[2][i]);
            close_line_->forward(sine_data[3][i]);
        }
        
        cci_ = std::make_shared<CCI>(high_line_, low_line_, close_line_, 20);
    }
    
    std::shared_ptr<LineRoot> high_line_;
    std::shared_ptr<LineRoot> low_line_;
    std::shared_ptr<LineRoot> close_line_;
    std::shared_ptr<CCI> cci_;
};

TEST_F(CCITest, CCICalculation) {
    // 计算CCI
    for (int i = 0; i < 30; ++i) {
        cci_->calculate();
        if (i < 29) {
            high_line_->forward();
            low_line_->forward();
            close_line_->forward();
        }
    }
    
    double cci_value = cci_->get(0);
    double typical_price = cci_->getCurrentTypicalPrice();
    
    EXPECT_NO_NAN(cci_value);
    EXPECT_NO_NAN(typical_price);
    EXPECT_GT(typical_price, 0.0);
}

TEST_F(CCITest, OverboughtOversoldDetection) {
    for (int i = 0; i < 30; ++i) {
        cci_->calculate();
        if (i < 29) {
            high_line_->forward();
            low_line_->forward();
            close_line_->forward();
        }
    }
    
    double overbought_status = cci_->getOverboughtOversoldStatus();
    double cci_strength = cci_->getCCIStrength();
    
    // 状态应该是-1, 0, 或1
    EXPECT_TRUE(overbought_status == -1.0 || overbought_status == 0.0 || overbought_status == 1.0);
    
    // 强度应该在0-100之间
    EXPECT_GE(cci_strength, 0.0);
    EXPECT_LE(cci_strength, 100.0);
}

/**
 * @brief 压力测试类
 */
class IndicatorStressTest : public BacktraderTestBase {
public:
    void TestIndicatorWithLargeDataset(const std::string& indicator_name) {
        auto large_data = TestDataGenerator::generateRandomOHLCV(50000, 100.0, 0.03, 12345);
        
        auto close_line = std::make_shared<LineRoot>(large_data[3].size(), "stress_close");
        auto high_line = std::make_shared<LineRoot>(large_data[1].size(), "stress_high");
        auto low_line = std::make_shared<LineRoot>(large_data[2].size(), "stress_low");
        
        for (size_t i = 0; i < large_data[0].size(); ++i) {
            high_line->forward(large_data[1][i]);
            low_line->forward(large_data[2][i]);
            close_line->forward(large_data[3][i]);
        }
        
        BENCHMARK_START();
        
        if (indicator_name == "SMA") {
            auto sma = std::make_shared<SMA>(close_line, 20);
            for (size_t i = 0; i < large_data[3].size(); ++i) {
                sma->calculate();
                if (i < large_data[3].size() - 1) {
                    close_line->forward();
                }
            }
        } else if (indicator_name == "RSI") {
            auto rsi = std::make_shared<RSI>(close_line, 14);
            for (size_t i = 0; i < large_data[3].size(); ++i) {
                rsi->calculate();
                if (i < large_data[3].size() - 1) {
                    close_line->forward();
                }
            }
        } else if (indicator_name == "MACD") {
            auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
            for (size_t i = 0; i < large_data[3].size(); ++i) {
                macd->calculate();
                if (i < large_data[3].size() - 1) {
                    close_line->forward();
                }
            }
        }
        
        BENCHMARK_END(5000.0); // 应该在5秒内完成50k数据点
    }
};

TEST_F(IndicatorStressTest, SMAStressTest) {
    TestIndicatorWithLargeDataset("SMA");
}

TEST_F(IndicatorStressTest, RSIStressTest) {
    TestIndicatorWithLargeDataset("RSI");
}

TEST_F(IndicatorStressTest, MACDStressTest) {
    TestIndicatorWithLargeDataset("MACD");
}

/**
 * @brief 参数化测试示例
 */
class ParameterizedSMATest : public BacktraderTestBase,
                           public ::testing::WithParamInterface<std::tuple<int, double>> {
};

TEST_P(ParameterizedSMATest, DifferentPeriodsAndData) {
    auto [period, base_price] = GetParam();
    
    auto test_data = TestDataGenerator::generateRandomOHLCV(100, base_price, 0.02);
    auto data_line = std::make_shared<LineRoot>(1000, "param_test");
    
    for (double value : test_data[3]) {
        data_line->forward(value);
    }
    
    auto sma = std::make_shared<SMA>(data_line, period);
    
    // 计算足够的点
    for (int i = 0; i < 50; ++i) {
        sma->calculate();
        if (i < 49) {
            data_line->forward();
        }
    }
    
    double sma_value = sma->get(0);
    
    if (period <= 50) {
        EXPECT_NO_NAN(sma_value);
        EXPECT_GT(sma_value, 0.0);
    }
}

INSTANTIATE_TEST_SUITE_P(
    DifferentConfigurations,
    ParameterizedSMATest,
    ::testing::Values(
        std::make_tuple(5, 50.0),
        std::make_tuple(10, 100.0),
        std::make_tuple(20, 200.0),
        std::make_tuple(50, 500.0)
    )
);

// 主函数
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}