#pragma once

#include "../observer.h"
#include "../order.h"
#include <memory>
#include <vector>

namespace backtrader {
namespace observers {

/**
 * BuySell - Buy/Sell signal observer
 * 
 * Tracks and displays buy and sell signals on the chart.
 * Shows when orders are executed and at what prices.
 */
class BuySell : public Observer {
public:
    // Parameters structure
    struct Params {
        double barplot = false;      // Use bar plot instead of markers
        double bardist = 0.015;      // Distance from high/low for markers
    };

    // Lines
    enum Lines {
        BUY = 0,
        SELL = 1
    };

    BuySell(const Params& params = Params{});
    virtual ~BuySell() = default;

    // Observer interface
    void next() override;
    void start() override;
    void stop() override;
    
    // Order tracking
    void notify_order(std::shared_ptr<Order> order) override;
    void notify_trade(std::shared_ptr<Trade> trade) override;

    // Signal access
    bool has_buy_signal(int lookback = 0) const;
    bool has_sell_signal(int lookback = 0) const;
    double get_buy_price(int lookback = 0) const;
    double get_sell_price(int lookback = 0) const;
    
    // Signal history
    std::vector<std::pair<int, double>> get_buy_signals() const;
    std::vector<std::pair<int, double>> get_sell_signals() const;

private:
    // Parameters
    Params params_;
    
    // Signal tracking
    std::vector<std::pair<int, double>> buy_signals_;   // (bar_index, price)
    std::vector<std::pair<int, double>> sell_signals_;  // (bar_index, price)
    
    // Current bar state
    bool current_bar_has_buy_ = false;
    bool current_bar_has_sell_ = false;
    double current_buy_price_ = 0.0;
    double current_sell_price_ = 0.0;
    
    // Internal methods
    void process_executed_order(std::shared_ptr<Order> order);
    void mark_buy_signal(double price);
    void mark_sell_signal(double price);
    
    // Price calculation for markers
    double calculate_buy_marker_price(double execution_price) const;
    double calculate_sell_marker_price(double execution_price) const;
    
    // Utility methods
    void update_line_values();
    void clear_current_signals();
};

} // namespace observers
} // namespace backtrader