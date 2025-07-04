#pragma once

#include "../broker.h"
#include "../order.h"
#include "../position.h"
#include "../comminfo.h"
#include <map>
#include <vector>
#include <memory>
#include <queue>

namespace backtrader {

// Backtest Broker Simulator
class BackBroker : public BrokerBase {
public:
    struct Params {
        double cash = 10000.0;          // Starting cash
        double commission = 0.0;        // Commission per trade
        double margin = 0.0;            // Margin requirement
        double mult = 1.0;              // Contract multiplier
        bool slip_perc = false;         // Use percentage slippage
        bool slip_fixed = false;        // Use fixed slippage
        double slip_open = false;       // Slip on open
        bool slip_match = true;         // Match slippage behavior
        int slip_limit = true;          // Limit slippage
        bool slip_out = false;          // Slip on exit
        bool coo = false;               // Cheat-on-open
        bool coc = false;               // Cheat-on-close
        double shortcash = true;        // Allow short selling
        double fundstartval = 100.0;    // Fund start value
        std::string fundmode = "";      // Fund mode
    } params;
    
    BackBroker();
    virtual ~BackBroker() = default;
    
    // BrokerBase interface implementation
    void start() override;
    void stop() override;
    
    double get_cash() override;
    void set_cash(double cash) override;
    
    double get_value(const std::vector<std::shared_ptr<DataSeries>>& datas = {}) override;
    
    std::shared_ptr<Order> submit(std::shared_ptr<Order> order) override;
    std::shared_ptr<Order> cancel(std::shared_ptr<Order> order) override;
    
    void next() override;
    
    // Position management
    std::shared_ptr<Position> get_position(std::shared_ptr<DataSeries> data);
    std::map<std::shared_ptr<DataSeries>, std::shared_ptr<Position>> get_positions();
    
    // Commission info
    void add_commission_info(std::shared_ptr<CommInfoBase> comminfo, 
                           const std::string& name = "");
    void set_commission(double commission = 0.0, double margin = 0.0, 
                       double mult = 1.0, const std::string& name = "");
    
    // Order execution
    void set_slippage_perc(double perc, bool slip_open = true, bool slip_limit = true,
                          bool slip_match = true, bool slip_out = false);
    void set_slippage_fixed(double fixed, bool slip_open = true, bool slip_limit = true,
                           bool slip_match = true, bool slip_out = false);
    
    // Getters/Setters
    void set_coo(bool coo) { params.coo = coo; }
    void set_coc(bool coc) { params.coc = coc; }
    void set_shortcash(bool shortcash) { params.shortcash = shortcash; }
    
private:
    double cash_;
    double value_;
    
    // Orders management
    std::vector<std::shared_ptr<Order>> pending_orders_;
    std::queue<std::shared_ptr<Order>> orders_queue_;
    
    // Positions
    std::map<std::shared_ptr<DataSeries>, std::shared_ptr<Position>> positions_;
    
    // Commission info
    std::map<std::string, std::shared_ptr<CommInfoBase>> commission_info_;
    std::shared_ptr<CommInfoBase> default_commission_info_;
    
    // Helper methods
    void process_orders();
    bool check_order_execution(std::shared_ptr<Order> order, std::shared_ptr<DataSeries> data);
    void execute_order(std::shared_ptr<Order> order, double price, double size);
    double calculate_commission(std::shared_ptr<Order> order, double price, double size);
    void update_position(std::shared_ptr<DataSeries> data, double size, double price);
    void update_cash_and_value();
    
    // Order execution helpers
    bool check_market_order(std::shared_ptr<Order> order, std::shared_ptr<DataSeries> data);
    bool check_limit_order(std::shared_ptr<Order> order, std::shared_ptr<DataSeries> data);
    bool check_stop_order(std::shared_ptr<Order> order, std::shared_ptr<DataSeries> data);
    bool check_stop_limit_order(std::shared_ptr<Order> order, std::shared_ptr<DataSeries> data);
    
    double apply_slippage(double price, std::shared_ptr<Order> order);
    bool validate_order_cash(std::shared_ptr<Order> order, double price);
};

// Alias
using BrokerBack = BackBroker;

} // namespace backtrader