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
        // 为每个数据源创建SMA指标
        for (int i = 0; i < datas_count(); ++i) {
            auto sma = std::make_shared<backtrader::indicators::SMA>(data(i));
            smas_.push_back(sma);
            
            if (print_enabled_) {
                std::cout << "Created SMA for data " << i 
                          << " with period " << sma->getParams().period << std::endl;
            }
        }
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
    
    // 获取最小周期
    int getMinPeriod() const {
        int min_period = 0;
        for (const auto& sma : smas_) {
            min_period = std::max(min_period, sma->getMinPeriod());
        }
        return min_period;
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
    
    // 加载多个数据源
    for (int i = 0; i < chkdatas; ++i) {
        auto data = getdata_feed(i);
        cerebro->adddata(data);
    }
    
    // 添加策略
    cerebro->addstrategy<MultiFrameStrategy>(false);
    
    // 运行回测
    auto results = cerebro->run();
    
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
        
        void next() override {
            BarInfo info;
            info.datetime = data(0)->datetime(0);
            info.close_daily = data(0)->close(0);
            info.close_weekly = data(1)->close(0);
            info.weekly_valid = !std::isnan(data(1)->close(0));
            
            bar_history.push_back(info);
        }
    };
    
    cerebro->addstrategy<SyncTestStrategy>();
    
    // 运行回测
    auto results = cerebro->run();
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
        }
        
        void next() override {
            if (!std::isnan(sma_daily->get(0))) {
                sma_daily_values.push_back(sma_daily->get(0));
            }
            if (!std::isnan(sma_weekly->get(0))) {
                sma_weekly_values.push_back(sma_weekly->get(0));
            }
        }
    };
    
    cerebro->addstrategy<CalcTestStrategy>();
    
    // 运行回测
    auto results = cerebro->run();
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
    
    // 运行回测
    auto results = cerebro->run();
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
    cerebro->adddata(getdata_feed(0));
    cerebro->adddata(getdata_feed(1));
    
    // 创建对齐测试策略
    class AlignmentStrategy : public backtrader::Strategy {
    public:
        struct AlignmentCheck {
            double daily_dt;
            double weekly_dt;
            bool aligned;
        };
        
        std::vector<AlignmentCheck> checks;
        
        void next() override {
            AlignmentCheck check;
            check.daily_dt = data(0)->datetime(0);
            check.weekly_dt = data(1)->datetime(0);
            
            // 检查日期对齐（同一天）
            auto daily_date = backtrader::utils::num2date(check.daily_dt);
            auto weekly_date = backtrader::utils::num2date(check.weekly_dt);
            
            check.aligned = (daily_date.tm_year == weekly_date.tm_year &&
                           daily_date.tm_mon == weekly_date.tm_mon &&
                           daily_date.tm_mday == weekly_date.tm_mday);
            
            checks.push_back(check);
        }
    };
    
    cerebro->addstrategy<AlignmentStrategy>();
    
    // 运行回测
    auto results = cerebro->run();
    auto strategy = std::dynamic_pointer_cast<AlignmentStrategy>(results[0]);
    
    // 统计对齐情况
    int aligned_count = 0;
    for (const auto& check : strategy->checks) {
        if (check.aligned) {
            aligned_count++;
        }
    }
    
    std::cout << "Data alignment: " << aligned_count 
              << " out of " << strategy->checks.size() 
              << " bars are aligned" << std::endl;
    
    // 大部分数据应该是对齐的
    double align_ratio = static_cast<double>(aligned_count) / strategy->checks.size();
    EXPECT_GT(align_ratio, 0.8) 
        << "Most data points should be aligned";
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
            // 执行一些计算
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
    
    // 运行回测
    auto results = cerebro->run();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Multi-timeframe performance test took " 
              << duration.count() << " ms" << std::endl;
    
    // 性能要求
    EXPECT_LT(duration.count(), 5000) 
        << "Performance test should complete within 5 seconds";
}