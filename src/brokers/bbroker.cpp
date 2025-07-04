#include "brokers/bbroker.h"
#include <algorithm>
#include <iostream>

namespace backtrader {

BackBroker::BackBroker() : BrokerBase(), cash_(0.0), value_(0.0) {
    cash_ = params.cash;
    value_ = params.cash;
    
    // Create default commission info
    default_commission_info_ = std::make_shared<CommInfoBase>();
}

void BackBroker::start() {
    BrokerBase::start();
    
    // Reset broker state
    cash_ = params.cash;
    value_ = params.cash;
    
    pending_orders_.clear();
    positions_.clear();
    
    // Clear order queue
    while (!orders_queue_.empty()) {
        orders_queue_.pop();
    }
}

void BackBroker::stop() {
    BrokerBase::stop();
    
    // Cancel all pending orders
    for (auto& order : pending_orders_) {
        if (order->status == Order::Status::Submitted || 
            order->status == Order::Status::Accepted) {
            order->status = Order::Status::Cancelled;
        }
    }
    
    pending_orders_.clear();
}

double BackBroker::get_cash() {
    return cash_;
}

void BackBroker::set_cash(double cash) {
    cash_ = cash;
    params.cash = cash;
}

double BackBroker::get_value(const std::vector<std::shared_ptr<DataSeries>>& datas) {
    double total_value = cash_;
    
    // Add position values
    for (const auto& [data, position] : positions_) {
        if (position && data && data->lines) {
            // Get current price (close price)
            auto close_line = data->lines->getline(4); // Close line
            if (close_line) {
                double current_price = (*close_line)[0];
                total_value += position->size * current_price;
            }
        }
    }
    
    return total_value;
}

std::shared_ptr<Order> BackBroker::submit(std::shared_ptr<Order> order) {
    if (!order) {
        return nullptr;
    }
    
    // Validate order
    if (!order->data) {
        order->status = Order::Status::Rejected;
        return order;
    }
    
    // Set order status
    order->status = Order::Status::Submitted;
    
    // Add to pending orders
    pending_orders_.push_back(order);
    
    // Immediately set to accepted (simplified)
    order->status = Order::Status::Accepted;
    
    return order;
}

std::shared_ptr<Order> BackBroker::cancel(std::shared_ptr<Order> order) {
    if (!order) {
        return nullptr;
    }
    
    // Find and remove from pending orders
    auto it = std::find(pending_orders_.begin(), pending_orders_.end(), order);
    if (it != pending_orders_.end()) {
        order->status = Order::Status::Cancelled;
        pending_orders_.erase(it);
    }
    
    return order;
}

void BackBroker::next() {
    // Process all pending orders
    process_orders();
    
    // Update cash and value
    update_cash_and_value();
}

std::shared_ptr<Position> BackBroker::get_position(std::shared_ptr<DataSeries> data) {
    auto it = positions_.find(data);
    if (it != positions_.end()) {
        return it->second;
    }
    
    // Create new position if not exists
    auto position = std::make_shared<Position>();
    positions_[data] = position;
    return position;
}

std::map<std::shared_ptr<DataSeries>, std::shared_ptr<Position>> BackBroker::get_positions() {
    return positions_;
}

void BackBroker::add_commission_info(std::shared_ptr<CommInfoBase> comminfo, 
                                     const std::string& name) {
    if (name.empty()) {
        default_commission_info_ = comminfo;
    } else {
        commission_info_[name] = comminfo;
    }
}

void BackBroker::set_commission(double commission, double margin, double mult, 
                                const std::string& name) {
    auto comminfo = std::make_shared<CommInfoBase>();
    comminfo->params.commission = commission;
    comminfo->params.margin = margin;
    comminfo->params.mult = mult;
    
    add_commission_info(comminfo, name);
}

void BackBroker::set_slippage_perc(double perc, bool slip_open, bool slip_limit,
                                   bool slip_match, bool slip_out) {
    params.slip_perc = true;
    params.slip_fixed = false;
    params.slip_open = slip_open;
    params.slip_limit = slip_limit;
    params.slip_match = slip_match;
    params.slip_out = slip_out;
    // Note: Would need to store the percentage value
}

void BackBroker::set_slippage_fixed(double fixed, bool slip_open, bool slip_limit,
                                    bool slip_match, bool slip_out) {
    params.slip_perc = false;
    params.slip_fixed = true;
    params.slip_open = slip_open;
    params.slip_limit = slip_limit;
    params.slip_match = slip_match;
    params.slip_out = slip_out;
    // Note: Would need to store the fixed value
}

void BackBroker::process_orders() {
    auto it = pending_orders_.begin();
    
    while (it != pending_orders_.end()) {
        auto order = *it;
        
        if (check_order_execution(order, order->data)) {
            // Order executed, remove from pending
            it = pending_orders_.erase(it);
        } else {
            ++it;
        }
    }
}

bool BackBroker::check_order_execution(std::shared_ptr<Order> order, 
                                       std::shared_ptr<DataSeries> data) {
    if (!order || !data) {
        return false;
    }
    
    switch (order->type) {
        case Order::Type::Market:
            return check_market_order(order, data);
        case Order::Type::Limit:
            return check_limit_order(order, data);
        case Order::Type::Stop:
            return check_stop_order(order, data);
        case Order::Type::StopLimit:
            return check_stop_limit_order(order, data);
        default:
            return false;
    }
}

bool BackBroker::check_market_order(std::shared_ptr<Order> order, 
                                    std::shared_ptr<DataSeries> data) {
    if (!data->lines) return false;
    
    // Market orders execute at next open
    auto open_line = data->lines->getline(1); // Open line
    if (!open_line) return false;
    
    double price = (*open_line)[0];
    price = apply_slippage(price, order);
    
    if (validate_order_cash(order, price)) {
        execute_order(order, price, order->size);
        return true;
    }
    
    return false;
}

bool BackBroker::check_limit_order(std::shared_ptr<Order> order, 
                                   std::shared_ptr<DataSeries> data) {
    if (!data->lines) return false;
    
    auto high_line = data->lines->getline(2); // High line
    auto low_line = data->lines->getline(3);  // Low line
    
    if (!high_line || !low_line) return false;
    
    double high = (*high_line)[0];
    double low = (*low_line)[0];
    double limit_price = order->price;
    
    bool can_execute = false;
    double exec_price = limit_price;
    
    if (order->isbuy()) {
        // Buy limit: execute if low <= limit_price
        can_execute = (low <= limit_price);
        exec_price = std::min(limit_price, high); // Best possible price
    } else {
        // Sell limit: execute if high >= limit_price
        can_execute = (high >= limit_price);
        exec_price = std::max(limit_price, low); // Best possible price
    }
    
    if (can_execute) {
        exec_price = apply_slippage(exec_price, order);
        if (validate_order_cash(order, exec_price)) {
            execute_order(order, exec_price, order->size);
            return true;
        }
    }
    
    return false;
}

bool BackBroker::check_stop_order(std::shared_ptr<Order> order, 
                                  std::shared_ptr<DataSeries> data) {
    if (!data->lines) return false;
    
    auto high_line = data->lines->getline(2); // High line
    auto low_line = data->lines->getline(3);  // Low line
    auto close_line = data->lines->getline(4); // Close line
    
    if (!high_line || !low_line || !close_line) return false;
    
    double high = (*high_line)[0];
    double low = (*low_line)[0];
    double close = (*close_line)[0];
    double stop_price = order->price;
    
    bool triggered = false;
    
    if (order->isbuy()) {
        // Buy stop: trigger if high >= stop_price
        triggered = (high >= stop_price);
    } else {
        // Sell stop: trigger if low <= stop_price
        triggered = (low <= stop_price);
    }
    
    if (triggered) {
        // Execute as market order at close price
        double exec_price = apply_slippage(close, order);
        if (validate_order_cash(order, exec_price)) {
            execute_order(order, exec_price, order->size);
            return true;
        }
    }
    
    return false;
}

bool BackBroker::check_stop_limit_order(std::shared_ptr<Order> order, 
                                        std::shared_ptr<DataSeries> data) {
    // Simplified: treat as stop order for now
    return check_stop_order(order, data);
}

void BackBroker::execute_order(std::shared_ptr<Order> order, double price, double size) {
    if (!order) return;
    
    // Calculate commission
    double commission = calculate_commission(order, price, size);
    
    // Update position
    update_position(order->data, order->isbuy() ? size : -size, price);
    
    // Update cash
    double cost = size * price;
    if (order->isbuy()) {
        cash_ -= (cost + commission);
    } else {
        cash_ += (cost - commission);
    }
    
    // Set order as completed
    order->status = Order::Status::Completed;
    order->executed_price = price;
    order->executed_size = size;
    order->executed_commission = commission;
}

double BackBroker::calculate_commission(std::shared_ptr<Order> order, double price, double size) {
    auto comminfo = default_commission_info_;
    
    if (comminfo) {
        return comminfo->get_commission(size, price);
    }
    
    // Fallback to simple commission
    return params.commission * size;
}

void BackBroker::update_position(std::shared_ptr<DataSeries> data, double size, double price) {
    auto position = get_position(data);
    if (position) {
        position->update(size, price);
    }
}

void BackBroker::update_cash_and_value() {
    value_ = get_value();
}

double BackBroker::apply_slippage(double price, std::shared_ptr<Order> order) {
    // Simplified slippage implementation
    return price; // No slippage for now
}

bool BackBroker::validate_order_cash(std::shared_ptr<Order> order, double price) {
    if (!order) return false;
    
    if (order->isbuy()) {
        double required_cash = order->size * price;
        return cash_ >= required_cash;
    }
    
    // For sell orders, check if we have enough position
    auto position = get_position(order->data);
    if (position) {
        return position->size >= order->size;
    }
    
    return params.shortcash; // Allow short selling if enabled
}

} // namespace backtrader