#include "../../include/analyzers/positions.h"
#include <algorithm>
#include <numeric>

namespace backtrader {
namespace analyzers {

Positions::Positions(const Params& params) : p(params) {
    position_history_.clear();
    closed_positions_.clear();
}

void Positions::start() {
    // Initialize analyzer
    position_history_.clear();
    closed_positions_.clear();
    active_positions_.clear();
}

void Positions::stop() {
    // Close any remaining open positions for analysis
    for (auto& pos_pair : active_positions_) {
        auto& pos_info = pos_pair.second;
        pos_info.exit_time = std::chrono::system_clock::now();
        pos_info.exit_price = pos_info.current_price; // Use last known price
        pos_info.pnl = calculate_pnl(pos_info);
        pos_info.is_closed = true;
        
        closed_positions_.push_back(pos_info);
    }
    
    // Calculate final statistics
    calculate_final_stats();
}

void Positions::prenext() {
    // Called before next() during minimum period
    next();
}

void Positions::next() {
    if (!broker_) {
        return;
    }
    
    // Get current positions from broker
    auto positions = broker_->get_positions();
    
    // Track position changes
    for (const auto& pos_pair : positions) {
        const std::string& symbol = pos_pair.first;
        const auto& position = pos_pair.second;
        
        // Check if this is a new position
        auto it = active_positions_.find(symbol);
        if (it == active_positions_.end()) {
            // New position opened
            PositionInfo pos_info;
            pos_info.symbol = symbol;
            pos_info.size = position.size;
            pos_info.entry_price = position.price;
            pos_info.entry_time = std::chrono::system_clock::now();
            pos_info.current_price = position.price;
            pos_info.current_value = position.value;
            pos_info.is_long = position.size > 0;
            pos_info.is_closed = false;
            
            active_positions_[symbol] = pos_info;
        } else {
            // Update existing position
            auto& pos_info = it->second;
            pos_info.current_price = position.price;
            pos_info.current_value = position.value;
            
            // Check if position size changed significantly
            if (std::abs(position.size - pos_info.size) > 1e-8) {
                // Position was modified
                pos_info.size = position.size;
            }
        }
    }
    
    // Check for closed positions
    std::vector<std::string> closed_symbols;
    for (auto& active_pair : active_positions_) {
        const std::string& symbol = active_pair.first;
        
        if (positions.find(symbol) == positions.end()) {
            // Position was closed
            auto& pos_info = active_pair.second;
            pos_info.exit_time = std::chrono::system_clock::now();
            pos_info.exit_price = pos_info.current_price;
            pos_info.pnl = calculate_pnl(pos_info);
            pos_info.is_closed = true;
            
            closed_positions_.push_back(pos_info);
            closed_symbols.push_back(symbol);
        }
    }
    
    // Remove closed positions from active
    for (const auto& symbol : closed_symbols) {
        active_positions_.erase(symbol);
    }
    
    // Store snapshot of current positions
    PositionSnapshot snapshot;
    snapshot.timestamp = std::chrono::system_clock::now();
    snapshot.total_positions = positions.size();
    snapshot.long_positions = 0;
    snapshot.short_positions = 0;
    snapshot.total_value = 0.0;
    
    for (const auto& pos_pair : positions) {
        const auto& position = pos_pair.second;
        if (position.size > 0) {
            snapshot.long_positions++;
        } else if (position.size < 0) {
            snapshot.short_positions++;
        }
        snapshot.total_value += std::abs(position.value);
    }
    
    position_history_.push_back(snapshot);
}

void Positions::notify_trade(const Trade& trade) {
    // Update position information based on trade
    auto it = active_positions_.find(trade.symbol);
    if (it != active_positions_.end()) {
        auto& pos_info = it->second;
        
        // Update average price if adding to position
        if ((pos_info.is_long && trade.size > 0) || (!pos_info.is_long && trade.size < 0)) {
            double total_value = pos_info.size * pos_info.entry_price + trade.size * trade.price;
            double new_size = pos_info.size + trade.size;
            if (new_size != 0) {
                pos_info.entry_price = total_value / new_size;
            }
            pos_info.size = new_size;
        }
    }
}

std::map<std::string, std::any> Positions::get_analysis() {
    calculate_final_stats();
    
    std::map<std::string, std::any> analysis;
    
    // Overall statistics
    analysis["total_positions"] = static_cast<int>(closed_positions_.size());
    analysis["active_positions"] = static_cast<int>(active_positions_.size());
    analysis["winning_positions"] = winning_positions_;
    analysis["losing_positions"] = losing_positions_;
    analysis["win_rate"] = win_rate_;
    
    // PnL statistics
    analysis["total_pnl"] = total_pnl_;
    analysis["avg_pnl"] = avg_pnl_;
    analysis["max_pnl"] = max_pnl_;
    analysis["min_pnl"] = min_pnl_;
    analysis["pnl_std_dev"] = pnl_std_dev_;
    
    // Duration statistics
    analysis["avg_duration_seconds"] = avg_duration_;
    analysis["max_duration_seconds"] = max_duration_;
    analysis["min_duration_seconds"] = min_duration_;
    
    // Long vs Short statistics
    analysis["long_positions"] = long_positions_;
    analysis["short_positions"] = short_positions_;
    analysis["long_pnl"] = long_pnl_;
    analysis["short_pnl"] = short_pnl_;
    analysis["long_win_rate"] = long_win_rate_;
    analysis["short_win_rate"] = short_win_rate_;
    
    // Average winning/losing trade
    analysis["avg_win"] = avg_win_;
    analysis["avg_loss"] = avg_loss_;
    analysis["profit_factor"] = profit_factor_;
    
    // Position size statistics
    analysis["avg_position_size"] = avg_position_size_;
    analysis["max_position_size"] = max_position_size_;
    
    // Simultaneous positions
    analysis["max_simultaneous_positions"] = max_simultaneous_positions_;
    analysis["avg_simultaneous_positions"] = avg_simultaneous_positions_;
    
    return analysis;
}

void Positions::calculate_final_stats() {
    if (closed_positions_.empty() && active_positions_.empty()) {
        return;
    }
    
    // Reset statistics
    winning_positions_ = 0;
    losing_positions_ = 0;
    total_pnl_ = 0.0;
    max_pnl_ = std::numeric_limits<double>::lowest();
    min_pnl_ = std::numeric_limits<double>::max();
    
    long_positions_ = 0;
    short_positions_ = 0;
    long_pnl_ = 0.0;
    short_pnl_ = 0.0;
    int long_wins = 0;
    int short_wins = 0;
    
    double total_duration = 0.0;
    max_duration_ = 0.0;
    min_duration_ = std::numeric_limits<double>::max();
    
    double total_size = 0.0;
    max_position_size_ = 0.0;
    
    double total_wins = 0.0;
    double total_losses = 0.0;
    
    // Analyze closed positions
    for (const auto& pos : closed_positions_) {
        // PnL statistics
        if (pos.pnl > 0) {
            winning_positions_++;
            total_wins += pos.pnl;
        } else if (pos.pnl < 0) {
            losing_positions_++;
            total_losses += std::abs(pos.pnl);
        }
        
        total_pnl_ += pos.pnl;
        max_pnl_ = std::max(max_pnl_, pos.pnl);
        min_pnl_ = std::min(min_pnl_, pos.pnl);
        
        // Long vs Short
        if (pos.is_long) {
            long_positions_++;
            long_pnl_ += pos.pnl;
            if (pos.pnl > 0) long_wins++;
        } else {
            short_positions_++;
            short_pnl_ += pos.pnl;
            if (pos.pnl > 0) short_wins++;
        }
        
        // Duration
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            pos.exit_time - pos.entry_time).count();
        total_duration += duration;
        max_duration_ = std::max(max_duration_, static_cast<double>(duration));
        min_duration_ = std::min(min_duration_, static_cast<double>(duration));
        
        // Position size
        double abs_size = std::abs(pos.size);
        total_size += abs_size;
        max_position_size_ = std::max(max_position_size_, abs_size);
    }
    
    // Calculate averages
    int total_positions = closed_positions_.size();
    if (total_positions > 0) {
        avg_pnl_ = total_pnl_ / total_positions;
        avg_duration_ = total_duration / total_positions;
        avg_position_size_ = total_size / total_positions;
        win_rate_ = static_cast<double>(winning_positions_) / total_positions;
        
        if (winning_positions_ > 0) {
            avg_win_ = total_wins / winning_positions_;
        }
        
        if (losing_positions_ > 0) {
            avg_loss_ = total_losses / losing_positions_;
        }
        
        if (total_losses > 0) {
            profit_factor_ = total_wins / total_losses;
        } else if (total_wins > 0) {
            profit_factor_ = std::numeric_limits<double>::max();
        }
    }
    
    // Calculate win rates
    if (long_positions_ > 0) {
        long_win_rate_ = static_cast<double>(long_wins) / long_positions_;
    }
    
    if (short_positions_ > 0) {
        short_win_rate_ = static_cast<double>(short_wins) / short_positions_;
    }
    
    // Calculate PnL standard deviation
    if (total_positions > 0) {
        double sum_squared_diff = 0.0;
        for (const auto& pos : closed_positions_) {
            double diff = pos.pnl - avg_pnl_;
            sum_squared_diff += diff * diff;
        }
        pnl_std_dev_ = std::sqrt(sum_squared_diff / total_positions);
    }
    
    // Calculate simultaneous positions statistics
    if (!position_history_.empty()) {
        max_simultaneous_positions_ = 0;
        double total_simultaneous = 0.0;
        
        for (const auto& snapshot : position_history_) {
            max_simultaneous_positions_ = std::max(max_simultaneous_positions_, 
                                                  snapshot.total_positions);
            total_simultaneous += snapshot.total_positions;
        }
        
        avg_simultaneous_positions_ = total_simultaneous / position_history_.size();
    }
}

double Positions::calculate_pnl(const PositionInfo& pos_info) const {
    if (pos_info.is_long) {
        return (pos_info.exit_price - pos_info.entry_price) * pos_info.size;
    } else {
        return (pos_info.entry_price - pos_info.exit_price) * std::abs(pos_info.size);
    }
}

std::vector<Positions::PositionInfo> Positions::get_closed_positions() const {
    return closed_positions_;
}

std::map<std::string, Positions::PositionInfo> Positions::get_active_positions() const {
    return active_positions_;
}

std::vector<Positions::PositionSnapshot> Positions::get_position_history() const {
    return position_history_;
}

std::vector<Positions::PositionInfo> Positions::get_winning_positions() const {
    std::vector<PositionInfo> winners;
    
    for (const auto& pos : closed_positions_) {
        if (pos.pnl > 0) {
            winners.push_back(pos);
        }
    }
    
    return winners;
}

std::vector<Positions::PositionInfo> Positions::get_losing_positions() const {
    std::vector<PositionInfo> losers;
    
    for (const auto& pos : closed_positions_) {
        if (pos.pnl < 0) {
            losers.push_back(pos);
        }
    }
    
    return losers;
}

Positions::PositionInfo Positions::get_best_position() const {
    if (closed_positions_.empty()) {
        return PositionInfo{};
    }
    
    auto best_it = std::max_element(closed_positions_.begin(), closed_positions_.end(),
                                    [](const PositionInfo& a, const PositionInfo& b) {
                                        return a.pnl < b.pnl;
                                    });
    
    return *best_it;
}

Positions::PositionInfo Positions::get_worst_position() const {
    if (closed_positions_.empty()) {
        return PositionInfo{};
    }
    
    auto worst_it = std::min_element(closed_positions_.begin(), closed_positions_.end(),
                                     [](const PositionInfo& a, const PositionInfo& b) {
                                         return a.pnl < b.pnl;
                                     });
    
    return *worst_it;
}

void Positions::set_broker(std::shared_ptr<Broker> broker) {
    broker_ = broker;
}

} // namespace analyzers
} // namespace backtrader