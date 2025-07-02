#pragma once

#include "strategy/StrategyBase.h"
#include "data/DataFeed.h"
#include "core/LineRoot.h"
#include <map>
#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <functional>
#include <algorithm>
#include <numeric>

namespace backtrader {
namespace portfolio {

using namespace strategy;
using namespace data;

/**
 * @brief 资产配置
 */
struct AssetAllocation {
    std::string symbol;           // 资产代码
    double target_weight;         // 目标权重 (0-1)
    double current_weight;        // 当前权重
    double current_value;         // 当前价值
    double shares_held;           // 持有股数
    double last_price;            // 最新价格
    bool is_active;               // 是否活跃
    
    AssetAllocation() : target_weight(0.0), current_weight(0.0), 
                       current_value(0.0), shares_held(0.0), 
                       last_price(0.0), is_active(true) {}
};

/**
 * @brief 再平衡配置
 */
struct RebalanceConfig {
    enum class Frequency {
        DAILY,      // 每日
        WEEKLY,     // 每周
        MONTHLY,    // 每月
        QUARTERLY,  // 每季度
        YEARLY,     // 每年
        THRESHOLD   // 阈值触发
    };
    
    Frequency frequency;                    // 再平衡频率
    double threshold;                       // 权重偏差阈值
    std::chrono::system_clock::time_point last_rebalance; // 上次再平衡时间
    int rebalance_day_of_week;             // 再平衡日(1-7, 周一到周日)
    int rebalance_day_of_month;            // 再平衡日(1-31)
    bool enabled;                          // 是否启用再平衡
    
    RebalanceConfig() : frequency(Frequency::MONTHLY), threshold(0.05),
                       rebalance_day_of_week(1), rebalance_day_of_month(1),
                       enabled(true) {}
};

/**
 * @brief 风险管理配置
 */
struct RiskConfig {
    double max_position_size;              // 单个资产最大权重
    double max_sector_exposure;            // 单个行业最大敞口
    double max_correlation;                // 最大相关性阈值
    double stop_loss_threshold;            // 止损阈值
    double volatility_scaling;             // 波动率调整因子
    bool enable_volatility_targeting;      // 是否启用波动率目标
    double target_volatility;              // 目标波动率
    
    RiskConfig() : max_position_size(0.3), max_sector_exposure(0.5),
                   max_correlation(0.8), stop_loss_threshold(-0.1),
                   volatility_scaling(1.0), enable_volatility_targeting(false),
                   target_volatility(0.15) {}
};

/**
 * @brief 组合统计信息
 */
struct PortfolioStats {
    double total_value;                    // 总价值
    double cash;                          // 现金
    double invested_value;                 // 投资价值
    double total_return;                   // 总收益率
    double daily_return;                   // 日收益率
    double volatility;                     // 波动率
    double sharpe_ratio;                   // 夏普比率
    double max_drawdown;                   // 最大回撤
    size_t num_positions;                  // 持仓数量
    double concentration;                  // 集中度 (HHI)
    std::chrono::system_clock::time_point last_update;
    
    PortfolioStats() : total_value(0.0), cash(0.0), invested_value(0.0),
                      total_return(0.0), daily_return(0.0), volatility(0.0),
                      sharpe_ratio(0.0), max_drawdown(0.0), num_positions(0),
                      concentration(0.0) {}
};

/**
 * @brief 优化目标类型
 */
enum class OptimizationObjective {
    EQUAL_WEIGHT,           // 等权重
    MARKET_CAP_WEIGHT,      // 市值加权
    RISK_PARITY,            // 风险平价
    MIN_VARIANCE,           // 最小方差
    MAX_SHARPE,             // 最大夏普比率
    MEAN_REVERSION,         // 均值回归
    MOMENTUM,               // 动量
    CUSTOM                  // 自定义
};

/**
 * @brief 组合管理器
 * 
 * 负责多资产组合的配置、再平衡、风险管理和性能监控
 */
class PortfolioManager {
private:
    // 基础配置
    std::string name_;
    double initial_capital_;
    double current_cash_;
    
    // 资产管理
    std::map<std::string, AssetAllocation> allocations_;
    std::map<std::string, std::shared_ptr<DataFeed>> data_feeds_;
    std::map<std::string, std::shared_ptr<StrategyBase>> strategies_;
    
    // 配置
    RebalanceConfig rebalance_config_;
    RiskConfig risk_config_;
    OptimizationObjective optimization_objective_;
    
    // 历史数据
    std::vector<double> portfolio_values_;
    std::vector<double> returns_;
    std::vector<PortfolioStats> stats_history_;
    
    // 当前状态
    PortfolioStats current_stats_;
    bool is_initialized_;
    
    // 回调函数
    std::function<void(const std::string&)> rebalance_callback_;
    std::function<void(const std::string&)> risk_callback_;
    std::function<void(const PortfolioStats&)> stats_callback_;
    
    // 风险监控
    std::map<std::string, std::vector<double>> price_history_;
    std::map<std::string, double> volatilities_;
    
public:
    /**
     * @brief 构造函数
     * @param name 组合名称
     * @param initial_capital 初始资金
     */
    explicit PortfolioManager(const std::string& name, double initial_capital)
        : name_(name),
          initial_capital_(initial_capital),
          current_cash_(initial_capital),
          optimization_objective_(OptimizationObjective::EQUAL_WEIGHT),
          is_initialized_(false) {}
    
    /**
     * @brief 添加资产
     * @param symbol 资产代码
     * @param data_feed 数据源
     * @param target_weight 目标权重
     * @param strategy 可选的交易策略
     */
    void addAsset(const std::string& symbol,
                  std::shared_ptr<DataFeed> data_feed,
                  double target_weight = 0.0,
                  std::shared_ptr<StrategyBase> strategy = nullptr) {
        
        if (data_feed) {
            data_feeds_[symbol] = data_feed;
        }
        
        if (strategy) {
            strategies_[symbol] = strategy;
        }
        
        AssetAllocation allocation;
        allocation.symbol = symbol;
        allocation.target_weight = target_weight;
        allocation.is_active = true;
        
        allocations_[symbol] = allocation;
    }
    
    /**
     * @brief 移除资产
     * @param symbol 资产代码
     */
    void removeAsset(const std::string& symbol) {
        // 先清仓
        if (allocations_.count(symbol)) {
            sellAll(symbol);
        }
        
        allocations_.erase(symbol);
        data_feeds_.erase(symbol);
        strategies_.erase(symbol);
        price_history_.erase(symbol);
        volatilities_.erase(symbol);
    }
    
    /**
     * @brief 设置资产权重
     * @param weights 权重映射
     */
    void setTargetWeights(const std::map<std::string, double>& weights) {
        double total_weight = 0.0;
        
        for (const auto& [symbol, weight] : weights) {
            if (allocations_.count(symbol)) {
                allocations_[symbol].target_weight = weight;
                total_weight += weight;
            }
        }
        
        // 权重归一化
        if (total_weight > 1.0) {
            for (auto& [symbol, allocation] : allocations_) {
                allocation.target_weight /= total_weight;
            }
        }
    }
    
    /**
     * @brief 设置优化目标
     * @param objective 优化目标
     */
    void setOptimizationObjective(OptimizationObjective objective) {
        optimization_objective_ = objective;
    }
    
    /**
     * @brief 设置再平衡配置
     * @param config 再平衡配置
     */
    void setRebalanceConfig(const RebalanceConfig& config) {
        rebalance_config_ = config;
    }
    
    /**
     * @brief 设置风险配置
     * @param config 风险配置
     */
    void setRiskConfig(const RiskConfig& config) {
        risk_config_ = config;
    }
    
    /**
     * @brief 初始化组合
     * @return 是否成功初始化
     */
    bool initialize() {
        if (allocations_.empty()) {
            return false;
        }
        
        // 初始化权重（如果没有设置）
        if (optimization_objective_ == OptimizationObjective::EQUAL_WEIGHT) {
            double equal_weight = 1.0 / allocations_.size();
            for (auto& [symbol, allocation] : allocations_) {
                if (allocation.target_weight == 0.0) {
                    allocation.target_weight = equal_weight;
                }
            }
        }
        
        // 初始化数据历史
        for (const auto& [symbol, data_feed] : data_feeds_) {
            price_history_[symbol].clear();
            volatilities_[symbol] = 0.0;
        }
        
        // 初始化组合价值历史
        portfolio_values_.clear();
        returns_.clear();
        stats_history_.clear();
        
        // 设置初始统计
        current_stats_.total_value = initial_capital_;
        current_stats_.cash = current_cash_;
        current_stats_.last_update = std::chrono::system_clock::now();
        
        is_initialized_ = true;
        return true;
    }
    
    /**
     * @brief 更新组合（每个交易周期调用）
     */
    void update() {
        if (!is_initialized_) {
            return;
        }
        
        // 更新价格数据
        updatePrices();
        
        // 更新持仓价值
        updatePositionValues();
        
        // 更新统计信息
        updateStats();
        
        // 检查是否需要再平衡
        if (shouldRebalance()) {
            rebalance();
        }
        
        // 风险监控
        checkRiskLimits();
        
        // 执行策略
        executeStrategies();
    }
    
    /**
     * @brief 手动再平衡
     * @return 是否成功再平衡
     */
    bool rebalance() {
        if (!is_initialized_) {
            return false;
        }
        
        // 计算目标权重
        auto target_weights = calculateOptimalWeights();
        
        // 应用风险限制
        target_weights = applyRiskLimits(target_weights);
        
        // 执行交易
        bool success = executeRebalance(target_weights);
        
        if (success) {
            rebalance_config_.last_rebalance = std::chrono::system_clock::now();
            
            if (rebalance_callback_) {
                rebalance_callback_("Portfolio rebalanced at " + 
                                   getCurrentTimeString());
            }
        }
        
        return success;
    }
    
    /**
     * @brief 获取组合统计
     * @return 当前统计信息
     */
    const PortfolioStats& getStats() const {
        return current_stats_;
    }
    
    /**
     * @brief 获取资产配置
     * @return 资产配置映射
     */
    const std::map<std::string, AssetAllocation>& getAllocations() const {
        return allocations_;
    }
    
    /**
     * @brief 获取组合价值历史
     * @return 价值历史
     */
    const std::vector<double>& getValueHistory() const {
        return portfolio_values_;
    }
    
    /**
     * @brief 获取收益率历史
     * @return 收益率历史
     */
    const std::vector<double>& getReturns() const {
        return returns_;
    }
    
    /**
     * @brief 设置再平衡回调
     * @param callback 回调函数
     */
    void setRebalanceCallback(std::function<void(const std::string&)> callback) {
        rebalance_callback_ = callback;
    }
    
    /**
     * @brief 设置风险回调
     * @param callback 回调函数
     */
    void setRiskCallback(std::function<void(const std::string&)> callback) {
        risk_callback_ = callback;
    }
    
    /**
     * @brief 设置统计回调
     * @param callback 回调函数
     */
    void setStatsCallback(std::function<void(const PortfolioStats&)> callback) {
        stats_callback_ = callback;
    }
    
    /**
     * @brief 获取权重偏差
     * @return 权重偏差映射
     */
    std::map<std::string, double> getWeightDeviations() const {
        std::map<std::string, double> deviations;
        
        for (const auto& [symbol, allocation] : allocations_) {
            double deviation = std::abs(allocation.current_weight - allocation.target_weight);
            deviations[symbol] = deviation;
        }
        
        return deviations;
    }
    
    /**
     * @brief 计算组合风险指标
     * @return 风险指标映射
     */
    std::map<std::string, double> calculateRiskMetrics() const {
        std::map<std::string, double> metrics;
        
        if (returns_.size() < 2) {
            return metrics;
        }
        
        // 计算波动率
        double mean_return = std::accumulate(returns_.begin(), returns_.end(), 0.0) / returns_.size();
        double variance = 0.0;
        for (double ret : returns_) {
            variance += std::pow(ret - mean_return, 2);
        }
        variance /= (returns_.size() - 1);
        double volatility = std::sqrt(variance) * std::sqrt(252.0); // 年化
        
        metrics["volatility"] = volatility;
        metrics["sharpe_ratio"] = current_stats_.sharpe_ratio;
        metrics["max_drawdown"] = current_stats_.max_drawdown;
        
        // 计算VaR (95%)
        std::vector<double> sorted_returns = returns_;
        std::sort(sorted_returns.begin(), sorted_returns.end());
        size_t var_index = static_cast<size_t>(sorted_returns.size() * 0.05);
        if (var_index < sorted_returns.size()) {
            metrics["var_95"] = sorted_returns[var_index];
        }
        
        // 计算集中度 (HHI)
        double hhi = 0.0;
        for (const auto& [symbol, allocation] : allocations_) {
            hhi += allocation.current_weight * allocation.current_weight;
        }
        metrics["concentration"] = hhi;
        
        return metrics;
    }
    
    /**
     * @brief 生成组合报告
     * @return 报告字符串
     */
    std::string generateReport() const {
        std::ostringstream report;
        
        report << "=== Portfolio Report: " << name_ << " ===\n";
        report << std::fixed << std::setprecision(2);
        
        // 基本信息
        report << "\n--- Portfolio Summary ---\n";
        report << "Total Value: $" << current_stats_.total_value << "\n";
        report << "Cash: $" << current_stats_.cash << "\n";
        report << "Invested Value: $" << current_stats_.invested_value << "\n";
        report << "Total Return: " << current_stats_.total_return << "%\n";
        report << "Daily Return: " << current_stats_.daily_return << "%\n";
        
        // 风险指标
        report << "\n--- Risk Metrics ---\n";
        report << "Volatility: " << std::setprecision(3) << current_stats_.volatility << "\n";
        report << "Sharpe Ratio: " << current_stats_.sharpe_ratio << "\n";
        report << "Max Drawdown: " << std::setprecision(2) << current_stats_.max_drawdown << "%\n";
        report << "Concentration (HHI): " << std::setprecision(3) << current_stats_.concentration << "\n";
        
        // 资产配置
        report << "\n--- Asset Allocations ---\n";
        report << std::setw(10) << "Symbol" << std::setw(12) << "Target%" 
               << std::setw(12) << "Current%" << std::setw(12) << "Value" 
               << std::setw(10) << "Shares" << "\n";
        report << std::string(66, '-') << "\n";
        
        for (const auto& [symbol, allocation] : allocations_) {
            report << std::setw(10) << symbol 
                   << std::setw(12) << std::setprecision(1) << allocation.target_weight * 100
                   << std::setw(12) << allocation.current_weight * 100
                   << std::setw(12) << std::setprecision(0) << allocation.current_value
                   << std::setw(10) << std::setprecision(2) << allocation.shares_held << "\n";
        }
        
        // 权重偏差
        auto deviations = getWeightDeviations();
        double max_deviation = 0.0;
        for (const auto& [symbol, deviation] : deviations) {
            max_deviation = std::max(max_deviation, deviation);
        }
        
        report << "\n--- Rebalancing Status ---\n";
        report << "Max Weight Deviation: " << std::setprecision(2) << max_deviation * 100 << "%\n";
        report << "Rebalance Threshold: " << rebalance_config_.threshold * 100 << "%\n";
        report << "Need Rebalance: " << (max_deviation > rebalance_config_.threshold ? "Yes" : "No") << "\n";
        
        return report.str();
    }
    
private:
    /**
     * @brief 更新价格数据
     */
    void updatePrices() {
        for (const auto& [symbol, data_feed] : data_feeds_) {
            if (data_feed && data_feed->close()) {
                double current_price = data_feed->close()->get(0);
                if (!isNaN(current_price)) {
                    allocations_[symbol].last_price = current_price;
                    
                    // 更新价格历史
                    price_history_[symbol].push_back(current_price);
                    if (price_history_[symbol].size() > 252) { // 保持一年的数据
                        price_history_[symbol].erase(price_history_[symbol].begin());
                    }
                    
                    // 计算波动率
                    if (price_history_[symbol].size() > 20) {
                        volatilities_[symbol] = calculateVolatility(symbol);
                    }
                }
            }
        }
    }
    
    /**
     * @brief 更新持仓价值
     */
    void updatePositionValues() {
        double total_invested = 0.0;
        
        for (auto& [symbol, allocation] : allocations_) {
            allocation.current_value = allocation.shares_held * allocation.last_price;
            total_invested += allocation.current_value;
        }
        
        current_stats_.invested_value = total_invested;
        current_stats_.total_value = current_stats_.cash + total_invested;
        
        // 更新权重
        if (current_stats_.total_value > 0) {
            for (auto& [symbol, allocation] : allocations_) {
                allocation.current_weight = allocation.current_value / current_stats_.total_value;
            }
        }
    }
    
    /**
     * @brief 更新统计信息
     */
    void updateStats() {
        // 更新价值历史
        portfolio_values_.push_back(current_stats_.total_value);
        
        // 计算收益率
        if (portfolio_values_.size() > 1) {
            size_t prev_idx = portfolio_values_.size() - 2;
            double prev_value = portfolio_values_[prev_idx];
            double current_return = (current_stats_.total_value - prev_value) / prev_value;
            returns_.push_back(current_return);
            current_stats_.daily_return = current_return * 100.0;
        }
        
        // 计算总收益率
        current_stats_.total_return = (current_stats_.total_value - initial_capital_) / initial_capital_ * 100.0;
        
        // 计算其他统计指标
        if (returns_.size() > 1) {
            calculateAdvancedStats();
        }
        
        // 更新持仓数量
        current_stats_.num_positions = 0;
        for (const auto& [symbol, allocation] : allocations_) {
            if (allocation.shares_held > 0) {
                current_stats_.num_positions++;
            }
        }
        
        // 计算集中度
        double hhi = 0.0;
        for (const auto& [symbol, allocation] : allocations_) {
            hhi += allocation.current_weight * allocation.current_weight;
        }
        current_stats_.concentration = hhi;
        
        current_stats_.last_update = std::chrono::system_clock::now();
        
        // 调用回调
        if (stats_callback_) {
            stats_callback_(current_stats_);
        }
    }
    
    /**
     * @brief 计算高级统计指标
     */
    void calculateAdvancedStats() {
        if (returns_.empty()) return;
        
        // 计算波动率
        double mean_return = std::accumulate(returns_.begin(), returns_.end(), 0.0) / returns_.size();
        double variance = 0.0;
        for (double ret : returns_) {
            variance += std::pow(ret - mean_return, 2);
        }
        variance /= (returns_.size() - 1);
        current_stats_.volatility = std::sqrt(variance) * std::sqrt(252.0); // 年化
        
        // 计算夏普比率（假设无风险利率为2%）
        double risk_free_rate = 0.02 / 252.0; // 日无风险利率
        double excess_return = mean_return - risk_free_rate;
        current_stats_.sharpe_ratio = (std::sqrt(variance) > 0) ? 
            excess_return / std::sqrt(variance) * std::sqrt(252.0) : 0.0;
        
        // 计算最大回撤
        double peak = initial_capital_;
        double max_dd = 0.0;
        
        for (double value : portfolio_values_) {
            if (value > peak) {
                peak = value;
            }
            double drawdown = (peak - value) / peak;
            max_dd = std::max(max_dd, drawdown);
        }
        
        current_stats_.max_drawdown = max_dd * 100.0;
    }
    
    /**
     * @brief 检查是否需要再平衡
     * @return 是否需要再平衡
     */
    bool shouldRebalance() {
        if (!rebalance_config_.enabled) {
            return false;
        }
        
        // 检查权重偏差
        if (rebalance_config_.frequency == RebalanceConfig::Frequency::THRESHOLD) {
            auto deviations = getWeightDeviations();
            for (const auto& [symbol, deviation] : deviations) {
                if (deviation > rebalance_config_.threshold) {
                    return true;
                }
            }
            return false;
        }
        
        // 检查时间频率
        auto now = std::chrono::system_clock::now();
        auto time_since_last = now - rebalance_config_.last_rebalance;
        
        switch (rebalance_config_.frequency) {
            case RebalanceConfig::Frequency::DAILY:
                return std::chrono::duration_cast<std::chrono::hours>(time_since_last).count() >= 24;
            case RebalanceConfig::Frequency::WEEKLY:
                return std::chrono::duration_cast<std::chrono::hours>(time_since_last).count() >= 7 * 24;
            case RebalanceConfig::Frequency::MONTHLY:
                return std::chrono::duration_cast<std::chrono::hours>(time_since_last).count() >= 30 * 24;
            case RebalanceConfig::Frequency::QUARTERLY:
                return std::chrono::duration_cast<std::chrono::hours>(time_since_last).count() >= 90 * 24;
            case RebalanceConfig::Frequency::YEARLY:
                return std::chrono::duration_cast<std::chrono::hours>(time_since_last).count() >= 365 * 24;
            default:
                return false;
        }
    }
    
    /**
     * @brief 计算最优权重
     * @return 最优权重映射
     */
    std::map<std::string, double> calculateOptimalWeights() {
        std::map<std::string, double> weights;
        
        switch (optimization_objective_) {
            case OptimizationObjective::EQUAL_WEIGHT:
                {
                    double equal_weight = 1.0 / allocations_.size();
                    for (const auto& [symbol, allocation] : allocations_) {
                        weights[symbol] = equal_weight;
                    }
                }
                break;
                
            case OptimizationObjective::RISK_PARITY:
                weights = calculateRiskParityWeights();
                break;
                
            case OptimizationObjective::MIN_VARIANCE:
                weights = calculateMinVarianceWeights();
                break;
                
            case OptimizationObjective::MOMENTUM:
                weights = calculateMomentumWeights();
                break;
                
            case OptimizationObjective::MEAN_REVERSION:
                weights = calculateMeanReversionWeights();
                break;
                
            default:
                // 使用当前目标权重
                for (const auto& [symbol, allocation] : allocations_) {
                    weights[symbol] = allocation.target_weight;
                }
        }
        
        return weights;
    }
    
    /**
     * @brief 计算风险平价权重
     * @return 权重映射
     */
    std::map<std::string, double> calculateRiskParityWeights() {
        std::map<std::string, double> weights;
        
        // 简化的风险平价：按波动率倒数加权
        double total_inv_vol = 0.0;
        for (const auto& [symbol, vol] : volatilities_) {
            if (vol > 0) {
                total_inv_vol += 1.0 / vol;
            }
        }
        
        if (total_inv_vol > 0) {
            for (const auto& [symbol, vol] : volatilities_) {
                if (vol > 0) {
                    weights[symbol] = (1.0 / vol) / total_inv_vol;
                } else {
                    weights[symbol] = 0.0;
                }
            }
        } else {
            // 回退到等权重
            double equal_weight = 1.0 / allocations_.size();
            for (const auto& [symbol, allocation] : allocations_) {
                weights[symbol] = equal_weight;
            }
        }
        
        return weights;
    }
    
    /**
     * @brief 计算最小方差权重
     * @return 权重映射
     */
    std::map<std::string, double> calculateMinVarianceWeights() {
        // 这里应该实现协方差矩阵优化
        // 为简化，使用风险平价的近似
        return calculateRiskParityWeights();
    }
    
    /**
     * @brief 计算动量权重
     * @return 权重映射
     */
    std::map<std::string, double> calculateMomentumWeights() {
        std::map<std::string, double> weights;
        std::map<std::string, double> momentum_scores;
        
        // 计算动量分数（简化为最近收益率）
        double total_momentum = 0.0;
        for (const auto& [symbol, price_hist] : price_history_) {
            if (price_hist.size() >= 20) {
                double recent_return = (price_hist.back() - price_hist[price_hist.size()-20]) / price_hist[price_hist.size()-20];
                momentum_scores[symbol] = std::max(0.0, recent_return); // 只考虑正动量
                total_momentum += momentum_scores[symbol];
            }
        }
        
        // 按动量分配权重
        if (total_momentum > 0) {
            for (const auto& [symbol, momentum] : momentum_scores) {
                weights[symbol] = momentum / total_momentum;
            }
        } else {
            // 回退到等权重
            double equal_weight = 1.0 / allocations_.size();
            for (const auto& [symbol, allocation] : allocations_) {
                weights[symbol] = equal_weight;
            }
        }
        
        return weights;
    }
    
    /**
     * @brief 计算均值回归权重
     * @return 权重映射
     */
    std::map<std::string, double> calculateMeanReversionWeights() {
        std::map<std::string, double> weights;
        
        // 简化的均值回归：偏向表现较差的资产
        std::map<std::string, double> performance_scores;
        double total_inv_performance = 0.0;
        
        for (const auto& [symbol, price_hist] : price_history_) {
            if (price_hist.size() >= 50) {
                double recent_return = (price_hist.back() - price_hist[price_hist.size()-50]) / price_hist[price_hist.size()-50];
                double inv_performance = 1.0 / (1.0 + recent_return); // 表现越差权重越高
                performance_scores[symbol] = inv_performance;
                total_inv_performance += inv_performance;
            }
        }
        
        if (total_inv_performance > 0) {
            for (const auto& [symbol, score] : performance_scores) {
                weights[symbol] = score / total_inv_performance;
            }
        } else {
            // 回退到等权重
            double equal_weight = 1.0 / allocations_.size();
            for (const auto& [symbol, allocation] : allocations_) {
                weights[symbol] = equal_weight;
            }
        }
        
        return weights;
    }
    
    /**
     * @brief 应用风险限制
     * @param weights 原始权重
     * @return 调整后权重
     */
    std::map<std::string, double> applyRiskLimits(const std::map<std::string, double>& weights) {
        std::map<std::string, double> adjusted_weights = weights;
        
        // 应用最大单个资产权重限制
        for (auto& [symbol, weight] : adjusted_weights) {
            if (weight > risk_config_.max_position_size) {
                weight = risk_config_.max_position_size;
            }
        }
        
        // 重新归一化
        double total_weight = 0.0;
        for (const auto& [symbol, weight] : adjusted_weights) {
            total_weight += weight;
        }
        
        if (total_weight > 0 && total_weight != 1.0) {
            for (auto& [symbol, weight] : adjusted_weights) {
                weight /= total_weight;
            }
        }
        
        return adjusted_weights;
    }
    
    /**
     * @brief 执行再平衡交易
     * @param target_weights 目标权重
     * @return 是否成功
     */
    bool executeRebalance(const std::map<std::string, double>& target_weights) {
        // 计算需要调整的金额
        double total_value = current_stats_.total_value;
        
        for (const auto& [symbol, target_weight] : target_weights) {
            if (allocations_.count(symbol)) {
                double target_value = total_value * target_weight;
                double current_value = allocations_[symbol].current_value;
                double diff = target_value - current_value;
                
                if (std::abs(diff) > total_value * 0.001) { // 0.1%阈值
                    if (diff > 0) {
                        // 需要买入
                        buy(symbol, diff);
                    } else {
                        // 需要卖出
                        sell(symbol, -diff);
                    }
                }
            }
        }
        
        return true;
    }
    
    /**
     * @brief 买入操作
     * @param symbol 资产代码
     * @param value 买入价值
     */
    void buy(const std::string& symbol, double value) {
        if (allocations_.count(symbol) && value > 0 && current_stats_.cash >= value) {
            double price = allocations_[symbol].last_price;
            if (price > 0) {
                double shares = value / price;
                allocations_[symbol].shares_held += shares;
                current_stats_.cash -= value;
            }
        }
    }
    
    /**
     * @brief 卖出操作
     * @param symbol 资产代码
     * @param value 卖出价值
     */
    void sell(const std::string& symbol, double value) {
        if (allocations_.count(symbol) && value > 0) {
            double price = allocations_[symbol].last_price;
            if (price > 0) {
                double shares_to_sell = value / price;
                double available_shares = allocations_[symbol].shares_held;
                
                if (shares_to_sell <= available_shares) {
                    allocations_[symbol].shares_held -= shares_to_sell;
                    current_stats_.cash += value;
                } else {
                    // 全部卖出
                    double proceeds = available_shares * price;
                    allocations_[symbol].shares_held = 0;
                    current_stats_.cash += proceeds;
                }
            }
        }
    }
    
    /**
     * @brief 清仓操作
     * @param symbol 资产代码
     */
    void sellAll(const std::string& symbol) {
        if (allocations_.count(symbol)) {
            double shares = allocations_[symbol].shares_held;
            double price = allocations_[symbol].last_price;
            
            if (shares > 0 && price > 0) {
                double proceeds = shares * price;
                allocations_[symbol].shares_held = 0;
                current_stats_.cash += proceeds;
            }
        }
    }
    
    /**
     * @brief 计算波动率
     * @param symbol 资产代码
     * @return 波动率
     */
    double calculateVolatility(const std::string& symbol) const {
        if (!price_history_.count(symbol)) {
            return 0.0;
        }
        
        const auto& prices = price_history_.at(symbol);
        if (prices.size() < 2) {
            return 0.0;
        }
        
        std::vector<double> returns;
        for (size_t i = 1; i < prices.size(); ++i) {
            double ret = (prices[i] - prices[i-1]) / prices[i-1];
            returns.push_back(ret);
        }
        
        if (returns.empty()) {
            return 0.0;
        }
        
        double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
        double variance = 0.0;
        for (double ret : returns) {
            variance += std::pow(ret - mean_return, 2);
        }
        variance /= (returns.size() - 1);
        
        return std::sqrt(variance) * std::sqrt(252.0); // 年化波动率
    }
    
    /**
     * @brief 检查风险限制
     */
    void checkRiskLimits() {
        // 检查单个资产权重限制
        for (const auto& [symbol, allocation] : allocations_) {
            if (allocation.current_weight > risk_config_.max_position_size) {
                if (risk_callback_) {
                    risk_callback_("Position size limit exceeded for " + symbol);
                }
            }
        }
        
        // 检查止损
        if (current_stats_.daily_return < risk_config_.stop_loss_threshold * 100.0) {
            if (risk_callback_) {
                risk_callback_("Stop loss triggered: daily return " + 
                              std::to_string(current_stats_.daily_return) + "%");
            }
        }
    }
    
    /**
     * @brief 执行策略
     */
    void executeStrategies() {
        for (const auto& [symbol, strategy] : strategies_) {
            if (strategy) {
                strategy->processNext();
            }
        }
    }
    
    /**
     * @brief 获取当前时间字符串
     * @return 时间字符串
     */
    std::string getCurrentTimeString() const {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
};

} // namespace portfolio
} // namespace backtrader