#pragma once

#include "order.h"
#include "dataseries.h"
#include <vector>
#include <memory>
#include <chrono>
#include <string>

namespace backtrader {

// Trade status enumeration
enum class TradeStatus {
    Created = 0,
    Open = 1,
    Closed = 2
};

// Trade history entry
class TradeHistory {
public:
    // Status information
    struct Status {
        TradeStatus status = TradeStatus::Created;
        std::chrono::system_clock::time_point dt;
        int barlen = 0;
        double size = 0.0;
        double price = 0.0;
        double value = 0.0;
        double pnl = 0.0;
        double pnlcomm = 0.0;
        std::string tz = "";
    } status;
    
    // Event information
    struct Event {
        std::shared_ptr<Order> order = nullptr;
        double size = 0.0;
        double price = 0.0;
        double commission = 0.0;
    } event;
    
    TradeHistory() = default;
    TradeHistory(TradeStatus status,
                std::chrono::system_clock::time_point dt,
                int barlen,
                double size,
                double price,
                double value,
                double pnl,
                double pnlcomm,
                const std::string& tz);
    virtual ~TradeHistory() = default;
    
    // Update event information
    void doupdate(std::shared_ptr<Order> order,
                  double size,
                  double price,
                  double commission);
    
    // String representation
    std::string to_string() const;
};

// Main Trade class
class Trade {
public:
    // Trade references
    int ref = 0;
    TradeStatus status = TradeStatus::Created;
    bool tradeid = false;
    double size = 0.0;
    double price = 0.0;
    double value = 0.0;
    double commission = 0.0;
    double pnl = 0.0;
    double pnlcomm = 0.0;
    int barlen = 0;
    
    // Dates and times
    std::chrono::system_clock::time_point dtopen;
    std::chrono::system_clock::time_point dtclose;
    int baropen = 0;
    int barclose = 0;
    
    // Data reference
    std::shared_ptr<DataSeries> data = nullptr;
    
    // History
    std::vector<TradeHistory> history;
    bool historynotify = false;
    
    Trade();
    explicit Trade(std::shared_ptr<DataSeries> data);
    virtual ~Trade() = default;
    
    // Trade operations
    void update(std::shared_ptr<Order> order,
                double size,
                double price,
                double value,
                double commission,
                double pnl,
                std::chrono::system_clock::time_point dt);
    
    // Status queries
    bool isopen() const;
    bool isclosed() const;
    bool justopened() const;
    
    // PnL calculations
    double pnl_unrealized(double price) const;
    double pnl_realized() const;
    
    // Size queries
    bool long_() const;
    bool short_() const;
    
    // String representation
    std::string to_string() const;
    
    // Clone
    std::shared_ptr<Trade> clone() const;
    
    // Comparison operators
    bool operator==(const Trade& other) const;
    bool operator!=(const Trade& other) const;
    
protected:
    static int next_ref_;
    
    // Internal state
    bool just_opened_ = false;
    
    // Helper methods
    void _addhistory(TradeStatus status,
                    std::chrono::system_clock::time_point dt,
                    int barlen,
                    double size,
                    double price,
                    double value,
                    double pnl,
                    double pnlcomm);
};

// Trade factory functions
std::shared_ptr<Trade> create_trade(std::shared_ptr<DataSeries> data = nullptr);

} // namespace backtrader