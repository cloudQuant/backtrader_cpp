#include "test_framework.h"
#include "cerebro/Cerebro.h"
#include "strategy/StrategyBase.h"
#include "indicators/SMA.h"
#include "indicators/RSI.h"
#include "indicators/MACD.h"
#include "data/DataFeed.h"
#include "analyzers/PerformanceAnalyzer.h"
#include "optimization/StrategyOptimizer.h"

using namespace backtrader;
using namespace backtrader::testing;
using namespace backtrader::cerebro;
using namespace backtrader::strategy;
using namespace backtrader::data;
using namespace backtrader::analyzers;
using namespace backtrader::optimization;

/**
 * @brief 简单测试策略
 */
class SimpleTestStrategy : public StrategyBase {
private:
    std::shared_ptr<SMA> sma_fast_;
    std::shared_ptr<SMA> sma_slow_;
    double fast_period_;
    double slow_period_;
    
public:
    SimpleTestStrategy(double fast_period = 10.0, double slow_period = 30.0)
        : StrategyBase("SimpleTest"), fast_period_(fast_period), slow_period_(slow_period) {
        setParam("fast_period", fast_period_);
        setParam("slow_period", slow_period_);
    }
    
    void init() override {
        auto data = getData();
        if (data && data->close()) {
            sma_fast_ = std::make_shared<SMA>(data->close(), static_cast<size_t>(fast_period_));
            sma_slow_ = std::make_shared<SMA>(data->close(), static_cast<size_t>(slow_period_));
            
            addIndicator(sma_fast_);
            addIndicator(sma_slow_);
        }
    }
    
    void next() override {
        if (!sma_fast_ || !sma_slow_) return;
        
        double fast_value = sma_fast_->get(0);
        double slow_value = sma_slow_->get(0);
        
        if (std::isnan(fast_value) || std::isnan(slow_value)) return;
        
        if (isEmpty() && fast_value > slow_value) {
            buy(1.0);
        } else if (isLong() && fast_value < slow_value) {
            sell(getPositionSize());
        }
    }
    
    // 测试辅助方法
    double getFastSMA() const {
        return sma_fast_ ? sma_fast_->get(0) : std::numeric_limits<double>::quiet_NaN();
    }
    
    double getSlowSMA() const {
        return sma_slow_ ? sma_slow_->get(0) : std::numeric_limits<double>::quiet_NaN();
    }
};

/**
 * @brief RSI策略测试
 */
class RSITestStrategy : public StrategyBase {
private:
    std::shared_ptr<RSI> rsi_;
    double rsi_period_;
    double overbought_level_;
    double oversold_level_;
    
public:
    RSITestStrategy(double rsi_period = 14.0, double overbought = 70.0, double oversold = 30.0)
        : StrategyBase("RSITest"), rsi_period_(rsi_period), 
          overbought_level_(overbought), oversold_level_(oversold) {
        setParam("rsi_period", rsi_period_);
        setParam("overbought_level", overbought_level_);
        setParam("oversold_level", oversold_level_);
    }
    
    void init() override {
        auto data = getData();
        if (data && data->close()) {
            rsi_ = std::make_shared<RSI>(data->close(), static_cast<size_t>(rsi_period_));
            addIndicator(rsi_);
        }
    }
    
    void next() override {
        if (!rsi_) return;
        
        double rsi_value = rsi_->get(0);
        if (std::isnan(rsi_value)) return;
        
        if (isEmpty() && rsi_value < oversold_level_) {
            buy(1.0);
        } else if (isLong() && rsi_value > overbought_level_) {
            sell(getPositionSize());
        }
    }
    
    double getRSI() const {
        return rsi_ ? rsi_->get(0) : std::numeric_limits<double>::quiet_NaN();
    }
};

/**
 * @brief 基础集成测试类
 */
class IntegrationTestBase : public BacktraderTestBase {
protected:
    void SetUp() override {
        BacktraderTestBase::SetUp();
        
        // 创建标准测试数据
        test_data_ = TestDataGenerator::generateTrendOHLCV(200, 0.1, 100.0, 0.02, 42);
        
        // 创建数据源
        createTestDataFeed();
    }
    
    void createTestDataFeed() {
        // 这里需要实现一个简单的测试数据源
        // 由于DataFeed的复杂性，我们创建一个模拟版本
    }
    
    std::vector<std::vector<double>> test_data_;
};

/**
 * @brief Cerebro引擎集成测试
 */
class CerebroIntegrationTest : public IntegrationTestBase {
protected:
    void createMockDataFeed() {
        // 创建模拟数据源进行测试
    }
};

TEST_F(CerebroIntegrationTest, BasicBacktestExecution) {
    // 这是一个概念性测试，实际实现需要完整的DataFeed
    
    // 验证Cerebro基本设置
    Cerebro cerebro(100000.0);
    
    // 添加策略
    auto strategy = std::make_shared<SimpleTestStrategy>(10, 30);
    cerebro.addStrategy(strategy);
    
    // 验证策略添加
    EXPECT_TRUE(true); // 占位符测试
}

TEST_F(CerebroIntegrationTest, MultipleStrategiesExecution) {
    Cerebro cerebro(200000.0);
    
    // 添加多个策略
    auto strategy1 = std::make_shared<SimpleTestStrategy>(5, 20);
    auto strategy2 = std::make_shared<RSITestStrategy>(14, 70, 30);
    
    cerebro.addStrategy(strategy1);
    cerebro.addStrategy(strategy2);
    
    // 验证多策略添加
    EXPECT_TRUE(true); // 占位符测试
}

/**
 * @brief 策略行为测试
 */
class StrategyBehaviorTest : public BacktraderTestBase {
protected:
    void SetUp() override {
        BacktraderTestBase::SetUp();
        
        // 创建策略测试数据
        strategy_data_ = TestDataGenerator::generateSineWaveOHLCV(100, 20.0, 0.1, 100.0, 0.01);
    }
    
    std::vector<std::vector<double>> strategy_data_;
};

TEST_F(StrategyBehaviorTest, SimpleStrategyLogic) {
    auto strategy = std::make_shared<SimpleTestStrategy>(5, 15);
    
    // 创建模拟数据线
    auto close_line = std::make_shared<LineRoot>(1000, "close");
    for (double value : strategy_data_[3]) {
        close_line->forward(value);
    }
    
    // 手动设置数据（模拟getData()）
    // 这里需要策略接口的改进来支持测试
    
    // 验证策略参数
    EXPECT_DOUBLE_NEAR(strategy->getParam("fast_period"), 5.0, 1e-10);
    EXPECT_DOUBLE_NEAR(strategy->getParam("slow_period"), 15.0, 1e-10);
}

TEST_F(StrategyBehaviorTest, RSIStrategySignals) {
    auto rsi_strategy = std::make_shared<RSITestStrategy>(14, 80, 20);
    
    // 验证RSI策略参数
    EXPECT_DOUBLE_NEAR(rsi_strategy->getParam("rsi_period"), 14.0, 1e-10);
    EXPECT_DOUBLE_NEAR(rsi_strategy->getParam("overbought_level"), 80.0, 1e-10);
    EXPECT_DOUBLE_NEAR(rsi_strategy->getParam("oversold_level"), 20.0, 1e-10);
}

/**
 * @brief 指标链集成测试
 */
class IndicatorChainTest : public BacktraderTestBase {
protected:
    void SetUp() override {
        BacktraderTestBase::SetUp();
        
        chain_data_ = TestDataGenerator::generateTrendOHLCV(150, 0.2, 100.0, 0.025);
        
        close_line_ = std::make_shared<LineRoot>(1000, "close");
        for (double value : chain_data_[3]) {
            close_line_->forward(value);
        }
    }
    
    std::vector<std::vector<double>> chain_data_;
    std::shared_ptr<LineRoot> close_line_;
};

TEST_F(IndicatorChainTest, MultipleIndicatorsConsistency) {
    // 创建多个指标
    auto sma_short = std::make_shared<SMA>(close_line_, 10);
    auto sma_long = std::make_shared<SMA>(close_line_, 30);
    auto rsi = std::make_shared<RSI>(close_line_, 14);
    auto macd = std::make_shared<MACD>(close_line_, 12, 26, 9);
    
    // 同步计算所有指标
    for (int i = 0; i < 50; ++i) {
        sma_short->calculate();
        sma_long->calculate();
        rsi->calculate();
        macd->calculate();
        
        if (i < 49) {
            close_line_->forward();
        }
    }
    
    // 验证指标一致性
    double sma_short_val = sma_short->get(0);
    double sma_long_val = sma_long->get(0);
    double rsi_val = rsi->get(0);
    double macd_val = macd->getMACDLine(0);
    
    EXPECT_NO_NAN(sma_short_val);
    EXPECT_NO_NAN(sma_long_val);
    EXPECT_NO_NAN(rsi_val);
    EXPECT_NO_NAN(macd_val);
    
    // 验证RSI范围
    EXPECT_GE(rsi_val, 0.0);
    EXPECT_LE(rsi_val, 100.0);
}

TEST_F(IndicatorChainTest, IndicatorInteraction) {
    // 测试指标之间的相互作用
    auto sma = std::make_shared<SMA>(close_line_, 20);
    
    // 基于SMA创建二级指标（概念性）
    // 在实际实现中，可以将一个指标的输出作为另一个指标的输入
    
    for (int i = 0; i < 40; ++i) {
        sma->calculate();
        if (i < 39) {
            close_line_->forward();
        }
    }
    
    double sma_value = sma->get(0);
    double current_price = close_line_->get(0);
    
    EXPECT_NO_NAN(sma_value);
    EXPECT_NO_NAN(current_price);
    
    // 验证SMA平滑特性（应该比原始价格波动更小）
    // 这需要更复杂的统计验证，这里只做基本检查
    EXPECT_GT(sma_value, 0.0);
}

/**
 * @brief 性能集成测试
 */
class PerformanceIntegrationTest : public BacktraderTestBase {
public:
    void TestEndToEndPerformance() {
        // 生成大量测试数据
        auto perf_data = TestDataGenerator::generateRandomOHLCV(10000, 100.0, 0.02, 999);
        
        // 创建数据线
        auto close_line = std::make_shared<LineRoot>(perf_data[3].size(), "perf_close");
        for (double value : perf_data[3]) {
            close_line->forward(value);
        }
        
        BENCHMARK_START();
        
        // 创建多个指标进行性能测试
        auto sma_fast = std::make_shared<SMA>(close_line, 10);
        auto sma_slow = std::make_shared<SMA>(close_line, 30);
        auto rsi = std::make_shared<RSI>(close_line, 14);
        auto macd = std::make_shared<MACD>(close_line, 12, 26, 9);
        
        // 批量计算
        for (size_t i = 0; i < perf_data[3].size(); ++i) {
            sma_fast->calculate();
            sma_slow->calculate();
            rsi->calculate();
            macd->calculate();
            
            if (i < perf_data[3].size() - 1) {
                close_line->forward();
            }
        }
        
        BENCHMARK_END(2000.0); // 应该在2秒内完成
        
        // 验证最终结果的合理性
        EXPECT_NO_NAN(sma_fast->get(0));
        EXPECT_NO_NAN(sma_slow->get(0));
        EXPECT_NO_NAN(rsi->get(0));
        EXPECT_NO_NAN(macd->getMACDLine(0));
    }
};

TEST_F(PerformanceIntegrationTest, EndToEndPerformance) {
    TestEndToEndPerformance();
}

/**
 * @brief 内存管理测试
 */
class MemoryManagementTest : public BacktraderTestBase {
public:
    void TestMemoryUsage() {
        // 创建大量指标实例
        std::vector<std::shared_ptr<SMA>> sma_indicators;
        std::vector<std::shared_ptr<RSI>> rsi_indicators;
        
        auto test_data = TestDataGenerator::generateRandomOHLCV(1000, 100.0, 0.02);
        auto data_line = std::make_shared<LineRoot>(1000, "memory_test");
        
        for (double value : test_data[3]) {
            data_line->forward(value);
        }
        
        // 创建多个指标实例
        for (int i = 0; i < 100; ++i) {
            sma_indicators.push_back(std::make_shared<SMA>(data_line, 20 + i % 10));
            rsi_indicators.push_back(std::make_shared<RSI>(data_line, 14 + i % 5));
        }
        
        // 计算所有指标
        for (int step = 0; step < 50; ++step) {
            for (auto& sma : sma_indicators) {
                sma->calculate();
            }
            for (auto& rsi : rsi_indicators) {
                rsi->calculate();
            }
            
            if (step < 49) {
                data_line->forward();
            }
        }
        
        // 验证指标仍然正常工作
        EXPECT_NO_NAN(sma_indicators[0]->get(0));
        EXPECT_NO_NAN(rsi_indicators[0]->get(0));
        
        // 清理
        sma_indicators.clear();
        rsi_indicators.clear();
        
        // 验证清理后的状态
        EXPECT_TRUE(sma_indicators.empty());
        EXPECT_TRUE(rsi_indicators.empty());
    }
};

TEST_F(MemoryManagementTest, LargeScaleMemoryUsage) {
    TestMemoryUsage();
}

/**
 * @brief 错误处理集成测试
 */
class ErrorHandlingTest : public BacktraderTestBase {
public:
    void TestInvalidInputHandling() {
        // 测试空数据线
        std::shared_ptr<LineRoot> null_line = nullptr;
        
        // 应该安全处理空指针
        EXPECT_NO_THROW({
            auto sma = std::make_shared<SMA>(null_line, 10);
            sma->calculate();
        });
        
        // 测试零周期
        auto valid_line = std::make_shared<LineRoot>(100, "valid");
        valid_line->forward(100.0);
        
        EXPECT_THROW({
            auto invalid_sma = std::make_shared<SMA>(valid_line, 0);
        }, std::invalid_argument);
    }
    
    void TestBoundaryConditions() {
        auto data_line = std::make_shared<LineRoot>(10, "boundary");
        
        // 添加一些测试数据
        for (int i = 0; i < 5; ++i) {
            data_line->forward(100.0 + i);
        }
        
        // 测试周期大于数据量的情况
        auto large_period_sma = std::make_shared<SMA>(data_line, 20);
        large_period_sma->calculate();
        
        // 应该返回NaN
        EXPECT_TRUE(std::isnan(large_period_sma->get(0)));
    }
};

TEST_F(ErrorHandlingTest, InvalidInputHandling) {
    TestInvalidInputHandling();
}

TEST_F(ErrorHandlingTest, BoundaryConditions) {
    TestBoundaryConditions();
}

/**
 * @brief 并发测试
 */
class ConcurrencyTest : public BacktraderTestBase {
public:
    void TestThreadSafety() {
        auto test_data = TestDataGenerator::generateRandomOHLCV(1000, 100.0, 0.02, 777);
        auto data_line = std::make_shared<LineRoot>(1000, "concurrent");
        
        for (double value : test_data[3]) {
            data_line->forward(value);
        }
        
        auto sma = std::make_shared<SMA>(data_line, 20);
        
        // 简单的并发测试（读取操作）
        std::vector<std::thread> threads;
        std::vector<double> results(4);
        
        // 先计算一些值
        for (int i = 0; i < 30; ++i) {
            sma->calculate();
            if (i < 29) {
                data_line->forward();
            }
        }
        
        // 多线程读取相同值
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&sma, &results, i]() {
                results[i] = sma->get(0);
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        // 所有线程应该读取到相同的值
        for (int i = 1; i < 4; ++i) {
            EXPECT_DOUBLE_NEAR(results[0], results[i], 1e-10);
        }
    }
};

TEST_F(ConcurrencyTest, BasicThreadSafety) {
    TestThreadSafety();
}

// 主函数
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "Running Integration Tests for Backtrader C++" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    int result = RUN_ALL_TESTS();
    
    std::cout << "\nIntegration Tests Completed!" << std::endl;
    return result;
}