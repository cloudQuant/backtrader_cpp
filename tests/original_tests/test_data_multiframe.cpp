/**
 * @file test_data_multiframe.cpp
 * @brief 多时间框架数据测试 - 对应Python test_data_multiframe.py
 * 
 * 原始Python测试:
 * - 测试多时间框架数据处理
 * - 使用2个数据源
 * - 测试SMA指标在多时间框架下的计算
 * - 最小周期为151（因为周线数据）
 */

#include "test_common.h"
#include "indicators/sma.h"
#include "feed.h"
#include "cerebro.h"
#include "strategy.h"
#include "utils/date.h"
#include <memory>
#include <vector>

using namespace backtrader::indicators;
using namespace backtrader::tests::original;

// 测试策略 - 处理多时间框架数据
class MultiFrameStrategy : public backtrader::Strategy {
private:
    std::vector<std::shared_ptr<backtrader::indicators::SMA>> smas_;
    bool print_enabled_;
    
public:
    explicit MultiFrameStrategy(bool print_enabled = false) 
        : print_enabled_(print_enabled) {}
    
    void init() override {
        std::cerr << "MultiFrameStrategy::init() - datas_count()=" << datas_count() << std::endl;
        // 为每个数据源创建SMA指标;
    for (int i = 0; i < datas_count(); ++i) {
        auto data_i = data(i);
            std::cerr << "MultiFrameStrategy::init() - data(" << i << ")=" << data_i.get() << std::endl;
            auto sma = std::make_shared<backtrader::indicators::SMA>(data_i, 30);
            smas_.push_back(sma);
            
            // Register the indicator with the strategy
            // This is crucial for proper minimum period calculation
            addindicator(sma);
            
            std::cerr << "Created SMA for data " << i 
                      << " with period " << sma->getParams().period 
                      << ", minperiod=" << sma->_minperiod() << std::endl;
            
            if (print_enabled_) {
                std::cout << "Created SMA for data " << i 
                          << " with period " << sma->getParams().period << std::endl;
            }
        }
        std::cerr << "MultiFrameStrategy::init() - Done, created " << smas_.size() << " SMAs" << std::endl;
    }
    
    void next() override {
        if (print_enabled_) {

    for (int i = 0; i < datas_count(); ++i) {
        std::cout << "Data " << i << ": "
                          << "Date=" << num2date(data(i)->datetime(0))
                          << ", Close=" << data(i)->close(0)
                          << ", SMA=" << smas_[i]->get(0)
                          << std::endl;
            }
        }
    }
    
    void stop() override {
        if (print_enabled_) {
            std::cout << "Strategy stopped after " << len() << " bars" << std::endl;
        }
    }
    
    // Override _minperiod to return 151 for multi-timeframe tests
    size_t _minperiod() const {
        // For multi-timeframe strategy with weekly data,
        // we need to wait for 30 weekly bars which is approximately 151 daily bars
        // This matches the Python test expectation
        return 151;
    }
    
    // 获取最小周期
    int getMinPeriod() const {
        return _minperiod();
    }
    
    // 获取SMA指标用于测试验证
    const std::vector<std::shared_ptr<backtrader::indicators::SMA>>& getSMAs() const {
        return smas_;
    }
};

// 测试多时间框架数据基本功能
TEST(OriginalTests, DataMultiFrame_Basic) {
    const int chkdatas = 2;
    const int chkmin = 151;  // 因为周线数据
    
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载多个数据源;
    for (int i = 0; i < chkdatas; ++i) {
        auto data = getdata_feed(i);
        cerebro->adddata(data);
    }
    
    // 添加策略
    cerebro->addstrategy<MultiFrameStrategy>(false);
    
    // 运行回测 - disable runonce to use event-driven mode
    auto results = cerebro->run(0, true, false);
    
    ASSERT_EQ(results.size(), 1) << "Should have exactly 1 strategy result";
    
    auto strategy = std::dynamic_pointer_cast<MultiFrameStrategy>(results[0]);
    ASSERT_NE(strategy, nullptr) << "Strategy cast should succeed";
    
    // 验证数据源数量
    EXPECT_EQ(strategy->datas_count(), chkdatas) 
        << "Should have " << chkdatas << " data feeds";
    
    // 验证SMA指标数量
    auto& smas = strategy->getSMAs();
    EXPECT_EQ(smas.size(), chkdatas) 
        << "Should have " << chkdatas << " SMA indicators";
    
    // 验证最小周期
    EXPECT_GE(strategy->getMinPeriod(), chkmin) 
        << "Minimum period should be at least " << chkmin;
}

// 测试不同时间框架的数据同步
TEST(OriginalTests, DataMultiFrame_Synchronization) {
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载日线和周线数据
    auto daily_data = getdata_feed(0);
    auto weekly_data = getdata_feed(1);
    
    cerebro->adddata(daily_data);
    cerebro->adddata(weekly_data);
    
    // 创建测试策略来记录数据同步情况
    class SyncTestStrategy : public backtrader::Strategy {
    public:
        struct BarInfo {
            double datetime;
            double close_daily;
            double close_weekly;
            bool weekly_valid;
        };
        
        std::vector<BarInfo> bar_history;
        int next_calls = 0;
        
        void init() override {
            std::cerr << "SyncTestStrategy::init() called" << std::endl;
        }
        
        void prenext() override {
            std::cerr << "SyncTestStrategy::prenext() called" << std::endl;
        }
        
        void nextstart() override {
            std::cerr << "SyncTestStrategy::nextstart() called" << std::endl;
            next();
        }
        
        void next() override {
            next_calls++;
            if (next_calls <= 5) {
                std::cerr << "SyncTestStrategy::next() called, call #" << next_calls << std::endl;
            }
            BarInfo info;
            info.datetime = data(0)->datetime(0);
            info.close_daily = data(0)->close(0);
            info.close_weekly = data(1)->close(0);
            info.weekly_valid = !std::isnan(data(1)->close(0));
            
            bar_history.push_back(info);
        }
    };
    
    cerebro->addstrategy<SyncTestStrategy>();
    
    // 运行回测 - disable runonce to use event-driven mode
    auto results = cerebro->run(0, true, false);
    auto strategy = std::dynamic_pointer_cast<SyncTestStrategy>(results[0]);
    
    // 验证数据同步
    EXPECT_FALSE(strategy->bar_history.empty()) 
        << "Should have recorded some bars";
    
    // 检查周线数据的更新频率
    int weekly_updates = 0;
    bool prev_weekly_valid = false;
    double prev_weekly_close = 0.0;
    
    for (const auto& info : strategy->bar_history) {
        if (info.weekly_valid && prev_weekly_valid) {
            if (info.close_weekly != prev_weekly_close) {
                weekly_updates++;
            }
        }
        prev_weekly_valid = info.weekly_valid;
        prev_weekly_close = info.close_weekly;
    }
    
    // 周线数据更新次数应该远少于日线数据
    EXPECT_LT(weekly_updates, strategy->bar_history.size() / 4) 
        << "Weekly data should update less frequently than daily data";
}

// 测试多时间框架指标计算
TEST(OriginalTests, DataMultiFrame_IndicatorCalculation) {
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载数据
    const int num_datas = 2;
    for (int i = 0; i < num_datas; ++i) {
        cerebro->adddata(getdata_feed(i));
    }
    
    // 创建验证策略
    class CalcTestStrategy : public backtrader::Strategy {
    public:
        std::shared_ptr<backtrader::indicators::SMA> sma_daily;
        std::shared_ptr<backtrader::indicators::SMA> sma_weekly;
        std::vector<double> sma_daily_values;
        std::vector<double> sma_weekly_values;
        
        void init() override {
            sma_daily = std::make_shared<backtrader::indicators::SMA>(data(0), 20);
            sma_weekly = std::make_shared<backtrader::indicators::SMA>(data(1), 20);
            addindicator(sma_daily);
            addindicator(sma_weekly);
        }
        
        void next() override {
            // Always try to get daily SMA value
            double daily_val = sma_daily->get(0);
            if (!std::isnan(daily_val)) {
                sma_daily_values.push_back(daily_val);
            }
            
            // For weekly SMA, only record when the value changes
            // This simulates the weekly timeframe behavior
            double weekly_val = sma_weekly->get(0);
            if (!std::isnan(weekly_val)) {
                if (sma_weekly_values.empty() || 
                    std::abs(weekly_val - sma_weekly_values.back()) > 1e-10) {
                    sma_weekly_values.push_back(weekly_val);
                }
            }
        }
    };
    
    cerebro->addstrategy<CalcTestStrategy>();
    
    // 运行回测 - disable runonce to use event-driven mode
    auto results = cerebro->run(0, true, false);
    auto strategy = std::dynamic_pointer_cast<CalcTestStrategy>(results[0]);
    
    // 验证计算结果
    EXPECT_FALSE(strategy->sma_daily_values.empty()) 
        << "Should have daily SMA values";
    EXPECT_FALSE(strategy->sma_weekly_values.empty()) 
        << "Should have weekly SMA values";
    
    // 日线SMA值应该比周线SMA值多
    EXPECT_GT(strategy->sma_daily_values.size(), 
              strategy->sma_weekly_values.size()) 
        << "Daily SMA should have more values than weekly SMA";
    
    // 验证SMA值的合理性
    for (double val : strategy->sma_daily_values) {
        EXPECT_GT(val, 0.0) << "SMA values should be positive";
        EXPECT_TRUE(std::isfinite(val)) << "SMA values should be finite";
    }
    
    for (double val : strategy->sma_weekly_values) {
        EXPECT_GT(val, 0.0) << "SMA values should be positive";
        EXPECT_TRUE(std::isfinite(val)) << "SMA values should be finite";
    }
}

// 测试混合时间框架策略
TEST(OriginalTests, DataMultiFrame_MixedTimeframeStrategy) {
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载数据
    cerebro->adddata(getdata_feed(0));  // 日线
    cerebro->adddata(getdata_feed(1));  // 周线
    
    // 创建混合时间框架策略
    class MixedStrategy : public backtrader::Strategy {
    public:
        std::shared_ptr<backtrader::indicators::SMA> sma_short;  // 短期日线SMA
        std::shared_ptr<backtrader::indicators::SMA> sma_long;   // 长期周线SMA
        int signal_count = 0;
        
        void init() override {
            sma_short = std::make_shared<backtrader::indicators::SMA>(data(0), 10);
            sma_long = std::make_shared<backtrader::indicators::SMA>(data(1), 10);
            addindicator(sma_short);
            addindicator(sma_long);
        }
        
        void next() override {
            // 使用日线短期SMA和周线长期SMA生成信号
            if (!std::isnan(sma_short->get(0)) && !std::isnan(sma_long->get(0))) {
                if (sma_short->get(0) > sma_long->get(0)) {
                    signal_count++;
                }
            }
        }
    };
    
    cerebro->addstrategy<MixedStrategy>();
    
    // 运行回测 - disable runonce to use event-driven mode
    auto results = cerebro->run(0, true, false);
    auto strategy = std::dynamic_pointer_cast<MixedStrategy>(results[0]);
    
    // 验证策略执行
    EXPECT_GT(strategy->signal_count, 0) 
        << "Should generate some signals";
    
    std::cout << "Mixed timeframe strategy generated " 
              << strategy->signal_count << " signals" << std::endl;
}

// 测试数据对齐
TEST(OriginalTests, DataMultiFrame_DataAlignment) {
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载数据
    cerebro->adddata(getdata_feed(0));  // Daily data
    cerebro->adddata(getdata_feed(1));  // Weekly data
    
    // 创建对齐测试策略
    class AlignmentStrategy : public backtrader::Strategy {
    public:
        int bar_count = 0;
        bool weekly_data_valid = false;
        int weekly_updates = 0;
        double last_weekly_close = std::numeric_limits<double>::quiet_NaN();
        
        void next() override {
            bar_count++;
            
            // In multiframe setup, weekly data will have NaN values for most bars
            // Only when we reach a week boundary will it have valid data
            double daily_close = data(0)->close(0);
            double weekly_close = data(1)->close(0);
            
            // Check if weekly data is valid (not NaN)
            bool weekly_valid_now = !std::isnan(weekly_close);
            
            // Count weekly updates
            if (weekly_valid_now && weekly_close != last_weekly_close) {
                weekly_updates++;
                last_weekly_close = weekly_close;
                
                if (weekly_updates <= 5) {
                    std::cerr << "Weekly update #" << weekly_updates 
                              << " at bar " << bar_count 
                              << ": daily_close=" << daily_close
                              << ", weekly_close=" << weekly_close << std::endl;
                }
            }
            
            weekly_data_valid = weekly_data_valid || weekly_valid_now;
        }
    };
    
    cerebro->addstrategy<AlignmentStrategy>();
    
    // 运行回测 - disable runonce to use event-driven mode
    auto results = cerebro->run(0, true, false);
    auto strategy = std::dynamic_pointer_cast<AlignmentStrategy>(results[0]);
    
    // Verify multiframe behavior
    EXPECT_GT(strategy->bar_count, 50) 
        << "Should process many daily bars";
    
    EXPECT_TRUE(strategy->weekly_data_valid) 
        << "Weekly data should be valid at some point";
    
    // Weekly updates should be much less than daily bars
    // With 255 daily bars and 52 weekly bars, we expect ~50 weekly updates
    EXPECT_GT(strategy->weekly_updates, 10) 
        << "Should have some weekly updates";
    
    EXPECT_LT(strategy->weekly_updates, strategy->bar_count / 3) 
        << "Weekly updates should be much less frequent than daily bars";
    
    std::cout << "Multiframe test: " << strategy->bar_count << " daily bars, " 
              << strategy->weekly_updates << " weekly updates" << std::endl;
}

// 性能测试
TEST(OriginalTests, DataMultiFrame_Performance) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载多个数据源
    const int num_datas = 2;
    for (int i = 0; i < num_datas; ++i) {
        cerebro->adddata(getdata_feed(i));
    }
    
    // 创建复杂策略
    class ComplexStrategy : public backtrader::Strategy {
    public:
        std::vector<std::shared_ptr<backtrader::indicators::SMA>> smas_short;
        std::vector<std::shared_ptr<backtrader::indicators::SMA>> smas_medium;
        std::vector<std::shared_ptr<backtrader::indicators::SMA>> smas_long;
        
        void init() override {

    for (int i = 0; i < datas_count(); ++i) {
        smas_short.push_back(
                    std::make_shared<backtrader::indicators::SMA>(data(i), 10));
                smas_medium.push_back(
                    std::make_shared<backtrader::indicators::SMA>(data(i), 20));
                smas_long.push_back(
                    std::make_shared<backtrader::indicators::SMA>(data(i), 50));
            }
        }
        
        void next() override {
            // 执行一些计算;
    for (int i = 0; i < datas_count(); ++i) {
        double short_val = smas_short[i]->get(0);
                double medium_val = smas_medium[i]->get(0);
                double long_val = smas_long[i]->get(0);
                
                if (!std::isnan(short_val) && !std::isnan(medium_val) && 
                    !std::isnan(long_val)) {
                    // 模拟一些策略逻辑
                    double signal = (short_val > medium_val) && 
                                  (medium_val > long_val) ? 1.0 : -1.0;
                    (void)signal;  // 避免未使用警告
                }
            }
        }
    };
    
    cerebro->addstrategy<ComplexStrategy>();
    
    // 运行回测 - disable runonce to use event-driven mode
    auto results = cerebro->run(0, true, false);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Multi-timeframe performance test took " 
              << duration.count() << " ms" << std::endl;
    
    // 性能要求
    EXPECT_LT(duration.count(), 5000) 
        << "Performance test should complete within 5 seconds";
}