#pragma once

#include "analyzers/AnalyzerBase.h"
#include "Trade.h"
#include <memory>
#include <limits>

namespace backtrader {

/**
 * @brief Trade statistics analyzer
 * 
 * Provides comprehensive statistics on closed trades including:
 * - Total open/closed trades
 * - Win/loss streaks
 * - Profit and loss analysis
 * - Long/short position analysis
 * - Trade duration statistics
 */
class TradeAnalyzer : public AnalyzerBase {
public:
    /**
     * @brief Trade statistics structure
     */
    struct TradeStats {
        // Total statistics
        struct Total {
            int total = 0;
            int open = 0;
            int closed = 0;
        } total;
        
        // Streak statistics
        struct Streak {
            struct StreakType {
                int current = 0;
                int longest = 0;
            } won, lost;
        } streak;
        
        // PnL statistics
        struct PnL {
            struct PnLType {
                double total = 0.0;
                double average = 0.0;
            } gross, net;
        } pnl;
        
        // Win/Loss statistics
        struct WinLoss {
            int total = 0;
            struct WinLossPnL {
                double total = 0.0;
                double average = 0.0;
                double max = 0.0;
            } pnl;
        } won, lost;
        
        // Long/Short statistics
        struct LongShort {
            int total = 0;
            struct LongShortPnL {
                double total = 0.0;
                double average = 0.0;
            } pnl;
            
            struct LongShortWinLoss {
                int won = 0;
                int lost = 0;
                struct LongShortWinLossPnL {
                    double total = 0.0;
                    double average = 0.0;
                    double max = 0.0;
                } pnl;
            } won, lost;
        } long_pos, short_pos;
        
        // Trade length statistics
        struct Length {
            int total = 0;
            double average = 0.0;
            int max = 0;
            int min = std::numeric_limits<int>::max();
            
            struct LengthWinLoss {
                int total = 0;
                double average = 0.0;
                int max = 0;
                int min = std::numeric_limits<int>::max();
            } won, lost;
            
            struct LengthLongShort {
                int total = 0;
                double average = 0.0;
                int max = 0;
                int min = std::numeric_limits<int>::max();
                
                struct LengthLongShortWinLoss {
                    int total = 0;
                    double average = 0.0;
                    int max = 0;
                    int min = std::numeric_limits<int>::max();
                } won, lost;
            } long_pos, short_pos;
        } len;
        
        // Utility methods
        double getWinRate() const {
            return (total.closed > 0) ? (static_cast<double>(won.total) / total.closed) : 0.0;
        }
        
        double getProfitFactor() const {
            return (lost.pnl.total != 0.0) ? (won.pnl.total / std::abs(lost.pnl.total)) : 0.0;
        }
        
        double getAverageWin() const {
            return won.pnl.average;
        }
        
        double getAverageLoss() const {
            return lost.pnl.average;
        }
        
        double getMaxWin() const {
            return won.pnl.max;
        }
        
        double getMaxLoss() const {
            return lost.pnl.max;
        }
        
        double getExpectancy() const {
            if (total.closed == 0) return 0.0;
            return (won.pnl.total + lost.pnl.total) / total.closed;
        }
        
        double getSharpeRatio() const {
            // Simplified Sharpe ratio calculation
            if (total.closed <= 1) return 0.0;
            
            double mean_return = getExpectancy();
            double variance = 0.0;
            
            // This is a simplified version - in practice we'd need individual trade returns
            // For now, we'll use a rough approximation
            if (won.total > 0 && lost.total > 0) {
                double win_var = std::pow(won.pnl.average - mean_return, 2) * won.total;
                double loss_var = std::pow(lost.pnl.average - mean_return, 2) * lost.total;
                variance = (win_var + loss_var) / (total.closed - 1);
            }
            
            double std_dev = std::sqrt(variance);
            return (std_dev != 0.0) ? (mean_return / std_dev) : 0.0;
        }
        
        // Reset all statistics
        void reset() {
            *this = TradeStats{};
        }
    };

private:
    TradeStats stats_;
    
public:
    explicit TradeAnalyzer(const std::string& name = "TradeAnalyzer")
        : AnalyzerBase(name) {}
    
    /**
     * @brief Initialize analyzer
     */
    void start() override {
        AnalyzerBase::start();
        stats_.reset();
    }
    
    /**
     * @brief Process trade notification
     * @param trade The trade object
     */
    void notifyTrade(const Trade& trade) override {
        if (trade.justOpened()) {
            // Trade just opened
            stats_.total.total++;
            stats_.total.open++;
            
        } else if (trade.isClosed()) {
            // Trade just closed
            processClosedTrade(trade);
        }
    }
    
    /**
     * @brief Get trade statistics
     * @return Reference to TradeStats structure
     */
    const TradeStats& getStats() const {
        return stats_;
    }
    
    /**
     * @brief Get analysis results as key-value pairs
     * @return Analysis results map
     */
    std::map<std::string, double> getAnalysis() const override {
        std::map<std::string, double> result;
        
        // Total statistics
        result["total.total"] = stats_.total.total;
        result["total.open"] = stats_.total.open;
        result["total.closed"] = stats_.total.closed;
        
        // Streak statistics
        result["streak.won.current"] = stats_.streak.won.current;
        result["streak.won.longest"] = stats_.streak.won.longest;
        result["streak.lost.current"] = stats_.streak.lost.current;
        result["streak.lost.longest"] = stats_.streak.lost.longest;
        
        // PnL statistics
        result["pnl.gross.total"] = stats_.pnl.gross.total;
        result["pnl.gross.average"] = stats_.pnl.gross.average;
        result["pnl.net.total"] = stats_.pnl.net.total;
        result["pnl.net.average"] = stats_.pnl.net.average;
        
        // Win/Loss statistics
        result["won.total"] = stats_.won.total;
        result["won.pnl.total"] = stats_.won.pnl.total;
        result["won.pnl.average"] = stats_.won.pnl.average;
        result["won.pnl.max"] = stats_.won.pnl.max;
        
        result["lost.total"] = stats_.lost.total;
        result["lost.pnl.total"] = stats_.lost.pnl.total;
        result["lost.pnl.average"] = stats_.lost.pnl.average;
        result["lost.pnl.max"] = stats_.lost.pnl.max;
        
        // Long/Short statistics
        result["long.total"] = stats_.long_pos.total;
        result["long.pnl.total"] = stats_.long_pos.pnl.total;
        result["long.pnl.average"] = stats_.long_pos.pnl.average;
        
        result["short.total"] = stats_.short_pos.total;
        result["short.pnl.total"] = stats_.short_pos.pnl.total;
        result["short.pnl.average"] = stats_.short_pos.pnl.average;
        
        // Length statistics
        result["len.total"] = stats_.len.total;
        result["len.average"] = stats_.len.average;
        result["len.max"] = stats_.len.max;
        result["len.min"] = (stats_.len.min == std::numeric_limits<int>::max()) ? 0 : stats_.len.min;
        
        // Derived metrics
        result["win_rate"] = stats_.getWinRate();
        result["profit_factor"] = stats_.getProfitFactor();
        result["expectancy"] = stats_.getExpectancy();
        result["sharpe_ratio"] = stats_.getSharpeRatio();
        
        return result;
    }
    
    /**
     * @brief Print analysis results
     */
    void printAnalysis() const override {
        std::cout << "=== Trade Analysis Results ===" << std::endl;
        std::cout << "Total Trades: " << stats_.total.total << std::endl;
        std::cout << "Closed Trades: " << stats_.total.closed << std::endl;
        std::cout << "Open Trades: " << stats_.total.open << std::endl;
        std::cout << std::endl;
        
        std::cout << "Win Rate: " << (stats_.getWinRate() * 100) << "%" << std::endl;
        std::cout << "Profit Factor: " << stats_.getProfitFactor() << std::endl;
        std::cout << "Expectancy: " << stats_.getExpectancy() << std::endl;
        std::cout << std::endl;
        
        std::cout << "Winning Trades: " << stats_.won.total << std::endl;
        std::cout << "  Total PnL: " << stats_.won.pnl.total << std::endl;
        std::cout << "  Average PnL: " << stats_.won.pnl.average << std::endl;
        std::cout << "  Max PnL: " << stats_.won.pnl.max << std::endl;
        std::cout << std::endl;
        
        std::cout << "Losing Trades: " << stats_.lost.total << std::endl;
        std::cout << "  Total PnL: " << stats_.lost.pnl.total << std::endl;
        std::cout << "  Average PnL: " << stats_.lost.pnl.average << std::endl;
        std::cout << "  Max Loss: " << stats_.lost.pnl.max << std::endl;
        std::cout << std::endl;
        
        std::cout << "Longest Win Streak: " << stats_.streak.won.longest << std::endl;
        std::cout << "Longest Loss Streak: " << stats_.streak.lost.longest << std::endl;
        std::cout << std::endl;
        
        std::cout << "Long Positions: " << stats_.long_pos.total << std::endl;
        std::cout << "  Total PnL: " << stats_.long_pos.pnl.total << std::endl;
        std::cout << "Short Positions: " << stats_.short_pos.total << std::endl;
        std::cout << "  Total PnL: " << stats_.short_pos.pnl.total << std::endl;
        std::cout << std::endl;
        
        std::cout << "Average Trade Length: " << stats_.len.average << " bars" << std::endl;
        std::cout << "Max Trade Length: " << stats_.len.max << " bars" << std::endl;
        std::cout << "Min Trade Length: " << 
            ((stats_.len.min == std::numeric_limits<int>::max()) ? 0 : stats_.len.min) 
            << " bars" << std::endl;
    }
    
private:
    /**
     * @brief Process a closed trade
     * @param trade The closed trade
     */
    void processClosedTrade(const Trade& trade) {
        // Update basic counters
        stats_.total.open--;
        stats_.total.closed++;
        
        // Determine win/loss and long/short
        bool won = (trade.getPnLComm() >= 0.0);
        bool lost = !won;
        bool is_long = trade.isLong();
        bool is_short = !is_long;
        
        // Update streak statistics
        updateStreaks(won, lost);
        
        // Update PnL statistics
        updatePnLStats(trade);
        
        // Update win/loss statistics
        updateWinLossStats(trade, won, lost);
        
        // Update long/short statistics
        updateLongShortStats(trade, won, lost, is_long, is_short);
        
        // Update length statistics
        updateLengthStats(trade, won, lost, is_long, is_short);
    }
    
    void updateStreaks(bool won, bool lost) {
        // Update win streak
        if (won) {
            stats_.streak.won.current++;
            stats_.streak.lost.current = 0;
            stats_.streak.won.longest = std::max(stats_.streak.won.longest, stats_.streak.won.current);
        } else {
            stats_.streak.lost.current++;
            stats_.streak.won.current = 0;
            stats_.streak.lost.longest = std::max(stats_.streak.lost.longest, stats_.streak.lost.current);
        }
    }
    
    void updatePnLStats(const Trade& trade) {
        // Gross PnL
        stats_.pnl.gross.total += trade.getPnL();
        stats_.pnl.gross.average = stats_.pnl.gross.total / stats_.total.closed;
        
        // Net PnL
        stats_.pnl.net.total += trade.getPnLComm();
        stats_.pnl.net.average = stats_.pnl.net.total / stats_.total.closed;
    }
    
    void updateWinLossStats(const Trade& trade, bool won, bool lost) {
        double pnl_comm = trade.getPnLComm();
        
        if (won) {
            stats_.won.total++;
            stats_.won.pnl.total += pnl_comm;
            stats_.won.pnl.average = stats_.won.pnl.total / stats_.won.total;
            stats_.won.pnl.max = std::max(stats_.won.pnl.max, pnl_comm);
        } else {
            stats_.lost.total++;
            stats_.lost.pnl.total += pnl_comm;
            stats_.lost.pnl.average = stats_.lost.pnl.total / stats_.lost.total;
            stats_.lost.pnl.max = std::min(stats_.lost.pnl.max, pnl_comm);
        }
    }
    
    void updateLongShortStats(const Trade& trade, bool won, bool lost, bool is_long, bool is_short) {
        double pnl_comm = trade.getPnLComm();
        
        if (is_long) {
            stats_.long_pos.total++;
            stats_.long_pos.pnl.total += pnl_comm;
            stats_.long_pos.pnl.average = stats_.long_pos.pnl.total / stats_.long_pos.total;
            
            if (won) {
                stats_.long_pos.won.won++;
                stats_.long_pos.won.pnl.total += pnl_comm;
                stats_.long_pos.won.pnl.average = stats_.long_pos.won.pnl.total / stats_.long_pos.won.won;
                stats_.long_pos.won.pnl.max = std::max(stats_.long_pos.won.pnl.max, pnl_comm);
            } else {
                stats_.long_pos.lost.lost++;
                stats_.long_pos.lost.pnl.total += pnl_comm;
                stats_.long_pos.lost.pnl.average = stats_.long_pos.lost.pnl.total / stats_.long_pos.lost.lost;
                stats_.long_pos.lost.pnl.max = std::min(stats_.long_pos.lost.pnl.max, pnl_comm);
            }
        } else {
            stats_.short_pos.total++;
            stats_.short_pos.pnl.total += pnl_comm;
            stats_.short_pos.pnl.average = stats_.short_pos.pnl.total / stats_.short_pos.total;
            
            if (won) {
                stats_.short_pos.won.won++;
                stats_.short_pos.won.pnl.total += pnl_comm;
                stats_.short_pos.won.pnl.average = stats_.short_pos.won.pnl.total / stats_.short_pos.won.won;
                stats_.short_pos.won.pnl.max = std::max(stats_.short_pos.won.pnl.max, pnl_comm);
            } else {
                stats_.short_pos.lost.lost++;
                stats_.short_pos.lost.pnl.total += pnl_comm;
                stats_.short_pos.lost.pnl.average = stats_.short_pos.lost.pnl.total / stats_.short_pos.lost.lost;
                stats_.short_pos.lost.pnl.max = std::min(stats_.short_pos.lost.pnl.max, pnl_comm);
            }
        }
    }
    
    void updateLengthStats(const Trade& trade, bool won, bool lost, bool is_long, bool is_short) {
        int bar_len = trade.getBarLen();
        
        // Overall length statistics
        stats_.len.total += bar_len;
        stats_.len.average = static_cast<double>(stats_.len.total) / stats_.total.closed;
        stats_.len.max = std::max(stats_.len.max, bar_len);
        stats_.len.min = std::min(stats_.len.min, bar_len);
        
        // Win/Loss length statistics
        if (won) {
            stats_.len.won.total += bar_len;
            stats_.len.won.average = static_cast<double>(stats_.len.won.total) / stats_.won.total;
            stats_.len.won.max = std::max(stats_.len.won.max, bar_len);
            stats_.len.won.min = std::min(stats_.len.won.min, bar_len);
        } else {
            stats_.len.lost.total += bar_len;
            stats_.len.lost.average = static_cast<double>(stats_.len.lost.total) / stats_.lost.total;
            stats_.len.lost.max = std::max(stats_.len.lost.max, bar_len);
            stats_.len.lost.min = std::min(stats_.len.lost.min, bar_len);
        }
        
        // Long/Short length statistics
        if (is_long) {
            stats_.len.long_pos.total += bar_len;
            stats_.len.long_pos.average = static_cast<double>(stats_.len.long_pos.total) / stats_.long_pos.total;
            stats_.len.long_pos.max = std::max(stats_.len.long_pos.max, bar_len);
            stats_.len.long_pos.min = std::min(stats_.len.long_pos.min, bar_len);
            
            if (won) {
                stats_.len.long_pos.won.total += bar_len;
                stats_.len.long_pos.won.average = static_cast<double>(stats_.len.long_pos.won.total) / stats_.long_pos.won.won;
                stats_.len.long_pos.won.max = std::max(stats_.len.long_pos.won.max, bar_len);
                stats_.len.long_pos.won.min = std::min(stats_.len.long_pos.won.min, bar_len);
            } else {
                stats_.len.long_pos.lost.total += bar_len;
                stats_.len.long_pos.lost.average = static_cast<double>(stats_.len.long_pos.lost.total) / stats_.long_pos.lost.lost;
                stats_.len.long_pos.lost.max = std::max(stats_.len.long_pos.lost.max, bar_len);
                stats_.len.long_pos.lost.min = std::min(stats_.len.long_pos.lost.min, bar_len);
            }
        } else {
            stats_.len.short_pos.total += bar_len;
            stats_.len.short_pos.average = static_cast<double>(stats_.len.short_pos.total) / stats_.short_pos.total;
            stats_.len.short_pos.max = std::max(stats_.len.short_pos.max, bar_len);
            stats_.len.short_pos.min = std::min(stats_.len.short_pos.min, bar_len);
            
            if (won) {
                stats_.len.short_pos.won.total += bar_len;
                stats_.len.short_pos.won.average = static_cast<double>(stats_.len.short_pos.won.total) / stats_.short_pos.won.won;
                stats_.len.short_pos.won.max = std::max(stats_.len.short_pos.won.max, bar_len);
                stats_.len.short_pos.won.min = std::min(stats_.len.short_pos.won.min, bar_len);
            } else {
                stats_.len.short_pos.lost.total += bar_len;
                stats_.len.short_pos.lost.average = static_cast<double>(stats_.len.short_pos.lost.total) / stats_.short_pos.lost.lost;
                stats_.len.short_pos.lost.max = std::max(stats_.len.short_pos.lost.max, bar_len);
                stats_.len.short_pos.lost.min = std::min(stats_.len.short_pos.lost.min, bar_len);
            }
        }
    }
};

} // namespace backtrader