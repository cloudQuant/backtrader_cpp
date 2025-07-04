#pragma once

#include "../broker.h"
#include "../order.h"
#include "../position.h"
#include "../comminfo.h"
#include "../stores/ctpstore.h"
#include <map>
#include <queue>
#include <vector>
#include <memory>
#include <string>
#include <deque>
#include <atomic>

namespace backtrader {
namespace brokers {

/**
 * CTPCommInfo - Commission information for CTP broker
 * 
 * Provides commission calculation specific to Chinese futures trading
 */
class CTPCommInfo : public CommInfoBase {
public:
    CTPCommInfo(double mult = 1.0, bool stocklike = false);
    virtual ~CTPCommInfo() = default;

    // Override commission calculations for futures
    double getvaluesize(double size, double price) override;
    double getoperationcost(double size, double price) override;
    double getcommission(double size, double price) override;
};

/**
 * CTPOrder - CTP-specific order implementation
 */
class CTPOrder : public Order {
public:
    CTPOrder(std::shared_ptr<DataSeries> data,
             Order::OrderType order_type,
             double size,
             double price,
             Order::ExecType exectype);
    
    virtual ~CTPOrder() = default;

    // CTP-specific fields
    std::string order_ref;
    std::string instrument_id;
    std::string investor_id;
    std::string broker_id;
    int front_id = 0;
    int session_id = 0;
    
    // CTP order status
    enum class CTPStatus {
        UNKNOWN,
        ALL_TRADED,
        PART_TRADED_QUEUEING,
        PART_TRADED_NOT_QUEUEING,
        NO_TRADE_QUEUEING,
        NO_TRADE_NOT_QUEUEING,
        CANCELED,
        ORDER_REJECTED
    };
    
    CTPStatus ctp_status = CTPStatus::UNKNOWN;
    
    void set_ctp_status(char status_char);
    std::string get_ctp_status_string() const;
};

/**
 * CTPBroker - Broker implementation for CTP futures trading
 * 
 * Maps orders/positions from CTP to the internal backtrader API.
 * Supports Chinese futures trading through CTP API.
 */
class CTPBroker : public BrokerBase {
public:
    // Parameters structure
    struct Params {
        bool use_positions = true;
        std::shared_ptr<CommInfoBase> commission = std::make_shared<CTPCommInfo>();
        double margin_ratio = 0.1;  // Default margin ratio
        bool auto_confirm_settlement = true;
    };

    CTPBroker(const Params& params = Params{},
              const stores::CTPStore::Params& store_params = {});
    
    virtual ~CTPBroker() = default;

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

    // CTP-specific methods
    void update_account();
    void update_positions();
    void update_orders();
    
    // Position management
    std::vector<std::map<std::string, std::any>> get_all_positions();
    std::map<std::string, std::any> get_position_detail(const std::string& instrument_id);
    
    // Settlement confirmation
    bool confirm_settlement();
    
    // Order management
    void process_order_return(const std::map<std::string, std::any>& order_data);
    void process_trade_return(const std::map<std::string, std::any>& trade_data);
    void process_order_error(const std::map<std::string, std::any>& error_data);

    // Properties
    const std::string& get_investor_id() const;
    const std::string& get_broker_id() const;
    const std::string& get_trading_day() const;
    double get_margin_ratio() const { return params_.margin_ratio; }

private:
    // Store reference
    std::shared_ptr<stores::CTPStore> store_;
    
    // Parameters
    Params params_;
    
    // Account state
    double starting_cash_;
    double starting_value_;
    double cash_;
    double value_;
    double available_cash_;
    double frozen_cash_;
    double margin_;
    
    // Order management
    std::map<std::string, std::shared_ptr<CTPOrder>> orders_;  // orders by order_ref
    std::deque<std::shared_ptr<Order>> notifications_;         // orders to be notified
    
    // Position tracking
    std::map<std::string, std::shared_ptr<Position>> positions_;  // instrument -> position
    
    // Request management
    std::atomic<int> request_id_{1};
    
    // Order creation
    std::shared_ptr<CTPOrder> create_ctp_order(
        std::shared_ptr<DataSeries> data,
        Order::OrderType order_type,
        double size,
        double price,
        Order::ExecType exectype);
    
    // Order submission to CTP
    bool submit_order(std::shared_ptr<CTPOrder> order);
    bool cancel_ctp_order(std::shared_ptr<CTPOrder> order);
    
    // CTP API helpers
    std::map<std::string, std::any> build_order_field(
        std::shared_ptr<CTPOrder> order) const;
    
    char get_ctp_direction(Order::OrderType order_type) const;
    char get_ctp_offset_flag(Order::OrderType order_type, bool has_position) const;
    char get_ctp_order_price_type(Order::ExecType exectype) const;
    char get_ctp_time_condition() const;
    char get_ctp_volume_condition() const;
    
    std::string get_instrument_id(std::shared_ptr<DataSeries> data) const;
    
    // Position management
    void load_existing_positions();
    void update_position_from_ctp(const std::map<std::string, std::any>& position_data);
    void calculate_position_pnl();
    
    // Account queries
    void query_account();
    void query_positions();
    void query_orders();
    void query_trades();
    
    // Order status processing
    void process_order_insert(std::shared_ptr<CTPOrder> order, 
                             const std::map<std::string, std::any>& response);
    void process_order_action(std::shared_ptr<CTPOrder> order,
                             const std::map<std::string, std::any>& response);
    void process_trade_execution(std::shared_ptr<CTPOrder> order,
                                const std::map<std::string, std::any>& trade_data);
    
    // Error handling
    void handle_ctp_error(const std::map<std::string, std::any>& error_info, 
                         const std::string& operation);
    
    // Notification handling
    void notify_order(std::shared_ptr<Order> order);
    
    // Utility methods
    double calculate_margin_requirement(const std::string& instrument_id, 
                                       double size, double price) const;
    bool validate_order(std::shared_ptr<CTPOrder> order) const;
    std::string generate_order_ref();
    
    // Settlement and trading day
    bool is_settlement_confirmed();
    void handle_trading_day_change();
    
    // Risk management
    bool check_risk_limits(std::shared_ptr<CTPOrder> order) const;
    double get_available_margin() const;
};

} // namespace brokers
} // namespace backtrader