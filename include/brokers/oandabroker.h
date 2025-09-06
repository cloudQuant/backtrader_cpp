#pragma once

#include "../broker.h"
#include "../order.h"
#include "../position.h"
#include "../comminfo.h"
#include "../stores/oandastore.h"
#include <map>
#include <queue>
#include <vector>
#include <memory>
#include <string>
#include <deque>

namespace backtrader {
namespace brokers {

/**
 * OandaCommInfo - Commission information for OANDA broker
 * 
 * Provides commission calculation specific to OANDA forex trading
 */
class OandaCommInfo : public CommInfoBase {
public:
    OandaCommInfo(double mult = 1.0, bool stocklike = false);
    virtual ~OandaCommInfo() = default;

    // Override commission calculations for forex
    double getvaluesize(double size, double price) override;
    double getoperationcost(double size, double price) override;
};

/**
 * OandaBroker - Broker implementation for OANDA forex trading
 * 
 * Maps orders/positions from OANDA to the internal backtrader API.
 * Supports forex trading through OANDA's REST API.
 * 
 * Params:
 *   - use_positions: Use existing positions when connecting (default: true)
 *   - commission: Commission info object (default: OandaCommInfo)
 */
class OandaBroker : public BrokerBase {
public:
    // Parameters structure
    struct Params {
        bool use_positions = true;
        std::shared_ptr<CommInfoBase> commission = std::make_shared<OandaCommInfo>();
    };

    OandaBroker(const Params& params = Params{},
                const stores::OandaStore::Params& store_params = {});
    
    virtual ~OandaBroker() = default;

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
    
    // Data lifecycle
    void data_started(std::shared_ptr<DataSeries> data);

    // OANDA-specific methods
    void update_positions();
    void update_orders();
    void process_order_event(const std::map<std::string, std::any>& event);
    void process_transaction_event(const std::map<std::string, std::any>& event);
    
    // Bracket order support
    std::shared_ptr<Order> create_bracket_order(
        std::shared_ptr<DataSeries> data,
        double size,
        double price,
        double stop_loss,
        double take_profit,
        Order::ExecType exectype = Order::ExecType::MARKET);

    // Properties
    const std::string& get_account_id() const;
    bool uses_positions() const { return params_.use_positions; }

private:
    // Store reference
    std::shared_ptr<stores::OandaStore> store_;
    
    // Parameters
    Params params_;
    
    // Account state
    double starting_cash_;
    double starting_value_;
    double cash_;
    double value_;
    
    // Order management
    std::map<std::string, std::shared_ptr<Order>> orders_;  // orders by OANDA order ID
    std::deque<std::shared_ptr<Order>> notifications_;      // orders to be notified
    
    // Pending operations
    std::map<std::shared_ptr<DataSeries>, std::vector<std::shared_ptr<Order>>> pending_orders_;
    
    // Bracket orders
    std::map<std::string, std::vector<std::string>> brackets_;  // parent -> [stop, take_profit]
    
    // Position tracking
    std::map<std::string, std::shared_ptr<Position>> positions_;  // instrument -> position
    
    // Internal order creation
    std::shared_ptr<Order> create_order(
        std::shared_ptr<DataSeries> data,
        Order::OrderType order_type,
        double size,
        double price,
        Order::ExecType exectype);
    
    // Order submission to OANDA
    bool submit_order(std::shared_ptr<Order> order);
    bool cancel_oanda_order(const std::string& order_id);
    
    // OANDA API helpers
    std::map<std::string, std::any> build_order_request(
        std::shared_ptr<Order> order) const;
    
    std::string get_oanda_order_type(Order::ExecType exectype) const;
    std::string get_oanda_side(Order::OrderType order_type) const;
    std::string get_instrument_name(std::shared_ptr<DataSeries> data) const;
    
    // Position management
    void load_existing_positions();
    void update_position_from_oanda(const std::map<std::string, std::any>& position_data);
    
    // Order status processing
    void process_order_fill(std::shared_ptr<Order> order, 
                           const std::map<std::string, std::any>& fill_data);
    void process_order_cancel(std::shared_ptr<Order> order);
    void process_order_reject(std::shared_ptr<Order> order, 
                             const std::string& reason);
    
    // Notification handling
    void notify_order(std::shared_ptr<Order> order);
    
    // Error handling
    void handle_oanda_error(const std::exception& e, const std::string& operation);
    
    // Utility methods
    double parse_oanda_price(const std::string& price_str) const;
    std::chrono::system_clock::time_point parse_oanda_time(const std::string& time_str) const;
    
    // Validation
    bool validate_order_size(double size, const std::string& instrument) const;
    bool validate_order_price(double price, const std::string& instrument) const;
};

} // namespace brokers
} // namespace backtrader