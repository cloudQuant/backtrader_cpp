/**
 * @file test_analyzer-timereturn.cpp
 * @brief TimeReturn Analyzer测试 - 对应Python test_analyzer-timereturn.py
 * 
 * 原始Python测试:
 * - 测试TimeReturn分析器功能
 * - 测试年度收益率计算
 * - 验证计算结果
 */

#include "test_common.h"
#include "analyzers/timereturn.h"
#include "strategy.h"
#include "indicators/sma.h"
#include "indicators/crossover.h"
#include "cerebro.h"
#include "position.h"
#include <chrono>
#include <iomanip>

using namespace backtrader::indicators;
using namespace backtrader::tests::original;

class RunStrategy : public backtrader::Strategy {
public:
    struct Params {
        int period;
        bool printdata;
        bool printops;
        bool stocklike;
        
        Params() : period(15), printdata(true), printops(true), stocklike(true) {}
    };

private:
    Params params_;
    std::shared_ptr<Order> orderid_;
    std::shared_ptr<backtrader::indicators::SMA> sma_;
    std::shared_ptr<backtrader::indicators::CrossOver> cross_;
    
    std::vector<std::string> buycreate_;
    std::vector<std::string> sellcreate_;
    std::vector<std::string> buyexec_;
    std::vector<std::string> sellexec_;
    
    std::chrono::high_resolution_clock::time_point tstart_;

public:
    explicit RunStrategy(const Params& params = Params()) 
        : params_(params) {}

    void log(const std::string& txt, double dt = 0.0, bool nodate = false) {
        if (!nodate) {
            if (dt == 0.0) {
                dt = data(0)->datetime(0);
            }
            auto date = num2date(dt);
            std::cout << date << ", " << txt << std::endl;
        } else {
            std::cout << "---------- " << txt << std::endl;
        }
    }

    void notify_order(const Order& order) override {
        if (order.status == Order::Status::Submitted ||
            order.status == Order::Status::Accepted) {
            return;  // Await further notifications
        }

        if (order.status == Order::Status::Completed) {
            if (order.isbuy()) {
                if (params_.printops) {
                    std::ostringstream oss;
                    oss << "BUY, " << std::fixed << std::setprecision(2) 
                        << order.executed.price;
                    log(oss.str(), timepoint_to_double(order.executed.dt));
                }
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << order.executed.price;
                buyexec_.push_back(oss.str());
            } else {
                if (params_.printops) {
                    std::ostringstream oss;
                    oss << "SELL, " << std::fixed << std::setprecision(2) 
                        << order.executed.price;
                    log(oss.str(), timepoint_to_double(order.executed.dt));
                }
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << order.executed.price;
                sellexec_.push_back(oss.str());
            }
        } else if (order.status == Order::Status::Expired ||
                   order.status == Order::Status::Canceled ||
                   order.status == Order::Status::Margin) {
            if (params_.printops) {
                log(Order::status_string(order.status) + " ,");
            }
        }

        // Allow new orders
        orderid_ = nullptr;
    }

    void init() override {
        // Flag to allow new orders in the system or not
        orderid_ = nullptr;
        
        sma_ = std::make_shared<backtrader::indicators::SMA>(data(0), params_.period);
        cross_ = std::make_shared<backtrader::indicators::CrossOver>(data(0), sma_, true);
    }

    void start() override {
        if (!params_.stocklike) {
            broker_ptr()->setcommission(2.0, 10.0, 1000.0);
        }

        if (params_.printdata) {
            log("-------------------------", 0.0, true);
            std::ostringstream oss;
            oss << "Starting portfolio value: " << std::fixed << std::setprecision(2) 
                << broker_ptr()->getvalue();
            log(oss.str(), 0.0, true);
        }

        tstart_ = std::chrono::high_resolution_clock::now();
        
        buycreate_.clear();
        sellcreate_.clear();
        buyexec_.clear();
        sellexec_.clear();
    }

    void stop() override {
        auto tend = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(tend - tstart_);
        
        if (params_.printdata) {
            log("Time used: " + std::to_string(duration.count()) + " ms");
            
            std::ostringstream oss;
            oss << "Final portfolio value: " << std::fixed << std::setprecision(2) 
                << broker_ptr()->getvalue();
            log(oss.str());
            
            oss.str("");
            oss << "Final cash value: " << std::fixed << std::setprecision(2) 
                << broker_ptr()->getcash();
            log(oss.str());
            
            log("-------------------------");
        }
    }

    void next() override {
        if (params_.printdata) {
            std::ostringstream oss;
            oss << "Open, High, Low, Close, " 
                << std::fixed << std::setprecision(2)
                << data(0)->open(0) << ", "
                << data(0)->high(0) << ", "
                << data(0)->low(0) << ", "
                << data(0)->close(0) << ", Sma, "
                << sma_->get(0);
            log(oss.str());
            
            oss.str("");
            oss << "Close " << std::fixed << std::setprecision(2)
                << data(0)->close(0) << " - Sma "
                << sma_->get(0);
            log(oss.str());
        }

        if (orderid_) {
            // if an order is active, no new orders are allowed
            return;
        }

        if (!position() || !position()->size) {
            if (cross_->get(0) > 0.0) {
                if (params_.printops) {
                    std::ostringstream oss;
                    oss << "BUY CREATE , " << std::fixed << std::setprecision(2)
                        << data(0)->close(0);
                    log(oss.str());
                }

                orderid_ = buy();
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << data(0)->close(0);
                buycreate_.push_back(oss.str());
            }
        } else if (cross_->get(0) < 0.0) {
            if (params_.printops) {
                std::ostringstream oss;
                oss << "SELL CREATE , " << std::fixed << std::setprecision(2)
                    << data(0)->close(0);
                log(oss.str());
            }

            orderid_ = close();
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << data(0)->close(0);
            sellcreate_.push_back(oss.str());
        }
    }

    // Getters for testing
    const std::vector<std::string>& getBuyCreate() const { return buycreate_; }
    const std::vector<std::string>& getSellCreate() const { return sellcreate_; }
    const std::vector<std::string>& getBuyExec() const { return buyexec_; }
    const std::vector<std::string>& getSellExec() const { return sellexec_; }
};

// 测试TimeReturn分析器 - 年度收益
TEST(OriginalTests, AnalyzerTimeReturn_YearlyReturns) {
    const int chkdatas = 1;
    
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载数据
    auto data = getdata_feed(0);
    cerebro->adddata(data);
    
    // 设置策略参数
    RunStrategy::Params params;
    params.printdata = false;
    params.printops = false;
    params.stocklike = false;
    
    // 添加策略
    cerebro->addstrategy<RunStrategy>(params);
    
    // 添加TimeReturn分析器 - 年度时间框架
    cerebro->addanalyzer<backtrader::analyzers::TimeReturn>("TimeReturn", 
                                                TimeFrame::Years);
    
    // 运行回测
    auto results = cerebro->run();
    
    ASSERT_EQ(results.size(), 1) << "Should have exactly 1 strategy result";
    
    auto& strategy = results[0];
    auto timereturn_analyzer = strategy->getanalyzer<backtrader::TimeReturn>("TimeReturn");
    ASSERT_NE(timereturn_analyzer, nullptr) << "TimeReturn analyzer should exist";
    
    auto analysis = timereturn_analyzer->get_analysis();
    
    // 应该有至少一个年度收益记录
    auto returns_it = analysis.find("returns");
    ASSERT_NE(returns_it, analysis.end()) << "Analysis should have returns";
    auto returns_map = std::get<std::map<std::string, double>>(returns_it->second);
    EXPECT_FALSE(returns_map.empty()) 
        << "Should have at least one yearly return";
    
    if (!returns_map.empty()) {
        // 获取第一个年度收益
        auto first_return = returns_map.begin()->second;
        
        // 验证收益值
        // 原Python测试期望值：0.2795 或 0.2794999999999983
        EXPECT_NEAR(first_return, 0.2795, 0.0001) 
            << "First yearly return should be approximately 0.2795";
        
        // 打印调试信息
        std::cout << "First yearly return: " << std::fixed 
                  << std::setprecision(16) << first_return << std::endl;
    }
}

// 测试TimeReturn分析器 - 月度收益
TEST(OriginalTests, AnalyzerTimeReturn_MonthlyReturns) {
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载数据
    auto data = getdata_feed(0);
    cerebro->adddata(data);
    
    // 设置策略参数
    RunStrategy::Params params;
    params.printdata = false;
    params.printops = false;
    params.stocklike = false;
    
    // 添加策略
    cerebro->addstrategy<RunStrategy>(params);
    
    // 添加TimeReturn分析器 - 月度时间框架
    cerebro->addanalyzer<backtrader::analyzers::TimeReturn>("TimeReturn", 
                                                TimeFrame::Months);
    
    // 运行回测
    auto results = cerebro->run();
    
    auto& strategy = results[0];
    auto timereturn_analyzer = strategy->getanalyzer<backtrader::TimeReturn>("TimeReturn");
    auto analysis = timereturn_analyzer->get_analysis();
    
    // 月度收益应该比年度收益多
    auto returns_it = analysis.find("returns");
    ASSERT_NE(returns_it, analysis.end()) << "Analysis should have returns";
    auto returns_map = std::get<std::map<std::string, double>>(returns_it->second);
    EXPECT_GT(returns_map.size(), 1) 
        << "Should have multiple monthly returns";
    
    // 验证所有收益值都是有限的
    for (const auto& [date, ret] : returns_map) {
        EXPECT_TRUE(std::isfinite(ret)) 
            << "Return value should be finite";
        EXPECT_GE(ret, -1.0) 
            << "Return should not be less than -100%";
    }
}

// 测试TimeReturn分析器 - 日收益
TEST(OriginalTests, AnalyzerTimeReturn_DailyReturns) {
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载数据
    auto data = getdata_feed(0);
    cerebro->adddata(data);
    
    // 设置策略参数
    RunStrategy::Params params;
    params.printdata = false;
    params.printops = false;
    params.stocklike = false;
    
    // 添加策略
    cerebro->addstrategy<RunStrategy>(params);
    
    // 添加TimeReturn分析器 - 日时间框架
    cerebro->addanalyzer<backtrader::analyzers::TimeReturn>("TimeReturn", 
                                                TimeFrame::Days);
    
    // 运行回测
    auto results = cerebro->run();
    
    auto& strategy = results[0];
    auto timereturn_analyzer = strategy->getanalyzer<backtrader::TimeReturn>("TimeReturn");
    auto analysis = timereturn_analyzer->get_analysis();
    
    // 日收益应该最多
    auto returns_it = analysis.find("returns");
    ASSERT_NE(returns_it, analysis.end()) << "Analysis should have returns";
    auto returns_map = std::get<std::map<std::string, double>>(returns_it->second);
    EXPECT_GT(returns_map.size(), 10) 
        << "Should have many daily returns";
    
    // 计算一些统计数据
    if (!returns_map.empty()) {
        double sum_returns = 0.0;
        double max_return = -std::numeric_limits<double>::infinity();
        double min_return = std::numeric_limits<double>::infinity();
        
        for (const auto& [date, ret] : returns_map) {
            sum_returns += ret;
            max_return = std::max(max_return, ret);
            min_return = std::min(min_return, ret);
        }
        
        double avg_return = sum_returns / returns_map.size();
        
        std::cout << "Daily returns statistics:" << std::endl;
        std::cout << "  Count: " << returns_map.size() << std::endl;
        std::cout << "  Average: " << std::fixed << std::setprecision(6) 
                  << avg_return << std::endl;
        std::cout << "  Max: " << max_return << std::endl;
        std::cout << "  Min: " << min_return << std::endl;
        
        // 基本合理性检查
        EXPECT_GT(max_return, min_return) 
            << "Max return should be greater than min return";
        EXPECT_GE(avg_return, -0.1) 
            << "Average daily return should be reasonable";
        EXPECT_LE(avg_return, 0.1) 
            << "Average daily return should be reasonable";
    }
}

// 测试TimeReturn分析器 - 空策略
TEST(OriginalTests, AnalyzerTimeReturn_NoTrades) {
    // 创建一个不交易的策略
    class NoTradeStrategy : public backtrader::Strategy {
    public:
        void next() override {
            // 不做任何交易
        }
    };
    
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载数据
    auto data = getdata_feed(0);
    cerebro->adddata(data);
    
    // 添加不交易的策略
    cerebro->addstrategy<NoTradeStrategy>();
    
    // 添加TimeReturn分析器
    cerebro->addanalyzer<backtrader::analyzers::TimeReturn>("TimeReturn", 
                                                TimeFrame::Years);
    
    // 运行回测
    auto results = cerebro->run();
    
    auto& strategy = results[0];
    auto timereturn_analyzer = strategy->getanalyzer<backtrader::TimeReturn>("TimeReturn");
    auto analysis = timereturn_analyzer->get_analysis();
    
    // 即使没有交易，也应该有收益记录（都是0）
    auto returns_it = analysis.find("returns");
    ASSERT_NE(returns_it, analysis.end()) << "Analysis should have returns";
    auto returns_map = std::get<std::map<std::string, double>>(returns_it->second);
    EXPECT_FALSE(returns_map.empty()) 
        << "Should have return records even without trades";
    
    // 所有收益应该是0
    for (const auto& [date, ret] : returns_map) {
        EXPECT_DOUBLE_EQ(ret, 0.0) 
            << "Return should be 0 without trades";
    }
}

// 测试TimeReturn分析器 - 多时间框架
TEST(OriginalTests, AnalyzerTimeReturn_MultipleTimeframes) {
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载数据
    auto data = getdata_feed(0);
    cerebro->adddata(data);
    
    // 设置策略参数
    RunStrategy::Params params;
    params.printdata = false;
    params.printops = false;
    params.stocklike = false;
    
    // 添加策略
    cerebro->addstrategy<RunStrategy>(params);
    
    // 添加多个TimeReturn分析器
    cerebro->addanalyzer<backtrader::analyzers::TimeReturn>("YearlyReturn", 
                                                TimeFrame::Years);
    cerebro->addanalyzer<backtrader::analyzers::TimeReturn>("MonthlyReturn", 
                                                TimeFrame::Months);
    cerebro->addanalyzer<backtrader::analyzers::TimeReturn>("DailyReturn", 
                                                TimeFrame::Days);
    
    // 运行回测
    auto results = cerebro->run();
    
    auto& strategy = results[0];
    
    auto yearly_analyzer = strategy->getanalyzer<backtrader::TimeReturn>("YearlyReturn");
    auto monthly_analyzer = strategy->getanalyzer<backtrader::TimeReturn>("MonthlyReturn");
    auto daily_analyzer = strategy->getanalyzer<backtrader::TimeReturn>("DailyReturn");
    
    auto yearly_analysis = yearly_analyzer->get_analysis();
    auto monthly_analysis = monthly_analyzer->get_analysis();
    auto daily_analysis = daily_analyzer->get_analysis();
    
    // 验证不同时间框架的收益数量关系
    auto yearly_returns_it = yearly_analysis.find("returns");
    auto monthly_returns_it = monthly_analysis.find("returns");
    auto daily_returns_it = daily_analysis.find("returns");
    
    ASSERT_NE(yearly_returns_it, yearly_analysis.end()) << "Yearly analysis should have returns";
    ASSERT_NE(monthly_returns_it, monthly_analysis.end()) << "Monthly analysis should have returns";
    ASSERT_NE(daily_returns_it, daily_analysis.end()) << "Daily analysis should have returns";
    
    auto yearly_returns_map = std::get<std::map<std::string, double>>(yearly_returns_it->second);
    auto monthly_returns_map = std::get<std::map<std::string, double>>(monthly_returns_it->second);
    auto daily_returns_map = std::get<std::map<std::string, double>>(daily_returns_it->second);
    
    EXPECT_LE(yearly_returns_map.size(), monthly_returns_map.size()) 
        << "Yearly returns should be <= monthly returns";
    EXPECT_LE(monthly_returns_map.size(), daily_returns_map.size()) 
        << "Monthly returns should be <= daily returns";
    
    std::cout << "Return counts by timeframe:" << std::endl;
    std::cout << "  Yearly: " << yearly_returns_map.size() << std::endl;
    std::cout << "  Monthly: " << monthly_returns_map.size() << std::endl;
    std::cout << "  Daily: " << daily_returns_map.size() << std::endl;
}

// 性能测试
TEST(OriginalTests, AnalyzerTimeReturn_Performance) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载数据
    auto data = getdata_feed(0);
    cerebro->adddata(data);
    
    // 设置策略参数
    RunStrategy::Params params;
    params.printdata = false;
    params.printops = false;
    params.stocklike = false;
    
    // 添加策略
    cerebro->addstrategy<RunStrategy>(params);
    
    // 添加多个时间框架的分析器
    cerebro->addanalyzer<backtrader::analyzers::TimeReturn>("Daily", TimeFrame::Days);
    cerebro->addanalyzer<backtrader::analyzers::TimeReturn>("Weekly", TimeFrame::Weeks);
    cerebro->addanalyzer<backtrader::analyzers::TimeReturn>("Monthly", TimeFrame::Months);
    cerebro->addanalyzer<backtrader::analyzers::TimeReturn>("Yearly", TimeFrame::Years);
    
    // 运行回测
    auto results = cerebro->run();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "TimeReturn analyzer test with multiple timeframes took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证所有分析器都正常工作
    auto& strategy = results[0];
    
    for (const auto& name : {"Daily", "Weekly", "Monthly", "Yearly"}) {
        auto analyzer = strategy->getanalyzer<backtrader::TimeReturn>(name);
        ASSERT_NE(analyzer, nullptr) 
            << "Analyzer " << name << " should exist";
        
        auto analysis = analyzer->get_analysis();
        auto returns_it = analysis.find("returns");
        ASSERT_NE(returns_it, analysis.end()) << "Analysis should have returns";
        auto returns_map = std::get<std::map<std::string, double>>(returns_it->second);
        EXPECT_FALSE(returns_map.empty()) 
            << "Analyzer " << name << " should have returns";
    }
    
    // 性能要求
    EXPECT_LT(duration.count(), 5000) 
        << "Performance test should complete within 5 seconds";
}