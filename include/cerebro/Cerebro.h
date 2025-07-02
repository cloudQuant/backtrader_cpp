#pragma once

#include "strategy/StrategyBase.h"
#include "broker/Broker.h"
#include "data/DataFeed.h"
#include "analyzers/AnalyzerBase.h"
#include <vector>
#include <memory>
#include <chrono>
#include <functional>

namespace backtrader {
namespace cerebro {

using namespace strategy;
using namespace broker;
using namespace data;

/**
 * @brief 回测结果
 */
struct BacktestResult {
    // 基本统计
    size_t total_trades;
    double total_return;
    double annualized_return;
    double max_drawdown;
    double sharpe_ratio;
    double win_rate;
    
    // 时间信息
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::chrono::duration<double> execution_time;
    
    // 权益曲线
    std::vector<double> equity_curve;
    std::vector<double> drawdown_curve;
    
    // 交易记录
    std::vector<Order> trade_history;
    
    BacktestResult() : total_trades(0), total_return(0.0), annualized_return(0.0),
                      max_drawdown(0.0), sharpe_ratio(0.0), win_rate(0.0),
                      execution_time(0.0) {}
};

/**
 * @brief 执行模式
 */
enum class ExecutionMode {
    RUNONCE,    // 向量化执行（一次性加载所有数据）
    RUNNEXT     // 逐步执行（逐个数据点处理）
};

/**
 * @brief Cerebro - 核心回测引擎
 * 
 * 协调数据源、策略、经纪商和分析器的执行
 * 提供完整的回测框架和结果分析
 */
class Cerebro {
private:
    // 核心组件
    std::vector<std::shared_ptr<StrategyBase>> strategies_;
    std::vector<std::shared_ptr<DataFeed>> data_feeds_;
    std::unique_ptr<Broker> broker_;
    std::vector<std::unique_ptr<analyzers::AnalyzerBase>> analyzers_;
    
    // 执行参数
    ExecutionMode execution_mode_;
    bool preload_data_;
    size_t exact_bars_;
    bool live_trading_;
    
    // 时间控制
    std::chrono::system_clock::time_point start_date_;
    std::chrono::system_clock::time_point end_date_;
    bool use_date_range_;
    
    // 性能设置
    size_t max_cpus_;
    bool enable_optimization_;
    
    // 回调函数
    std::function<void(size_t)> progress_callback_;
    std::function<void(const std::string&)> log_callback_;
    
    // 运行状态
    bool is_running_;
    size_t current_bar_;
    size_t total_bars_;
    
public:
    /**
     * @brief 构造函数
     * @param initial_cash 初始资金
     */
    explicit Cerebro(double initial_cash = 100000.0)
        : execution_mode_(ExecutionMode::RUNNEXT),
          preload_data_(true),
          exact_bars_(0),
          live_trading_(false),
          use_date_range_(false),
          max_cpus_(1),
          enable_optimization_(false),
          is_running_(false),
          current_bar_(0),
          total_bars_(0) {
        
        // 创建默认broker（没有数据源，稍后设置）
        broker_ = std::make_unique<Broker>(nullptr, initial_cash);
    }
    
    /**
     * @brief 添加策略
     * @param strategy 策略实例
     */
    void addStrategy(std::shared_ptr<StrategyBase> strategy) {
        if (strategy) {
            strategies_.push_back(strategy);
        }
    }
    
    /**
     * @brief 添加数据源
     * @param data_feed 数据源
     */
    void addDataFeed(std::shared_ptr<DataFeed> data_feed) {
        if (data_feed) {
            data_feeds_.push_back(data_feed);
        }
    }
    
    /**
     * @brief 添加分析器
     * @param analyzer 分析器
     */
    void addAnalyzer(std::unique_ptr<analyzers::AnalyzerBase> analyzer) {
        if (analyzer) {
            analyzers_.push_back(std::move(analyzer));
        }
    }
    
    /**
     * @brief 设置执行模式
     * @param mode 执行模式
     */
    void setExecutionMode(ExecutionMode mode) {
        execution_mode_ = mode;
    }
    
    /**
     * @brief 设置数据预加载
     * @param preload 是否预加载
     */
    void setPreload(bool preload) {
        preload_data_ = preload;
    }
    
    /**
     * @brief 设置时间范围
     * @param start_date 开始时间
     * @param end_date 结束时间
     */
    void setDateRange(const std::chrono::system_clock::time_point& start_date,
                     const std::chrono::system_clock::time_point& end_date) {
        start_date_ = start_date;
        end_date_ = end_date;
        use_date_range_ = true;
    }
    
    /**
     * @brief 设置经纪商
     * @param broker 经纪商实例
     */
    void setBroker(std::unique_ptr<Broker> broker) {
        broker_ = std::move(broker);
    }
    
    /**
     * @brief 获取经纪商
     * @return 经纪商引用
     */
    Broker& getBroker() {
        return *broker_;
    }
    
    /**
     * @brief 设置手续费
     * @param commission 手续费率或固定费用
     * @param is_percentage 是否为百分比
     */
    void setCommission(double commission, bool is_percentage = true) {
        if (broker_) {
            if (is_percentage) {
                broker_->setCommissionModel(
                    std::make_unique<PercentageCommissionModel>(commission)
                );
            } else {
                broker_->setCommissionModel(
                    std::make_unique<FixedCommissionModel>(commission)
                );
            }
        }
    }
    
    /**
     * @brief 设置滑点
     * @param slippage 滑点值
     * @param is_percentage 是否为百分比
     */
    void setSlippage(double slippage, bool is_percentage = false) {
        if (broker_) {
            if (is_percentage) {
                broker_->setSlippageModel(
                    std::make_unique<PercentageSlippageModel>(slippage)
                );
            } else {
                broker_->setSlippageModel(
                    std::make_unique<FixedSlippageModel>(slippage)
                );
            }
        }
    }
    
    /**
     * @brief 设置进度回调
     * @param callback 进度回调函数
     */
    void setProgressCallback(std::function<void(size_t)> callback) {
        progress_callback_ = callback;
    }
    
    /**
     * @brief 设置日志回调
     * @param callback 日志回调函数
     */
    void setLogCallback(std::function<void(const std::string&)> callback) {
        log_callback_ = callback;
    }
    
    /**
     * @brief 运行回测
     * @return 回测结果
     */
    BacktestResult run() {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        BacktestResult result;
        result.start_time = std::chrono::system_clock::now();
        
        try {
            // 验证配置
            if (!validateConfiguration()) {
                logMessage("Configuration validation failed");
                return result;
            }
            
            // 初始化
            if (!initialize()) {
                logMessage("Initialization failed");
                return result;
            }
            
            // 执行回测
            logMessage("Starting backtest execution");
            is_running_ = true;
            
            if (execution_mode_ == ExecutionMode::RUNONCE) {
                runOnceMode();
            } else {
                runNextMode();
            }
            
            // 完成分析
            finalize();
            
            // 生成结果
            result = generateResult();
            
        } catch (const std::exception& e) {
            logMessage("Error during execution: " + std::string(e.what()));
        }
        
        is_running_ = false;
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.end_time = std::chrono::system_clock::now();
        result.execution_time = end_time - start_time;
        
        logMessage("Backtest completed");
        return result;
    }
    
    /**
     * @brief 停止回测
     */
    void stop() {
        is_running_ = false;
    }
    
    /**
     * @brief 获取当前进度
     * @return 进度百分比 (0-100)
     */
    double getProgress() const {
        if (total_bars_ == 0) return 0.0;
        return (static_cast<double>(current_bar_) / total_bars_) * 100.0;
    }
    
    /**
     * @brief 获取运行状态
     * @return true if running
     */
    bool isRunning() const {
        return is_running_;
    }
    
private:
    /**
     * @brief 验证配置
     * @return true if valid
     */
    bool validateConfiguration() {
        if (strategies_.empty()) {
            logMessage("No strategies added");
            return false;
        }
        
        if (data_feeds_.empty()) {
            logMessage("No data feeds added");
            return false;
        }
        
        if (!broker_) {
            logMessage("No broker configured");
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief 初始化系统
     * @return true if successful
     */
    bool initialize() {
        // 设置broker的数据源（使用第一个数据源）
        if (!data_feeds_.empty()) {
            // 重新创建broker以使用正确的数据源
            double initial_cash = broker_->getAccountInfo().cash;
            auto old_broker = std::move(broker_);
            broker_ = std::make_unique<Broker>(data_feeds_[0], initial_cash);
            
            // 复制配置
            // 这里应该复制手续费模型、滑点模型等配置
        }
        
        // 为策略添加数据源
        for (auto& strategy : strategies_) {
            for (auto& data_feed : data_feeds_) {
                strategy->addDataFeed(data_feed);
            }
        }
        
        // 预加载数据
        if (preload_data_) {
            loadAllData();
        }
        
        // 初始化策略
        for (auto& strategy : strategies_) {
            strategy->initialize();
        }
        
        // 初始化分析器
        for (auto& analyzer : analyzers_) {
            analyzer->initialize();
        }
        
        return true;
    }
    
    /**
     * @brief 加载所有数据
     */
    void loadAllData() {
        total_bars_ = 0;
        
        for (auto& data_feed : data_feeds_) {
            auto static_feed = std::dynamic_pointer_cast<StaticDataFeed>(data_feed);
            if (static_feed) {
                size_t loaded = static_feed->loadBatch();
                total_bars_ = std::max(total_bars_, loaded);
                logMessage("Loaded " + std::to_string(loaded) + " bars from " + 
                          data_feed->getName());
            }
        }
    }
    
    /**
     * @brief RunOnce模式执行
     */
    void runOnceMode() {
        logMessage("Executing in RunOnce mode");
        
        // 在RunOnce模式下，所有指标一次性计算
        // 然后逐个调用策略的next方法
        
        while (hasMoreData() && is_running_) {
            processNextBar();
            current_bar_++;
            
            if (progress_callback_ && current_bar_ % 100 == 0) {
                progress_callback_(current_bar_);
            }
        }
    }
    
    /**
     * @brief RunNext模式执行
     */
    void runNextMode() {
        logMessage("Executing in RunNext mode");
        
        while (hasMoreData() && is_running_) {
            processNextBar();
            current_bar_++;
            
            if (progress_callback_ && current_bar_ % 100 == 0) {
                progress_callback_(current_bar_);
            }
        }
    }
    
    /**
     * @brief 处理下一个数据点
     */
    void processNextBar() {
        // 更新数据源
        for (auto& data_feed : data_feeds_) {
            if (data_feed->hasNext()) {
                data_feed->next();
            }
        }
        
        // 更新broker和市场
        if (broker_) {
            broker_->updateMarket();
        }
        
        // 执行策略
        for (auto& strategy : strategies_) {
            strategy->processNext();
        }
        
        // 更新分析器
        for (auto& analyzer : analyzers_) {
            analyzer->next();
        }
    }
    
    /**
     * @brief 检查是否还有更多数据
     * @return true if has more data
     */
    bool hasMoreData() {
        for (auto& data_feed : data_feeds_) {
            if (data_feed->hasNext()) {
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief 完成处理
     */
    void finalize() {
        // 完成策略
        for (auto& strategy : strategies_) {
            strategy->finalize();
        }
        
        // 完成分析器
        for (auto& analyzer : analyzers_) {
            analyzer->finalize();
        }
    }
    
    /**
     * @brief 生成回测结果
     * @return 回测结果
     */
    BacktestResult generateResult() {
        BacktestResult result;
        
        if (!broker_) {
            return result;
        }
        
        // 获取基本统计
        auto account = broker_->getAccountInfo();
        auto trading_stats = broker_->getTradingStatistics();
        
        result.total_trades = trading_stats.total_trades;
        result.total_return = (account.equity / 100000.0 - 1.0) * 100.0; // 假设初始资金10万
        result.max_drawdown = account.max_drawdown;
        result.win_rate = trading_stats.win_rate;
        result.sharpe_ratio = trading_stats.sharpe_ratio;
        
        // 计算年化收益率（简化）
        auto duration = result.end_time - result.start_time;
        auto days = std::chrono::duration_cast<std::chrono::hours>(duration).count() / 24;
        if (days > 0) {
            double years = static_cast<double>(days) / 365.25;
            result.annualized_return = std::pow(1.0 + result.total_return / 100.0, 1.0 / years) - 1.0;
            result.annualized_return *= 100.0;
        }
        
        // 获取权益曲线
        result.equity_curve = broker_->getEquityCurve();
        
        // 计算回撤曲线
        result.drawdown_curve.reserve(result.equity_curve.size());
        double peak = 0.0;
        for (double equity : result.equity_curve) {
            peak = std::max(peak, equity);
            double drawdown = peak - equity;
            result.drawdown_curve.push_back(drawdown);
        }
        
        // 获取交易历史
        result.trade_history = broker_->getOrderHistory();
        
        return result;
    }
    
    /**
     * @brief 记录日志
     * @param message 日志消息
     */
    void logMessage(const std::string& message) {
        if (log_callback_) {
            log_callback_(message);
        }
    }
};

} // namespace cerebro
} // namespace backtrader