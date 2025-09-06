#include "../../include/observers/timereturn.h"
#include <cmath>
#include <numeric>
#include <algorithm>

namespace backtrader {
namespace observers {

TimeReturn::TimeReturn(const Params& params) 
    : p(params), initial_value_(0.0), period_start_value_(0.0),
      value_initialized_(false), bars_in_period_(0) {
    
    // Initialize lines
    lines.resize(2);
    lines[0].clear(); // period returns
    lines[1].clear(); // cumulative returns
    
    // Set line names
    line_names.resize(2);
    line_names[0] = "timereturn";
    line_names[1] = "cumulative";
}

void TimeReturn::prenext() {
    // Called before next() during minimum period
    next();
}

void TimeReturn::next() {
    if (!broker_ || !data_) {
        return;
    }
    
    // Get current portfolio value
    double current_value = broker_->get_value();
    
    // Get current datetime
    double current_time = 0.0;
    if (!data_->lines.empty() && !data_->lines[0].empty()) {
        current_time = data_->lines[0].back();
    }
    
    // Initialize on first call
    if (!value_initialized_) {
        initial_value_ = current_value;
        period_start_value_ = current_value;
        period_start_time_ = current_time;
        value_initialized_ = true;
        
        // First bar has zero return
        lines[0].push_back(0.0);
        lines[1].push_back(0.0);
        bars_in_period_ = 1;
        return;
    }
    
    bars_in_period_++;
    
    // Check if we've completed a period
    bool period_completed = is_period_completed(current_time);
    
    if (period_completed) {
        // Calculate period return
        double period_return = 0.0;
        if (period_start_value_ > 0.0) {
            period_return = (current_value - period_start_value_) / period_start_value_;
        }
        
        // Store period return
        lines[0].push_back(period_return * 100.0); // As percentage
        
        // Calculate cumulative return
        double cumulative_return = 0.0;
        if (initial_value_ > 0.0) {
            cumulative_return = (current_value - initial_value_) / initial_value_;
        }
        lines[1].push_back(cumulative_return * 100.0); // As percentage
        
        // Update period tracking
        PeriodData period;
        period.start_time = period_start_time_;
        period.end_time = current_time;
        period.start_value = period_start_value_;
        period.end_value = current_value;
        period.period_return = period_return;
        period.bars_in_period = bars_in_period_;
        
        period_history_.push_back(period);
        
        // Reset for next period
        period_start_value_ = current_value;
        period_start_time_ = current_time;
        bars_in_period_ = 0;
    } else {
        // Not end of period, carry forward last values
        if (!lines[0].empty()) {
            lines[0].push_back(lines[0].back());
        } else {
            lines[0].push_back(0.0);
        }
        
        // Update cumulative return
        double cumulative_return = 0.0;
        if (initial_value_ > 0.0) {
            cumulative_return = (current_value - initial_value_) / initial_value_;
        }
        lines[1].push_back(cumulative_return * 100.0);
    }
}

bool TimeReturn::is_period_completed(double current_time) const {
    // Convert timestamps to time structures
    std::tm current_tm = timestamp_to_tm(current_time);
    std::tm start_tm = timestamp_to_tm(period_start_time_);
    
    switch (p.timeframe) {
        case TimeFrame::Days:
            // Check if we've moved to a new day
            return (current_tm.tm_year != start_tm.tm_year ||
                    current_tm.tm_yday != start_tm.tm_yday);
            
        case TimeFrame::Weeks:
            // Check if we've moved to a new week
            return is_new_week(start_tm, current_tm);
            
        case TimeFrame::Months:
            // Check if we've moved to a new month
            return (current_tm.tm_year != start_tm.tm_year ||
                    current_tm.tm_mon != start_tm.tm_mon);
            
        case TimeFrame::Years:
            // Check if we've moved to a new year
            return (current_tm.tm_year != start_tm.tm_year);
            
        default:
            // For custom periods, check bar count
            return (bars_in_period_ >= p.compression);
    }
}

std::tm TimeReturn::timestamp_to_tm(double timestamp) const {
    std::time_t time_t_val = static_cast<std::time_t>(timestamp);
    return *std::localtime(&time_t_val);
}

bool TimeReturn::is_new_week(const std::tm& start_tm, const std::tm& current_tm) const {
    // Simple week detection based on Sunday as week start
    if (current_tm.tm_year != start_tm.tm_year) {
        return true;
    }
    
    // Calculate week number
    int start_week = start_tm.tm_yday / 7;
    int current_week = current_tm.tm_yday / 7;
    
    return (current_week != start_week);
}

double TimeReturn::get_current_period_return() const {
    if (!lines[0].empty()) {
        return lines[0].back() / 100.0; // Convert from percentage
    }
    return 0.0;
}

double TimeReturn::get_cumulative_return() const {
    if (!lines[1].empty()) {
        return lines[1].back() / 100.0; // Convert from percentage
    }
    return 0.0;
}

std::map<std::string, double> TimeReturn::get_analysis() const {
    std::map<std::string, double> analysis;
    
    if (period_history_.empty()) {
        return analysis;
    }
    
    // Basic statistics
    analysis["total_periods"] = static_cast<double>(period_history_.size());
    analysis["cumulative_return"] = get_cumulative_return();
    
    // Extract period returns
    std::vector<double> period_returns;
    period_returns.reserve(period_history_.size());
    
    for (const auto& period : period_history_) {
        period_returns.push_back(period.period_return);
    }
    
    // Calculate mean return
    double sum = std::accumulate(period_returns.begin(), period_returns.end(), 0.0);
    double mean = sum / period_returns.size();
    analysis["mean_return"] = mean;
    
    // Calculate standard deviation
    double sq_sum = 0.0;
    for (double ret : period_returns) {
        sq_sum += (ret - mean) * (ret - mean);
    }
    double std_dev = std::sqrt(sq_sum / period_returns.size());
    analysis["std_dev"] = std_dev;
    
    // Calculate Sharpe ratio for the period
    double annualized_periods = get_annualized_periods();
    double annualized_return = mean * annualized_periods;
    double annualized_vol = std_dev * std::sqrt(annualized_periods);
    
    analysis["annualized_return"] = annualized_return;
    analysis["annualized_volatility"] = annualized_vol;
    
    if (annualized_vol > 0.0) {
        double excess_return = annualized_return - p.risk_free_rate;
        analysis["sharpe_ratio"] = excess_return / annualized_vol;
    } else {
        analysis["sharpe_ratio"] = 0.0;
    }
    
    // Min/Max returns
    auto minmax = std::minmax_element(period_returns.begin(), period_returns.end());
    analysis["min_return"] = *minmax.first;
    analysis["max_return"] = *minmax.second;
    
    // Count positive and negative periods
    int positive_periods = 0;
    int negative_periods = 0;
    
    for (double ret : period_returns) {
        if (ret > 0.0) {
            positive_periods++;
        } else if (ret < 0.0) {
            negative_periods++;
        }
    }
    
    analysis["positive_periods"] = static_cast<double>(positive_periods);
    analysis["negative_periods"] = static_cast<double>(negative_periods);
    analysis["win_rate"] = static_cast<double>(positive_periods) / period_returns.size();
    
    // Calculate best and worst consecutive periods
    analysis["best_consecutive"] = calculate_best_consecutive(period_returns);
    analysis["worst_consecutive"] = calculate_worst_consecutive(period_returns);
    
    // Calculate average bars per period
    double total_bars = 0.0;
    for (const auto& period : period_history_) {
        total_bars += period.bars_in_period;
    }
    analysis["avg_bars_per_period"] = total_bars / period_history_.size();
    
    return analysis;
}

double TimeReturn::get_annualized_periods() const {
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
            // Estimate based on average bars per period
            if (!period_history_.empty()) {
                double avg_bars = 0.0;
                for (const auto& period : period_history_) {
                    avg_bars += period.bars_in_period;
                }
                avg_bars /= period_history_.size();
                
                // Assume daily bars
                return 252.0 / avg_bars;
            }
            return 252.0;
    }
}

double TimeReturn::calculate_best_consecutive(const std::vector<double>& returns) const {
    if (returns.empty()) {
        return 0.0;
    }
    
    double best_sum = returns[0];
    double current_sum = returns[0];
    
    for (size_t i = 1; i < returns.size(); ++i) {
        if (current_sum > 0) {
            current_sum += returns[i];
        } else {
            current_sum = returns[i];
        }
        
        best_sum = std::max(best_sum, current_sum);
    }
    
    return best_sum;
}

double TimeReturn::calculate_worst_consecutive(const std::vector<double>& returns) const {
    if (returns.empty()) {
        return 0.0;
    }
    
    double worst_sum = returns[0];
    double current_sum = returns[0];
    
    for (size_t i = 1; i < returns.size(); ++i) {
        if (current_sum < 0) {
            current_sum += returns[i];
        } else {
            current_sum = returns[i];
        }
        
        worst_sum = std::min(worst_sum, current_sum);
    }
    
    return worst_sum;
}

void TimeReturn::reset() {
    initial_value_ = 0.0;
    period_start_value_ = 0.0;
    value_initialized_ = false;
    bars_in_period_ = 0;
    period_history_.clear();
    
    for (auto& line : lines) {
        line.clear();
    }
}

void TimeReturn::set_broker(std::shared_ptr<Broker> broker) {
    broker_ = broker;
}

void TimeReturn::set_data(std::shared_ptr<DataSeries> data) {
    data_ = data;
}

std::vector<TimeReturn::PeriodData> TimeReturn::get_period_history() const {
    return period_history_;
}

std::vector<double> TimeReturn::get_period_returns() const {
    std::vector<double> returns;
    returns.reserve(period_history_.size());
    
    for (const auto& period : period_history_) {
        returns.push_back(period.period_return);
    }
    
    return returns;
}

TimeReturn::PeriodData TimeReturn::get_best_period() const {
    if (period_history_.empty()) {
        return PeriodData{};
    }
    
    auto best_it = std::max_element(period_history_.begin(), period_history_.end(),
                                    [](const PeriodData& a, const PeriodData& b) {
                                        return a.period_return < b.period_return;
                                    });
    
    return *best_it;
}

TimeReturn::PeriodData TimeReturn::get_worst_period() const {
    if (period_history_.empty()) {
        return PeriodData{};
    }
    
    auto worst_it = std::min_element(period_history_.begin(), period_history_.end(),
                                     [](const PeriodData& a, const PeriodData& b) {
                                         return a.period_return < b.period_return;
                                     });
    
    return *worst_it;
}

} // namespace observers
} // namespace backtrader