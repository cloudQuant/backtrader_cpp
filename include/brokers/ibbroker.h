#pragma once

#include "../broker.h"
#include "../order.h"
#include "../position.h"
#include <map>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <queue>

namespace backtrader {

// IBOrderState - wraps IB order state information
struct IBOrderState {
    std::string status;
    double init_margin = 0.0;
    double maint_margin = 0.0;
    double equity_with_loan = 0.0;
    double commission = 0.0;
    double min_commission = 0.0;
    double max_commission = 0.0;
    std::string commission_currency;
    std::string warning_text;
    
    std::string to_string() const;
};

// IBOrder - extends Order with IB-specific functionality
class IBOrder : public Order {
public:
    // IB-specific parameters
    struct IBParams {
        std::string order_type = "MKT";      // IB order type
        double limit_price = 0.0;           // Limit price
        double aux_price = 0.0;              // Auxiliary price (for stop orders)
        std::string time_in_force = "DAY";   // Time in force
        bool outside_rth = false;            // Allow outside regular trading hours
        bool hidden = false;                 // Hidden order
        int min_qty = 0;                     // Minimum quantity
        double percent_offset = 0.0;         // Percent offset for relative orders
        std::string rule80a = "";            // Rule 80A
        bool all_or_none = false;            // All or none order
        int block_order = 0;                 // Block order flag
        bool sweep_to_fill = false;          // Sweep to fill
        int display_size = 0;                // Display size for iceberg orders
        std::string good_after_time = "";    // Good after time
        std::string good_till_date = "";     // Good till date
        bool override_percentage_constraints = false;
    } ib_params;
    
    // IB order ID
    int ib_order_id = 0;
    
    // Order state
    IBOrderState order_state;
    
    IBOrder();
    virtual ~IBOrder() = default;
    
    // Order interface
    std::string to_string() const override;
    
    // IB-specific methods
    void apply_ib_parameters(const std::map<std::string, std::string>& kwargs);
    void set_ib_order_type();
};

// IBBroker - Interactive Brokers broker implementation
class IBBroker : public BrokerBase {
public:
    struct Params {
        std::string host = "127.0.0.1";      // TWS/Gateway host
        int port = 7497;                     // TWS/Gateway port (7497=TWS, 4002=Gateway)
        int client_id = 1;                   // Client ID
        bool use_rth = true;                 // Use regular trading hours only
        int timeout = 3.0;                   // Connection timeout
        bool reconnect = true;               // Auto-reconnect on disconnect
        int reconnect_timeout = 5;           // Reconnection timeout
        double cash = 10000.0;               // Initial cash (for paper trading)
        bool fund_mode = false;              // Fund mode for performance tracking
        
        // Order management
        bool fill_model = true;              // Use fill model for simulated fills
        double commission_rate = 0.005;      // Default commission rate
        
        // Connection settings
        bool paper_trading = true;           // Paper trading mode
        std::string account = "";            // Specific account (empty = default)
        
        // Threading settings
        int max_worker_threads = 4;         // Max worker threads
        bool use_threading = true;           // Enable threading for order processing
    } params;
    
    IBBroker();
    virtual ~IBBroker();
    
    // Broker interface
    void start() override;
    void stop() override;
    double get_cash() const override;
    double get_value() const override;
    void set_fundmode(bool fundmode) override;
    bool get_fundmode() const override;
    
    // Order management
    Order* buy(Strategy* strategy, Data* data, double size, double price = 0.0,
               Order::Type order_type = Order::Type::Market,
               const std::map<std::string, std::string>& kwargs = {}) override;
    
    Order* sell(Strategy* strategy, Data* data, double size, double price = 0.0,
                Order::Type order_type = Order::Type::Market,
                const std::map<std::string, std::string>& kwargs = {}) override;
    
    bool cancel(Order* order) override;
    
    // Position management
    Position get_position(Data* data) const override;
    std::map<Data*, Position> get_positions() const override;
    
    // Account information
    std::map<std::string, double> get_account_values() const;
    
    // Connection management
    bool connect();
    void disconnect();
    bool is_connected() const { return connected_; }
    
    // Order notifications (called by IB callbacks)
    void notify_order_status(int order_id, const std::string& status,
                           int filled, int remaining, double avg_fill_price,
                           int perm_id, int parent_id, double last_fill_price,
                           int client_id, const std::string& why_held);
    
    void notify_execution(int req_id, const std::string& symbol,
                         const std::string& side, int quantity,
                         double price, int perm_id, int client_id,
                         const std::string& exec_id, const std::string& time);
    
    void notify_commission_report(const std::string& exec_id, double commission,
                                const std::string& currency, double realized_pnl,
                                double yield, int yield_redemption_date);
    
private:
    // Connection state
    bool connected_;
    bool connecting_;
    std::thread connection_thread_;
    std::mutex connection_mutex_;
    std::condition_variable connection_cv_;
    
    // Order management
    std::map<int, std::shared_ptr<IBOrder>> orders_;  // IB order ID -> Order
    std::map<Order*, int> order_to_ib_id_;            // Order -> IB order ID
    int next_order_id_;
    std::mutex orders_mutex_;
    
    // Position tracking
    std::map<std::string, Position> positions_;       // Symbol -> Position
    std::mutex positions_mutex_;
    
    // Account data
    std::map<std::string, double> account_values_;
    double cash_;
    double portfolio_value_;
    std::mutex account_mutex_;
    
    // Threading
    std::vector<std::thread> worker_threads_;
    std::queue<std::function<void()>> task_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    bool shutdown_;
    
    // Helper methods
    int get_next_order_id();
    std::shared_ptr<IBOrder> create_ib_order(Strategy* strategy, Data* data,
                                            double size, double price,
                                            Order::Type order_type, bool is_buy,
                                            const std::map<std::string, std::string>& kwargs);
    
    void submit_order(std::shared_ptr<IBOrder> order);
    void process_order_update(int order_id, const std::string& status);
    void process_execution(const std::string& symbol, const std::string& side,
                          int quantity, double price);
    
    // Threading helpers
    void worker_thread_main();
    void enqueue_task(std::function<void()> task);
    
    // Connection helpers
    void connection_thread_main();
    void handle_disconnect();
    void reconnect();
    
    // IB API wrapper methods (would interface with actual IB API)
    bool ib_connect(const std::string& host, int port, int client_id);
    void ib_disconnect();
    void ib_place_order(int order_id, const std::string& symbol, const IBOrder& order);
    void ib_cancel_order(int order_id);
    void ib_request_account_updates(bool subscribe);
    void ib_request_positions();
};

} // namespace backtrader