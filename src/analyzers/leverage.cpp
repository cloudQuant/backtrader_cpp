#include "../../include/analyzers/leverage.h"
#include <algorithm>
#include <cmath>

namespace backtrader {
namespace analyzers {

Leverage::Leverage(const Params& params) : p(params) {
    leverage_history_.clear();
    trade_leverage_.clear();
}

void Leverage::start() {
    // Initialize analyzer
    leverage_history_.clear();
    trade_leverage_.clear();
}

void Leverage::stop() {
    // Finalize analysis
    calculate_final_stats();
}

void Leverage::prenext() {
    // Called before next() during minimum period
    next();
}

void Leverage::next() {
    if (!broker_) {
        return;
    }
    
    // Get current portfolio value and positions
    double portfolio_value = broker_->get_value();
    double cash = broker_->get_cash();
    
    // Calculate total position value
    double total_position_value = 0.0;
    auto positions = broker_->get_positions();
    
    for (const auto& pos_pair : positions) {
        const auto& position = pos_pair.second;
        total_position_value += std::abs(position.value);
    }
    
    // Calculate current leverage
    double current_leverage = 0.0;
    if (portfolio_value > 0.0) {
        current_leverage = total_position_value / portfolio_value;
    }
    
    // Store leverage data
    LeverageData data;
    data.portfolio_value = portfolio_value;
    data.position_value = total_position_value;
    data.cash = cash;
    data.leverage = current_leverage;
    data.timestamp = std::chrono::system_clock::now();
    
    leverage_history_.push_back(data);
}

void Leverage::notify_trade(const Trade& trade) {
    // Track leverage at trade execution
    if (!broker_) {
        return;
    }
    
    double portfolio_value = broker_->get_value();
    double position_value = std::abs(trade.value);
    
    TradeLeverage tl;
    tl.trade_id = trade.id;
    tl.leverage = (portfolio_value > 0.0) ? position_value / portfolio_value : 0.0;
    tl.trade_size = std::abs(trade.size);
    tl.trade_value = std::abs(trade.value);
    tl.portfolio_value = portfolio_value;
    
    trade_leverage_.push_back(tl);
}

std::map<std::string, std::any> Leverage::get_analysis() {
    calculate_final_stats();
    
    std::map<std::string, std::any> analysis;
    
    // Basic statistics
    analysis["current_leverage"] = get_current_leverage();
    analysis["max_leverage"] = max_leverage_;
    analysis["min_leverage"] = min_leverage_;
    analysis["avg_leverage"] = avg_leverage_;
    analysis["std_leverage"] = std_leverage_;
    
    // Time-based statistics
    analysis["time_at_max_leverage"] = time_at_max_leverage_;
    analysis["time_overleveraged"] = time_overleveraged_;
    analysis["periods_overleveraged"] = periods_overleveraged_;
    
    // Trade-based statistics
    analysis["avg_trade_leverage"] = avg_trade_leverage_;
    analysis["max_trade_leverage"] = max_trade_leverage_;
    analysis["trades_overleveraged"] = trades_overleveraged_;
    
    // Risk metrics
    analysis["leverage_risk_score"] = calculate_leverage_risk_score();
    
    // Detailed history
    analysis["leverage_history_size"] = static_cast<int>(leverage_history_.size());
    analysis["trade_leverage_count"] = static_cast<int>(trade_leverage_.size());
    
    return analysis;
}

void Leverage::calculate_final_stats() {
    if (leverage_history_.empty()) {
        return;
    }
    
    // Calculate basic statistics
    max_leverage_ = 0.0;
    min_leverage_ = std::numeric_limits<double>::max();
    double sum_leverage = 0.0;
    
    for (const auto& data : leverage_history_) {
        max_leverage_ = std::max(max_leverage_, data.leverage);
        min_leverage_ = std::min(min_leverage_, data.leverage);
        sum_leverage += data.leverage;
        
        if (data.leverage == max_leverage_) {
            time_at_max_leverage_ = data.timestamp;
        }
        
        if (data.leverage > p.target_leverage) {
            periods_overleveraged_++;
        }
    }
    
    avg_leverage_ = sum_leverage / leverage_history_.size();
    
    // Calculate standard deviation
    double sum_squared_diff = 0.0;
    for (const auto& data : leverage_history_) {
        double diff = data.leverage - avg_leverage_;
        sum_squared_diff += diff * diff;
    }
    std_leverage_ = std::sqrt(sum_squared_diff / leverage_history_.size());
    
    // Calculate time overleveraged
    time_overleveraged_ = static_cast<double>(periods_overleveraged_) / leverage_history_.size();
    
    // Calculate trade leverage statistics
    if (!trade_leverage_.empty()) {
        max_trade_leverage_ = 0.0;
        double sum_trade_leverage = 0.0;
        trades_overleveraged_ = 0;
        
        for (const auto& tl : trade_leverage_) {
            max_trade_leverage_ = std::max(max_trade_leverage_, tl.leverage);
            sum_trade_leverage += tl.leverage;
            
            if (tl.leverage > p.target_leverage) {
                trades_overleveraged_++;
            }
        }
        
        avg_trade_leverage_ = sum_trade_leverage / trade_leverage_.size();
    }
}

double Leverage::calculate_leverage_risk_score() const {
    // Calculate a risk score based on leverage metrics
    double risk_score = 0.0;
    
    // Factor 1: Maximum leverage (0-40 points)
    if (max_leverage_ > 3.0) {
        risk_score += 40.0;
    } else if (max_leverage_ > 2.0) {
        risk_score += 30.0;
    } else if (max_leverage_ > 1.5) {
        risk_score += 20.0;
    } else if (max_leverage_ > 1.0) {
        risk_score += 10.0;
    }
    
    // Factor 2: Average leverage (0-30 points)
    if (avg_leverage_ > 2.0) {
        risk_score += 30.0;
    } else if (avg_leverage_ > 1.5) {
        risk_score += 20.0;
    } else if (avg_leverage_ > 1.0) {
        risk_score += 10.0;
    }
    
    // Factor 3: Time overleveraged (0-30 points)
    risk_score += time_overleveraged_ * 30.0;
    
    return std::min(risk_score, 100.0);
}

double Leverage::get_current_leverage() const {
    if (!leverage_history_.empty()) {
        return leverage_history_.back().leverage;
    }
    return 0.0;
}

double Leverage::get_max_leverage() const {
    return max_leverage_;
}

double Leverage::get_avg_leverage() const {
    return avg_leverage_;
}

std::vector<Leverage::LeverageData> Leverage::get_leverage_history() const {
    return leverage_history_;
}

std::vector<Leverage::TradeLeverage> Leverage::get_trade_leverage() const {
    return trade_leverage_;
}

std::vector<double> Leverage::get_leverage_values() const {
    std::vector<double> values;
    values.reserve(leverage_history_.size());
    
    for (const auto& data : leverage_history_) {
        values.push_back(data.leverage);
    }
    
    return values;
}

Leverage::LeverageData Leverage::get_max_leverage_point() const {
    if (leverage_history_.empty()) {
        return LeverageData{};
    }
    
    auto max_it = std::max_element(leverage_history_.begin(), leverage_history_.end(),
                                   [](const LeverageData& a, const LeverageData& b) {
                                       return a.leverage < b.leverage;
                                   });
    
    return *max_it;
}

double Leverage::get_leverage_volatility() const {
    return std_leverage_;
}

std::map<std::string, double> Leverage::get_leverage_distribution() const {
    std::map<std::string, double> distribution;
    
    if (leverage_history_.empty()) {
        return distribution;
    }
    
    // Count leverage in different ranges
    int unleveraged = 0;    // < 1.0
    int low_leverage = 0;   // 1.0 - 1.5
    int mid_leverage = 0;   // 1.5 - 2.0
    int high_leverage = 0;  // 2.0 - 3.0
    int extreme_leverage = 0; // > 3.0
    
    for (const auto& data : leverage_history_) {
        if (data.leverage < 1.0) {
            unleveraged++;
        } else if (data.leverage <= 1.5) {
            low_leverage++;
        } else if (data.leverage <= 2.0) {
            mid_leverage++;
        } else if (data.leverage <= 3.0) {
            high_leverage++;
        } else {
            extreme_leverage++;
        }
    }
    
    double total = static_cast<double>(leverage_history_.size());
    
    distribution["unleveraged_pct"] = unleveraged / total;
    distribution["low_leverage_pct"] = low_leverage / total;
    distribution["mid_leverage_pct"] = mid_leverage / total;
    distribution["high_leverage_pct"] = high_leverage / total;
    distribution["extreme_leverage_pct"] = extreme_leverage / total;
    
    return distribution;
}

void Leverage::set_broker(std::shared_ptr<Broker> broker) {
    broker_ = broker;
}

} // namespace analyzers
} // namespace backtrader