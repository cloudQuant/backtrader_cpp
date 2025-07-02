#pragma once

#include "AnalyzerBase.h"
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace backtrader {
namespace analyzers {

/**
 * @brief 性能指标结构
 */
struct PerformanceMetrics {
    // 收益指标
    double total_return;          // 总收益率 (%)
    double annualized_return;     // 年化收益率 (%)
    double cumulative_return;     // 累积收益率 (%)
    
    // 风险指标
    double volatility;            // 波动率 (%)
    double max_drawdown;          // 最大回撤 (%)
    double current_drawdown;      // 当前回撤 (%)
    double calmar_ratio;          // 卡玛比率
    double sterling_ratio;        // 斯特林比率
    
    // 风险调整收益指标
    double sharpe_ratio;          // 夏普比率
    double sortino_ratio;         // 索提诺比率
    double information_ratio;     // 信息比率
    double treynor_ratio;         // 特雷诺比率
    
    // 交易指标
    size_t total_trades;          // 总交易次数
    size_t winning_trades;        // 盈利交易次数
    size_t losing_trades;         // 亏损交易次数
    double win_rate;              // 胜率 (%)
    double avg_win;               // 平均盈利
    double avg_loss;              // 平均亏损
    double profit_factor;         // 盈亏比
    double largest_win;           // 最大单笔盈利
    double largest_loss;          // 最大单笔亏损
    
    // 一致性指标
    double consecutive_wins;      // 最大连续盈利次数
    double consecutive_losses;    // 最大连续亏损次数
    double avg_trade_duration;    // 平均持仓时间 (天)
    double recovery_factor;       // 恢复因子
    
    // 分布指标
    double skewness;              // 收益偏度
    double kurtosis;              // 收益峰度
    double var_95;                // 95% VaR
    double cvar_95;               // 95% CVaR
    
    // 时间相关指标
    double beta;                  // Beta系数 (相对基准)
    double alpha;                 // Alpha系数 (相对基准)
    double tracking_error;        // 跟踪误差
    double correlation;           // 与基准相关性
    
    PerformanceMetrics() {
        // 初始化所有指标为0或NaN
        total_return = 0.0;
        annualized_return = 0.0;
        cumulative_return = 0.0;
        volatility = 0.0;
        max_drawdown = 0.0;
        current_drawdown = 0.0;
        calmar_ratio = 0.0;
        sterling_ratio = 0.0;
        sharpe_ratio = 0.0;
        sortino_ratio = 0.0;
        information_ratio = 0.0;
        treynor_ratio = 0.0;
        total_trades = 0;
        winning_trades = 0;
        losing_trades = 0;
        win_rate = 0.0;
        avg_win = 0.0;
        avg_loss = 0.0;
        profit_factor = 0.0;
        largest_win = 0.0;
        largest_loss = 0.0;
        consecutive_wins = 0;
        consecutive_losses = 0;
        avg_trade_duration = 0.0;
        recovery_factor = 0.0;
        skewness = 0.0;
        kurtosis = 0.0;
        var_95 = 0.0;
        cvar_95 = 0.0;
        beta = 0.0;
        alpha = 0.0;
        tracking_error = 0.0;
        correlation = 0.0;
    }
};

/**
 * @brief 交易记录
 */
struct TradeRecord {
    std::chrono::system_clock::time_point entry_time;
    std::chrono::system_clock::time_point exit_time;
    double entry_price;
    double exit_price;
    double size;
    double pnl;
    double commission;
    double slippage;
    double duration_hours;
    bool is_long;
    
    TradeRecord() : entry_price(0.0), exit_price(0.0), size(0.0), 
                   pnl(0.0), commission(0.0), slippage(0.0), 
                   duration_hours(0.0), is_long(true) {}
};

/**
 * @brief 周期性能统计
 */
struct PeriodStats {
    std::string period_name;      // 期间名称
    double return_pct;            // 收益率
    double volatility_pct;        // 波动率
    double max_dd_pct;            // 最大回撤
    double sharpe_ratio;          // 夏普比率
    size_t trade_count;           // 交易次数
    double win_rate;              // 胜率
    
    PeriodStats() : return_pct(0.0), volatility_pct(0.0), max_dd_pct(0.0),
                   sharpe_ratio(0.0), trade_count(0), win_rate(0.0) {}
};

/**
 * @brief 性能分析器
 * 
 * 提供全面的策略性能分析，包括收益、风险、交易统计等多个维度
 */
class PerformanceAnalyzer : public AnalyzerBase {
private:
    // 基础数据
    std::vector<double> equity_curve_;
    std::vector<double> returns_;
    std::vector<double> benchmark_returns_;
    std::vector<TradeRecord> trades_;
    
    // 配置参数
    double initial_capital_;
    double risk_free_rate_;        // 无风险利率 (年化)
    size_t trading_days_per_year_; // 每年交易日数
    
    // 实时统计
    double peak_equity_;
    double current_equity_;
    double total_commission_;
    double total_slippage_;
    
    // 连续统计
    int current_win_streak_;
    int current_loss_streak_;
    int max_win_streak_;
    int max_loss_streak_;
    
    // 分析结果
    PerformanceMetrics metrics_;
    std::vector<PeriodStats> period_stats_;
    
    // 基准数据
    std::shared_ptr<LineRoot> benchmark_line_;
    bool use_benchmark_;
    
public:
    /**
     * @brief 构造函数
     * @param initial_capital 初始资金
     * @param risk_free_rate 无风险利率 (年化)
     */
    explicit PerformanceAnalyzer(double initial_capital = 100000.0,
                                double risk_free_rate = 0.02)
        : AnalyzerBase("PerformanceAnalyzer"),
          initial_capital_(initial_capital),
          risk_free_rate_(risk_free_rate),
          trading_days_per_year_(252),
          peak_equity_(initial_capital),
          current_equity_(initial_capital),
          total_commission_(0.0),
          total_slippage_(0.0),
          current_win_streak_(0),
          current_loss_streak_(0),
          max_win_streak_(0),
          max_loss_streak_(0),
          use_benchmark_(false) {}
    
    /**
     * @brief 设置基准
     * @param benchmark_line 基准数据线
     */
    void setBenchmark(std::shared_ptr<LineRoot> benchmark_line) {
        benchmark_line_ = benchmark_line;
        use_benchmark_ = true;
    }
    
    /**
     * @brief 设置无风险利率
     * @param rate 年化无风险利率
     */
    void setRiskFreeRate(double rate) {
        risk_free_rate_ = rate;
    }
    
    /**
     * @brief 设置每年交易日数
     * @param days 交易日数
     */
    void setTradingDaysPerYear(size_t days) {
        trading_days_per_year_ = days;
    }
    
    /**
     * @brief 初始化分析器
     */
    void initialize() override {
        equity_curve_.clear();
        returns_.clear();
        benchmark_returns_.clear();
        trades_.clear();
        period_stats_.clear();
        
        peak_equity_ = initial_capital_;
        current_equity_ = initial_capital_;
        
        // 添加初始权益点
        equity_curve_.push_back(initial_capital_);
    }
    
    /**
     * @brief 处理下一个数据点
     */
    void next() override {
        // 获取当前权益
        double equity = getCurrentEquity();
        current_equity_ = equity;
        
        // 更新权益曲线
        equity_curve_.push_back(equity);
        
        // 计算收益率
        if (equity_curve_.size() > 1) {
            double prev_equity = equity_curve_[equity_curve_.size() - 2];
            double return_pct = (equity - prev_equity) / prev_equity;
            returns_.push_back(return_pct);
        }
        
        // 更新峰值
        if (equity > peak_equity_) {
            peak_equity_ = equity;
        }
        
        // 处理基准数据
        if (use_benchmark_ && benchmark_line_) {
            processBenchmarkData();
        }
        
        // 实时更新性能指标
        updateRealTimeMetrics();
    }
    
    /**
     * @brief 添加交易记录
     * @param trade 交易记录
     */
    void addTrade(const TradeRecord& trade) {
        trades_.push_back(trade);
        
        // 更新连续统计
        if (trade.pnl > 0) {
            current_win_streak_++;
            current_loss_streak_ = 0;
            max_win_streak_ = std::max(max_win_streak_, current_win_streak_);
        } else if (trade.pnl < 0) {
            current_loss_streak_++;
            current_win_streak_ = 0;
            max_loss_streak_ = std::max(max_loss_streak_, current_loss_streak_);
        }
        
        total_commission_ += trade.commission;
        total_slippage_ += trade.slippage;
    }
    
    /**
     * @brief 完成分析
     */
    void finalize() override {
        calculateAllMetrics();
        generatePeriodStats();
    }
    
    /**
     * @brief 获取性能指标
     * @return 性能指标
     */
    const PerformanceMetrics& getMetrics() const {
        return metrics_;
    }
    
    /**
     * @brief 获取权益曲线
     * @return 权益曲线
     */
    const std::vector<double>& getEquityCurve() const {
        return equity_curve_;
    }
    
    /**
     * @brief 获取收益率序列
     * @return 收益率序列
     */
    const std::vector<double>& getReturns() const {
        return returns_;
    }
    
    /**
     * @brief 获取交易记录
     * @return 交易记录
     */
    const std::vector<TradeRecord>& getTrades() const {
        return trades_;
    }
    
    /**
     * @brief 获取周期统计
     * @return 周期统计
     */
    const std::vector<PeriodStats>& getPeriodStats() const {
        return period_stats_;
    }
    
    /**
     * @brief 生成性能报告
     * @param detailed 是否生成详细报告
     * @return 报告字符串
     */
    std::string generateReport(bool detailed = true) const {
        std::ostringstream report;
        
        report << "=== Performance Analysis Report ===\n";
        report << std::fixed << std::setprecision(2);
        
        // 基本信息
        report << "\n--- Basic Information ---\n";
        report << "Initial Capital: $" << initial_capital_ << "\n";
        report << "Final Equity: $" << current_equity_ << "\n";
        report << "Total Commission: $" << total_commission_ << "\n";
        report << "Total Slippage: $" << total_slippage_ << "\n";
        
        // 收益指标
        report << "\n--- Return Metrics ---\n";
        report << "Total Return: " << metrics_.total_return << "%\n";
        report << "Annualized Return: " << metrics_.annualized_return << "%\n";
        report << "Cumulative Return: " << metrics_.cumulative_return << "%\n";
        
        // 风险指标
        report << "\n--- Risk Metrics ---\n";
        report << "Volatility: " << metrics_.volatility << "%\n";
        report << "Max Drawdown: " << metrics_.max_drawdown << "%\n";
        report << "Current Drawdown: " << metrics_.current_drawdown << "%\n";
        
        // 风险调整收益
        report << "\n--- Risk-Adjusted Returns ---\n";
        report << "Sharpe Ratio: " << std::setprecision(3) << metrics_.sharpe_ratio << "\n";
        report << "Sortino Ratio: " << metrics_.sortino_ratio << "\n";
        report << "Calmar Ratio: " << metrics_.calmar_ratio << "\n";
        if (use_benchmark_) {
            report << "Information Ratio: " << metrics_.information_ratio << "\n";
            report << "Treynor Ratio: " << metrics_.treynor_ratio << "\n";
        }
        
        // 交易统计
        report << "\n--- Trading Statistics ---\n";
        report << std::setprecision(2);
        report << "Total Trades: " << metrics_.total_trades << "\n";
        report << "Winning Trades: " << metrics_.winning_trades << "\n";
        report << "Losing Trades: " << metrics_.losing_trades << "\n";
        report << "Win Rate: " << metrics_.win_rate << "%\n";
        report << "Profit Factor: " << std::setprecision(3) << metrics_.profit_factor << "\n";
        report << "Average Win: $" << std::setprecision(2) << metrics_.avg_win << "\n";
        report << "Average Loss: $" << metrics_.avg_loss << "\n";
        report << "Largest Win: $" << metrics_.largest_win << "\n";
        report << "Largest Loss: $" << metrics_.largest_loss << "\n";
        
        // 一致性指标
        report << "\n--- Consistency Metrics ---\n";
        report << "Max Consecutive Wins: " << static_cast<int>(metrics_.consecutive_wins) << "\n";
        report << "Max Consecutive Losses: " << static_cast<int>(metrics_.consecutive_losses) << "\n";
        report << "Average Trade Duration: " << metrics_.avg_trade_duration << " days\n";
        report << "Recovery Factor: " << std::setprecision(3) << metrics_.recovery_factor << "\n";
        
        // 分布统计
        if (detailed) {
            report << "\n--- Distribution Statistics ---\n";
            report << std::setprecision(3);
            report << "Skewness: " << metrics_.skewness << "\n";
            report << "Kurtosis: " << metrics_.kurtosis << "\n";
            report << "95% VaR: " << std::setprecision(2) << metrics_.var_95 << "%\n";
            report << "95% CVaR: " << metrics_.cvar_95 << "%\n";
        }
        
        // 基准比较
        if (use_benchmark_ && detailed) {
            report << "\n--- Benchmark Comparison ---\n";
            report << std::setprecision(3);
            report << "Beta: " << metrics_.beta << "\n";
            report << "Alpha: " << std::setprecision(2) << metrics_.alpha << "%\n";
            report << "Tracking Error: " << metrics_.tracking_error << "%\n";
            report << "Correlation: " << std::setprecision(3) << metrics_.correlation << "\n";
        }
        
        // 周期统计
        if (detailed && !period_stats_.empty()) {
            report << "\n--- Period Analysis ---\n";
            for (const auto& period : period_stats_) {
                report << period.period_name << ": ";
                report << "Return=" << std::setprecision(2) << period.return_pct << "%, ";
                report << "Volatility=" << period.volatility_pct << "%, ";
                report << "Sharpe=" << std::setprecision(3) << period.sharpe_ratio << "\n";
            }
        }
        
        return report.str();
    }
    
    /**
     * @brief 计算当前回撤
     * @return 回撤百分比
     */
    double getCurrentDrawdown() const {
        if (peak_equity_ <= 0) return 0.0;
        return (peak_equity_ - current_equity_) / peak_equity_ * 100.0;
    }
    
    /**
     * @brief 获取实时夏普比率
     * @return 夏普比率
     */
    double getRealTimeSharpe() const {
        if (returns_.size() < 2) return 0.0;
        
        double avg_return = std::accumulate(returns_.begin(), returns_.end(), 0.0) / returns_.size();
        double excess_return = avg_return - risk_free_rate_ / trading_days_per_year_;
        
        if (returns_.size() < 2) return 0.0;
        
        double variance = 0.0;
        for (double ret : returns_) {
            variance += std::pow(ret - avg_return, 2);
        }
        variance /= (returns_.size() - 1);
        double volatility = std::sqrt(variance);
        
        return (volatility > 0) ? excess_return / volatility * std::sqrt(trading_days_per_year_) : 0.0;
    }
    
private:
    /**
     * @brief 获取当前权益
     * @return 当前权益值
     */
    double getCurrentEquity() {
        // 这里应该从broker获取实际权益
        // 为简化，直接返回当前值
        return current_equity_;
    }
    
    /**
     * @brief 处理基准数据
     */
    void processBenchmarkData() {
        if (!benchmark_line_) return;
        
        double current_benchmark = benchmark_line_->get(0);
        double prev_benchmark = benchmark_line_->get(-1);
        
        if (!isNaN(current_benchmark) && !isNaN(prev_benchmark) && prev_benchmark != 0) {
            double benchmark_return = (current_benchmark - prev_benchmark) / prev_benchmark;
            benchmark_returns_.push_back(benchmark_return);
        }
    }
    
    /**
     * @brief 更新实时指标
     */
    void updateRealTimeMetrics() {
        metrics_.current_drawdown = getCurrentDrawdown();
        metrics_.sharpe_ratio = getRealTimeSharpe();
        
        if (!equity_curve_.empty()) {
            metrics_.total_return = (current_equity_ - initial_capital_) / initial_capital_ * 100.0;
        }
    }
    
    /**
     * @brief 计算所有指标
     */
    void calculateAllMetrics() {
        calculateReturnMetrics();
        calculateRiskMetrics();
        calculateRiskAdjustedMetrics();
        calculateTradingMetrics();
        calculateConsistencyMetrics();
        calculateDistributionMetrics();
        
        if (use_benchmark_) {
            calculateBenchmarkMetrics();
        }
    }
    
    /**
     * @brief 计算收益指标
     */
    void calculateReturnMetrics() {
        if (equity_curve_.empty()) return;
        
        metrics_.total_return = (current_equity_ - initial_capital_) / initial_capital_ * 100.0;
        metrics_.cumulative_return = metrics_.total_return;
        
        // 年化收益率（简化计算）
        double trading_periods = static_cast<double>(equity_curve_.size() - 1);
        if (trading_periods > 0) {
            double periods_per_year = trading_days_per_year_;
            double years = trading_periods / periods_per_year;
            if (years > 0) {
                metrics_.annualized_return = std::pow(current_equity_ / initial_capital_, 1.0 / years) - 1.0;
                metrics_.annualized_return *= 100.0;
            }
        }
    }
    
    /**
     * @brief 计算风险指标
     */
    void calculateRiskMetrics() {
        // 计算波动率
        if (returns_.size() > 1) {
            double mean_return = std::accumulate(returns_.begin(), returns_.end(), 0.0) / returns_.size();
            double variance = 0.0;
            for (double ret : returns_) {
                variance += std::pow(ret - mean_return, 2);
            }
            variance /= (returns_.size() - 1);
            metrics_.volatility = std::sqrt(variance) * std::sqrt(trading_days_per_year_) * 100.0;
        }
        
        // 计算最大回撤
        double max_dd = 0.0;
        double peak = initial_capital_;
        
        for (double equity : equity_curve_) {
            if (equity > peak) {
                peak = equity;
            }
            double drawdown = (peak - equity) / peak;
            max_dd = std::max(max_dd, drawdown);
        }
        
        metrics_.max_drawdown = max_dd * 100.0;
        metrics_.current_drawdown = getCurrentDrawdown();
    }
    
    /**
     * @brief 计算风险调整收益指标
     */
    void calculateRiskAdjustedMetrics() {
        if (returns_.empty() || metrics_.volatility == 0) return;
        
        double avg_return = std::accumulate(returns_.begin(), returns_.end(), 0.0) / returns_.size();
        double annualized_avg_return = avg_return * trading_days_per_year_ * 100.0;
        double excess_return = annualized_avg_return - risk_free_rate_ * 100.0;
        
        // 夏普比率
        metrics_.sharpe_ratio = excess_return / metrics_.volatility;
        
        // 索提诺比率（只考虑负收益的波动）
        std::vector<double> negative_returns;
        for (double ret : returns_) {
            if (ret < 0) {
                negative_returns.push_back(ret);
            }
        }
        
        if (!negative_returns.empty()) {
            double mean_neg = std::accumulate(negative_returns.begin(), negative_returns.end(), 0.0) / negative_returns.size();
            double downside_variance = 0.0;
            for (double ret : negative_returns) {
                downside_variance += std::pow(ret - mean_neg, 2);
            }
            downside_variance /= negative_returns.size();
            double downside_volatility = std::sqrt(downside_variance) * std::sqrt(trading_days_per_year_) * 100.0;
            
            if (downside_volatility > 0) {
                metrics_.sortino_ratio = excess_return / downside_volatility;
            }
        }
        
        // 卡玛比率
        if (metrics_.max_drawdown > 0) {
            metrics_.calmar_ratio = metrics_.annualized_return / metrics_.max_drawdown;
        }
        
        // 恢复因子
        if (metrics_.max_drawdown > 0) {
            metrics_.recovery_factor = metrics_.total_return / metrics_.max_drawdown;
        }
    }
    
    /**
     * @brief 计算交易指标
     */
    void calculateTradingMetrics() {
        if (trades_.empty()) return;
        
        metrics_.total_trades = trades_.size();
        
        double total_pnl = 0.0;
        double total_wins = 0.0;
        double total_losses = 0.0;
        double total_duration = 0.0;
        
        for (const auto& trade : trades_) {
            total_pnl += trade.pnl;
            total_duration += trade.duration_hours / 24.0; // 转换为天
            
            if (trade.pnl > 0) {
                metrics_.winning_trades++;
                total_wins += trade.pnl;
                metrics_.largest_win = std::max(metrics_.largest_win, trade.pnl);
            } else if (trade.pnl < 0) {
                metrics_.losing_trades++;
                total_losses += std::abs(trade.pnl);
                metrics_.largest_loss = std::min(metrics_.largest_loss, trade.pnl);
            }
        }
        
        metrics_.win_rate = (metrics_.total_trades > 0) ? 
            static_cast<double>(metrics_.winning_trades) / metrics_.total_trades * 100.0 : 0.0;
        
        metrics_.avg_win = (metrics_.winning_trades > 0) ? total_wins / metrics_.winning_trades : 0.0;
        metrics_.avg_loss = (metrics_.losing_trades > 0) ? total_losses / metrics_.losing_trades : 0.0;
        
        metrics_.profit_factor = (total_losses > 0) ? total_wins / total_losses : 0.0;
        
        metrics_.avg_trade_duration = (metrics_.total_trades > 0) ? 
            total_duration / metrics_.total_trades : 0.0;
    }
    
    /**
     * @brief 计算一致性指标
     */
    void calculateConsistencyMetrics() {
        metrics_.consecutive_wins = max_win_streak_;
        metrics_.consecutive_losses = max_loss_streak_;
    }
    
    /**
     * @brief 计算分布指标
     */
    void calculateDistributionMetrics() {
        if (returns_.size() < 3) return;
        
        double mean = std::accumulate(returns_.begin(), returns_.end(), 0.0) / returns_.size();
        
        // 计算偏度和峰度
        double variance = 0.0;
        double skew_sum = 0.0;
        double kurt_sum = 0.0;
        
        for (double ret : returns_) {
            double diff = ret - mean;
            variance += diff * diff;
            skew_sum += diff * diff * diff;
            kurt_sum += diff * diff * diff * diff;
        }
        
        variance /= (returns_.size() - 1);
        double std_dev = std::sqrt(variance);
        
        if (std_dev > 0) {
            metrics_.skewness = (skew_sum / returns_.size()) / std::pow(std_dev, 3);
            metrics_.kurtosis = (kurt_sum / returns_.size()) / std::pow(std_dev, 4) - 3.0;
        }
        
        // 计算VaR和CVaR
        std::vector<double> sorted_returns = returns_;
        std::sort(sorted_returns.begin(), sorted_returns.end());
        
        size_t var_index = static_cast<size_t>(sorted_returns.size() * 0.05);
        if (var_index < sorted_returns.size()) {
            metrics_.var_95 = sorted_returns[var_index] * 100.0;
            
            // CVaR是VaR以下收益的平均值
            if (var_index > 0) {
                double cvar_sum = 0.0;
                for (size_t i = 0; i < var_index; ++i) {
                    cvar_sum += sorted_returns[i];
                }
                metrics_.cvar_95 = (cvar_sum / var_index) * 100.0;
            }
        }
    }
    
    /**
     * @brief 计算基准相关指标
     */
    void calculateBenchmarkMetrics() {
        if (benchmark_returns_.size() != returns_.size() || returns_.size() < 2) {
            return;
        }
        
        // 计算相关性
        double mean_ret = std::accumulate(returns_.begin(), returns_.end(), 0.0) / returns_.size();
        double mean_bench = std::accumulate(benchmark_returns_.begin(), benchmark_returns_.end(), 0.0) / benchmark_returns_.size();
        
        double covariance = 0.0;
        double var_ret = 0.0;
        double var_bench = 0.0;
        
        for (size_t i = 0; i < returns_.size(); ++i) {
            double diff_ret = returns_[i] - mean_ret;
            double diff_bench = benchmark_returns_[i] - mean_bench;
            
            covariance += diff_ret * diff_bench;
            var_ret += diff_ret * diff_ret;
            var_bench += diff_bench * diff_bench;
        }
        
        covariance /= (returns_.size() - 1);
        var_ret /= (returns_.size() - 1);
        var_bench /= (returns_.size() - 1);
        
        double std_ret = std::sqrt(var_ret);
        double std_bench = std::sqrt(var_bench);
        
        if (std_ret > 0 && std_bench > 0) {
            metrics_.correlation = covariance / (std_ret * std_bench);
        }
        
        // Beta
        if (var_bench > 0) {
            metrics_.beta = covariance / var_bench;
        }
        
        // Alpha (年化)
        double portfolio_return = mean_ret * trading_days_per_year_ * 100.0;
        double benchmark_return = mean_bench * trading_days_per_year_ * 100.0;
        metrics_.alpha = portfolio_return - (risk_free_rate_ * 100.0 + metrics_.beta * (benchmark_return - risk_free_rate_ * 100.0));
        
        // 跟踪误差
        std::vector<double> excess_returns;
        for (size_t i = 0; i < returns_.size(); ++i) {
            excess_returns.push_back(returns_[i] - benchmark_returns_[i]);
        }
        
        if (!excess_returns.empty()) {
            double mean_excess = std::accumulate(excess_returns.begin(), excess_returns.end(), 0.0) / excess_returns.size();
            double variance_excess = 0.0;
            for (double excess : excess_returns) {
                variance_excess += std::pow(excess - mean_excess, 2);
            }
            variance_excess /= (excess_returns.size() - 1);
            metrics_.tracking_error = std::sqrt(variance_excess) * std::sqrt(trading_days_per_year_) * 100.0;
            
            // 信息比率
            if (metrics_.tracking_error > 0) {
                metrics_.information_ratio = (portfolio_return - benchmark_return) / metrics_.tracking_error;
            }
        }
    }
    
    /**
     * @brief 生成周期统计
     */
    void generatePeriodStats() {
        // 这里可以生成月度、季度、年度统计
        // 为简化，只生成整体统计
        PeriodStats overall;
        overall.period_name = "Overall";
        overall.return_pct = metrics_.total_return;
        overall.volatility_pct = metrics_.volatility;
        overall.max_dd_pct = metrics_.max_drawdown;
        overall.sharpe_ratio = metrics_.sharpe_ratio;
        overall.trade_count = metrics_.total_trades;
        overall.win_rate = metrics_.win_rate;
        
        period_stats_.push_back(overall);
    }
};

} // namespace analyzers
} // namespace backtrader