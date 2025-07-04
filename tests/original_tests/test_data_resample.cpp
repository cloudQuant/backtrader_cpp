/**
 * @file test_data_resample.cpp
 * @brief 数据重采样测试 - 对应Python test_data_resample.py
 * 
 * 原始Python测试:
 * - 测试数据重采样功能
 * - 将日线数据重采样为周线数据
 * - 期望值: [["3836.453333", "3703.962333", "3741.802000"]]
 * - 最小周期30，测试runonce模式
 */

#include "test_common.h"
#include "indicators/sma.h"
#include "data/DataResample.h"
#include "cerebro/Cerebro.h"
#include "strategy.h"
#include <memory>
#include <vector>
#include <iomanip>
#include <sstream>

using namespace backtrader::indicators;
using namespace backtrader::tests::original;

// 测试策略，记录重采样过程中的数据
class ResampleTestStrategy : public Strategy {
private:
    std::shared_ptr<indicators::SMA> sma_;
    std::vector<std::string> sma_values_;
    int next_count_;
    bool print_enabled_;
    
public:
    explicit ResampleTestStrategy(bool print_enabled = false) 
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

// 测试基本数据重采样功能 - runonce模式
TEST(OriginalTests, DataResample_RunOnce) {
    const int chkmin = 30;
    const std::vector<std::string> expected_values = {
        "3836.453333", "3703.962333", "3741.802000"
    };
    
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    
    // 设置运行模式 - runonce=true
    cerebro->setRunOnce(true);
    
    // 加载数据并设置重采样
    auto data = getdata(0);
    auto resample_data = std::make_shared<DataResample>(data);
    resample_data->resample(TimeFrame::Weeks, 1);
    cerebro->adddata(resample_data);
    
    // 添加策略
    cerebro->addstrategy<ResampleTestStrategy>(false);
    
    // 运行回测
    auto results = cerebro->run();
    
    ASSERT_EQ(results.size(), 1) << "Should have exactly 1 strategy result";
    
    auto strategy = std::dynamic_pointer_cast<ResampleTestStrategy>(results[0]);
    ASSERT_NE(strategy, nullptr) << "Strategy cast should succeed";
    
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

// 测试基本数据重采样功能 - 非runonce模式
TEST(OriginalTests, DataResample_NoRunOnce) {
    const int chkmin = 30;
    const std::vector<std::string> expected_values = {
        "3836.453333", "3703.962333", "3741.802000"
    };
    
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    
    // 设置运行模式 - runonce=false
    cerebro->setRunOnce(false);
    
    // 加载数据并设置重采样
    auto data = getdata(0);
    auto resample_data = std::make_shared<DataResample>(data);
    resample_data->resample(TimeFrame::Weeks, 1);
    cerebro->adddata(resample_data);
    
    // 添加策略
    cerebro->addstrategy<ResampleTestStrategy>(false);
    
    // 运行回测
    auto results = cerebro->run();
    
    ASSERT_EQ(results.size(), 1) << "Should have exactly 1 strategy result";
    
    auto strategy = std::dynamic_pointer_cast<ResampleTestStrategy>(results[0]);
    ASSERT_NE(strategy, nullptr) << "Strategy cast should succeed";
    
    // 验证最小周期
    EXPECT_EQ(strategy->getSMA()->getMinPeriod(), chkmin) 
        << "SMA minimum period should be " << chkmin;
    
    // 验证SMA值
    const auto& sma_values = strategy->getSMAValues();
    EXPECT_FALSE(sma_values.empty()) << "Should have SMA values";
    
    // 期望值验证（与runonce模式应该相同）
    int data_length = sma_values.size();
    std::vector<int> check_points = {
        0,                                    
        std::max(0, data_length - chkmin),   
        std::max(0, (data_length - chkmin) / 2)  
    };
    
    for (size_t i = 0; i < check_points.size() && i < expected_values.size() && 
         check_points[i] < static_cast<int>(sma_values.size()); ++i) {
        EXPECT_EQ(sma_values[check_points[i]], expected_values[i])
            << "SMA value mismatch at check point " << i 
            << " (index=" << check_points[i] << "): "
            << "expected " << expected_values[i] 
            << ", got " << sma_values[check_points[i]];
    }
}

// 测试runonce vs 非runonce模式的一致性
TEST(OriginalTests, DataResample_RunOnceConsistency) {
    // 测试runonce=true
    auto cerebro1 = std::make_unique<Cerebro>();
    cerebro1->setRunOnce(true);
    
    auto data1 = getdata(0);
    auto resample_data1 = std::make_shared<DataResample>(data1);
    resample_data1->resample(TimeFrame::Weeks, 1);
    cerebro1->adddata(resample_data1);
    cerebro1->addstrategy<ResampleTestStrategy>(false);
    
    auto results1 = cerebro1->run();
    auto strategy1 = std::dynamic_pointer_cast<ResampleTestStrategy>(results1[0]);
    
    // 测试runonce=false
    auto cerebro2 = std::make_unique<Cerebro>();
    cerebro2->setRunOnce(false);
    
    auto data2 = getdata(0);
    auto resample_data2 = std::make_shared<DataResample>(data2);
    resample_data2->resample(TimeFrame::Weeks, 1);
    cerebro2->adddata(resample_data2);
    cerebro2->addstrategy<ResampleTestStrategy>(false);
    
    auto results2 = cerebro2->run();
    auto strategy2 = std::dynamic_pointer_cast<ResampleTestStrategy>(results2[0]);
    
    // 比较结果
    EXPECT_EQ(strategy1->getNextCount(), strategy2->getNextCount())
        << "runonce and non-runonce should have same number of bars";
    
    const auto& sma1 = strategy1->getSMAValues();
    const auto& sma2 = strategy2->getSMAValues();
    
    EXPECT_EQ(sma1.size(), sma2.size())
        << "runonce and non-runonce should have same number of SMA values";
    
    for (size_t i = 0; i < std::min(sma1.size(), sma2.size()); ++i) {
        EXPECT_EQ(sma1[i], sma2[i])
            << "SMA values should be identical at index " << i;
    }
}

// 测试不同压缩比的重采样
TEST(OriginalTests, DataResample_DifferentCompression) {
    // 测试1周压缩
    auto cerebro1 = std::make_unique<Cerebro>();
    auto data1 = getdata(0);
    auto resample_data1 = std::make_shared<DataResample>(data1);
    resample_data1->resample(TimeFrame::Weeks, 1);
    cerebro1->adddata(resample_data1);
    cerebro1->addstrategy<ResampleTestStrategy>(false);
    auto results1 = cerebro1->run();
    auto strategy1 = std::dynamic_pointer_cast<ResampleTestStrategy>(results1[0]);
    
    // 测试2周压缩
    auto cerebro2 = std::make_unique<Cerebro>();
    auto data2 = getdata(0);
    auto resample_data2 = std::make_shared<DataResample>(data2);
    resample_data2->resample(TimeFrame::Weeks, 2);
    cerebro2->adddata(resample_data2);
    cerebro2->addstrategy<ResampleTestStrategy>(false);
    auto results2 = cerebro2->run();
    auto strategy2 = std::dynamic_pointer_cast<ResampleTestStrategy>(results2[0]);
    
    // 2周压缩应该产生更少的bar
    EXPECT_LT(strategy2->getNextCount(), strategy1->getNextCount())
        << "2-week compression should produce fewer bars than 1-week";
    
    std::cout << "1-week compression: " << strategy1->getNextCount() << " bars" << std::endl;
    std::cout << "2-week compression: " << strategy2->getNextCount() << " bars" << std::endl;
}

// 测试不同时间框架的重采样
TEST(OriginalTests, DataResample_DifferentTimeframes) {
    struct TimeframeTest {
        TimeFrame timeframe;
        std::string name;
    };
    
    std::vector<TimeframeTest> tests = {
        {TimeFrame::Days, "Daily"},
        {TimeFrame::Weeks, "Weekly"},
        {TimeFrame::Months, "Monthly"}
    };
    
    std::vector<int> bar_counts;
    
    for (const auto& test : tests) {
        auto cerebro = std::make_unique<Cerebro>();
        auto data = getdata(0);
        auto resample_data = std::make_shared<DataResample>(data);
        resample_data->resample(test.timeframe, 1);
        cerebro->adddata(resample_data);
        cerebro->addstrategy<ResampleTestStrategy>(false);
        
        auto results = cerebro->run();
        auto strategy = std::dynamic_pointer_cast<ResampleTestStrategy>(results[0]);
        
        bar_counts.push_back(strategy->getNextCount());
        std::cout << test.name << " resample: " << strategy->getNextCount() 
                  << " bars" << std::endl;
    }
    
    // 验证时间框架关系：日线 >= 周线 >= 月线
    EXPECT_GE(bar_counts[0], bar_counts[1]) << "Daily should have >= Weekly bars";
    EXPECT_GE(bar_counts[1], bar_counts[2]) << "Weekly should have >= Monthly bars";
}

// 测试重采样数据的OHLC完整性
TEST(OriginalTests, DataResample_OHLCIntegrity) {
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
    
    // 设置重采样
    auto data = getdata(0);
    auto resample_data = std::make_shared<DataResample>(data);
    resample_data->resample(TimeFrame::Weeks, 1);
    cerebro->adddata(resample_data);
    
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

// 测试重采样与原始数据的关系
TEST(OriginalTests, DataResample_CompareOriginal) {
    // 先获取原始数据的统计
    class OriginalDataStrategy : public Strategy {
    public:
        int bar_count = 0;
        double total_volume = 0.0;
        double sum_close = 0.0;
        double min_low = std::numeric_limits<double>::infinity();
        double max_high = -std::numeric_limits<double>::infinity();
        
        void next() override {
            bar_count++;
            total_volume += data(0)->volume(0);
            sum_close += data(0)->close(0);
            min_low = std::min(min_low, data(0)->low(0));
            max_high = std::max(max_high, data(0)->high(0));
        }
    };
    
    // 测试原始数据
    auto cerebro1 = std::make_unique<Cerebro>();
    cerebro1->adddata(getdata(0));
    cerebro1->addstrategy<OriginalDataStrategy>();
    auto results1 = cerebro1->run();
    auto original_strategy = std::dynamic_pointer_cast<OriginalDataStrategy>(results1[0]);
    
    // 测试重采样数据
    auto cerebro2 = std::make_unique<Cerebro>();
    auto data = getdata(0);
    auto resample_data = std::make_shared<DataResample>(data);
    resample_data->resample(TimeFrame::Weeks, 1);
    cerebro2->adddata(resample_data);
    cerebro2->addstrategy<OriginalDataStrategy>();
    auto results2 = cerebro2->run();
    auto resample_strategy = std::dynamic_pointer_cast<OriginalDataStrategy>(results2[0]);
    
    // 比较结果
    std::cout << "Original data: " << original_strategy->bar_count 
              << " bars, total volume: " << original_strategy->total_volume << std::endl;
    std::cout << "Resampled data: " << resample_strategy->bar_count 
              << " bars, total volume: " << resample_strategy->total_volume << std::endl;
    
    // 重采样数据应该有更少的bar但总成交量应该相等
    EXPECT_LT(resample_strategy->bar_count, original_strategy->bar_count) 
        << "Resampled data should have fewer bars";
    EXPECT_NEAR(resample_strategy->total_volume, original_strategy->total_volume, 
                original_strategy->total_volume * 0.01) 
        << "Total volume should be approximately preserved";
    
    // 价格范围应该相同
    EXPECT_NEAR(resample_strategy->min_low, original_strategy->min_low, 0.01)
        << "Minimum low should be preserved";
    EXPECT_NEAR(resample_strategy->max_high, original_strategy->max_high, 0.01)
        << "Maximum high should be preserved";
}

// 测试重采样时间对齐
TEST(OriginalTests, DataResample_TimeAlignment) {
    // 创建时间检查策略
    class TimeAlignmentStrategy : public Strategy {
    public:
        std::vector<double> datetimes;
        std::vector<std::string> date_strings;
        
        void next() override {
            double dt = data(0)->datetime(0);
            datetimes.push_back(dt);
            date_strings.push_back(num2date(dt));
        }
    };
    
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    
    // 设置重采样
    auto data = getdata(0);
    auto resample_data = std::make_shared<DataResample>(data);
    resample_data->resample(TimeFrame::Weeks, 1);
    cerebro->adddata(resample_data);
    
    // 添加策略
    cerebro->addstrategy<TimeAlignmentStrategy>();
    
    // 运行回测
    auto results = cerebro->run();
    auto strategy = std::dynamic_pointer_cast<TimeAlignmentStrategy>(results[0]);
    
    // 验证时间顺序
    EXPECT_GT(strategy->datetimes.size(), 1) << "Should have multiple datetime points";
    
    for (size_t i = 1; i < strategy->datetimes.size(); ++i) {
        EXPECT_GE(strategy->datetimes[i], strategy->datetimes[i-1]) 
            << "Datetime should be in ascending order at position " << i;
    }
    
    // 验证周对齐（周线数据应该在周末或周初）
    for (size_t i = 0; i < std::min(size_t(5), strategy->date_strings.size()); ++i) {
        std::cout << "Resampled date " << i << ": " << strategy->date_strings[i] << std::endl;
    }
}

// 性能测试
TEST(OriginalTests, DataResample_Performance) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 测试两种模式的性能
    for (bool runonce : {true, false}) {
        auto cerebro = std::make_unique<Cerebro>();
        cerebro->setRunOnce(runonce);
        
        // 设置重采样
        auto data = getdata(0);
        auto resample_data = std::make_shared<DataResample>(data);
        resample_data->resample(TimeFrame::Weeks, 1);
        cerebro->adddata(resample_data);
        
        // 添加复杂策略
        class ComplexResampleStrategy : public Strategy {
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
        
        cerebro->addstrategy<ComplexResampleStrategy>();
        
        // 运行回测
        auto results = cerebro->run();
        
        std::cout << "Resample performance test (runonce=" << runonce 
                  << ") completed" << std::endl;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Data resample performance test took " 
              << duration.count() << " ms" << std::endl;
    
    // 性能要求
    EXPECT_LT(duration.count(), 5000) 
        << "Performance test should complete within 5 seconds";
}