#include "analyzers/tradeanalyzer.h"
#include <algorithm>
#include <sstream>

namespace backtrader {

TradeAnalyzer::TradeAnalyzer() : Analyzer() {
    // Initialize with default values
    stats_.total.total = 0;
}

void TradeAnalyzer::start() {
    Analyzer::start();
    
    // Reset all statistics
    stats_ = TradeStats{};
    stats_.total.total = 0;
}

void TradeAnalyzer::stop() {
    Analyzer::stop();
}

void TradeAnalyzer::notify_trade(const Trade& trade) {
    if (trade.just_opened) {
        // Trade just opened
        stats_.total.total++;
        stats_.total.open++;
    } else if (trade.status == Trade::Status::Closed) {
        // Trade just closed
        stats_.total.open--;
        stats_.total.closed++;
        
        // Determine if trade was won or lost
        bool won = (trade.pnl_comm >= 0.0);
        bool is_long = trade.is_long;
        
        // Update streak statistics
        update_streak(won);
        
        // Update PnL statistics
        update_pnl_stats(trade);
        
        // Update won/lost statistics
        update_won_lost_stats(trade, won);
        
        // Update long/short statistics
        update_long_short_stats(trade, is_long);
        
        // Update length statistics
        update_length_stats(trade);
    }
}

std::map<std::string, double> TradeAnalyzer::get_analysis() {
    return flatten_stats();
}

void TradeAnalyzer::update_streak(bool won) {
    if (won) {
        // Won streak
        stats_.streak.won.current = stats_.streak.won.current * 1 + 1;
        stats_.streak.won.longest = std::max(stats_.streak.won.longest, stats_.streak.won.current);
        // Reset lost streak
        stats_.streak.lost.current = 0;
    } else {
        // Lost streak
        stats_.streak.lost.current = stats_.streak.lost.current * 1 + 1;
        stats_.streak.lost.longest = std::max(stats_.streak.lost.longest, stats_.streak.lost.current);
        // Reset won streak
        stats_.streak.won.current = 0;
    }
}

void TradeAnalyzer::update_pnl_stats(const Trade& trade) {
    // Update gross PnL
    stats_.pnl.gross.total += trade.pnl;
    stats_.pnl.gross.average = stats_.pnl.gross.total / stats_.total.closed;
    
    // Update net PnL
    stats_.pnl.net.total += trade.pnl_comm;
    stats_.pnl.net.average = stats_.pnl.net.total / stats_.total.closed;
}

void TradeAnalyzer::update_won_lost_stats(const Trade& trade, bool won) {
    if (won) {
        // Won trade statistics
        stats_.won.total++;
        stats_.won.pnl.total += trade.pnl_comm;
        stats_.won.pnl.average = stats_.won.pnl.total / stats_.won.total;
        stats_.won.pnl.max = std::max(stats_.won.pnl.max, trade.pnl_comm);
    } else {
        // Lost trade statistics
        stats_.lost.total++;
        stats_.lost.pnl.total += trade.pnl_comm;
        stats_.lost.pnl.average = stats_.lost.pnl.total / stats_.lost.total;
        stats_.lost.pnl.max = std::min(stats_.lost.pnl.max, trade.pnl_comm);
    }
}

void TradeAnalyzer::update_long_short_stats(const Trade& trade, bool is_long) {
    bool won = (trade.pnl_comm >= 0.0);
    
    if (is_long) {
        // Long trade statistics
        stats_.long_trades.total++;
        stats_.long_trades.pnl.total += trade.pnl_comm;
        stats_.long_trades.pnl.average = stats_.long_trades.pnl.total / stats_.long_trades.total;
        
        if (won) {
            stats_.long_trades.won++;
            stats_.long_trades.pnl.won.total += trade.pnl_comm;
            stats_.long_trades.pnl.won.average = stats_.long_trades.pnl.won.total / stats_.long_trades.won;
            stats_.long_trades.pnl.won.max = std::max(stats_.long_trades.pnl.won.max, trade.pnl_comm);
        } else {
            stats_.long_trades.lost++;
            stats_.long_trades.pnl.lost.total += trade.pnl_comm;
            stats_.long_trades.pnl.lost.average = stats_.long_trades.pnl.lost.total / stats_.long_trades.lost;
            stats_.long_trades.pnl.lost.max = std::min(stats_.long_trades.pnl.lost.max, trade.pnl_comm);
        }
    } else {
        // Short trade statistics
        stats_.short_trades.total++;
        stats_.short_trades.pnl.total += trade.pnl_comm;
        stats_.short_trades.pnl.average = stats_.short_trades.pnl.total / stats_.short_trades.total;
        
        if (won) {
            stats_.short_trades.won++;
            stats_.short_trades.pnl.won.total += trade.pnl_comm;
            stats_.short_trades.pnl.won.average = stats_.short_trades.pnl.won.total / stats_.short_trades.won;
            stats_.short_trades.pnl.won.max = std::max(stats_.short_trades.pnl.won.max, trade.pnl_comm);
        } else {
            stats_.short_trades.lost++;
            stats_.short_trades.pnl.lost.total += trade.pnl_comm;
            stats_.short_trades.pnl.lost.average = stats_.short_trades.pnl.lost.total / stats_.short_trades.lost;
            stats_.short_trades.pnl.lost.max = std::min(stats_.short_trades.pnl.lost.max, trade.pnl_comm);
        }
    }
}

void TradeAnalyzer::update_length_stats(const Trade& trade) {
    bool won = (trade.pnl_comm >= 0.0);
    bool is_long = trade.is_long;
    int bar_length = trade.bar_length;
    
    // Overall length statistics
    stats_.len.total += bar_length;
    stats_.len.average = static_cast<double>(stats_.len.total) / stats_.total.closed;
    stats_.len.max = std::max(stats_.len.max, bar_length);
    stats_.len.min = std::min(stats_.len.min, bar_length);
    
    // Won/Lost length statistics
    if (won) {
        stats_.len.won.total += bar_length;
        stats_.len.won.average = static_cast<double>(stats_.len.won.total) / (stats_.won.total ? stats_.won.total : 1);
        stats_.len.won.max = std::max(stats_.len.won.max, bar_length);
        if (bar_length > 0) {
            stats_.len.won.min = std::min(stats_.len.won.min, bar_length);
        }
    } else {
        stats_.len.lost.total += bar_length;
        stats_.len.lost.average = static_cast<double>(stats_.len.lost.total) / (stats_.lost.total ? stats_.lost.total : 1);
        stats_.len.lost.max = std::max(stats_.len.lost.max, bar_length);
        if (bar_length > 0) {
            stats_.len.lost.min = std::min(stats_.len.lost.min, bar_length);
        }
    }
    
    // Long/Short length statistics
    if (is_long) {
        stats_.len.long_trades.total += bar_length;
        stats_.len.long_trades.average = static_cast<double>(stats_.len.long_trades.total) / (stats_.long_trades.total ? stats_.long_trades.total : 1);
        stats_.len.long_trades.max = std::max(stats_.len.long_trades.max, bar_length);
        stats_.len.long_trades.min = std::min(stats_.len.long_trades.min, bar_length ? bar_length : stats_.len.long_trades.min);
        
        if (won) {
            stats_.len.long_trades.won.total += bar_length;
            stats_.len.long_trades.won.average = static_cast<double>(stats_.len.long_trades.won.total) / (stats_.long_trades.won ? stats_.long_trades.won : 1);
            stats_.len.long_trades.won.max = std::max(stats_.len.long_trades.won.max, bar_length);
            stats_.len.long_trades.won.min = std::min(stats_.len.long_trades.won.min, bar_length ? bar_length : stats_.len.long_trades.won.min);
        } else {
            stats_.len.long_trades.lost.total += bar_length;
            stats_.len.long_trades.lost.average = static_cast<double>(stats_.len.long_trades.lost.total) / (stats_.long_trades.lost ? stats_.long_trades.lost : 1);
            stats_.len.long_trades.lost.max = std::max(stats_.len.long_trades.lost.max, bar_length);
            stats_.len.long_trades.lost.min = std::min(stats_.len.long_trades.lost.min, bar_length ? bar_length : stats_.len.long_trades.lost.min);
        }
    } else {
        stats_.len.short_trades.total += bar_length;
        stats_.len.short_trades.average = static_cast<double>(stats_.len.short_trades.total) / (stats_.short_trades.total ? stats_.short_trades.total : 1);
        stats_.len.short_trades.max = std::max(stats_.len.short_trades.max, bar_length);
        stats_.len.short_trades.min = std::min(stats_.len.short_trades.min, bar_length ? bar_length : stats_.len.short_trades.min);
        
        if (won) {
            stats_.len.short_trades.won.total += bar_length;
            stats_.len.short_trades.won.average = static_cast<double>(stats_.len.short_trades.won.total) / (stats_.short_trades.won ? stats_.short_trades.won : 1);
            stats_.len.short_trades.won.max = std::max(stats_.len.short_trades.won.max, bar_length);
            stats_.len.short_trades.won.min = std::min(stats_.len.short_trades.won.min, bar_length ? bar_length : stats_.len.short_trades.won.min);
        } else {
            stats_.len.short_trades.lost.total += bar_length;
            stats_.len.short_trades.lost.average = static_cast<double>(stats_.len.short_trades.lost.total) / (stats_.short_trades.lost ? stats_.short_trades.lost : 1);
            stats_.len.short_trades.lost.max = std::max(stats_.len.short_trades.lost.max, bar_length);
            stats_.len.short_trades.lost.min = std::min(stats_.len.short_trades.lost.min, bar_length ? bar_length : stats_.len.short_trades.lost.min);
        }
    }
}

std::map<std::string, double> TradeAnalyzer::flatten_stats() const {
    std::map<std::string, double> analysis;
    
    // Total statistics
    analysis["total.total"] = static_cast<double>(stats_.total.total);
    analysis["total.open"] = static_cast<double>(stats_.total.open);
    analysis["total.closed"] = static_cast<double>(stats_.total.closed);
    
    // Streak statistics
    analysis["streak.won.current"] = static_cast<double>(stats_.streak.won.current);
    analysis["streak.won.longest"] = static_cast<double>(stats_.streak.won.longest);
    analysis["streak.lost.current"] = static_cast<double>(stats_.streak.lost.current);
    analysis["streak.lost.longest"] = static_cast<double>(stats_.streak.lost.longest);
    
    // PnL statistics
    analysis["pnl.gross.total"] = stats_.pnl.gross.total;
    analysis["pnl.gross.average"] = stats_.pnl.gross.average;
    analysis["pnl.net.total"] = stats_.pnl.net.total;
    analysis["pnl.net.average"] = stats_.pnl.net.average;
    
    // Won/Lost statistics
    analysis["won.total"] = static_cast<double>(stats_.won.total);
    analysis["won.pnl.total"] = stats_.won.pnl.total;
    analysis["won.pnl.average"] = stats_.won.pnl.average;
    analysis["won.pnl.max"] = stats_.won.pnl.max;
    
    analysis["lost.total"] = static_cast<double>(stats_.lost.total);
    analysis["lost.pnl.total"] = stats_.lost.pnl.total;
    analysis["lost.pnl.average"] = stats_.lost.pnl.average;
    analysis["lost.pnl.max"] = stats_.lost.pnl.max;
    
    // Long/Short statistics
    analysis["long.total"] = static_cast<double>(stats_.long_trades.total);
    analysis["long.pnl.total"] = stats_.long_trades.pnl.total;
    analysis["long.pnl.average"] = stats_.long_trades.pnl.average;
    analysis["long.won"] = static_cast<double>(stats_.long_trades.won);
    analysis["long.lost"] = static_cast<double>(stats_.long_trades.lost);
    
    analysis["short.total"] = static_cast<double>(stats_.short_trades.total);
    analysis["short.pnl.total"] = stats_.short_trades.pnl.total;
    analysis["short.pnl.average"] = stats_.short_trades.pnl.average;
    analysis["short.won"] = static_cast<double>(stats_.short_trades.won);
    analysis["short.lost"] = static_cast<double>(stats_.short_trades.lost);
    
    // Length statistics
    analysis["len.total"] = static_cast<double>(stats_.len.total);
    analysis["len.average"] = stats_.len.average;
    analysis["len.max"] = static_cast<double>(stats_.len.max);
    analysis["len.min"] = static_cast<double>(stats_.len.min);
    
    return analysis;
}

} // namespace backtrader