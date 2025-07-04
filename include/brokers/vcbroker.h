#pragma once

#include "../broker.h"
#include "../order.h"
#include "../position.h"
#include "../comminfo.h"
#include "../stores/vcstore.h"
#include <map>
#include <queue>
#include <vector>
#include <memory>
#include <string>
#include <deque>

namespace backtrader {
namespace brokers {

/**
 * VCCommInfo - Commission information for Visual Chart broker
 * 
 * Provides commission calculation specific to Visual Chart trading
 */
class VCCommInfo : public CommInfoBase {
public:
    VCCommInfo(double mult = 1.0, bool stocklike = true);
    virtual ~VCCommInfo() = default;

    // Override commission calculations
    double getvaluesize(double size, double price) override;
    double getoperationcost(double size, double price) override;
    double getcommission(double size, double price) override;
};

/**
 * VCOrder - Visual Chart specific order implementation
 */
class VCOrder : public Order {
public:
    VCOrder(std::shared_ptr<DataSeries> data,
            Order::OrderType order_type,
            double size,
            double price,
            Order::ExecType exectype);
    
    virtual ~VCOrder() = default;

    // VC-specific fields
    std::string vc_order_id;
    std::string symbol;
    std::string account;
    
    // VC order status
    enum class VCStatus {
        PENDING,
        ACCEPTED,
        PARTIAL_FILL,
        FILLED,
        CANCELED,
        REJECTED,
        EXPIRED
    };
    
    VCStatus vc_status = VCStatus::PENDING;
    
    void set_vc_status(const std::string& status_str);
    std::string get_vc_status_string() const;
    
    // Execution details
    double filled_quantity = 0.0;
    double remaining_quantity = 0.0;
    double average_price = 0.0;
    std::vector<std::map<std::string, std::any>> fills;
};

/**
 * VCBroker - Broker implementation for Visual Chart trading
 * 
 * Maps orders/positions from Visual Chart to the internal backtrader API.
 * Supports equity and derivatives trading through Visual Chart platform.
 */
class VCBroker : public BrokerBase {
public:
    // Parameters structure
    struct Params {
        bool use_positions = true;
        std::shared_ptr<CommInfoBase> commission = std::make_shared<VCCommInfo>();
        std::string default_account;
        bool real_time_updates = true;
        int order_timeout = 30;  // seconds
    };

    VCBroker(const Params& params = Params{},
             const stores::VCStore::Params& store_params = {});
    
    virtual ~VCBroker() = default;

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

    // Visual Chart specific methods
    void update_account_info();
    void update_positions();
    void update_orders();
    
    // Order management
    void process_order_update(const std::map<std::string, std::any>& order_data);
    void process_execution_report(const std::map<std::string, std::any>& execution_data);
    void process_order_rejection(const std::map<std::string, std::any>& rejection_data);
    
    // Position management
    std::vector<std::map<std::string, std::any>> get_all_positions();
    std::map<std::string, std::any> get_account_summary();
    
    // Order modification
    bool modify_order(std::shared_ptr<VCOrder> order, 
                     double new_price, 
                     double new_quantity = 0.0);

    // Properties
    const std::string& get_default_account() const { return params_.default_account; }
    bool uses_positions() const { return params_.use_positions; }
    bool uses_real_time_updates() const { return params_.real_time_updates; }

private:
    // Store reference
    std::shared_ptr<stores::VCStore> store_;
    
    // Parameters
    Params params_;
    
    // Account state
    double starting_cash_;
    double starting_value_;
    double cash_;
    double value_;
    double buying_power_;
    
    // Order management
    std::map<std::string, std::shared_ptr<VCOrder>> orders_;  // orders by VC order ID
    std::deque<std::shared_ptr<Order>> notifications_;        // orders to be notified
    
    // Position tracking
    std::map<std::string, std::shared_ptr<Position>> positions_;  // symbol -> position
    
    // Account information
    std::map<std::string, std::any> account_info_;
    
    // Order creation
    std::shared_ptr<VCOrder> create_vc_order(
        std::shared_ptr<DataSeries> data,
        Order::OrderType order_type,
        double size,
        double price,
        Order::ExecType exectype);
    
    // Order submission to Visual Chart
    bool submit_order(std::shared_ptr<VCOrder> order);
    bool cancel_vc_order(std::shared_ptr<VCOrder> order);
    bool modify_vc_order(std::shared_ptr<VCOrder> order, 
                         double new_price, 
                         double new_quantity);
    
    // Visual Chart API helpers
    std::map<std::string, std::any> build_order_message(
        std::shared_ptr<VCOrder> order) const;
    
    std::string get_vc_order_type(Order::ExecType exectype) const;
    std::string get_vc_side(Order::OrderType order_type) const;
    std::string get_vc_time_in_force(Order::ExecType exectype) const;
    std::string get_symbol_name(std::shared_ptr<DataSeries> data) const;
    
    // Position management
    void load_existing_positions();
    void update_position_from_vc(const std::map<std::string, std::any>& position_data);
    
    // Account queries
    void query_account_info();
    void query_positions();
    void query_orders();
    void query_executions();
    
    // Order status processing
    void process_order_acknowledgment(std::shared_ptr<VCOrder> order,
                                    const std::map<std::string, std::any>& ack_data);
    void process_partial_fill(std::shared_ptr<VCOrder> order,
                             const std::map<std::string, std::any>& fill_data);
    void process_order_fill(std::shared_ptr<VCOrder> order,
                           const std::map<std::string, std::any>& fill_data);
    void process_order_cancellation(std::shared_ptr<VCOrder> order);
    
    // Error handling
    void handle_vc_error(const std::map<std::string, std::any>& error_info,
                        const std::string& operation);
    
    // Notification handling
    void notify_order(std::shared_ptr<Order> order);
    
    // Utility methods
    bool validate_order(std::shared_ptr<VCOrder> order) const;
    bool check_buying_power(std::shared_ptr<VCOrder> order) const;
    double calculate_order_value(std::shared_ptr<VCOrder> order) const;
    
    // Real-time updates
    void enable_real_time_updates();
    void disable_real_time_updates();
    void process_real_time_update(const std::map<std::string, std::any>& update);
    
    // Symbol validation
    bool is_valid_symbol(const std::string& symbol) const;
    std::map<std::string, std::any> get_symbol_info(const std::string& symbol) const;
    
    // Order timeout handling
    void start_order_timeout_timer(std::shared_ptr<VCOrder> order);
    void handle_order_timeout(std::shared_ptr<VCOrder> order);
};

} // namespace brokers
} // namespace backtrader