#include "../../include/observers/drawdown.h"
#include <algorithm>
#include <cmath>

namespace backtrader {
namespace observers {

DrawDown::DrawDown(const Params& params) : p(params), max_value_(0.0), current_dd_(0.0), max_dd_(0.0) {
    // Initialize lines
    lines.resize(2);
    lines[0].clear(); // drawdown line
    lines[1].clear(); // max drawdown line
    
    // Set line names
    line_names.resize(2);
    line_names[0] = "drawdown";
    line_names[1] = "maxdrawdown";
}

void DrawDown::prenext() {
    // Called before next() during minimum period
    next();
}

void DrawDown::next() {
    // Get current portfolio value
    double current_value = 0.0;
    
    if (broker_) {
        current_value = broker_->get_value();
    } else {
        // Fallback: use close price if no broker
        if (data_ && !data_->lines.empty() && !data_->lines[4].empty()) {
            current_value = data_->lines[4].back();
        }
    }
    
    // Update max value
    if (current_value > max_value_) {
        max_value_ = current_value;
    }
    
    // Calculate current drawdown
    if (max_value_ > 0.0) {
        current_dd_ = (max_value_ - current_value) / max_value_;
    } else {
        current_dd_ = 0.0;
    }
    
    // Update max drawdown
    if (current_dd_ > max_dd_) {
        max_dd_ = current_dd_;
    }
    
    // Store values in lines
    if (p.fund) {
        // Fund mode: store actual dollar amounts
        lines[0].push_back(max_value_ - current_value);
        lines[1].push_back(max_value_ - (max_value_ * (1.0 - max_dd_)));
    } else {
        // Percentage mode: store as percentages
        lines[0].push_back(current_dd_ * 100.0);
        lines[1].push_back(max_dd_ * 100.0);
    }
}

double DrawDown::get_current_drawdown() const {
    return current_dd_;
}

double DrawDown::get_max_drawdown() const {
    return max_dd_;
}

double DrawDown::get_max_value() const {
    return max_value_;
}

std::map<std::string, double> DrawDown::get_analysis() const {
    std::map<std::string, double> analysis;
    
    analysis["max_drawdown"] = max_dd_;
    analysis["max_drawdown_percent"] = max_dd_ * 100.0;
    analysis["current_drawdown"] = current_dd_;
    analysis["current_drawdown_percent"] = current_dd_ * 100.0;
    analysis["max_value"] = max_value_;
    
    if (!lines[0].empty()) {
        // Calculate additional statistics
        const auto& dd_line = lines[0];
        
        // Average drawdown
        double sum_dd = 0.0;
        int count_dd = 0;
        
        for (double dd : dd_line) {
            if (dd > 0.0) {
                sum_dd += dd;
                count_dd++;
            }
        }
        
        if (count_dd > 0) {
            analysis["average_drawdown"] = sum_dd / count_dd;
            analysis["drawdown_periods"] = static_cast<double>(count_dd);
        } else {
            analysis["average_drawdown"] = 0.0;
            analysis["drawdown_periods"] = 0.0;
        }
        
        // Longest drawdown period
        int current_period = 0;
        int max_period = 0;
        
        for (double dd : dd_line) {
            if (dd > 0.0) {
                current_period++;
                max_period = std::max(max_period, current_period);
            } else {
                current_period = 0;
            }
        }
        
        analysis["max_drawdown_duration"] = static_cast<double>(max_period);
    }
    
    return analysis;
}

void DrawDown::reset() {
    max_value_ = 0.0;
    current_dd_ = 0.0;
    max_dd_ = 0.0;
    
    lines[0].clear();
    lines[1].clear();
}

std::vector<DrawDown::DrawdownPeriod> DrawDown::get_drawdown_periods() const {
    std::vector<DrawdownPeriod> periods;
    
    if (lines[0].empty()) {
        return periods;
    }
    
    const auto& dd_line = lines[0];
    DrawdownPeriod current_period;
    bool in_drawdown = false;
    
    for (size_t i = 0; i < dd_line.size(); ++i) {
        double dd = dd_line[i];
        
        if (dd > 0.0 && !in_drawdown) {
            // Start of new drawdown period
            current_period.start_index = i;
            current_period.start_value = dd;
            current_period.max_drawdown = dd;
            current_period.duration = 1;
            in_drawdown = true;
        } else if (dd > 0.0 && in_drawdown) {
            // Continue drawdown period
            current_period.max_drawdown = std::max(current_period.max_drawdown, dd);
            current_period.duration++;
        } else if (dd <= 0.0 && in_drawdown) {
            // End of drawdown period
            current_period.end_index = i - 1;
            current_period.recovery_index = i;
            periods.push_back(current_period);
            in_drawdown = false;
        }
    }
    
    // Handle case where we're still in drawdown at the end
    if (in_drawdown) {
        current_period.end_index = dd_line.size() - 1;
        current_period.recovery_index = -1; // Not recovered yet
        periods.push_back(current_period);
    }
    
    return periods;
}

double DrawDown::get_recovery_factor() const {
    if (max_dd_ <= 0.0) {
        return 0.0; // No drawdown, no recovery factor
    }
    
    // Recovery factor = Total return / Max drawdown
    if (broker_) {
        double initial_value = broker_->get_cash(); // Simplified
        double current_value = broker_->get_value();
        double total_return = (current_value - initial_value) / initial_value;
        
        return total_return / max_dd_;
    }
    
    return 0.0;
}

double DrawDown::get_ulcer_index() const {
    // Ulcer Index: RMS of drawdowns
    if (lines[0].empty()) {
        return 0.0;
    }
    
    const auto& dd_line = lines[0];
    double sum_squared = 0.0;
    
    for (double dd : dd_line) {
        sum_squared += dd * dd;
    }
    
    return std::sqrt(sum_squared / dd_line.size());
}

double DrawDown::get_pain_index() const {
    // Pain Index: Average of all drawdowns
    if (lines[0].empty()) {
        return 0.0;
    }
    
    const auto& dd_line = lines[0];
    double sum = 0.0;
    
    for (double dd : dd_line) {
        sum += dd;
    }
    
    return sum / dd_line.size();
}

void DrawDown::set_broker(std::shared_ptr<Broker> broker) {
    broker_ = broker;
}

void DrawDown::set_data(std::shared_ptr<DataSeries> data) {
    data_ = data;
}

} // namespace observers
} // namespace backtrader