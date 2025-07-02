#pragma once

#include "strategy/StrategyBase.h"
#include "broker/Broker.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>

namespace backtrader {
namespace analyzers {

using namespace strategy;
using namespace broker;

/**
 * @brief 分析器基类
 * 
 * 提供策略和交易分析的基础框架
 * 分析器在回测过程中收集数据，并在结束时生成分析报告
 */
class AnalyzerBase {
protected:
    std::string name_;
    std::unordered_map<std::string, double> results_;
    std::shared_ptr<StrategyBase> strategy_;
    std::shared_ptr<Broker> broker_;
    
    // 状态标志
    bool initialized_;
    bool finalized_;
    
public:
    /**
     * @brief 构造函数
     * @param name 分析器名称
     */
    explicit AnalyzerBase(const std::string& name = "Analyzer")
        : name_(name), initialized_(false), finalized_(false) {}
    
    virtual ~AnalyzerBase() = default;
    
    /**
     * @brief 获取分析器名称
     * @return 名称
     */
    const std::string& getName() const { return name_; }
    
    /**
     * @brief 设置策略引用
     * @param strategy 策略实例
     */
    void setStrategy(std::shared_ptr<StrategyBase> strategy) {
        strategy_ = strategy;
    }
    
    /**
     * @brief 设置经纪商引用
     * @param broker 经纪商实例
     */
    void setBroker(std::shared_ptr<Broker> broker) {
        broker_ = broker;
    }
    
    /**
     * @brief 初始化分析器
     * 在回测开始前调用
     */
    virtual void initialize() {
        initialized_ = true;
        finalized_ = false;
        results_.clear();
    }
    
    /**
     * @brief 处理下一个数据点
     * 在每个时间步调用
     */
    virtual void next() = 0;
    
    /**
     * @brief 完成分析
     * 在回测结束后调用
     */
    virtual void finalize() {
        finalized_ = true;
    }
    
    /**
     * @brief 获取分析结果
     * @return 结果映射
     */
    const std::unordered_map<std::string, double>& getResults() const {
        return results_;
    }
    
    /**
     * @brief 获取特定结果
     * @param key 结果键
     * @param default_value 默认值
     * @return 结果值
     */
    double getResult(const std::string& key, double default_value = 0.0) const {
        auto it = results_.find(key);
        return (it != results_.end()) ? it->second : default_value;
    }
    
    /**
     * @brief 生成报告
     * @return 报告字符串
     */
    virtual std::string generateReport() const {
        std::ostringstream oss;
        oss << "=== " << name_ << " Analysis Report ===\n";
        
        for (const auto& [key, value] : results_) {
            oss << key << ": " << value << "\n";
        }
        
        return oss.str();
    }
    
protected:
    /**
     * @brief 设置结果值
     * @param key 结果键
     * @param value 结果值
     */
    void setResult(const std::string& key, double value) {
        results_[key] = value;
    }
    
    /**
     * @brief 检查是否已初始化
     * @return true if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief 检查是否已完成
     * @return true if finalized
     */
    bool isFinalized() const { return finalized_; }
};

/**
 * @brief 收益分析器
 * 
 * 分析策略的收益表现
 */
class ReturnsAnalyzer : public AnalyzerBase {
private:
    double initial_value_;
    double previous_value_;
    std::vector<double> returns_;
    std::vector<double> equity_curve_;
    
public:
    explicit ReturnsAnalyzer() : AnalyzerBase("Returns"),
                                initial_value_(0.0), previous_value_(0.0) {}
    
    void initialize() override {
        AnalyzerBase::initialize();
        
        if (broker_) {
            initial_value_ = broker_->getAccountInfo().equity;
            previous_value_ = initial_value_;
        }
        
        returns_.clear();
        equity_curve_.clear();
    }
    
    void next() override {
        if (!broker_) return;
        
        double current_value = broker_->getAccountInfo().equity;
        equity_curve_.push_back(current_value);
        
        if (previous_value_ > 0.0) {
            double return_rate = (current_value - previous_value_) / previous_value_;
            returns_.push_back(return_rate);
        }
        
        previous_value_ = current_value;
    }
    
    void finalize() override {
        AnalyzerBase::finalize();
        
        if (returns_.empty() || initial_value_ == 0.0) return;
        
        // 总收益率
        double total_return = (previous_value_ - initial_value_) / initial_value_;
        setResult("total_return", total_return * 100.0);
        
        // 平均收益率
        double sum_returns = 0.0;
        for (double ret : returns_) {
            sum_returns += ret;
        }
        double avg_return = sum_returns / returns_.size();
        setResult("avg_return", avg_return * 100.0);
        
        // 收益率标准差（波动率）
        double sum_squared_diff = 0.0;
        for (double ret : returns_) {
            double diff = ret - avg_return;
            sum_squared_diff += diff * diff;
        }
        double volatility = std::sqrt(sum_squared_diff / returns_.size());
        setResult("volatility", volatility * 100.0);
        
        // 夏普比率（简化，假设无风险利率为0）
        double sharpe_ratio = (volatility > 0.0) ? avg_return / volatility : 0.0;
        setResult("sharpe_ratio", sharpe_ratio);
        
        // 最大回撤
        double max_drawdown = calculateMaxDrawdown();
        setResult("max_drawdown", max_drawdown * 100.0);
    }
    
private:
    double calculateMaxDrawdown() {
        if (equity_curve_.empty()) return 0.0;
        
        double peak = equity_curve_[0];
        double max_dd = 0.0;
        
        for (double value : equity_curve_) {
            if (value > peak) {
                peak = value;
            }
            
            double drawdown = (peak - value) / peak;
            if (drawdown > max_dd) {
                max_dd = drawdown;
            }
        }
        
        return max_dd;
    }
};

/**
 * @brief 交易分析器
 * 
 * 分析交易的统计信息
 */
class TradesAnalyzer : public AnalyzerBase {
private:
    std::vector<double> trade_pnls_;
    size_t winning_trades_;
    size_t losing_trades_;
    double total_commission_;
    
public:
    explicit TradesAnalyzer() : AnalyzerBase("Trades"),
                               winning_trades_(0), losing_trades_(0),
                               total_commission_(0.0) {}
    
    void initialize() override {
        AnalyzerBase::initialize();
        trade_pnls_.clear();
        winning_trades_ = 0;
        losing_trades_ = 0;
        total_commission_ = 0.0;
    }
    
    void next() override {
        if (!broker_) return;
        
        // 这里需要检测新的交易完成
        // 实际实现中需要监听broker的交易事件
        auto stats = broker_->getTradingStatistics();
        winning_trades_ = stats.winning_trades;
        losing_trades_ = stats.losing_trades;
        
        total_commission_ = broker_->getAccountInfo().total_commission;
    }
    
    void finalize() override {
        AnalyzerBase::finalize();
        
        size_t total_trades = winning_trades_ + losing_trades_;
        
        setResult("total_trades", static_cast<double>(total_trades));
        setResult("winning_trades", static_cast<double>(winning_trades_));
        setResult("losing_trades", static_cast<double>(losing_trades_));
        
        double win_rate = (total_trades > 0) ? 
                         static_cast<double>(winning_trades_) / total_trades : 0.0;
        setResult("win_rate", win_rate * 100.0);
        
        setResult("total_commission", total_commission_);
        
        if (broker_) {
            auto stats = broker_->getTradingStatistics();
            setResult("profit_factor", stats.profit_factor);
        }
    }
};

/**
 * @brief 回撤分析器
 * 
 * 详细分析回撤情况
 */
class DrawdownAnalyzer : public AnalyzerBase {
private:
    std::vector<double> equity_curve_;
    std::vector<double> drawdown_curve_;
    double peak_equity_;
    double current_drawdown_;
    double max_drawdown_;
    size_t max_drawdown_duration_;
    size_t current_drawdown_duration_;
    
public:
    explicit DrawdownAnalyzer() : AnalyzerBase("Drawdown"),
                                 peak_equity_(0.0), current_drawdown_(0.0),
                                 max_drawdown_(0.0), max_drawdown_duration_(0),
                                 current_drawdown_duration_(0) {}
    
    void initialize() override {
        AnalyzerBase::initialize();
        
        if (broker_) {
            peak_equity_ = broker_->getAccountInfo().equity;
        }
        
        equity_curve_.clear();
        drawdown_curve_.clear();
        current_drawdown_ = 0.0;
        max_drawdown_ = 0.0;
        max_drawdown_duration_ = 0;
        current_drawdown_duration_ = 0;
    }
    
    void next() override {
        if (!broker_) return;
        
        double current_equity = broker_->getAccountInfo().equity;
        equity_curve_.push_back(current_equity);
        
        // 更新峰值
        if (current_equity > peak_equity_) {
            peak_equity_ = current_equity;
            current_drawdown_duration_ = 0;
        } else {
            current_drawdown_duration_++;
        }
        
        // 计算当前回撤
        current_drawdown_ = (peak_equity_ - current_equity) / peak_equity_;
        drawdown_curve_.push_back(current_drawdown_);
        
        // 更新最大回撤
        if (current_drawdown_ > max_drawdown_) {
            max_drawdown_ = current_drawdown_;
        }
        
        // 更新最大回撤持续时间
        if (current_drawdown_duration_ > max_drawdown_duration_) {
            max_drawdown_duration_ = current_drawdown_duration_;
        }
    }
    
    void finalize() override {
        AnalyzerBase::finalize();
        
        setResult("max_drawdown", max_drawdown_ * 100.0);
        setResult("max_drawdown_duration", static_cast<double>(max_drawdown_duration_));
        setResult("current_drawdown", current_drawdown_ * 100.0);
        setResult("current_drawdown_duration", static_cast<double>(current_drawdown_duration_));
        
        // 计算平均回撤
        if (!drawdown_curve_.empty()) {
            double sum_drawdown = 0.0;
            for (double dd : drawdown_curve_) {
                sum_drawdown += dd;
            }
            double avg_drawdown = sum_drawdown / drawdown_curve_.size();
            setResult("avg_drawdown", avg_drawdown * 100.0);
        }
    }
};

} // namespace analyzers
} // namespace backtrader