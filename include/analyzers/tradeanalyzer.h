#pragma once

#include "../analyzer.h"
#include <map>
#include <string>
#include <limits>

namespace backtrader {

// TradeAnalyzer - provides comprehensive statistics on closed trades
class TradeAnalyzer : public Analyzer {
public:
    // Trade statistics structure
    struct TradeStats {
        struct Total {
            int total = 0;
            int open = 0;
            int closed = 0;
        } total;
        
        struct Streak {
            struct WonLost {
                int current = 0;
                int longest = 0;
            } won, lost;
        } streak;
        
        struct PnL {
            struct GrossNet {
                double total = 0.0;
                double average = 0.0;
            } gross, net;
        } pnl;
        
        struct WonLost {
            int total = 0;
            struct WonLostPnL {
                double total = 0.0;
                double average = 0.0;
                double max = 0.0;
            } pnl;
        } won, lost;
        
        struct LongShort {
            int total = 0;
            struct LongShortPnL {
                double total = 0.0;
                double average = 0.0;
                struct WonLostPnL {
                    double total = 0.0;
                    double average = 0.0;
                    double max = 0.0;
                } won, lost;
            } pnl;
            int won = 0;
            int lost = 0;
        } long_trades, short_trades;
        
        struct Length {
            int total = 0;
            double average = 0.0;
            int max = 0;
            int min = std::numeric_limits<int>::max();
            
            struct WonLostLen {
                int total = 0;
                double average = 0.0;
                int max = 0;
                int min = std::numeric_limits<int>::max();
            } won, lost;
            
            struct LongShortLen {
                int total = 0;
                double average = 0.0;
                int max = 0;
                int min = std::numeric_limits<int>::max();
                
                struct WonLostLen {
                    int total = 0;
                    double average = 0.0;
                    int max = 0;
                    int min = std::numeric_limits<int>::max();
                } won, lost;
            } long_trades, short_trades;
        } len;
    };
    
    TradeAnalyzer();
    virtual ~TradeAnalyzer() = default;
    
    // Analyzer interface
    void start() override;
    void stop() override;
    void notify_trade(const Trade& trade) override;
    
    // Get analysis data
    std::map<std::string, double> get_analysis() override;
    
    // Get trade statistics
    const TradeStats& get_trade_stats() const { return stats_; }
    
private:
    TradeStats stats_;
    
    // Helper methods
    void update_streak(bool won);
    void update_pnl_stats(const Trade& trade);
    void update_won_lost_stats(const Trade& trade, bool won);
    void update_long_short_stats(const Trade& trade, bool is_long);
    void update_length_stats(const Trade& trade);
    std::map<std::string, double> flatten_stats() const;
};

} // namespace backtrader