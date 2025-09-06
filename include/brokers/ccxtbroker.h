#pragma once

#include "../broker.h"
#include "../order.h"
#include "../position.h"
#include "../stores/ccxtstore.h"
#include <map>
#include <queue>
#include <vector>
#include <memory>
#include <string>

namespace backtrader {
namespace brokers {

/**
 * CCXTOrder - CCXT-specific order implementation
 * 
 * Extends the base Order class with CCXT-specific functionality
 */
class CCXTOrder : public Order {
public:
    CCXTOrder(std::shared_ptr<void> owner, 
              std::shared_ptr<DataSeries> data,
              ExecType exectype,
              OrderType side,
              double amount,
              double price,
              const std::map<std::string, std::any>& ccxt_order);
    
    virtual ~CCXTOrder() = default;

    // CCXT-specific methods
    const std::map<std::string, std::any>& get_ccxt_order() const { return ccxt_order_; }
    void add_fill(const std::map<std::string, std::any>& fill);
    const std::vector<std::map<std::string, std::any>>& get_fills() const { return executed_fills_; }

private:
    std::map<std::string, std::any> ccxt_order_;
    std::vector<std::map<std::string, std::any>> executed_fills_;
};

/**
 * CCXTBroker - Broker implementation for CCXT cryptocurrency trading
 * 
 * Maps orders/positions from CCXT to the internal backtrader API.
 * Supports multiple exchanges through the CCXT library.
 */
class CCXTBroker : public BrokerBase {
public:
    // Order type mappings
    using OrderTypeMap = std::map<Order::OrderType, std::string>;
    using StatusMappings = std::map<std::string, std::map<std::string, std::string>>;
    
    // Broker mapping structure
    struct BrokerMapping {
        OrderTypeMap order_types;
        StatusMappings mappings;
    };

    CCXTBroker(const BrokerMapping* broker_mapping = nullptr, 
               bool debug = false,
               const stores::CCXTStore::Params& store_params = {});
    
    virtual ~CCXTBroker() = default;

    // BrokerBase interface implementation
    std::shared_ptr<Order> buy(std::shared_ptr<DataSeries> data,
                              double size,
                              double price = 0.0,
                              Order::ExecType exectype = Order::ExecType::MARKET,
                              bool valid = true) override;
    
    std::shared_ptr<Order> sell(std::shared_ptr<DataSeries> data,
                               double size,
                               double price = 0.0,
                               Order::ExecType exectype = Order::ExecType::MARKET,
                               bool valid = true) override;
    
    void cancel(std::shared_ptr<Order> order) override;
    
    double get_cash() override;
    double get_value(const std::vector<std::shared_ptr<DataSeries>>& datas = {}) override;
    
    std::shared_ptr<Position> get_position(std::shared_ptr<DataSeries> data, 
                                          bool clone = true) override;
    
    void next() override;
    void start() override;
    void stop() override;

    // CCXT-specific methods
    std::pair<double, double> get_balance();
    std::map<std::string, double> get_wallet_balance(
        const std::vector<std::string>& currency_list,
        const std::map<std::string, std::any>& params = {});
    
    // Private endpoint access
    std::map<std::string, std::any> private_end_point(
        const std::string& path,
        const std::map<std::string, std::any>& params = {});
    
    // Order management
    void update_order_status(std::shared_ptr<CCXTOrder> order);
    void process_order_fills(std::shared_ptr<CCXTOrder> order);
    
    // Configuration
    void set_order_types(const OrderTypeMap& order_types);
    void set_mappings(const StatusMappings& mappings);

    // Properties
    const std::string& get_currency() const { return currency_; }
    bool is_debug() const { return debug_; }

private:
    // Store reference
    std::shared_ptr<stores::CCXTStore> store_;
    
    // Configuration
    OrderTypeMap order_types_;
    StatusMappings mappings_;
    std::string currency_;
    bool debug_;
    int indent_ = 4;  // For pretty printing
    
    // Account state
    double starting_cash_;
    double starting_value_;
    double cash_;
    double value_;
    
    // Position tracking
    std::map<std::shared_ptr<DataSeries>, std::shared_ptr<Position>> positions_;
    
    // Order management
    std::vector<std::shared_ptr<CCXTOrder>> open_orders_;
    std::queue<std::shared_ptr<Order>> notifications_;
    
    // Timing
    std::chrono::system_clock::time_point last_op_time_;
    
    // Default mappings
    static OrderTypeMap create_default_order_types();
    static StatusMappings create_default_mappings();
    
    // Internal order processing
    std::shared_ptr<CCXTOrder> create_order(
        std::shared_ptr<DataSeries> data,
        Order::OrderType order_type,
        double size,
        double price,
        Order::ExecType exectype);
    
    std::string get_ccxt_order_type(Order::ExecType exectype) const;
    std::string get_ccxt_side(Order::OrderType order_type) const;
    
    // Order status checking
    bool is_order_closed(const std::map<std::string, std::any>& order) const;
    bool is_order_canceled(const std::map<std::string, std::any>& order) const;
    
    // Notification handling
    void notify_order(std::shared_ptr<Order> order);
    
    // Error handling
    void handle_ccxt_error(const std::exception& e, const std::string& operation);
    
    // Debugging
    void debug_print(const std::string& message) const;
    void debug_print_order(const std::map<std::string, std::any>& order) const;
};

} // namespace brokers
} // namespace backtrader