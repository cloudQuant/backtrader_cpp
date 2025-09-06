#include "../../include/analyzers/periodstats.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace backtrader {
namespace analyzers {

PeriodStats::PeriodStats(const Params& params) 
    : p(params), period_start_value_(0.0), value_initialized_(false),
      bars_in_period_(0) {
    period_returns_.clear();
}

void PeriodStats::start() {
    // Initialize analyzer
    period_returns_.clear();
    period_history_.clear();
    period_start_value_ = 0.0;
    value_initialized_ = false;
    bars_in_period_ = 0;
}

void PeriodStats::stop() {
    // Finalize the last period if needed
    if (value_initialized_ && bars_in_period_ > 0) {
        finalize_current_period();
    }
    
    // Calculate final statistics
    calculate_final_stats();
}

void PeriodStats::prenext() {
    // Called before next() during minimum period
    next();
}

void PeriodStats::next() {
    if (!broker_ || !data_) {
        return;
    }
    
    // Get current portfolio value and time
    double current_value = broker_->get_value();
    double current_time = 0.0;
    
    if (!data_->lines.empty() && !data_->lines[0].empty()) {
        current_time = data_->lines[0].back();
    }
    
    // Initialize on first call
    if (!value_initialized_) {
        period_start_value_ = current_value;
        period_start_time_ = current_time;
        value_initialized_ = true;
        bars_in_period_ = 1;
        return;
    }
    
    bars_in_period_++;
    
    // Check if period is complete
    if (is_period_complete(current_time)) {
        // Calculate period return
        double period_return = 0.0;
        if (period_start_value_ > 0.0) {
            period_return = (current_value - period_start_value_) / period_start_value_;
        }
        
        // Store period data
        PeriodData pd;
        pd.start_time = period_start_time_;
        pd.end_time = current_time;
        pd.start_value = period_start_value_;
        pd.end_value = current_value;
        pd.period_return = period_return;
        pd.bars_count = bars_in_period_;
        
        period_history_.push_back(pd);
        period_returns_.push_back(period_return);
        
        // Reset for next period
        period_start_value_ = current_value;
        period_start_time_ = current_time;
        bars_in_period_ = 0;
    }
}

bool PeriodStats::is_period_complete(double current_time) const {
    std::tm current_tm = timestamp_to_tm(current_time);
    std::tm start_tm = timestamp_to_tm(period_start_time_);
    
    switch (p.timeframe) {
        case TimeFrame::Days:
            return (current_tm.tm_year != start_tm.tm_year ||
                    current_tm.tm_yday != start_tm.tm_yday);
            
        case TimeFrame::Weeks:
            return is_new_week(start_tm, current_tm);
            
        case TimeFrame::Months:
            return (current_tm.tm_year != start_tm.tm_year ||
                    current_tm.tm_mon != start_tm.tm_mon);
            
        case TimeFrame::Years:
            return (current_tm.tm_year != start_tm.tm_year);
            
        default:
            // For custom periods, check bar count
            return (bars_in_period_ >= p.compression);
    }
}

void PeriodStats::finalize_current_period() {
    if (!broker_) {
        return;
    }
    
    double current_value = broker_->get_value();
    double current_time = period_start_time_ + (bars_in_period_ * 86400); // Estimate
    
    double period_return = 0.0;
    if (period_start_value_ > 0.0) {
        period_return = (current_value - period_start_value_) / period_start_value_;
    }
    
    PeriodData pd;
    pd.start_time = period_start_time_;
    pd.end_time = current_time;
    pd.start_value = period_start_value_;
    pd.end_value = current_value;
    pd.period_return = period_return;
    pd.bars_count = bars_in_period_;
    
    period_history_.push_back(pd);
    period_returns_.push_back(period_return);
}

std::map<std::string, std::any> PeriodStats::get_analysis() {
    calculate_final_stats();
    
    std::map<std::string, std::any> analysis;
    
    // Basic statistics
    analysis["total_periods"] = static_cast<int>(period_returns_.size());
    analysis["average_return"] = average_return_;
    analysis["total_return"] = total_return_;
    analysis["compound_return"] = compound_return_;
    
    // Risk metrics
    analysis["std_dev"] = std_dev_;
    analysis["downside_deviation"] = downside_deviation_;
    analysis["max_drawdown"] = max_drawdown_;
    
    // Performance metrics
    analysis["best_period"] = best_period_;
    analysis["worst_period"] = worst_period_;
    analysis["positive_periods"] = positive_periods_;
    analysis["negative_periods"] = negative_periods_;
    analysis["win_rate"] = win_rate_;
    
    // Risk-adjusted returns
    analysis["sharpe_ratio"] = sharpe_ratio_;
    analysis["sortino_ratio"] = sortino_ratio_;
    analysis["calmar_ratio"] = calmar_ratio_;
    
    // Consistency metrics
    analysis["consistency_score"] = consistency_score_;
    analysis["consecutive_wins_max"] = consecutive_wins_max_;
    analysis["consecutive_losses_max"] = consecutive_losses_max_;
    
    // Annualized metrics
    analysis["annualized_return"] = annualized_return_;
    analysis["annualized_volatility"] = annualized_volatility_;
    
    return analysis;
}

void PeriodStats::calculate_final_stats() {
    if (period_returns_.empty()) {
        return;
    }
    
    // Calculate basic statistics
    calculate_basic_stats();
    
    // Calculate risk metrics
    calculate_risk_metrics();
    
    // Calculate performance metrics
    calculate_performance_metrics();
    
    // Calculate consistency metrics
    calculate_consistency_metrics();
}

void PeriodStats::calculate_basic_stats() {
    // Average return
    double sum = std::accumulate(period_returns_.begin(), period_returns_.end(), 0.0);
    average_return_ = sum / period_returns_.size();
    
    // Total return
    total_return_ = sum;
    
    // Compound return
    double compound = 1.0;
    for (double ret : period_returns_) {
        compound *= (1.0 + ret);
    }
    compound_return_ = compound - 1.0;
    
    // Standard deviation
    double sum_squared_diff = 0.0;
    for (double ret : period_returns_) {
        double diff = ret - average_return_;
        sum_squared_diff += diff * diff;
    }
    
    if (period_returns_.size() > 1) {
        std_dev_ = std::sqrt(sum_squared_diff / (period_returns_.size() - 1));
    } else {
        std_dev_ = 0.0;
    }
    
    // Best and worst periods
    auto minmax = std::minmax_element(period_returns_.begin(), period_returns_.end());
    worst_period_ = *minmax.first;
    best_period_ = *minmax.second;
    
    // Positive/negative periods
    positive_periods_ = 0;
    negative_periods_ = 0;
    
    for (double ret : period_returns_) {
        if (ret > 0.0) {
            positive_periods_++;
        } else if (ret < 0.0) {
            negative_periods_++;
        }
    }
    
    win_rate_ = static_cast<double>(positive_periods_) / period_returns_.size();
}

void PeriodStats::calculate_risk_metrics() {
    // Downside deviation
    double sum_negative_squared = 0.0;
    int negative_count = 0;
    
    for (double ret : period_returns_) {
        if (ret < 0.0) {
            sum_negative_squared += ret * ret;
            negative_count++;
        }
    }
    
    if (negative_count > 0) {
        downside_deviation_ = std::sqrt(sum_negative_squared / negative_count);
    } else {
        downside_deviation_ = 0.0;
    }
    
    // Max drawdown
    max_drawdown_ = calculate_max_drawdown();
}

double PeriodStats::calculate_max_drawdown() const {
    if (period_history_.empty()) {
        return 0.0;
    }
    
    double max_value = period_history_[0].start_value;
    double max_dd = 0.0;
    
    for (const auto& period : period_history_) {
        max_value = std::max(max_value, period.end_value);
        double dd = (max_value - period.end_value) / max_value;
        max_dd = std::max(max_dd, dd);
    }
    
    return max_dd;
}

void PeriodStats::calculate_performance_metrics() {
    // Annualization factor
    double annualization_factor = get_annualization_factor();
    
    // Annualized return
    if (period_returns_.size() > 0) {
        double periods_per_year = annualization_factor;
        double years = period_returns_.size() / periods_per_year;
        
        if (years > 0) {
            annualized_return_ = std::pow(1.0 + compound_return_, 1.0 / years) - 1.0;
        } else {
            annualized_return_ = compound_return_ * periods_per_year;
        }
    }
    
    // Annualized volatility
    annualized_volatility_ = std_dev_ * std::sqrt(annualization_factor);
    
    // Sharpe ratio
    if (annualized_volatility_ > 0.0) {
        double excess_return = annualized_return_ - p.risk_free_rate;
        sharpe_ratio_ = excess_return / annualized_volatility_;
    } else {
        sharpe_ratio_ = 0.0;
    }
    
    // Sortino ratio
    if (downside_deviation_ > 0.0) {
        double annualized_downside = downside_deviation_ * std::sqrt(annualization_factor);
        double excess_return = annualized_return_ - p.risk_free_rate;
        sortino_ratio_ = excess_return / annualized_downside;
    } else {
        sortino_ratio_ = 0.0;
    }
    
    // Calmar ratio
    if (max_drawdown_ > 0.0) {
        calmar_ratio_ = annualized_return_ / max_drawdown_;
    } else {
        calmar_ratio_ = 0.0;
    }
}

void PeriodStats::calculate_consistency_metrics() {
    // Consecutive wins/losses
    consecutive_wins_max_ = 0;
    consecutive_losses_max_ = 0;
    
    int current_wins = 0;
    int current_losses = 0;
    
    for (double ret : period_returns_) {
        if (ret > 0.0) {
            current_wins++;
            current_losses = 0;
            consecutive_wins_max_ = std::max(consecutive_wins_max_, current_wins);
        } else if (ret < 0.0) {
            current_losses++;
            current_wins = 0;
            consecutive_losses_max_ = std::max(consecutive_losses_max_, current_losses);
        } else {
            current_wins = 0;
            current_losses = 0;
        }
    }
    
    // Consistency score (based on win rate and return stability)
    double win_component = win_rate_ * 50.0; // 0-50 points
    double stability_component = 0.0;
    
    if (average_return_ > 0.0 && std_dev_ > 0.0) {
        double coefficient_of_variation = std_dev_ / average_return_;
        stability_component = std::max(0.0, 50.0 * (1.0 - coefficient_of_variation));
    }
    
    consistency_score_ = win_component + stability_component;
}

double PeriodStats::get_annualization_factor() const {
    switch (p.timeframe) {
        case TimeFrame::Days:
            return 252.0; // Trading days
        case TimeFrame::Weeks:
            return 52.0;
        case TimeFrame::Months:
            return 12.0;
        case TimeFrame::Years:
            return 1.0;
        default:
            return 252.0; // Default to daily
    }
}

std::tm PeriodStats::timestamp_to_tm(double timestamp) const {
    std::time_t time_t_val = static_cast<std::time_t>(timestamp);
    return *std::localtime(&time_t_val);
}

bool PeriodStats::is_new_week(const std::tm& start_tm, const std::tm& current_tm) const {
    if (current_tm.tm_year != start_tm.tm_year) {
        return true;
    }
    
    int start_week = start_tm.tm_yday / 7;
    int current_week = current_tm.tm_yday / 7;
    
    return (current_week != start_week);
}

std::vector<double> PeriodStats::get_period_returns() const {
    return period_returns_;
}

std::vector<PeriodStats::PeriodData> PeriodStats::get_period_history() const {
    return period_history_;
}

PeriodStats::PeriodData PeriodStats::get_best_period_data() const {
    if (period_history_.empty()) {
        return PeriodData{};
    }
    
    auto best_it = std::max_element(period_history_.begin(), period_history_.end(),
                                    [](const PeriodData& a, const PeriodData& b) {
                                        return a.period_return < b.period_return;
                                    });
    
    return *best_it;
}

PeriodStats::PeriodData PeriodStats::get_worst_period_data() const {
    if (period_history_.empty()) {
        return PeriodData{};
    }
    
    auto worst_it = std::min_element(period_history_.begin(), period_history_.end(),
                                     [](const PeriodData& a, const PeriodData& b) {
                                         return a.period_return < b.period_return;
                                     });
    
    return *worst_it;
}

std::map<std::string, double> PeriodStats::get_return_distribution() const {
    std::map<std::string, double> distribution;
    
    if (period_returns_.empty()) {
        return distribution;
    }
    
    // Sort returns for percentile calculation
    std::vector<double> sorted_returns = period_returns_;
    std::sort(sorted_returns.begin(), sorted_returns.end());
    
    size_t n = sorted_returns.size();
    
    // Percentiles
    distribution["min"] = sorted_returns[0];
    distribution["percentile_5"] = sorted_returns[static_cast<size_t>(n * 0.05)];
    distribution["percentile_25"] = sorted_returns[static_cast<size_t>(n * 0.25)];
    distribution["median"] = sorted_returns[static_cast<size_t>(n * 0.50)];
    distribution["percentile_75"] = sorted_returns[static_cast<size_t>(n * 0.75)];
    distribution["percentile_95"] = sorted_returns[static_cast<size_t>(n * 0.95)];
    distribution["max"] = sorted_returns[n - 1];
    
    return distribution;
}

void PeriodStats::set_broker(std::shared_ptr<Broker> broker) {
    broker_ = broker;
}

void PeriodStats::set_data(std::shared_ptr<DataSeries> data) {
    data_ = data;
}

} // namespace analyzers
} // namespace backtrader