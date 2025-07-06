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
        int maxtrades;  // -1 means None/unlimited
        bool printdata;
        bool printops;
        bool stocklike;
        
        Params() : period(15), maxtrades(-1), printdata(true), printops(true), stocklike(true) {}
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
        if (trade.isclosed()) {
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
        tradecount_ = 0;
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
        // 创建backtrader::Cerebro
        auto cerebro = std::make_unique<backtrader::Cerebro>();
        
        // 加载数据
        auto data = getdata_feed(0);
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
        cerebro->addanalyzer<backtrader::analyzers::SQN>("SQN");
        
        // 运行回测
        auto results = cerebro->run();
        
        ASSERT_EQ(results.size(), 1) << "Should have exactly 1 strategy result";
        
        auto& strategy = results[0];
        auto sqn_analyzer = strategy->getanalyzer<backtrader::SQN>("SQN");
        ASSERT_NE(sqn_analyzer, nullptr) << "SQN analyzer should exist";
        
        auto analysis = sqn_analyzer->get_analysis();
        
        // 验证结果
        auto sqn_it = analysis.find("sqn");
        auto trades_it = analysis.find("trades");
        ASSERT_NE(sqn_it, analysis.end()) << "Analysis should have sqn";
        ASSERT_NE(trades_it, analysis.end()) << "Analysis should have trades";
        
        double sqn_value = std::get<double>(sqn_it->second);
        int trades_value = std::get<int>(trades_it->second);
        
        if (maxtrades == 0 || maxtrades == 1) {
            EXPECT_DOUBLE_EQ(sqn_value, 0.0) 
                << "SQN should be 0 for " << maxtrades << " trades";
            EXPECT_EQ(trades_value, maxtrades) 
                << "Trade count should match maxtrades";
        } else {
            // 处理不同精度
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(12) << sqn_value;
            std::string sqn_str = oss.str();
            EXPECT_EQ(sqn_str.substr(0, 14), "0.912550316439") 
                << "SQN value mismatch: " << sqn_str;
            EXPECT_EQ(trades_value, 11) 
                << "Should have 11 trades for unlimited trading";
        }
    }
}

// 测试SQN分析器详细功能
TEST(OriginalTests, AnalyzerSQN_DetailedTest) {
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载数据
    auto data = getdata_feed(0);
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
    cerebro->addanalyzer<backtrader::analyzers::SQN>("SQN");
    
    // 运行回测
    auto results = cerebro->run();
    
    ASSERT_EQ(results.size(), 1) << "Should have exactly 1 strategy result";
    
    auto& strategy = results[0];
    auto sqn_analyzer = strategy->getanalyzer<backtrader::SQN>("SQN");
    ASSERT_NE(sqn_analyzer, nullptr) << "SQN analyzer should exist";
    
    auto analysis = sqn_analyzer->get_analysis();
    
    // 验证SQN值范围
    auto sqn_it = analysis.find("sqn");
    auto trades_it = analysis.find("trades");
    ASSERT_NE(sqn_it, analysis.end()) << "Analysis should have sqn";
    ASSERT_NE(trades_it, analysis.end()) << "Analysis should have trades";
    
    double sqn_value = std::get<double>(sqn_it->second);
    int trades_value = std::get<int>(trades_it->second);
    
    EXPECT_GT(sqn_value, -10.0) << "SQN should be reasonable";
    EXPECT_LT(sqn_value, 10.0) << "SQN should be reasonable";
    
    // 验证交易次数
    EXPECT_GT(trades_value, 0) << "Should have some trades";
    
    // 验证其他统计数据
    if (trades_value > 0) {
        EXPECT_TRUE(std::isfinite(sqn_value)) << "SQN should be finite";
    }
}

// 测试空交易的SQN
TEST(OriginalTests, AnalyzerSQN_NoTrades) {
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载数据
    auto data = getdata_feed(0);
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
    cerebro->addanalyzer<backtrader::analyzers::SQN>("SQN");
    
    // 运行回测
    auto results = cerebro->run();
    
    auto& strategy = results[0];
    auto sqn_analyzer = strategy->getanalyzer<backtrader::SQN>("SQN");
    auto analysis = sqn_analyzer->get_analysis();
    
    // 没有交易时，SQN应该是0
    auto sqn_it = analysis.find("sqn");
    auto trades_it = analysis.find("trades");
    ASSERT_NE(sqn_it, analysis.end()) << "Analysis should have sqn";
    ASSERT_NE(trades_it, analysis.end()) << "Analysis should have trades";
    
    double sqn_value = std::get<double>(sqn_it->second);
    int trades_value = std::get<int>(trades_it->second);
    
    EXPECT_DOUBLE_EQ(sqn_value, 0.0) << "SQN should be 0 with no trades";
    EXPECT_EQ(trades_value, 0) << "Should have 0 trades";
}

// 测试单次交易的SQN
TEST(OriginalTests, AnalyzerSQN_SingleTrade) {
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载数据
    auto data = getdata_feed(0);
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
    cerebro->addanalyzer<backtrader::analyzers::SQN>("SQN");
    
    // 运行回测
    auto results = cerebro->run();
    
    auto& strategy = results[0];
    auto sqn_analyzer = strategy->getanalyzer<backtrader::SQN>("SQN");
    auto analysis = sqn_analyzer->get_analysis();
    
    // 单次交易时，标准差为0，SQN应该是0
    auto sqn_it = analysis.find("sqn");
    auto trades_it = analysis.find("trades");
    ASSERT_NE(sqn_it, analysis.end()) << "Analysis should have sqn";
    ASSERT_NE(trades_it, analysis.end()) << "Analysis should have trades";
    
    double sqn_value = std::get<double>(sqn_it->second);
    int trades_value = std::get<int>(trades_it->second);
    
    EXPECT_DOUBLE_EQ(sqn_value, 0.0) << "SQN should be 0 with single trade";
    EXPECT_EQ(trades_value, 1) << "Should have exactly 1 trade";
}

// 性能测试
TEST(OriginalTests, AnalyzerSQN_Performance) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 创建backtrader::Cerebro
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    // 加载数据
    auto data = getdata_feed(0);
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
    cerebro->addanalyzer<backtrader::analyzers::SQN>("SQN1");
    cerebro->addanalyzer<backtrader::analyzers::SQN>("SQN2");
    cerebro->addanalyzer<backtrader::analyzers::SQN>("SQN3");
    
    // 运行回测
    auto results = cerebro->run();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "SQN analyzer test with multiple analyzers took " 
              << duration.count() << " ms" << std::endl;
    
    // 验证所有分析器都产生相同结果
    auto& strategy = results[0];
    auto sqn1 = strategy->getanalyzer<backtrader::SQN>("SQN1");
    auto sqn2 = strategy->getanalyzer<backtrader::SQN>("SQN2");
    auto sqn3 = strategy->getanalyzer<backtrader::SQN>("SQN3");
    
    auto analysis1 = sqn1->get_analysis();
    auto analysis2 = sqn2->get_analysis();
    auto analysis3 = sqn3->get_analysis();
    
    auto sqn1_it = analysis1.find("sqn");
    auto sqn2_it = analysis2.find("sqn");
    auto sqn3_it = analysis3.find("sqn");
    
    ASSERT_NE(sqn1_it, analysis1.end()) << "Analysis1 should have sqn";
    ASSERT_NE(sqn2_it, analysis2.end()) << "Analysis2 should have sqn";
    ASSERT_NE(sqn3_it, analysis3.end()) << "Analysis3 should have sqn";
    
    double sqn1_value = std::get<double>(sqn1_it->second);
    double sqn2_value = std::get<double>(sqn2_it->second);
    double sqn3_value = std::get<double>(sqn3_it->second);
    
    EXPECT_DOUBLE_EQ(sqn1_value, sqn2_value) 
        << "All SQN analyzers should produce same result";
    EXPECT_DOUBLE_EQ(sqn2_value, sqn3_value) 
        << "All SQN analyzers should produce same result";
    
    // 性能要求
    EXPECT_LT(duration.count(), 5000) 
        << "Performance test should complete within 5 seconds";
}