#include "observers/trades.h"
#include <algorithm>
#include <limits>

namespace backtrader {

// Trades Observer implementation
Trades::Trades() : Observer() {
    // Initialize PnL lines
    pnl_plus_line_.clear();
    pnl_minus_line_.clear();
    
    // Initialize statistics
    stats_ = TradeStats{};
    stats_.trades_win_min = std::numeric_limits<double>::max();
    stats_.trades_loss_min = std::numeric_limits<double>::max();
    stats_.trades_length_min = std::numeric_limits<int>::max();
}

void Trades::next() {
    // Add NaN values to maintain line continuity when no trades are closed
    pnl_plus_line_.push_back(std::numeric_limits<double>::quiet_NaN());
    pnl_minus_line_.push_back(std::numeric_limits<double>::quiet_NaN());
}

void Trades::notify_trade(const Trade& trade) {
    // Only process closed trades
    if (!trade.is_closed) {
        return;
    }
    
    // Get PnL value based on parameters
    double pnl = params.pnlcomm ? trade.pnl_comm : trade.pnl;
    
    // Plot the trade PnL
    plot_trade_pnl(pnl);
    
    // Update trade statistics
    update_trade_stats(trade);
}

void Trades::update_trade_stats(const Trade& trade) {
    // Update total trades count
    stats_.total_trades++;
    
    // Update long/short counts
    if (trade.is_long) {
        stats_.trades_long++;
    } else {
        stats_.trades_short++;
    }
    
    // Get PnL values
    double pnl = params.pnlcomm ? trade.pnl_comm : trade.pnl;
    double pnl_gross = trade.pnl;
    
    // Update win/loss statistics
    if (pnl >= 0.0) {
        // Winning trade
        stats_.trades_plus++;
        stats_.trades_win += pnl;
        stats_.trades_win_max = std::max(stats_.trades_win_max, pnl);
        if (stats_.trades_win_min == std::numeric_limits<double>::max()) {
            stats_.trades_win_min = pnl;
        } else {
            stats_.trades_win_min = std::min(stats_.trades_win_min, pnl);
        }
        
        // Gross statistics
        if (pnl_gross >= 0.0) {
            stats_.trades_plus_gross++;
        }
    } else {
        // Losing trade
        stats_.trades_minus++;
        stats_.trades_loss += pnl;
        stats_.trades_loss_max = std::max(stats_.trades_loss_max, pnl); // Max loss (closest to 0)
        if (stats_.trades_loss_min == std::numeric_limits<double>::max()) {
            stats_.trades_loss_min = pnl;
        } else {
            stats_.trades_loss_min = std::min(stats_.trades_loss_min, pnl); // Min loss (most negative)
        }
        
        // Gross statistics
        if (pnl_gross < 0.0) {
            stats_.trades_minus_gross++;
        }
    }
    
    // Update length statistics
    int trade_length = trade.bar_length;
    stats_.trades_length += trade_length;
    stats_.trades_length_max = std::max(stats_.trades_length_max, trade_length);
    if (stats_.trades_length_min == std::numeric_limits<int>::max()) {
        stats_.trades_length_min = trade_length;
    } else {
        stats_.trades_length_min = std::min(stats_.trades_length_min, trade_length);
    }
}

void Trades::plot_trade_pnl(double pnl) {
    // Ensure lines are same size as current bar
    while (pnl_plus_line_.size() <= static_cast<size_t>(current_bar_)) {
        pnl_plus_line_.push_back(std::numeric_limits<double>::quiet_NaN());
    }
    while (pnl_minus_line_.size() <= static_cast<size_t>(current_bar_)) {
        pnl_minus_line_.push_back(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Plot on appropriate line based on PnL sign
    if (pnl >= 0.0) {
        // Positive PnL - plot on plus line
        pnl_plus_line_[current_bar_] = pnl;
    } else {
        // Negative PnL - plot on minus line
        pnl_minus_line_[current_bar_] = pnl;
    }
}

// DataTrades Observer implementation
DataTrades::DataTrades() : Observer() {
    // Initialize trade lines map
    trade_lines_.clear();
}

void DataTrades::start() {
    Observer::start();
    
    // Initialize trade lines for each data feed
    initialize_data_lines();
}

void DataTrades::next() {
    // Add NaN values to all data lines to maintain continuity
    for (auto& pair : trade_lines_) {
        pair.second.push_back(std::numeric_limits<double>::quiet_NaN());
    }
}

void DataTrades::notify_trade(const Trade& trade) {
    // Only process closed trades
    if (!trade.is_closed) {
        return;
    }
    
    // Get the data ID for this trade
    int data_id = trade.data_id;
    
    // Plot the trade PnL on the appropriate data line
    plot_data_trade(data_id, trade.pnl);
}

const std::vector<double>& DataTrades::get_trade_line(int data_id) const {
    static const std::vector<double> empty_line;
    
    auto it = trade_lines_.find(data_id);
    if (it != trade_lines_.end()) {
        return it->second;
    }
    
    return empty_line;
}

void DataTrades::initialize_data_lines() {
    // Initialize trade lines for each data feed
    // In practice, this would iterate through all available data feeds
    // For now, we'll initialize as needed when trades are reported
    trade_lines_.clear();
}

void DataTrades::plot_data_trade(int data_id, double pnl) {
    // Ensure the data line exists
    if (trade_lines_.find(data_id) == trade_lines_.end()) {
        trade_lines_[data_id] = std::vector<double>();
        
        // Fill with NaN values up to current bar
        for (int i = 0; i <= current_bar_; ++i) {
            trade_lines_[data_id].push_back(std::numeric_limits<double>::quiet_NaN());
        }
    }
    
    auto& data_line = trade_lines_[data_id];
    
    // Ensure line is correct size
    while (data_line.size() <= static_cast<size_t>(current_bar_)) {
        data_line.push_back(std::numeric_limits<double>::quiet_NaN());
    }
    
    // Plot the PnL at current bar
    data_line[current_bar_] = pnl;
}

} // namespace backtrader