#pragma once

#include "dataseries.h"
#include "broker.h"
#include <memory>
#include <vector>
#include <string>
#include <chrono>

namespace backtrader {

// Forward declarations
class Strategy;

// Order execution bit - holds info about partial executions
class OrderExecutionBit {
public:
    std::chrono::system_clock::time_point dt;
    double size = 0.0;
    double price = 0.0;
    double closed = 0.0;
    double opened = 0.0;
    double closedvalue = 0.0;
    double openedvalue = 0.0;
    double closedcomm = 0.0;
    double openedcomm = 0.0;
    double value = 0.0;
    double comm = 0.0;
    double pnl = 0.0;
    double psize = 0.0;
    double pprice = 0.0;
    
    OrderExecutionBit() = default;
    OrderExecutionBit(std::chrono::system_clock::time_point dt_val,
                     double size_val,
                     double price_val,
                     double closed_val = 0.0,
                     double closedvalue_val = 0.0,
                     double closedcomm_val = 0.0,
                     double opened_val = 0.0,
                     double openedvalue_val = 0.0,
                     double openedcomm_val = 0.0,
                     double pnl_val = 0.0,
                     double psize_val = 0.0,
                     double pprice_val = 0.0);
    virtual ~OrderExecutionBit() = default;
};

// Order data - holds actual order information
class OrderData {
public:
    std::vector<OrderExecutionBit> exbits;
    std::chrono::system_clock::time_point dt;
    double size = 0.0;
    double price = 0.0;
    double pricelimit = 0.0;
    double trailamount = 0.0;
    double trailpercent = 0.0;
    double value = 0.0;
    double comm = 0.0;
    double pnl = 0.0;
    double margin = 0.0;
    double psize = 0.0;
    double pprice = 0.0;
    double remsize = 0.0; // remaining size
    
    OrderData() = default;
    virtual ~OrderData() = default;
    
    // Add execution bit
    void addbit(const OrderExecutionBit& bit);
};

// Main Order class
class Order {
public:
    // Type aliases for compatibility
    using Status = OrderStatus;
    
    // Order creation parameters
    std::shared_ptr<DataSeries> data;
    double size = 0.0;
    double price = 0.0;
    double pricelimit = 0.0;
    double trailamount = 0.0;
    double trailpercent = 0.0;
    OrderType type = OrderType::Market;
    bool transmit = true;
    std::shared_ptr<Order> parent = nullptr;
    bool simulated = false;
    
    // Order state
    OrderStatus status = OrderStatus::Created;
    std::shared_ptr<Strategy> owner = nullptr;
    
    // Execution information
    OrderData created;
    OrderData executed;
    
    // Reference information
    int ref = 0;
    std::string info;
    
    Order() = default;
    virtual ~Order() = default;
    
    // Order type checking
    bool isbuy() const;
    bool issell() const;
    
    // Order status checking
    bool alive() const;
    bool iscompleted() const;
    bool ispartial() const;
    bool isaccepted() const;
    bool issubmitted() const;
    
    // Remaining size
    double remaining() const;
    
    // String representation
    std::string to_string() const;
    
    // Status string utility
    static std::string status_string(OrderStatus status);
    
    // Clone order
    std::shared_ptr<Order> clone() const;
    
protected:
    static int next_ref_;
};

// Buy order specialization
class BuyOrder : public Order {
public:
    BuyOrder(std::shared_ptr<DataSeries> data,
             double size,
             double price = 0.0,
             OrderType type = OrderType::Market);
    virtual ~BuyOrder() = default;
};

// Sell order specialization  
class SellOrder : public Order {
public:
    SellOrder(std::shared_ptr<DataSeries> data,
              double size,
              double price = 0.0,
              OrderType type = OrderType::Market);
    virtual ~SellOrder() = default;
};

// Order factory functions
std::shared_ptr<Order> create_order(std::shared_ptr<DataSeries> data,
                                   double size,
                                   double price = 0.0,
                                   OrderType type = OrderType::Market,
                                   bool is_buy = true);

std::shared_ptr<BuyOrder> create_buy_order(std::shared_ptr<DataSeries> data,
                                          double size,
                                          double price = 0.0,
                                          OrderType type = OrderType::Market);

std::shared_ptr<SellOrder> create_sell_order(std::shared_ptr<DataSeries> data,
                                            double size,
                                            double price = 0.0,
                                            OrderType type = OrderType::Market);

} // namespace backtrader