#pragma once

#include "../observer.h"
#include "../trade.h"
#include <vector>
#include <map>

namespace backtrader {

// Trades Observer - tracks completed trades and plots PnL levels
class Trades : public Observer {
public:
    struct Params {
        bool pnlcomm = true;            // Show net profit/loss (after commission)
    } params;
    
    // Lines
    enum Lines {
        PNLPLUS = 0,                    // Positive PnL line
        PNLMINUS = 1                    // Negative PnL line
    };
    
    // Trade statistics
    struct TradeStats {
        int total_trades = 0;
        int trades_long = 0;
        int trades_short = 0;
        int trades_plus = 0;            // Winning trades count
        int trades_minus = 0;           // Losing trades count
        int trades_plus_gross = 0;      // Winning trades before commission
        int trades_minus_gross = 0;     // Losing trades before commission
        
        double trades_win = 0.0;        // Total winning amount
        double trades_win_max = 0.0;    // Maximum single win
        double trades_win_min = 0.0;    // Minimum single win
        
        double trades_loss = 0.0;       // Total losing amount
        double trades_loss_max = 0.0;   // Maximum single loss
        double trades_loss_min = 0.0;   // Minimum single loss
        
        int trades_length = 0;          // Total length of all trades
        int trades_length_max = 0;      // Maximum trade length
        int trades_length_min = 0;      // Minimum trade length
    };
    
    Trades();
    virtual ~Trades() = default;
    
    // Observer interface
    void next() override;
    void notify_trade(const Trade& trade) override;
    
    // Get trade statistics
    const TradeStats& get_stats() const { return stats_; }
    
    // Get PnL lines
    const std::vector<double>& get_pnl_plus_line() const { return pnl_plus_line_; }
    const std::vector<double>& get_pnl_minus_line() const { return pnl_minus_line_; }
    
private:
    // PnL tracking lines
    std::vector<double> pnl_plus_line_;   // Positive PnL points
    std::vector<double> pnl_minus_line_;  // Negative PnL points
    
    // Trade statistics
    TradeStats stats_;
    
    // Helper methods
    void update_trade_stats(const Trade& trade);
    void plot_trade_pnl(double pnl);
};

// DataTrades Observer - tracks trades for multiple data feeds
class DataTrades : public Observer {
public:
    struct Params {
        bool usenames = true;           // Use data names for line names
    } params;
    
    DataTrades();
    virtual ~DataTrades() = default;
    
    // Observer interface
    void start() override;
    void next() override;
    void notify_trade(const Trade& trade) override;
    
    // Get trade data for specific data feed
    const std::vector<double>& get_trade_line(int data_id) const;
    
private:
    // Trade lines for each data feed
    std::map<int, std::vector<double>> trade_lines_;
    
    // Helper methods
    void initialize_data_lines();
    void plot_data_trade(int data_id, double pnl);
};

} // namespace backtrader