#pragma once

#include "dataseries.h"
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <string>

namespace backtrader {

// Forward declarations
class Order;
class Position;
class CommInfo;

// Order types
enum class OrderType {
    Market = 0,
    Limit = 1,
    Stop = 2,
    StopLimit = 3,
    Close = 4
};

// Order status
enum class OrderStatus {
    Created = 0,
    Submitted = 1,
    Accepted = 2,
    Partial = 3,
    Completed = 4,
    Canceled = 5,
    Expired = 6,
    Margin = 7,
    Rejected = 8
};

// Execution info
class ExecutionInfo {
public:
    double price = 0.0;
    double size = 0.0;
    double value = 0.0;
    double comm = 0.0;
    double pnl = 0.0;
    double pnlcomm = 0.0;
    double remsize = 0.0;
    
    ExecutionInfo() = default;
    virtual ~ExecutionInfo() = default;
};

// Base broker class
class BrokerBase {
public:
    BrokerBase() = default;
    virtual ~BrokerBase() = default;
    
    // Cash and value management
    virtual void setcash(double cash) = 0;
    virtual double getcash() const = 0;
    virtual double getvalue() const = 0;
    
    // Position management
    virtual std::shared_ptr<Position> getposition(std::shared_ptr<DataSeries> data) const = 0;
    virtual std::map<DataSeries*, std::shared_ptr<Position>> getpositions() const = 0;
    
    // Order management
    virtual std::shared_ptr<Order> submit(std::shared_ptr<Order> order) = 0;
    virtual bool cancel(std::shared_ptr<Order> order) = 0;
    virtual std::vector<std::shared_ptr<Order>> get_orders_open(std::shared_ptr<DataSeries> data = nullptr) = 0;
    
    // Commission info
    virtual void setcommission(double commission, 
                              double margin = 0.0, 
                              double mult = 1.0,
                              std::shared_ptr<DataSeries> data = nullptr) = 0;
    virtual std::shared_ptr<CommInfo> getcommissioninfo(std::shared_ptr<DataSeries> data) const = 0;
    
    // Notifications
    virtual void next() = 0;
    virtual void start() {}
    virtual void stop() {}
    
    // Notification queue
    virtual std::shared_ptr<Order> get_notification() = 0;
    virtual bool has_notifications() const = 0;
};

// Back testing broker
class BackBroker : public BrokerBase {
public:
    // Parameters
    struct Params {
        double cash = 10000.0;
        bool checksubmit = true;
        bool eosbar = false;
        double slip_perc = 0.0;
        double slip_fixed = 0.0;
        bool slip_open = false;
        bool slip_match = true;
        bool slip_limit = true;
        bool slip_out = false;
        bool coc = false;
        bool coo = false;
        int int2pnl = 1;
        double shortcash = 1.0;
        double fundstartval = 100.0;
        std::string fundmode = "";
        bool percabs = false;
    } params;
    
    BackBroker();
    virtual ~BackBroker() = default;
    
    // Cash and value management
    void setcash(double cash) override;
    double getcash() const override;
    double getvalue() const override;
    
    // Position management
    std::shared_ptr<Position> getposition(std::shared_ptr<DataSeries> data) const override;
    std::map<DataSeries*, std::shared_ptr<Position>> getpositions() const override;
    
    // Order management
    std::shared_ptr<Order> submit(std::shared_ptr<Order> order) override;
    bool cancel(std::shared_ptr<Order> order) override;
    std::vector<std::shared_ptr<Order>> get_orders_open(std::shared_ptr<DataSeries> data = nullptr) override;
    
    // Commission info
    void setcommission(double commission, 
                      double margin = 0.0, 
                      double mult = 1.0,
                      std::shared_ptr<DataSeries> data = nullptr) override;
    std::shared_ptr<CommInfo> getcommissioninfo(std::shared_ptr<DataSeries> data) const override;
    
    // Execution
    void next() override;
    void start() override;
    void stop() override;
    
    // Notification queue implementation
    std::shared_ptr<Order> get_notification() override;
    bool has_notifications() const override;
    
    // Filler function type
    using FillerFunc = std::function<double(std::shared_ptr<Order>, double, int)>;
    void set_filler(FillerFunc filler);
    
    // Slippage calculation
    double get_slippage(std::shared_ptr<Order> order, double price, bool is_buy) const;
    
    // Order execution
    bool execute_order(std::shared_ptr<Order> order, double ago = 0);
    
private:
    double cash_;
    double value_;
    
    // Positions by data
    // Use raw pointer as key since DataSeries lifetime is managed by Cerebro
    std::map<DataSeries*, std::shared_ptr<Position>> positions_;
    std::map<DataSeries*, std::shared_ptr<DataSeries>> data_refs_;  // Store data series references
    
    // Commission info by data
    std::map<std::shared_ptr<DataSeries>, std::shared_ptr<CommInfo>> comminfo_;
    
    // Default commission info for all data when not specified
    std::shared_ptr<CommInfo> default_comminfo_;
    
    // Orders
    std::vector<std::shared_ptr<Order>> orders_;
    std::vector<std::shared_ptr<Order>> pending_orders_;
    std::vector<std::shared_ptr<Order>> new_orders_;  // Orders submitted this bar
    
    // Notification queue
    std::vector<std::shared_ptr<Order>> notifications_;
    
    // Filler function
    FillerFunc filler_;
    
    // Internal methods
    void _process_orders();
    bool _check_cash(std::shared_ptr<Order> order);
    void _update_value();
    double _get_order_price(std::shared_ptr<Order> order, double ago = 0);
    bool _can_execute(std::shared_ptr<Order> order, double price, double ago = 0);
    void _execute_order(std::shared_ptr<Order> order, double price, double size);
    void _update_position(std::shared_ptr<Order> order, double size, double price);
};

// Broker aliases
using BrokerBack = BackBroker;

} // namespace backtrader