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
#include "analyzers/TimeReturn.h"
#include "strategy/Strategy.h"
#include "indicators/SMA.h"
#include "indicators/CrossOver.h"
#include "cerebro/Cerebro.h"
#include <chrono>
#include <iomanip>

using namespace backtrader;
using namespace backtrader::tests::original;

class RunStrategy : public Strategy {
public:
    struct Params {
        int period = 15;
        bool printdata = true;
        bool printops = true;
        bool stocklike = true;
    };

private:
    Params params_;
    std::shared_ptr<Order> orderid_;
    std::shared_ptr<indicators::SMA> sma_;
    std::shared_ptr<indicators::CrossOver> cross_;
    
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
                    log(oss.str(), order.executed.dt);
                }
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << order.executed.price;
                buyexec_.push_back(oss.str());
            } else {
                if (params_.printops) {
                    std::ostringstream oss;
                    oss << "SELL, " << std::fixed << std::setprecision(2) 
                        << order.executed.price;
                    log(oss.str(), order.executed.dt);
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
        
        sma_ = std::make_shared<indicators::SMA>(data(0), params_.period);
        cross_ = std::make_shared<indicators::CrossOver>(data(0)->close(), sma_, true);
    }

    void start() override {
        if (!params_.stocklike) {
            broker()->setcommission(2.0, 10.0, 1000.0);
        }

        if (params_.printdata) {
            log("-------------------------", 0.0, true);
            std::ostringstream oss;
            oss << "Starting portfolio value: " << std::fixed << std::setprecision(2) 
                << broker()->getvalue();
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
                << broker()->getvalue();
            log(oss.str());
            
            oss.str("");
            oss << "Final cash value: " << std::fixed << std::setprecision(2) 
                << broker()->getcash();
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

        if (!position()->size) {
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
    
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    
    // 加载数据
    auto data = getdata(0);
    cerebro->adddata(data);
    
    // 设置策略参数
    RunStrategy::Params params;
    params.printdata = false;
    params.printops = false;
    params.stocklike = false;
    
    // 添加策略
    cerebro->addstrategy<RunStrategy>(params);
    
    // 添加TimeReturn分析器 - 年度时间框架
    cerebro->addanalyzer<analyzers::TimeReturn>("TimeReturn", 
                                                TimeFrame::Years);
    
    // 运行回测
    auto results = cerebro->run();
    
    ASSERT_EQ(results.size(), 1) << "Should have exactly 1 strategy result";
    
    auto& strategy = results[0];
    auto timereturn_analyzer = strategy->getanalyzer("TimeReturn");
    ASSERT_NE(timereturn_analyzer, nullptr) << "TimeReturn analyzer should exist";
    
    auto analysis = timereturn_analyzer->get_analysis();
    
    // 应该有至少一个年度收益记录
    EXPECT_FALSE(analysis.returns.empty()) 
        << "Should have at least one yearly return";
    
    if (!analysis.returns.empty()) {
        // 获取第一个年度收益
        auto first_return = analysis.returns.begin()->second;
        
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
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    
    // 加载数据
    auto data = getdata(0);
    cerebro->adddata(data);
    
    // 设置策略参数
    RunStrategy::Params params;
    params.printdata = false;
    params.printops = false;
    params.stocklike = false;
    
    // 添加策略
    cerebro->addstrategy<RunStrategy>(params);
    
    // 添加TimeReturn分析器 - 月度时间框架
    cerebro->addanalyzer<analyzers::TimeReturn>("TimeReturn", 
                                                TimeFrame::Months);
    
    // 运行回测
    auto results = cerebro->run();
    
    auto& strategy = results[0];
    auto timereturn_analyzer = strategy->getanalyzer("TimeReturn");
    auto analysis = timereturn_analyzer->get_analysis();
    
    // 月度收益应该比年度收益多
    EXPECT_GT(analysis.returns.size(), 1) 
        << "Should have multiple monthly returns";
    
    // 验证所有收益值都是有限的
    for (const auto& [date, ret] : analysis.returns) {
        EXPECT_TRUE(std::isfinite(ret)) 
            << "Return value should be finite";
        EXPECT_GE(ret, -1.0) 
            << "Return should not be less than -100%";
    }
}

// 测试TimeReturn分析器 - 日收益
TEST(OriginalTests, AnalyzerTimeReturn_DailyReturns) {
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    
    // 加载数据
    auto data = getdata(0);
    cerebro->adddata(data);
    
    // 设置策略参数
    RunStrategy::Params params;
    params.printdata = false;
    params.printops = false;
    params.stocklike = false;
    
    // 添加策略
    cerebro->addstrategy<RunStrategy>(params);
    
    // 添加TimeReturn分析器 - 日时间框架
    cerebro->addanalyzer<analyzers::TimeReturn>("TimeReturn", 
                                                TimeFrame::Days);
    
    // 运行回测
    auto results = cerebro->run();
    
    auto& strategy = results[0];
    auto timereturn_analyzer = strategy->getanalyzer("TimeReturn");
    auto analysis = timereturn_analyzer->get_analysis();
    
    // 日收益应该最多
    EXPECT_GT(analysis.returns.size(), 10) 
        << "Should have many daily returns";
    
    // 计算一些统计数据
    if (!analysis.returns.empty()) {
        double sum_returns = 0.0;
        double max_return = -std::numeric_limits<double>::infinity();
        double min_return = std::numeric_limits<double>::infinity();
        
        for (const auto& [date, ret] : analysis.returns) {
            sum_returns += ret;
            max_return = std::max(max_return, ret);
            min_return = std::min(min_return, ret);
        }
        
        double avg_return = sum_returns / analysis.returns.size();
        
        std::cout << "Daily returns statistics:" << std::endl;
        std::cout << "  Count: " << analysis.returns.size() << std::endl;
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
    class NoTradeStrategy : public Strategy {
    public:
        void next() override {
            // 不做任何交易
        }
    };
    
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    
    // 加载数据
    auto data = getdata(0);
    cerebro->adddata(data);
    
    // 添加不交易的策略
    cerebro->addstrategy<NoTradeStrategy>();
    
    // 添加TimeReturn分析器
    cerebro->addanalyzer<analyzers::TimeReturn>("TimeReturn", 
                                                TimeFrame::Years);
    
    // 运行回测
    auto results = cerebro->run();
    
    auto& strategy = results[0];
    auto timereturn_analyzer = strategy->getanalyzer("TimeReturn");
    auto analysis = timereturn_analyzer->get_analysis();
    
    // 即使没有交易，也应该有收益记录（都是0）
    EXPECT_FALSE(analysis.returns.empty()) 
        << "Should have return records even without trades";
    
    // 所有收益应该是0
    for (const auto& [date, ret] : analysis.returns) {
        EXPECT_DOUBLE_EQ(ret, 0.0) 
            << "Return should be 0 without trades";
    }
}

// 测试TimeReturn分析器 - 多时间框架
TEST(OriginalTests, AnalyzerTimeReturn_MultipleTimeframes) {
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    
    // 加载数据
    auto data = getdata(0);
    cerebro->adddata(data);
    
    // 设置策略参数
    RunStrategy::Params params;
    params.printdata = false;
    params.printops = false;
    params.stocklike = false;
    
    // 添加策略
    cerebro->addstrategy<RunStrategy>(params);
    
    // 添加多个TimeReturn分析器
    cerebro->addanalyzer<analyzers::TimeReturn>("YearlyReturn", 
                                                TimeFrame::Years);
    cerebro->addanalyzer<analyzers::TimeReturn>("MonthlyReturn", 
                                                TimeFrame::Months);
    cerebro->addanalyzer<analyzers::TimeReturn>("DailyReturn", 
                                                TimeFrame::Days);
    
    // 运行回测
    auto results = cerebro->run();
    
    auto& strategy = results[0];
    
    auto yearly_analyzer = strategy->getanalyzer("YearlyReturn");
    auto monthly_analyzer = strategy->getanalyzer("MonthlyReturn");
    auto daily_analyzer = strategy->getanalyzer("DailyReturn");
    
    auto yearly_analysis = yearly_analyzer->get_analysis();
    auto monthly_analysis = monthly_analyzer->get_analysis();
    auto daily_analysis = daily_analyzer->get_analysis();
    
    // 验证不同时间框架的收益数量关系
    EXPECT_LE(yearly_analysis.returns.size(), monthly_analysis.returns.size()) 
        << "Yearly returns should be <= monthly returns";
    EXPECT_LE(monthly_analysis.returns.size(), daily_analysis.returns.size()) 
        << "Monthly returns should be <= daily returns";
    
    std::cout << "Return counts by timeframe:" << std::endl;
    std::cout << "  Yearly: " << yearly_analysis.returns.size() << std::endl;
    std::cout << "  Monthly: " << monthly_analysis.returns.size() << std::endl;
    std::cout << "  Daily: " << daily_analysis.returns.size() << std::endl;
}

// 性能测试
TEST(OriginalTests, AnalyzerTimeReturn_Performance) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    
    // 加载数据
    auto data = getdata(0);
    cerebro->adddata(data);
    
    // 设置策略参数
    RunStrategy::Params params;
    params.printdata = false;
    params.printops = false;
    params.stocklike = false;
    
    // 添加策略
    cerebro->addstrategy<RunStrategy>(params);
    
    // 添加多个时间框架的分析器
    cerebro->addanalyzer<analyzers::TimeReturn>("Daily", TimeFrame::Days);
    cerebro->addanalyzer<analyzers::TimeReturn>("Weekly", TimeFrame::Weeks);
    cerebro->addanalyzer<analyzers::TimeReturn>("Monthly", TimeFrame::Months);
    cerebro->addanalyzer<analyzers::TimeReturn>("Yearly", TimeFrame::Years);
    
    // 运行回测
    auto results = cerebro->run();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "TimeReturn analyzer test with multiple timeframes took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证所有分析器都正常工作
    auto& strategy = results[0];
    
    for (const auto& name : {"Daily", "Weekly", "Monthly", "Yearly"}) {
        auto analyzer = strategy->getanalyzer(name);
        ASSERT_NE(analyzer, nullptr) 
            << "Analyzer " << name << " should exist";
        
        auto analysis = analyzer->get_analysis();
        EXPECT_FALSE(analysis.returns.empty()) 
            << "Analyzer " << name << " should have returns";
    }
    
    // 性能要求
    EXPECT_LT(duration.count(), 5000) 
        << "Performance test should complete within 5 seconds";
}