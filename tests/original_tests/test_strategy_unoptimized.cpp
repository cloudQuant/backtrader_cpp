/**
 * @file test_strategy_unoptimized.cpp
 * @brief 非优化策略测试 - 对应Python test_strategy_unoptimized.py
 * 
 * 原始Python测试:
 * - 测试基本策略执行（非优化模式）
 * - 验证买卖信号的创建和执行价格
 * - 测试股票和期货两种模式下的策略表现
 * - 期望值: 固定的买卖价格数组和最终资产值
 */

#include "test_common.h"
#include "strategy.h"
#include "cerebro.h"
#include "indicators/sma.h"
#include "indicators/crossover.h"
#include "broker.h"
#include "order.h"
#include <memory>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <chrono>

using namespace backtrader::indicators;
using namespace backtrader::tests::original;

// 期望的买卖价格
const std::vector<std::string> EXPECTED_BUY_CREATE = {
    "3641.42", "3798.46", "3874.61", "3860.00", "3843.08", "3648.33",
    "3526.84", "3632.93", "3788.96", "3841.31", "4045.22", "4052.89"
};

const std::vector<std::string> EXPECTED_SELL_CREATE = {
    "3763.73", "3811.45", "3823.11", "3821.97", "3837.86", "3604.33",
    "3562.56", "3772.21", "3780.18", "3974.62", "4048.16"
};

const std::vector<std::string> EXPECTED_BUY_EXEC = {
    "3643.35", "3801.03", "3872.37", "3863.57", "3845.32", "3656.43",
    "3542.65", "3639.65", "3799.86", "3840.20", "4047.63", "4052.55"
};

const std::vector<std::string> EXPECTED_SELL_EXEC = {
    "3763.95", "3811.85", "3822.35", "3822.57", "3829.82", "3598.58",
    "3545.92", "3766.80", "3782.15", "3979.73", "4045.05"
};

// 非优化策略类
class UnoptimizedRunStrategy : public backtrader::Strategy {
private:
    int period_;
    bool print_data_;
    bool print_ops_;
    bool stock_like_;
    std::shared_ptr<Order> order_id_;
    std::shared_ptr<SMA> sma_;
    std::shared_ptr<CrossOver> cross_;
    std::chrono::high_resolution_clock::time_point start_time_;

public:
    // 记录买卖价格的向量
    std::vector<std::string> buy_create_;
    std::vector<std::string> sell_create_;
    std::vector<std::string> buy_exec_;
    std::vector<std::string> sell_exec_;

    struct Params {
        int period;
        bool printdata;
        bool printops;
        bool stocklike;
        
        Params() : period(15), printdata(true), printops(true), stocklike(true) {}
    };

    explicit UnoptimizedRunStrategy(const Params& params = {}) 
        : period_(params.period), print_data_(params.printdata), 
          print_ops_(params.printops), stock_like_(params.stocklike) {}

    void log(const std::string& txt, double dt = 0.0, bool nodate = false) {
        if (nodate) {
            std::cout << "---------- " << txt << std::endl;
        } else {
            if (dt == 0.0) {
                dt = data(0)->datetime(0);
            }
            std::string date_str = num2date(dt);
            std::cout << date_str << ", " << txt << std::endl;
        }
    }

    void notify_order(std::shared_ptr<Order> order) override {
        static int notify_count = 0;
        notify_count++;
        
        // 记录所有订单通知
        // if (notify_count <= 50 || order->status == OrderStatus::Completed) {
        //     std::cout << "notify_order #" << notify_count << " status=" << static_cast<int>(order->status) 
        //               << " (" << order->status_string(order->status) << ")"
        //               << ", size=" << order->size
        //               << ", exec_price=" << order->executed.price << std::endl;
        // }
        
        if (order->status == OrderStatus::Submitted || 
            order->status == OrderStatus::Accepted) {
            return;  // 等待进一步通知
        }

        if (order->status == OrderStatus::Completed) {
            if (order->type == OrderType::Market && order->size > 0) {
                if (print_ops_) {
                    std::ostringstream oss;
                    oss << "BUY, " << std::fixed << std::setprecision(2) 
                        << order->executed.price;
                    log(oss.str());
                }
                
                std::ostringstream price_ss;
                price_ss << std::fixed << std::setprecision(2) << order->executed.price;
                buy_exec_.push_back(price_ss.str());
            } else {  // 卖出订单
                if (print_ops_) {
                    std::ostringstream oss;
                    oss << "SELL, " << std::fixed << std::setprecision(2) 
                        << order->executed.price;
                    log(oss.str());
                }
                
                std::ostringstream price_ss;
                price_ss << std::fixed << std::setprecision(2) << order->executed.price;
                sell_exec_.push_back(price_ss.str());
            }
        } else if (order->status == OrderStatus::Expired || 
                   order->status == OrderStatus::Canceled || 
                   order->status == OrderStatus::Margin) {
            if (print_ops_) {
                log(order->status_string(order->status));
            }
        }

        // 允许新订单
        order_id_ = nullptr;
    }

    void init() override {
        // 创建指标 - 在init时创建，这样它们可以随着数据一起forward
        auto data_feed = data(0);
        if (data_feed) {
            // 创建SMA指标
            sma_ = std::make_shared<SMA>(data_feed, period_);
            addindicator(sma_);  // 添加到指标列表
            
            // 创建交叉信号
            cross_ = std::make_shared<CrossOver>(data_feed, sma_);
            addindicator(cross_);  // 添加到指标列表
            
            if (print_data_) {
                std::cout << "*** Indicators created in init" << std::endl;
            }
        }
    }
    
    void prenext() override {
        static int prenext_count = 0;
        prenext_count++;
        if (prenext_count <= 3 || prenext_count == 15) {
            double close_val = data(0)->close(0);
            if (std::isnan(close_val)) {
                std::cout << "prenext() bar #" << prenext_count 
                          << " - close: nan (data size=" << data(0)->size() << ")" << std::endl;
            } else {
                std::cout << "prenext() bar #" << prenext_count 
                          << " - close: " << close_val << std::endl;
            }
        }
    }
    
    void nextstart() override {
        std::cout << "nextstart() - close: " << data(0)->close(0) << std::endl;
        // 调用next方法处理第一个bar
        next();
    }

    void start() override {
            if (!stock_like_) {
            // 期货模式
            broker_ptr()->setcommission(2.0, 1000.0, 10.0);  // commission, margin, mult
        }
        // 股票模式不设置手续费（使用默认的0），与Python保持一致

        if (print_data_) {
            log("-------------------------", 0.0, true);
            std::ostringstream oss;
            oss << "Starting portfolio value: " << std::fixed << std::setprecision(2) 
                << broker_ptr()->getvalue();
            log(oss.str(), 0.0, true);
            
            // Debug: Check minperiod
            std::cout << "*** Strategy minperiod: " << _minperiod() << std::endl;
        }

        start_time_ = std::chrono::high_resolution_clock::now();

        // 初始化记录向量
        buy_create_.clear();
        sell_create_.clear();
        buy_exec_.clear();
        sell_exec_.clear();
    }

    void stop() override {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
        
        // Debug: Print last close price at stop
        if (data(0)) {
            double last_close = data(0)->close(0);
            std::cout << "Strategy stop() - Last close price: " << last_close << std::endl;
        }
        
        if (print_data_) {
            std::ostringstream oss;
            oss << "Time used: " << duration.count() << " us";
            log(oss.str());
            
            oss.str("");
            oss << "Final portfolio value: " << std::fixed << std::setprecision(2) 
                << broker_ptr()->getvalue();
            log(oss.str());
            
            oss.str("");
            oss << "Final cash value: " << std::fixed << std::setprecision(2) 
                << broker_ptr()->getcash();
            log(oss.str());
            
            log("-------------------------");

            std::cout << "buycreate" << std::endl;
    
    for (const auto& price : buy_create_) {
                std::cout << price << " ";
            }
            std::cout << std::endl;

            std::cout << "sellcreate" << std::endl;
    
    for (const auto& price : sell_create_) {
                std::cout << price << " ";
            }
            std::cout << std::endl;

            std::cout << "buyexec (" << buy_exec_.size() << " items)" << std::endl;
    
    for (const auto& price : buy_exec_) {
                std::cout << price << " ";
            }
            std::cout << std::endl;

            std::cout << "sellexec (" << sell_exec_.size() << " items)" << std::endl;
    
    for (const auto& price : sell_exec_) {
                std::cout << price << " ";
            }
            std::cout << std::endl;
        }
    }

    void next() override {
        static int next_count = 0;
        next_count++;
        
        // Debug last bar
        if (next_count >= 238) {
            std::cout << "Strategy next() #" << next_count 
                      << " - close price: " << data(0)->close(0) << std::endl;
        }
        
        // 获取当前仓位
        double position_size = getposition();
        
        // 检查是否处理了特定的价格点以及前一个bar
        double close_price = data(0)->close(0);
        if ((next_count >= 71 && next_count <= 73) || (next_count >= 170 && next_count <= 172)) {
            std::cout << "DEBUG bar #" << next_count << " close=" << std::fixed << std::setprecision(2) 
                      << close_price 
                      << ", position=" << position_size 
                      << ", order_id=" << (order_id_ ? "active" : "null") 
                      << ", cross=" << (cross_ ? cross_->get(0) : 0.0) << std::endl;
        }
        
        if (std::abs(close_price - 3843.08) < 0.01 || std::abs(close_price - 3841.31) < 0.01) {
            std::cout << "PROCESSING bar #" << next_count << " with close=" << std::fixed << std::setprecision(2) 
                      << close_price 
                      << ", position=" << position_size 
                      << ", order_id=" << (order_id_ ? "active" : "null") << std::endl;
            if (cross_) {
                std::cout << "  CrossOver value=" << cross_->get(0) 
                          << ", SMA=" << (sma_ ? sma_->get(0) : 0.0) << std::endl;
            }
        }
        
        if (next_count >= 70 && next_count <= 75) {
            std::cout << "next() #" << next_count << " order_id_=" << (order_id_ ? "set" : "null") << std::endl;
        }
        if (print_data_) {
            // Debug: Check data object
            auto d = data(0);
            if (!d) {
                std::cout << "ERROR: data(0) is null!" << std::endl;
                return;
            }
            
            std::ostringstream oss;
            oss << "Open, High, Low, Close, " 
                << std::fixed << std::setprecision(2)
                << d->open(0) << ", " << d->high(0) << ", "
                << d->low(0) << ", " << d->close(0);
            
            // 只有当SMA有数据时才访问
            if (sma_) {
                double sma_val = sma_->get(0);
                oss << ", Sma, " << std::setprecision(6) << sma_val;
                
                // Debug: 当SMA首次产生有效值时输出
                static bool first_valid_sma = true;
                if (first_valid_sma && !std::isnan(sma_val)) {
                    std::cout << "*** SMA first valid value, sma=" << sma_val << std::endl;
                    first_valid_sma = false;
                }
            } else {
                oss << ", Sma, NaN";
            }
            log(oss.str());

            oss.str("");
            oss << "Close " << std::fixed << std::setprecision(2) << data(0)->close(0);
            if (sma_) {
                oss << " - Sma " << std::setprecision(2) << sma_->get(0);
            } else {
                oss << " - Sma NaN";
            }
            log(oss.str());
        }

        // Check if there's an active order
        if (order_id_) {
            // If an order is active, no new orders are allowed
            return;
        }
        
        if (next_count >= 70 && next_count <= 75) {
            double cross_val = cross_ ? cross_->get(0) : 0.0;
            std::cout << "next() #" << next_count << " position_size=" << position_size 
                      << ", cross=" << cross_val;
            
            // Debug: Check buffer directly
            if (cross_) {
                auto cross_line = cross_->lines->getline(0);
                if (auto buffer = std::dynamic_pointer_cast<backtrader::LineBuffer>(cross_line)) {
                    std::cout << ", buffer.size=" << buffer->size() 
                              << ", buffer[" << buffer->size()-1 << "]=" << buffer->array()[buffer->size()-1];
                }
            }
            std::cout << std::endl;
        }
        
        
        // Debug: 检查CrossOver状态
        if (cross_) {
            double cross_val = cross_->get(0);
            static bool first_valid_cross = true;
            if (first_valid_cross && !std::isnan(cross_val)) {
                std::cout << "*** CrossOver first valid value, cross=" << cross_val << std::endl;
                first_valid_cross = false;
            }
            
            // 打印所有正的CrossOver值
            if (cross_val > 0.0 && position_size == 0.0) {
                std::cout << "POSITIVE CROSS at close=" << std::fixed << std::setprecision(2) 
                          << data(0)->close(0) << ", cross_val=" << cross_val << std::endl;
            }
            
            // 专门检查缺失的两个信号点
            double close_price = data(0)->close(0);
            if (std::abs(close_price - 3843.08) < 0.01 || std::abs(close_price - 3841.31) < 0.01) {
                std::cout << "MISSING SIGNAL CHECK: close=" << std::fixed << std::setprecision(2) << close_price 
                          << ", cross_val=" << cross_val << ", position_size=" << position_size << std::endl;
            }
        }

        if (position_size == 0.0) {
            // 没有仓位时，检查买入信号（只有当CrossOver有数据时）
            if (cross_) {
                double cross_val = cross_->get(0);
                // Debug output for missing buy signals
                double close_price = data(0)->close(0);
                if (std::abs(close_price - 3843.08) < 0.01 || std::abs(close_price - 3841.31) < 0.01) {
                    std::cout << "DEBUG: At close price " << std::fixed << std::setprecision(2) << close_price 
                              << ", cross value = " << cross_val 
                              << ", SMA = " << (sma_ ? sma_->get(0) : 0.0) << std::endl;
                }
                if (cross_val > 0.0) {
                if (print_ops_) {
                    std::ostringstream oss;
                    oss << "BUY CREATE , " << std::fixed << std::setprecision(2) 
                        << data(0)->close(0);
                    log(oss.str());
                }

                order_id_ = buy();
                
                
                std::ostringstream price_ss;
                price_ss << std::fixed << std::setprecision(2) << data(0)->close(0);
                buy_create_.push_back(price_ss.str());
                }
            }
        } else {
            // 有仓位时，检查卖出信号（只有当CrossOver有数据时）
            if (cross_ && cross_->get(0) < 0.0) {
                if (print_ops_) {
                    std::ostringstream oss;
                    oss << "SELL CREATE , " << std::fixed << std::setprecision(2) 
                        << data(0)->close(0);
                    log(oss.str());
                }

                order_id_ = close();
                
                
                std::ostringstream price_ss;
                price_ss << std::fixed << std::setprecision(2) << data(0)->close(0);
                sell_create_.push_back(price_ss.str());
            }
        }
    }

    // Getter方法用于测试验证
    int getPeriod() const { return period_; }
    bool isStockLike() const { return stock_like_; }
};

// 运行策略测试的辅助函数 - 返回策略和cerebro的pair
std::pair<std::shared_ptr<UnoptimizedRunStrategy>, std::unique_ptr<backtrader::Cerebro>> runStrategyTestPair(bool stocklike, bool print_results = false) {
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    
    auto csv_data = getdata_feed(0);
    std::cout << "Loaded CSV data feed" << std::endl;
    if (csv_data) {
        std::cout << "Data feed created successfully" << std::endl;
    }
    cerebro->adddata(csv_data);

    // 设置策略参数
    UnoptimizedRunStrategy::Params params;
    params.period = 15;
    params.printdata = print_results;
    params.printops = print_results;
    params.stocklike = stocklike;

    // 添加策略
    cerebro->addstrategy<UnoptimizedRunStrategy>(params);

    // 禁用runonce模式，使用next模式
    cerebro->setRunOnce(false);

    // 运行回测
    auto results = cerebro->run();
    EXPECT_EQ(results.size(), 1) << "Should have exactly 1 strategy result";

    auto strategy = std::dynamic_pointer_cast<UnoptimizedRunStrategy>(results[0]);
    EXPECT_NE(strategy, nullptr) << "Strategy cast should succeed";

    return {strategy, std::move(cerebro)};
}

// 运行策略测试的辅助函数
std::shared_ptr<UnoptimizedRunStrategy> runStrategyTest(bool stocklike, bool print_results = false) {
    auto [strategy, cerebro] = runStrategyTestPair(stocklike, print_results);
    return strategy;
}

// 测试股票模式策略
TEST(OriginalTests, StrategyUnoptimized_StockMode) {
    auto strategy = runStrategyTest(true, false);   // 关闭debug输出

    // 验证最终资产值（股票模式）
    double final_value = strategy->broker_ptr()->getvalue();
    double final_cash = strategy->broker_ptr()->getcash();
    
    // Debug: Check data size
    auto data = strategy->data(0);
    std::cout << "Stock Mode - Data size: " << data->size() 
              << ", Data buflen: " << data->buflen() << std::endl;
    
    std::ostringstream value_ss, cash_ss;
    value_ss << std::fixed << std::setprecision(2) << final_value;
    cash_ss << std::fixed << std::setprecision(2) << final_cash;
    
    // Debug: Print actual values
    std::cout << "Stock Mode - Actual final value: " << value_ss.str() << ", Expected: 10284.10" << std::endl;
    std::cout << "Stock Mode - Actual final cash: " << cash_ss.str() << ", Expected: 6164.16" << std::endl;
    
    // Debug: Calculate total commission
    double total_buy_value = 0.0;
    double total_sell_value = 0.0;
    for (const auto& p : strategy->buy_exec_) {
        total_buy_value += std::stod(p);
    }
    for (const auto& p : strategy->sell_exec_) {
        total_sell_value += std::stod(p);
    }
    std::cout << "Stock Mode - Total buy value: " << std::fixed << std::setprecision(2) << total_buy_value 
              << ", Total sell value: " << total_sell_value << std::endl;
    double position_size = strategy->getposition();
    std::cout << "Stock Mode - Position size: " << position_size << std::endl;
    
    // Debug position value
    if (position_size != 0) {
        auto data = strategy->data(0);
        // Get the last close price (index 0 is the current/last bar)
        double last_close = (*data->lines->getline(backtrader::DataSeries::Close))[0];
        double position_value = position_size * last_close;
        std::cout << "Stock Mode - Last close: " << std::fixed << std::setprecision(2) << last_close 
                  << ", Position value: " << position_value << std::endl;
        
        // Also check other prices for debugging
        double last_open = (*data->lines->getline(backtrader::DataSeries::Open))[0];
        double last_high = (*data->lines->getline(backtrader::DataSeries::High))[0];
        double last_low = (*data->lines->getline(backtrader::DataSeries::Low))[0];
        std::cout << "Stock Mode - Last OHLC: " << last_open << ", " << last_high 
                  << ", " << last_low << ", " << last_close << std::endl;
    }
    
    // Updated expected values to match Python test
    EXPECT_EQ(value_ss.str(), "10284.10") << "Stock mode final value should match expected";
    EXPECT_EQ(cash_ss.str(), "6164.16") << "Stock mode final cash should match expected";

    // Debug: Print actual buy/sell prices
    std::cout << "Actual buy_create: ";
    for (const auto& p : strategy->buy_create_) std::cout << p << " ";
    std::cout << std::endl;
    
    std::cout << "Expected buy_create: ";
    for (const auto& p : EXPECTED_BUY_CREATE) std::cout << p << " ";
    std::cout << std::endl;
    
    // 验证买入创建价格
    EXPECT_EQ(strategy->buy_create_.size(), EXPECTED_BUY_CREATE.size()) 
        << "Buy create prices count should match";
    for (size_t i = 0; i < std::min(strategy->buy_create_.size(), EXPECTED_BUY_CREATE.size()); ++i) {
        EXPECT_EQ(strategy->buy_create_[i], EXPECTED_BUY_CREATE[i])
            << "Buy create price mismatch at index " << i;
    }

    // 验证卖出创建价格
    EXPECT_EQ(strategy->sell_create_.size(), EXPECTED_SELL_CREATE.size()) 
        << "Sell create prices count should match";
    for (size_t i = 0; i < std::min(strategy->sell_create_.size(), EXPECTED_SELL_CREATE.size()); ++i) {
        EXPECT_EQ(strategy->sell_create_[i], EXPECTED_SELL_CREATE[i])
            << "Sell create price mismatch at index " << i;
    }

    // 验证买入执行价格
    EXPECT_EQ(strategy->buy_exec_.size(), EXPECTED_BUY_EXEC.size()) 
        << "Buy exec prices count should match";
    for (size_t i = 0; i < std::min(strategy->buy_exec_.size(), EXPECTED_BUY_EXEC.size()); ++i) {
        EXPECT_EQ(strategy->buy_exec_[i], EXPECTED_BUY_EXEC[i])
            << "Buy exec price mismatch at index " << i;
    }

    // 验证卖出执行价格
    EXPECT_EQ(strategy->sell_exec_.size(), EXPECTED_SELL_EXEC.size()) 
        << "Sell exec prices count should match";
    for (size_t i = 0; i < std::min(strategy->sell_exec_.size(), EXPECTED_SELL_EXEC.size()); ++i) {
        EXPECT_EQ(strategy->sell_exec_[i], EXPECTED_SELL_EXEC[i])
            << "Sell exec price mismatch at index " << i;
    }
}

// 测试期货模式策略
TEST(OriginalTests, StrategyUnoptimized_FuturesMode) {
    auto strategy = runStrategyTest(false, false);   // 关闭debug输出

    // 验证最终资产值（期货模式）
    double final_value = strategy->broker_ptr()->getvalue();
    double final_cash = strategy->broker_ptr()->getcash();
    
    // Debug: Print final position and detailed info
    double final_position = strategy->getposition();
    auto data_series = std::dynamic_pointer_cast<backtrader::DataSeries>(strategy->datas[0]);
    auto pos = strategy->broker_ptr()->getposition(data_series);
    double current_price = strategy->datas[0]->close(0);
    
    std::cout << "Futures Mode - Final position size: " << final_position << std::endl;
    if (pos && pos->size != 0) {
        std::cout << "Position entry price: " << pos->price << std::endl;
        std::cout << "Current price: " << current_price << std::endl;
        double pnl = (current_price - pos->price) * pos->size * 10.0;  // mult=10
        std::cout << "Unrealized P&L: " << pnl << std::endl;
    }
    std::cout << "Futures Mode - Final value: " << final_value << ", Final cash: " << final_cash << std::endl;
    std::cout << "Futures Mode - Buy orders: " << strategy->buy_create_.size() 
              << ", Sell orders: " << strategy->sell_create_.size() << std::endl;
    
    std::ostringstream value_ss, cash_ss;
    value_ss << std::fixed << std::setprecision(2) << final_value;
    cash_ss << std::fixed << std::setprecision(2) << final_cash;
    
    EXPECT_EQ(value_ss.str(), "12795.00") << "Futures mode final value should match expected";
    EXPECT_EQ(cash_ss.str(), "11795.00") << "Futures mode final cash should match expected";

    // 期货模式下的买卖价格应该与股票模式相同
    EXPECT_EQ(strategy->buy_create_, EXPECTED_BUY_CREATE) << "Futures mode buy create prices should match";
    EXPECT_EQ(strategy->sell_create_, EXPECTED_SELL_CREATE) << "Futures mode sell create prices should match";
    EXPECT_EQ(strategy->buy_exec_, EXPECTED_BUY_EXEC) << "Futures mode buy exec prices should match";
    EXPECT_EQ(strategy->sell_exec_, EXPECTED_SELL_EXEC) << "Futures mode sell exec prices should match";
}

// 测试策略参数验证
TEST(OriginalTests, StrategyUnoptimized_ParameterValidation) {
    // 测试股票模式
    auto stock_strategy = runStrategyTest(true, false);
    EXPECT_TRUE(stock_strategy->isStockLike()) << "Should be in stock mode";
    EXPECT_EQ(stock_strategy->getPeriod(), 15) << "Period should be 15";

    // 测试期货模式
    auto futures_strategy = runStrategyTest(false, false);
    EXPECT_FALSE(futures_strategy->isStockLike()) << "Should be in futures mode";
    EXPECT_EQ(futures_strategy->getPeriod(), 15) << "Period should be 15";
}

// 测试交易序列
TEST(OriginalTests, StrategyUnoptimized_TradingSequence) {
    auto strategy = runStrategyTest(true, false);

    // 验证交易序列的合理性
    size_t buy_count = strategy->buy_create_.size();
    size_t sell_count = strategy->sell_create_.size();
    
    // 买入次数应该等于或比卖出次数多1（最后可能还持有仓位）
    EXPECT_TRUE(buy_count == sell_count || buy_count == sell_count + 1)
        << "Buy count should equal sell count or be one more";

    // 买入执行和创建数量应该相等
    EXPECT_EQ(strategy->buy_exec_.size(), strategy->buy_create_.size())
        << "Buy exec count should equal buy create count";

    // 卖出执行和创建数量应该相等
    EXPECT_EQ(strategy->sell_exec_.size(), strategy->sell_create_.size())
        << "Sell exec count should equal sell create count";
}

// 测试指标值
TEST(OriginalTests, StrategyUnoptimized_IndicatorValues) {
    auto cerebro = std::make_unique<backtrader::Cerebro>();
    auto csv_data = getdata_feed(0);
    cerebro->adddata(csv_data);

    UnoptimizedRunStrategy::Params params;
    params.printdata = false;
    params.stocklike = true;

    cerebro->addstrategy<UnoptimizedRunStrategy>(params);
    
    // Disable runonce mode to ensure proper execution
    cerebro->setRunOnce(false);

    auto results = cerebro->run();
    auto strategy = std::dynamic_pointer_cast<UnoptimizedRunStrategy>(results[0]);

    // 验证策略执行了交易
    EXPECT_GT(strategy->buy_create_.size(), 0) << "Strategy should have created buy orders";
    EXPECT_GT(strategy->sell_create_.size(), 0) << "Strategy should have created sell orders";
    
    // 验证最终状态合理
    double final_value = strategy->broker_ptr()->getvalue();
    EXPECT_GT(final_value, 0.0) << "Final portfolio value should be positive";
}

// 测试不同模式的对比
TEST(OriginalTests, StrategyUnoptimized_ModeComparison) {
    auto [stock_strategy, stock_cerebro] = runStrategyTestPair(true, false);
    auto [futures_strategy, futures_cerebro] = runStrategyTestPair(false, false);

    // 交易信号应该相同
    EXPECT_EQ(stock_strategy->buy_create_, futures_strategy->buy_create_)
        << "Both modes should have same buy signals";
    EXPECT_EQ(stock_strategy->sell_create_, futures_strategy->sell_create_)
        << "Both modes should have same sell signals";
    EXPECT_EQ(stock_strategy->buy_exec_, futures_strategy->buy_exec_)
        << "Both modes should have same buy executions";
    EXPECT_EQ(stock_strategy->sell_exec_, futures_strategy->sell_exec_)
        << "Both modes should have same sell executions";

    // 最终资产值应该不同（由于不同的手续费结构）
    double stock_value = stock_strategy->broker_ptr()->getvalue();
    double futures_value = futures_strategy->broker_ptr()->getvalue();
    
    EXPECT_NE(stock_value, futures_value) 
        << "Different modes should produce different final values";
}

// 测试策略时间统计
TEST(OriginalTests, StrategyUnoptimized_Timing) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    auto strategy = runStrategyTest(true, false);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "Strategy execution time: " << duration.count() << " ms" << std::endl;

    // 验证策略执行成功
    EXPECT_GT(strategy->buy_create_.size(), 0) << "Strategy should have executed";
    
    // 性能要求
    EXPECT_LT(duration.count(), 1000) << "Strategy should execute within 1 second";
}

// 测试价格精度
TEST(OriginalTests, StrategyUnoptimized_PricePrecision) {
    auto strategy = runStrategyTest(true, false);

    // 验证所有价格都是正确的2位小数格式
    
    for (const auto& price : strategy->buy_create_) {
        EXPECT_EQ(price.length(), 7) << "Buy create price should have correct format: " << price;
        EXPECT_EQ(price.find('.'), 4) << "Buy create price should have decimal point at position 4: " << price;
    }
    
    for (const auto& price : strategy->sell_create_) {
        EXPECT_EQ(price.length(), 7) << "Sell create price should have correct format: " << price;
        EXPECT_EQ(price.find('.'), 4) << "Sell create price should have decimal point at position 4: " << price;
    }
    
    for (const auto& price : strategy->buy_exec_) {
        EXPECT_EQ(price.length(), 7) << "Buy exec price should have correct format: " << price;
        EXPECT_EQ(price.find('.'), 4) << "Buy exec price should have decimal point at position 4: " << price;
    }
    
    for (const auto& price : strategy->sell_exec_) {
        EXPECT_EQ(price.length(), 7) << "Sell exec price should have correct format: " << price;
        EXPECT_EQ(price.find('.'), 4) << "Sell exec price should have decimal point at position 4: " << price;
    }
}

// 测试订单通知机制
TEST(OriginalTests, StrategyUnoptimized_OrderNotification) {
    auto strategy = runStrategyTest(true, false);

    // 验证买入和卖出执行数量与创建数量匹配
    EXPECT_EQ(strategy->buy_exec_.size(), strategy->buy_create_.size())
        << "All buy orders should be executed";
    EXPECT_EQ(strategy->sell_exec_.size(), strategy->sell_create_.size())
        << "All sell orders should be executed";

    // 验证执行价格与创建价格的关系（执行价格可能略有不同）;
    for (size_t i = 0; i < std::min(strategy->buy_create_.size(), strategy->buy_exec_.size()); ++i) {
        double create_price = std::stod(strategy->buy_create_[i]);
        double exec_price = std::stod(strategy->buy_exec_[i]);
        
        // 执行价格应该在合理范围内
        EXPECT_NEAR(exec_price, create_price, 20.0) 
            << "Buy exec price should be close to create price at index " << i;
    }
}

// 性能基准测试
TEST(OriginalTests, StrategyUnoptimized_Performance) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 运行多次策略测试
    const int num_runs = 10;
    for (int i = 0; i < num_runs; ++i) {
        auto strategy = runStrategyTest(i % 2 == 0, false);  // 交替使用股票和期货模式
        
        // 验证每次运行的基本结果
        EXPECT_GT(strategy->buy_create_.size(), 0) << "Run " << i << " should have buy signals";
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Strategy performance test: " << num_runs 
              << " runs in " << duration.count() << " ms" << std::endl;
    
    // 性能要求
    EXPECT_LT(duration.count(), 5000) 
        << "Performance test should complete within 5 seconds";
}