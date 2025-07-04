/**
 * @file test_data_replay.cpp
 * @brief 数据重放测试 - 对应Python test_data_replay.py
 * 
 * 原始Python测试:
 * - 测试数据重放功能
 * - 将日线数据重放为周线数据
 * - 期望值: [["3836.453333", "3703.962333", "3741.802000"]]
 * - 113次next调用，最小周期30
 */

#include "test_common.h"
#include "indicators/sma.h"
#include "data/DataReplay.h"
#include "cerebro/Cerebro.h"
#include "strategy.h"
#include <memory>
#include <vector>
#include <iomanip>
#include <sstream>

using namespace backtrader::indicators;
using namespace backtrader::tests::original;

// 测试策略，记录重放过程中的数据
class ReplayTestStrategy : public Strategy {
private:
    std::shared_ptr<indicators::SMA> sma_;
    std::vector<std::string> sma_values_;
    int next_count_;
    bool print_enabled_;
    
public:
    explicit ReplayTestStrategy(bool print_enabled = false) 
        : next_count_(0), print_enabled_(print_enabled) {}
    
    void init() override {
        sma_ = std::make_shared<indicators::SMA>(data(0), 30);
    }
    
    void next() override {
        next_count_++;
        
        if (print_enabled_) {
            std::cout << "Bar " << next_count_ 
                      << ": Date=" << num2date(data(0)->datetime(0))
                      << ", Open=" << data(0)->open(0)
                      << ", High=" << data(0)->high(0)
                      << ", Low=" << data(0)->low(0)
                      << ", Close=" << data(0)->close(0)
                      << ", Volume=" << data(0)->volume(0)
                      << std::endl;
        }
        
        // 记录SMA值
        if (!std::isnan(sma_->get(0))) {
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << sma_->get(0);
            sma_values_.push_back(ss.str());
        }
    }
    
    void stop() override {
        if (print_enabled_) {
            std::cout << "Strategy stopped after " << next_count_ 
                      << " bars, SMA values recorded: " << sma_values_.size() 
                      << std::endl;
        }
    }
    
    // 用于测试验证的getter
    int getNextCount() const { return next_count_; }
    const std::vector<std::string>& getSMAValues() const { return sma_values_; }
    std::shared_ptr<indicators::SMA> getSMA() const { return sma_; }
};

// 测试基本数据重放功能
TEST(OriginalTests, DataReplay_Basic) {
    const int chknext = 113;
    const int chkmin = 30;
    const std::vector<std::string> expected_values = {
        "3836.453333", "3703.962333", "3741.802000"
    };
    
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    
    // 设置运行模式
    cerebro->setRunOnce(false);
    cerebro->setPreload(false);
    
    // 加载数据并设置重放
    auto data = getdata(0);
    auto replay_data = std::make_shared<DataReplay>(data);
    replay_data->replay(TimeFrame::Weeks, 1);
    cerebro->adddata(replay_data);
    
    // 添加策略
    cerebro->addstrategy<ReplayTestStrategy>(false);
    
    // 运行回测
    auto results = cerebro->run();
    
    ASSERT_EQ(results.size(), 1) << "Should have exactly 1 strategy result";
    
    auto strategy = std::dynamic_pointer_cast<ReplayTestStrategy>(results[0]);
    ASSERT_NE(strategy, nullptr) << "Strategy cast should succeed";
    
    // 验证next调用次数
    EXPECT_EQ(strategy->getNextCount(), chknext) 
        << "Should have " << chknext << " next calls";
    
    // 验证最小周期
    EXPECT_EQ(strategy->getSMA()->getMinPeriod(), chkmin) 
        << "SMA minimum period should be " << chkmin;
    
    // 验证SMA值
    const auto& sma_values = strategy->getSMAValues();
    EXPECT_FALSE(sma_values.empty()) << "Should have SMA values";
    
    // 获取检查点
    int data_length = sma_values.size();
    std::vector<int> check_points = {
        0,                                    // 第一个有效值
        std::max(0, data_length - chkmin),   // 倒数第(data_length - chkmin)个值
        std::max(0, (data_length - chkmin) / 2)  // 中间值
    };
    
    // 验证期望值
    for (size_t i = 0; i < check_points.size() && i < expected_values.size() && 
         check_points[i] < static_cast<int>(sma_values.size()); ++i) {
        EXPECT_EQ(sma_values[check_points[i]], expected_values[i])
            << "SMA value mismatch at check point " << i 
            << " (index=" << check_points[i] << "): "
            << "expected " << expected_values[i] 
            << ", got " << sma_values[check_points[i]];
    }
}

// 测试不同的重放参数
TEST(OriginalTests, DataReplay_DifferentParameters) {
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    cerebro->setRunOnce(false);
    cerebro->setPreload(false);
    
    // 测试不同的压缩比
    auto data = getdata(0);
    auto replay_data = std::make_shared<DataReplay>(data);
    replay_data->replay(TimeFrame::Weeks, 2);  // 2周压缩
    cerebro->adddata(replay_data);
    
    // 添加策略
    cerebro->addstrategy<ReplayTestStrategy>(false);
    
    // 运行回测
    auto results = cerebro->run();
    auto strategy = std::dynamic_pointer_cast<ReplayTestStrategy>(results[0]);
    
    // 2周压缩应该产生更少的bar
    EXPECT_LT(strategy->getNextCount(), 113) 
        << "2-week compression should produce fewer bars than weekly";
    
    std::cout << "2-week compression produced " << strategy->getNextCount() 
              << " bars" << std::endl;
}

// 测试月线重放
TEST(OriginalTests, DataReplay_Monthly) {
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    cerebro->setRunOnce(false);
    cerebro->setPreload(false);
    
    // 设置月线重放
    auto data = getdata(0);
    auto replay_data = std::make_shared<DataReplay>(data);
    replay_data->replay(TimeFrame::Months, 1);
    cerebro->adddata(replay_data);
    
    // 添加策略
    cerebro->addstrategy<ReplayTestStrategy>(false);
    
    // 运行回测
    auto results = cerebro->run();
    auto strategy = std::dynamic_pointer_cast<ReplayTestStrategy>(results[0]);
    
    // 月线应该产生比周线更少的bar
    EXPECT_LT(strategy->getNextCount(), 113) 
        << "Monthly replay should produce fewer bars than weekly";
    
    std::cout << "Monthly replay produced " << strategy->getNextCount() 
              << " bars" << std::endl;
}

// 测试重放数据的OHLC完整性
TEST(OriginalTests, DataReplay_OHLCIntegrity) {
    // 创建数据记录策略
    class OHLCStrategy : public Strategy {
    public:
        struct BarData {
            double datetime;
            double open;
            double high;
            double low;
            double close;
            double volume;
        };
        
        std::vector<BarData> bars;
        
        void next() override {
            BarData bar;
            bar.datetime = data(0)->datetime(0);
            bar.open = data(0)->open(0);
            bar.high = data(0)->high(0);
            bar.low = data(0)->low(0);
            bar.close = data(0)->close(0);
            bar.volume = data(0)->volume(0);
            
            bars.push_back(bar);
        }
    };
    
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    cerebro->setRunOnce(false);
    cerebro->setPreload(false);
    
    // 设置重放
    auto data = getdata(0);
    auto replay_data = std::make_shared<DataReplay>(data);
    replay_data->replay(TimeFrame::Weeks, 1);
    cerebro->adddata(replay_data);
    
    // 添加策略
    cerebro->addstrategy<OHLCStrategy>();
    
    // 运行回测
    auto results = cerebro->run();
    auto strategy = std::dynamic_pointer_cast<OHLCStrategy>(results[0]);
    
    // 验证OHLC数据完整性
    EXPECT_FALSE(strategy->bars.empty()) << "Should have bar data";
    
    for (size_t i = 0; i < strategy->bars.size(); ++i) {
        const auto& bar = strategy->bars[i];
        
        // 验证基本OHLC关系
        EXPECT_LE(bar.low, bar.high) 
            << "Low should be <= High at bar " << i;
        EXPECT_GE(bar.open, bar.low) 
            << "Open should be >= Low at bar " << i;
        EXPECT_LE(bar.open, bar.high) 
            << "Open should be <= High at bar " << i;
        EXPECT_GE(bar.close, bar.low) 
            << "Close should be >= Low at bar " << i;
        EXPECT_LE(bar.close, bar.high) 
            << "Close should be <= High at bar " << i;
        
        // 验证数据有效性
        EXPECT_TRUE(std::isfinite(bar.open)) 
            << "Open should be finite at bar " << i;
        EXPECT_TRUE(std::isfinite(bar.high)) 
            << "High should be finite at bar " << i;
        EXPECT_TRUE(std::isfinite(bar.low)) 
            << "Low should be finite at bar " << i;
        EXPECT_TRUE(std::isfinite(bar.close)) 
            << "Close should be finite at bar " << i;
        EXPECT_GE(bar.volume, 0.0) 
            << "Volume should be non-negative at bar " << i;
    }
}

// 测试重放数据的时间顺序
TEST(OriginalTests, DataReplay_TimeOrder) {
    // 创建时间检查策略
    class TimeOrderStrategy : public Strategy {
    public:
        std::vector<double> datetimes;
        
        void next() override {
            datetimes.push_back(data(0)->datetime(0));
        }
    };
    
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    cerebro->setRunOnce(false);
    cerebro->setPreload(false);
    
    // 设置重放
    auto data = getdata(0);
    auto replay_data = std::make_shared<DataReplay>(data);
    replay_data->replay(TimeFrame::Weeks, 1);
    cerebro->adddata(replay_data);
    
    // 添加策略
    cerebro->addstrategy<TimeOrderStrategy>();
    
    // 运行回测
    auto results = cerebro->run();
    auto strategy = std::dynamic_pointer_cast<TimeOrderStrategy>(results[0]);
    
    // 验证时间顺序
    EXPECT_GT(strategy->datetimes.size(), 1) << "Should have multiple datetime points";
    
    for (size_t i = 1; i < strategy->datetimes.size(); ++i) {
        EXPECT_GE(strategy->datetimes[i], strategy->datetimes[i-1]) 
            << "Datetime should be in ascending order at position " << i;
    }
}

// 测试重放与原始数据的关系
TEST(OriginalTests, DataReplay_CompareOriginal) {
    // 先获取原始数据的统计
    class OriginalDataStrategy : public Strategy {
    public:
        int bar_count = 0;
        double total_volume = 0.0;
        double sum_close = 0.0;
        
        void next() override {
            bar_count++;
            total_volume += data(0)->volume(0);
            sum_close += data(0)->close(0);
        }
    };
    
    // 测试原始数据
    auto cerebro1 = std::make_unique<Cerebro>();
    cerebro1->adddata(getdata(0));
    cerebro1->addstrategy<OriginalDataStrategy>();
    auto results1 = cerebro1->run();
    auto original_strategy = std::dynamic_pointer_cast<OriginalDataStrategy>(results1[0]);
    
    // 测试重放数据
    auto cerebro2 = std::make_unique<Cerebro>();
    cerebro2->setRunOnce(false);
    cerebro2->setPreload(false);
    
    auto data = getdata(0);
    auto replay_data = std::make_shared<DataReplay>(data);
    replay_data->replay(TimeFrame::Weeks, 1);
    cerebro2->adddata(replay_data);
    cerebro2->addstrategy<OriginalDataStrategy>();
    auto results2 = cerebro2->run();
    auto replay_strategy = std::dynamic_pointer_cast<OriginalDataStrategy>(results2[0]);
    
    // 比较结果
    std::cout << "Original data: " << original_strategy->bar_count 
              << " bars, total volume: " << original_strategy->total_volume << std::endl;
    std::cout << "Replayed data: " << replay_strategy->bar_count 
              << " bars, total volume: " << replay_strategy->total_volume << std::endl;
    
    // 重放数据应该有更少的bar但总成交量应该相等
    EXPECT_LT(replay_strategy->bar_count, original_strategy->bar_count) 
        << "Replayed data should have fewer bars";
    EXPECT_NEAR(replay_strategy->total_volume, original_strategy->total_volume, 
                original_strategy->total_volume * 0.01) 
        << "Total volume should be approximately preserved";
}

// 测试重放过程中的数据更新
TEST(OriginalTests, DataReplay_DataUpdates) {
    // 创建更新跟踪策略
    class UpdateTrackingStrategy : public Strategy {
    public:
        struct UpdateInfo {
            double datetime;
            double open;
            double high;
            double low;
            double close;
            bool is_new_bar;
        };
        
        std::vector<UpdateInfo> updates;
        double last_datetime = 0.0;
        
        void next() override {
            UpdateInfo info;
            info.datetime = data(0)->datetime(0);
            info.open = data(0)->open(0);
            info.high = data(0)->high(0);
            info.low = data(0)->low(0);
            info.close = data(0)->close(0);
            info.is_new_bar = (info.datetime != last_datetime);
            
            updates.push_back(info);
            last_datetime = info.datetime;
        }
    };
    
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    cerebro->setRunOnce(false);
    cerebro->setPreload(false);
    
    // 设置重放
    auto data = getdata(0);
    auto replay_data = std::make_shared<DataReplay>(data);
    replay_data->replay(TimeFrame::Weeks, 1);
    cerebro->adddata(replay_data);
    
    // 添加策略
    cerebro->addstrategy<UpdateTrackingStrategy>();
    
    // 运行回测
    auto results = cerebro->run();
    auto strategy = std::dynamic_pointer_cast<UpdateTrackingStrategy>(results[0]);
    
    // 分析更新模式
    EXPECT_FALSE(strategy->updates.empty()) << "Should have updates";
    
    int new_bar_count = 0;
    int update_count = 0;
    
    for (const auto& update : strategy->updates) {
        if (update.is_new_bar) {
            new_bar_count++;
        } else {
            update_count++;
        }
    }
    
    std::cout << "Replay updates: " << new_bar_count << " new bars, " 
              << update_count << " updates" << std::endl;
    
    // 应该有一些更新（重放特性）
    EXPECT_GT(update_count, 0) << "Should have some bar updates during replay";
}

// 性能测试
TEST(OriginalTests, DataReplay_Performance) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    cerebro->setRunOnce(false);
    cerebro->setPreload(false);
    
    // 设置重放
    auto data = getdata(0);
    auto replay_data = std::make_shared<DataReplay>(data);
    replay_data->replay(TimeFrame::Weeks, 1);
    cerebro->adddata(replay_data);
    
    // 添加复杂策略
    class ComplexReplayStrategy : public Strategy {
    public:
        std::shared_ptr<indicators::SMA> sma_short;
        std::shared_ptr<indicators::SMA> sma_medium;
        std::shared_ptr<indicators::SMA> sma_long;
        
        void init() override {
            sma_short = std::make_shared<indicators::SMA>(data(0), 10);
            sma_medium = std::make_shared<indicators::SMA>(data(0), 20);
            sma_long = std::make_shared<indicators::SMA>(data(0), 50);
        }
        
        void next() override {
            // 执行一些计算
            double short_val = sma_short->get(0);
            double medium_val = sma_medium->get(0);
            double long_val = sma_long->get(0);
            
            if (!std::isnan(short_val) && !std::isnan(medium_val) && 
                !std::isnan(long_val)) {
                // 模拟策略逻辑
                double signal = (short_val > medium_val && medium_val > long_val) ? 
                               1.0 : -1.0;
                (void)signal;  // 避免未使用警告
            }
        }
    };
    
    cerebro->addstrategy<ComplexReplayStrategy>();
    
    // 运行回测
    auto results = cerebro->run();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Data replay performance test took " 
              << duration.count() << " ms" << std::endl;
    
    // 性能要求
    EXPECT_LT(duration.count(), 3000) 
        << "Performance test should complete within 3 seconds";
}