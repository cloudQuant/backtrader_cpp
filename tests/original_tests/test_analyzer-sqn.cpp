/**
 * @file test_analyzer-sqn.cpp
 * @brief SQN (System Quality Number) Analyzer测试 - 对应Python test_analyzer-sqn.py
 * 
 * 原始Python测试:
 * - 测试SQN分析器功能
 * - 测试不同交易次数限制(None, 0, 1)
 * - 验证计算结果
 */

#include "test_common.h"
#include "analyzers/sqn.h"
#include "strategy.h"
#include "indicators/sma.h"
#include "indicators/crossover.h"
#include "cerebro/Cerebro.h"
#include <chrono>
#include <iomanip>

using namespace backtrader::indicators;
using namespace backtrader::tests::original;

class RunStrategy : public Strategy {
public:
    struct Params {
        int period = 15;
        int maxtrades = -1;  // -1 means None/unlimited
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
    int tradecount_ = 0;
    
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

    void notify_trade(const Trade& trade) override {
        if (trade.isclosed) {
            tradecount_++;
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
        tradecount_ = 0;
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
            if (params_.maxtrades < 0 || tradecount_ < params_.maxtrades) {
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
    int getTradeCount() const { return tradecount_; }
};

// 测试SQN分析器
TEST(OriginalTests, AnalyzerSQN_BasicTest) {
    const int chkdatas = 1;
    
    // 测试不同的最大交易次数限制
    std::vector<int> maxtrades_values = {-1, 0, 1};  // -1 means None/unlimited
    
    for (int maxtrades : maxtrades_values) {
        // 创建Cerebro
        auto cerebro = std::make_unique<Cerebro>();
        
        // 加载数据
        auto data = getdata(0);
        cerebro->adddata(data);
        
        // 设置策略参数
        RunStrategy::Params params;
        params.maxtrades = maxtrades;
        params.printdata = false;
        params.printops = false;
        params.stocklike = false;
        
        // 添加策略
        cerebro->addstrategy<RunStrategy>(params);
        
        // 添加SQN分析器
        cerebro->addanalyzer<analyzers::SQN>("SQN");
        
        // 运行回测
        auto results = cerebro->run();
        
        ASSERT_EQ(results.size(), 1) << "Should have exactly 1 strategy result";
        
        auto& strategy = results[0];
        auto sqn_analyzer = strategy->getanalyzer("SQN");
        ASSERT_NE(sqn_analyzer, nullptr) << "SQN analyzer should exist";
        
        auto analysis = sqn_analyzer->get_analysis();
        
        // 验证结果
        if (maxtrades == 0 || maxtrades == 1) {
            EXPECT_DOUBLE_EQ(analysis.sqn, 0.0) 
                << "SQN should be 0 for " << maxtrades << " trades";
            EXPECT_EQ(analysis.trades, maxtrades) 
                << "Trade count should match maxtrades";
        } else {
            // 处理不同精度
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(12) << analysis.sqn;
            std::string sqn_str = oss.str();
            EXPECT_EQ(sqn_str.substr(0, 14), "0.912550316439") 
                << "SQN value mismatch: " << sqn_str;
            EXPECT_EQ(analysis.trades, 11) 
                << "Should have 11 trades for unlimited trading";
        }
    }
}

// 测试SQN分析器详细功能
TEST(OriginalTests, AnalyzerSQN_DetailedTest) {
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    
    // 加载数据
    auto data = getdata(0);
    cerebro->adddata(data);
    
    // 设置策略参数
    RunStrategy::Params params;
    params.maxtrades = -1;  // unlimited
    params.printdata = false;
    params.printops = false;
    params.stocklike = false;
    
    // 添加策略
    cerebro->addstrategy<RunStrategy>(params);
    
    // 添加SQN分析器
    cerebro->addanalyzer<analyzers::SQN>("SQN");
    
    // 运行回测
    auto results = cerebro->run();
    
    ASSERT_EQ(results.size(), 1) << "Should have exactly 1 strategy result";
    
    auto& strategy = results[0];
    auto sqn_analyzer = strategy->getanalyzer("SQN");
    ASSERT_NE(sqn_analyzer, nullptr) << "SQN analyzer should exist";
    
    auto analysis = sqn_analyzer->get_analysis();
    
    // 验证SQN值范围
    EXPECT_GT(analysis.sqn, -10.0) << "SQN should be reasonable";
    EXPECT_LT(analysis.sqn, 10.0) << "SQN should be reasonable";
    
    // 验证交易次数
    EXPECT_GT(analysis.trades, 0) << "Should have some trades";
    
    // 验证其他统计数据
    if (analysis.trades > 0) {
        EXPECT_TRUE(std::isfinite(analysis.sqn)) << "SQN should be finite";
    }
}

// 测试空交易的SQN
TEST(OriginalTests, AnalyzerSQN_NoTrades) {
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    
    // 加载数据
    auto data = getdata(0);
    cerebro->adddata(data);
    
    // 设置策略参数 - 不允许交易
    RunStrategy::Params params;
    params.maxtrades = 0;
    params.printdata = false;
    params.printops = false;
    params.stocklike = false;
    
    // 添加策略
    cerebro->addstrategy<RunStrategy>(params);
    
    // 添加SQN分析器
    cerebro->addanalyzer<analyzers::SQN>("SQN");
    
    // 运行回测
    auto results = cerebro->run();
    
    auto& strategy = results[0];
    auto sqn_analyzer = strategy->getanalyzer("SQN");
    auto analysis = sqn_analyzer->get_analysis();
    
    // 没有交易时，SQN应该是0
    EXPECT_DOUBLE_EQ(analysis.sqn, 0.0) << "SQN should be 0 with no trades";
    EXPECT_EQ(analysis.trades, 0) << "Should have 0 trades";
}

// 测试单次交易的SQN
TEST(OriginalTests, AnalyzerSQN_SingleTrade) {
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    
    // 加载数据
    auto data = getdata(0);
    cerebro->adddata(data);
    
    // 设置策略参数 - 只允许一次交易
    RunStrategy::Params params;
    params.maxtrades = 1;
    params.printdata = false;
    params.printops = false;
    params.stocklike = false;
    
    // 添加策略
    cerebro->addstrategy<RunStrategy>(params);
    
    // 添加SQN分析器
    cerebro->addanalyzer<analyzers::SQN>("SQN");
    
    // 运行回测
    auto results = cerebro->run();
    
    auto& strategy = results[0];
    auto sqn_analyzer = strategy->getanalyzer("SQN");
    auto analysis = sqn_analyzer->get_analysis();
    
    // 单次交易时，标准差为0，SQN应该是0
    EXPECT_DOUBLE_EQ(analysis.sqn, 0.0) << "SQN should be 0 with single trade";
    EXPECT_EQ(analysis.trades, 1) << "Should have exactly 1 trade";
}

// 性能测试
TEST(OriginalTests, AnalyzerSQN_Performance) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 创建Cerebro
    auto cerebro = std::make_unique<Cerebro>();
    
    // 加载数据
    auto data = getdata(0);
    cerebro->adddata(data);
    
    // 设置策略参数
    RunStrategy::Params params;
    params.maxtrades = -1;
    params.printdata = false;
    params.printops = false;
    params.stocklike = false;
    
    // 添加策略
    cerebro->addstrategy<RunStrategy>(params);
    
    // 添加多个分析器
    cerebro->addanalyzer<analyzers::SQN>("SQN1");
    cerebro->addanalyzer<analyzers::SQN>("SQN2");
    cerebro->addanalyzer<analyzers::SQN>("SQN3");
    
    // 运行回测
    auto results = cerebro->run();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "SQN analyzer test with multiple analyzers took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证所有分析器都产生相同结果
    auto& strategy = results[0];
    auto sqn1 = strategy->getanalyzer("SQN1");
    auto sqn2 = strategy->getanalyzer("SQN2");
    auto sqn3 = strategy->getanalyzer("SQN3");
    
    auto analysis1 = sqn1->get_analysis();
    auto analysis2 = sqn2->get_analysis();
    auto analysis3 = sqn3->get_analysis();
    
    EXPECT_DOUBLE_EQ(analysis1.sqn, analysis2.sqn) 
        << "All SQN analyzers should produce same result";
    EXPECT_DOUBLE_EQ(analysis2.sqn, analysis3.sqn) 
        << "All SQN analyzers should produce same result";
    
    // 性能要求
    EXPECT_LT(duration.count(), 5000) 
        << "Performance test should complete within 5 seconds";
}