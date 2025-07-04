#include "brokers/ibbroker.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>

namespace backtrader {

// IBOrderState implementation
std::string IBOrderState::to_string() const {
    std::stringstream ss;
    ss << "--- ORDERSTATE BEGIN\n";
    ss << "Status: " << status << "\n";
    ss << "InitMargin: " << init_margin << "\n";
    ss << "MaintMargin: " << maint_margin << "\n";
    ss << "EquityWithLoan: " << equity_with_loan << "\n";
    ss << "Commission: " << commission << "\n";
    ss << "MinCommission: " << min_commission << "\n";
    ss << "MaxCommission: " << max_commission << "\n";
    ss << "CommissionCurrency: " << commission_currency << "\n";
    ss << "WarningText: " << warning_text << "\n";
    ss << "--- ORDERSTATE END";
    return ss.str();
}

// IBOrder implementation
IBOrder::IBOrder() : Order() {
    // Initialize IB-specific parameters
    ib_order_id = 0;
}

std::string IBOrder::to_string() const {
    std::stringstream ss;
    ss << Order::to_string() << "\n";
    ss << "Ref: " << ref << "\n";
    ss << "OrderId: " << ib_order_id << "\n";
    ss << "OrderType: " << ib_params.order_type << "\n";
    ss << "LimitPrice: " << ib_params.limit_price << "\n";
    ss << "AuxPrice: " << ib_params.aux_price << "\n";
    ss << "TimeInForce: " << ib_params.time_in_force;
    return ss.str();
}

void IBOrder::apply_ib_parameters(const std::map<std::string, std::string>& kwargs) {
    // Apply IB-specific parameters from kwargs
    for (const auto& pair : kwargs) {
        const std::string& key = pair.first;
        const std::string& value = pair.second;
        
        if (key == "orderType") {
            ib_params.order_type = value;
        } else if (key == "lmtPrice") {
            ib_params.limit_price = std::stod(value);
        } else if (key == "auxPrice") {
            ib_params.aux_price = std::stod(value);
        } else if (key == "tif") {
            ib_params.time_in_force = value;
        } else if (key == "outsideRth") {
            ib_params.outside_rth = (value == "1" || value == "true");
        } else if (key == "hidden") {
            ib_params.hidden = (value == "1" || value == "true");
        } else if (key == "minQty") {
            ib_params.min_qty = std::stoi(value);
        } else if (key == "percentOffset") {
            ib_params.percent_offset = std::stod(value);
        } else if (key == "allOrNone") {
            ib_params.all_or_none = (value == "1" || value == "true");
        } else if (key == "sweepToFill") {
            ib_params.sweep_to_fill = (value == "1" || value == "true");
        } else if (key == "displaySize") {
            ib_params.display_size = std::stoi(value);
        }
    }
}

void IBOrder::set_ib_order_type() {
    // Set IB order type based on backtrader order type
    switch (order_type) {
        case Type::Market:
            ib_params.order_type = "MKT";
            break;
        case Type::Limit:
            ib_params.order_type = "LMT";
            ib_params.limit_price = price;
            break;
        case Type::Stop:
            ib_params.order_type = "STP";
            ib_params.aux_price = price;
            break;
        case Type::StopLimit:
            ib_params.order_type = "STP LMT";
            ib_params.limit_price = pricelimit;
            ib_params.aux_price = price;
            break;
        case Type::Close:
            ib_params.order_type = "MOC";  // Market on Close
            break;
        default:
            ib_params.order_type = "MKT";
            break;
    }
}

// IBBroker implementation
IBBroker::IBBroker() : BrokerBase(), connected_(false), connecting_(false),
                       next_order_id_(1), cash_(0.0), portfolio_value_(0.0), shutdown_(false) {
    // Initialize with default parameters
}

IBBroker::~IBBroker() {
    stop();
}

void IBBroker::start() {
    BrokerBase::start();
    
    // Initialize account values
    cash_ = params.cash;
    portfolio_value_ = params.cash;
    shutdown_ = false;
    
    // Start worker threads if threading is enabled
    if (params.use_threading) {
        for (int i = 0; i < params.max_worker_threads; ++i) {
            worker_threads_.emplace_back(&IBBroker::worker_thread_main, this);
        }
    }
    
    // Connect to IB
    if (!connect()) {
        throw std::runtime_error("Failed to connect to Interactive Brokers");
    }
}

void IBBroker::stop() {
    // Shutdown worker threads
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        shutdown_ = true;
    }
    queue_cv_.notify_all();
    
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
    
    // Disconnect from IB
    disconnect();
    
    BrokerBase::stop();
}

double IBBroker::get_cash() const {
    std::lock_guard<std::mutex> lock(account_mutex_);
    return cash_;
}

double IBBroker::get_value() const {
    std::lock_guard<std::mutex> lock(account_mutex_);
    return portfolio_value_;
}

void IBBroker::set_fundmode(bool fundmode) {
    params.fund_mode = fundmode;
}

bool IBBroker::get_fundmode() const {
    return params.fund_mode;
}

Order* IBBroker::buy(Strategy* strategy, Data* data, double size, double price,
                     Order::Type order_type, const std::map<std::string, std::string>& kwargs) {
    auto ib_order = create_ib_order(strategy, data, size, price, order_type, true, kwargs);
    if (!ib_order) {
        return nullptr;
    }
    
    submit_order(ib_order);
    return ib_order.get();
}

Order* IBBroker::sell(Strategy* strategy, Data* data, double size, double price,
                      Order::Type order_type, const std::map<std::string, std::string>& kwargs) {
    auto ib_order = create_ib_order(strategy, data, size, price, order_type, false, kwargs);
    if (!ib_order) {
        return nullptr;
    }
    
    submit_order(ib_order);
    return ib_order.get();
}

bool IBBroker::cancel(Order* order) {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    
    auto it = order_to_ib_id_.find(order);
    if (it == order_to_ib_id_.end()) {
        return false;
    }
    
    int ib_order_id = it->second;
    
    // Cancel order through IB API
    ib_cancel_order(ib_order_id);
    
    // Update order status
    order->status = Order::Status::Canceled;
    
    return true;
}

Position IBBroker::get_position(Data* data) const {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    
    auto it = positions_.find(data->get_symbol());
    if (it != positions_.end()) {
        return it->second;
    }
    
    return Position(); // Empty position
}

std::map<Data*, Position> IBBroker::get_positions() const {
    std::map<Data*, Position> result;
    
    std::lock_guard<std::mutex> lock(positions_mutex_);
    
    // Convert symbol-based positions to data-based positions
    // This would require a symbol->data mapping in practice
    for (const auto& pair : positions_) {
        // Simplified - would need proper symbol to data mapping
        result[nullptr] = pair.second;
    }
    
    return result;
}

std::map<std::string, double> IBBroker::get_account_values() const {
    std::lock_guard<std::mutex> lock(account_mutex_);
    return account_values_;
}

bool IBBroker::connect() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    if (connected_ || connecting_) {
        return connected_;
    }
    
    connecting_ = true;
    
    // Start connection in separate thread
    connection_thread_ = std::thread(&IBBroker::connection_thread_main, this);
    
    // Wait for connection with timeout
    auto timeout = std::chrono::seconds(params.timeout);
    auto result = connection_cv_.wait_for(connection_mutex_, timeout, [this] { return connected_ || !connecting_; });
    
    return connected_;
}

void IBBroker::disconnect() {
    std::lock_guard<std::mutex> lock(connection_mutex_);
    
    if (!connected_) {
        return;
    }
    
    ib_disconnect();
    connected_ = false;
    
    if (connection_thread_.joinable()) {
        connection_thread_.join();
    }
}

void IBBroker::notify_order_status(int order_id, const std::string& status,
                                  int filled, int remaining, double avg_fill_price,
                                  int perm_id, int parent_id, double last_fill_price,
                                  int client_id, const std::string& why_held) {
    std::lock_guard<std::mutex> lock(orders_mutex_);
    
    auto it = orders_.find(order_id);
    if (it == orders_.end()) {
        return;
    }
    
    auto& order = it->second;
    
    // Update order status
    if (status == "Filled") {
        order->status = Order::Status::Completed;
        order->executed_size = filled;
        order->executed_price = avg_fill_price;
    } else if (status == "PartiallyFilled") {
        order->status = Order::Status::Partial;
        order->executed_size = filled;
        order->executed_price = avg_fill_price;
    } else if (status == "Cancelled") {
        order->status = Order::Status::Canceled;
    } else if (status == "Submitted") {
        order->status = Order::Status::Submitted;
    }
    
    // Enqueue order processing task
    if (params.use_threading) {
        enqueue_task([this, order_id, status]() {
            process_order_update(order_id, status);
        });
    } else {
        process_order_update(order_id, status);
    }
}

void IBBroker::notify_execution(int req_id, const std::string& symbol,
                               const std::string& side, int quantity,
                               double price, int perm_id, int client_id,
                               const std::string& exec_id, const std::string& time) {
    // Process execution
    if (params.use_threading) {
        enqueue_task([this, symbol, side, quantity, price]() {
            process_execution(symbol, side, quantity, price);
        });
    } else {
        process_execution(symbol, side, quantity, price);
    }
}

void IBBroker::notify_commission_report(const std::string& exec_id, double commission,
                                       const std::string& currency, double realized_pnl,
                                       double yield, int yield_redemption_date) {
    // Update commission and PnL information
    // Implementation would update order commission and account values
}

int IBBroker::get_next_order_id() {
    return next_order_id_++;
}

std::shared_ptr<IBOrder> IBBroker::create_ib_order(Strategy* strategy, Data* data,
                                                   double size, double price,
                                                   Order::Type order_type, bool is_buy,
                                                   const std::map<std::string, std::string>& kwargs) {
    auto order = std::make_shared<IBOrder>();
    
    // Set basic order properties
    order->strategy = strategy;
    order->data = data;
    order->size = is_buy ? size : -size;
    order->price = price;
    order->order_type = order_type;
    order->status = Order::Status::Created;
    order->ib_order_id = get_next_order_id();
    
    // Apply IB-specific parameters
    order->apply_ib_parameters(kwargs);
    order->set_ib_order_type();
    
    // Store order mappings
    {
        std::lock_guard<std::mutex> lock(orders_mutex_);
        orders_[order->ib_order_id] = order;
        order_to_ib_id_[order.get()] = order->ib_order_id;
    }
    
    return order;
}

void IBBroker::submit_order(std::shared_ptr<IBOrder> order) {
    // Submit order to IB
    std::string symbol = order->data->get_symbol();
    ib_place_order(order->ib_order_id, symbol, *order);
    
    // Update order status
    order->status = Order::Status::Submitted;
}

void IBBroker::process_order_update(int order_id, const std::string& status) {
    // Process order status updates
    // Implementation would handle order lifecycle and notifications
}

void IBBroker::process_execution(const std::string& symbol, const std::string& side,
                                int quantity, double price) {
    std::lock_guard<std::mutex> lock(positions_mutex_);
    
    // Update position
    Position& position = positions_[symbol];
    
    if (side == "BOT") {  // Bought
        position.size += quantity;
        position.price = ((position.price * (position.size - quantity)) + (price * quantity)) / position.size;
    } else {  // Sold
        position.size -= quantity;
        if (position.size == 0) {
            position.price = 0.0;
        }
    }
}

void IBBroker::worker_thread_main() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { return !task_queue_.empty() || shutdown_; });
            
            if (shutdown_ && task_queue_.empty()) {
                break;
            }
            
            task = std::move(task_queue_.front());
            task_queue_.pop();
        }
        
        try {
            task();
        } catch (const std::exception&) {
            // Log error and continue
        }
    }
}

void IBBroker::enqueue_task(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push(std::move(task));
    }
    queue_cv_.notify_one();
}

void IBBroker::connection_thread_main() {
    // Connection management thread
    bool success = ib_connect(params.host, params.port, params.client_id);
    
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        connected_ = success;
        connecting_ = false;
    }
    connection_cv_.notify_all();
    
    // Monitor connection and handle reconnection
    while (connected_ && !shutdown_) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Check connection health
        // Implementation would ping IB or check connection status
    }
}

void IBBroker::handle_disconnect() {
    connected_ = false;
    
    if (params.reconnect && !shutdown_) {
        // Schedule reconnection
        std::this_thread::sleep_for(std::chrono::seconds(params.reconnect_timeout));
        reconnect();
    }
}

void IBBroker::reconnect() {
    if (shutdown_) {
        return;
    }
    
    // Attempt to reconnect
    connect();
}

// IB API wrapper methods (placeholder implementations)
bool IBBroker::ib_connect(const std::string& host, int port, int client_id) {
    // Placeholder - would connect to actual IB API
    return true;
}

void IBBroker::ib_disconnect() {
    // Placeholder - would disconnect from IB API
}

void IBBroker::ib_place_order(int order_id, const std::string& symbol, const IBOrder& order) {
    // Placeholder - would place order through IB API
}

void IBBroker::ib_cancel_order(int order_id) {
    // Placeholder - would cancel order through IB API
}

void IBBroker::ib_request_account_updates(bool subscribe) {
    // Placeholder - would request account updates from IB API
}

void IBBroker::ib_request_positions() {
    // Placeholder - would request position data from IB API
}

} // namespace backtrader