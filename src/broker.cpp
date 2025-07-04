#include "broker.h"
#include "order.h"
#include "position.h"
#include "comminfo.h"
#include <algorithm>
#include <cmath>

namespace backtrader {

BackBroker::BackBroker() : cash_(params.cash), value_(params.cash) {
}

void BackBroker::setcash(double cash) {
    cash_ = cash;
    value_ = cash;
}

double BackBroker::getcash() const {
    return cash_;
}

double BackBroker::getvalue() const {
    return value_;
}

std::shared_ptr<Position> BackBroker::getposition(std::shared_ptr<DataSeries> data) const {
    auto it = positions_.find(data);
    if (it != positions_.end()) {
        return it->second;
    }
    
    // Return empty position if not found
    auto position = std::make_shared<Position>();
    return position;
}

std::map<std::shared_ptr<DataSeries>, std::shared_ptr<Position>> BackBroker::getpositions() const {
    return positions_;
}

std::shared_ptr<Order> BackBroker::submit(std::shared_ptr<Order> order) {
    if (params.checksubmit && !_check_cash(order)) {
        order->status = OrderStatus::Rejected;
        return order;
    }
    
    order->status = OrderStatus::Submitted;
    pending_orders_.push_back(order);
    orders_.push_back(order);
    
    return order;
}

bool BackBroker::cancel(std::shared_ptr<Order> order) {
    auto it = std::find(pending_orders_.begin(), pending_orders_.end(), order);
    if (it != pending_orders_.end()) {
        pending_orders_.erase(it);
        order->status = OrderStatus::Canceled;
        return true;
    }
    return false;
}

std::vector<std::shared_ptr<Order>> BackBroker::get_orders_open(std::shared_ptr<DataSeries> data) {
    std::vector<std::shared_ptr<Order>> open_orders;
    
    for (auto& order : pending_orders_) {
        if (!data || order->data == data) {
            if (order->status == OrderStatus::Submitted || 
                order->status == OrderStatus::Accepted ||
                order->status == OrderStatus::Partial) {
                open_orders.push_back(order);
            }
        }
    }
    
    return open_orders;
}

void BackBroker::setcommission(double commission, double margin, double mult, std::shared_ptr<DataSeries> data) {
    auto comminfo = std::make_shared<CommInfo>();
    comminfo->commission = commission;
    comminfo->margin = margin;
    comminfo->mult = mult;
    
    if (data) {
        comminfo_[data] = comminfo;
    } else {
        // Set as default for all data
        // This would require storing a default comminfo
    }
}

std::shared_ptr<CommInfo> BackBroker::getcommissioninfo(std::shared_ptr<DataSeries> data) const {
    auto it = comminfo_.find(data);
    if (it != comminfo_.end()) {
        return it->second;
    }
    
    // Return default comminfo
    return std::make_shared<CommInfo>();
}

void BackBroker::next() {
    _process_orders();
    _update_value();
}

void BackBroker::start() {
    // Initialize broker for new run
    pending_orders_.clear();
    orders_.clear();
}

void BackBroker::stop() {
    // Cleanup after run
}

void BackBroker::set_filler(FillerFunc filler) {
    filler_ = filler;
}

double BackBroker::get_slippage(std::shared_ptr<Order> order, double price, bool is_buy) const {
    double slippage = 0.0;
    
    if (params.slip_perc > 0.0) {
        slippage = price * params.slip_perc;
    } else if (params.slip_fixed > 0.0) {
        slippage = params.slip_fixed;
    }
    
    // Apply slippage direction
    if (is_buy) {
        return slippage; // Positive for buy (price goes up)
    } else {
        return -slippage; // Negative for sell (price goes down)
    }
}

bool BackBroker::execute_order(std::shared_ptr<Order> order, double ago) {
    if (!order->data || order->data->empty()) {
        return false;
    }
    
    double price = _get_order_price(order, ago);
    if (price <= 0.0 || std::isnan(price)) {
        return false;
    }
    
    if (!_can_execute(order, price, ago)) {
        return false;
    }
    
    // Apply slippage
    bool is_buy = order->isbuy();
    double slipped_price = price + get_slippage(order, price, is_buy);
    
    // Check price limits if slip_match is enabled
    if (params.slip_match) {
        double high = (*order->data->lines->getline(DataSeries::High))[static_cast<int>(ago)];
        double low = (*order->data->lines->getline(DataSeries::Low))[static_cast<int>(ago)];
        
        slipped_price = std::max(low, std::min(high, slipped_price));
    }
    
    // Determine execution size
    double exec_size = order->size - order->executed.size;
    if (filler_) {
        exec_size = filler_(order, slipped_price, static_cast<int>(ago));
        exec_size = std::min(exec_size, order->size - order->executed.size);
    }
    
    if (exec_size > 0.0) {
        _execute_order(order, slipped_price, exec_size);
        return true;
    }
    
    return false;
}

void BackBroker::_process_orders() {
    auto it = pending_orders_.begin();
    while (it != pending_orders_.end()) {
        auto order = *it;
        
        if (execute_order(order)) {
            if (order->executed.size >= order->size) {
                order->status = OrderStatus::Completed;
                it = pending_orders_.erase(it);
            } else {
                order->status = OrderStatus::Partial;
                ++it;
            }
        } else {
            ++it;
        }
    }
}

bool BackBroker::_check_cash(std::shared_ptr<Order> order) {
    // Simplified cash check
    double required_cash = order->size * _get_order_price(order, 0);
    return cash_ >= required_cash;
}

void BackBroker::_update_value() {
    value_ = cash_;
    
    // Add position values
    for (const auto& pos_pair : positions_) {
        auto data = pos_pair.first;
        auto position = pos_pair.second;
        
        if (data && !data->empty() && position->size != 0.0) {
            double current_price = (*data->lines->getline(DataSeries::Close))[0];
            value_ += position->size * current_price;
        }
    }
}

double BackBroker::_get_order_price(std::shared_ptr<Order> order, double ago) {
    if (!order->data || order->data->empty()) {
        return 0.0;
    }
    
    switch (order->type) {
        case OrderType::Market:
            return (*order->data->lines->getline(DataSeries::Open))[static_cast<int>(ago)];
        case OrderType::Close:
            return (*order->data->lines->getline(DataSeries::Close))[static_cast<int>(ago)];
        case OrderType::Limit:
        case OrderType::Stop:
        case OrderType::StopLimit:
            return order->price;
        default:
            return (*order->data->lines->getline(DataSeries::Close))[static_cast<int>(ago)];
    }
}

bool BackBroker::_can_execute(std::shared_ptr<Order> order, double price, double ago) {
    if (!order->data || order->data->empty()) {
        return false;
    }
    
    double high = (*order->data->lines->getline(DataSeries::High))[static_cast<int>(ago)];
    double low = (*order->data->lines->getline(DataSeries::Low))[static_cast<int>(ago)];
    
    switch (order->type) {
        case OrderType::Market:
        case OrderType::Close:
            return true;
            
        case OrderType::Limit:
            if (order->isbuy()) {
                return low <= order->price;
            } else {
                return high >= order->price;
            }
            
        case OrderType::Stop:
            if (order->isbuy()) {
                return high >= order->price;
            } else {
                return low <= order->price;
            }
            
        case OrderType::StopLimit:
            // This would require more complex logic
            return true;
            
        default:
            return true;
    }
}

void BackBroker::_execute_order(std::shared_ptr<Order> order, double price, double size) {
    // Update order execution info
    order->executed.price = price;
    order->executed.size += size;
    order->executed.value += size * price;
    
    // Calculate commission
    auto comminfo = getcommissioninfo(order->data);
    double commission = comminfo->getcommission(size, price);
    order->executed.comm += commission;
    
    // Update cash
    if (order->isbuy()) {
        cash_ -= (size * price + commission);
    } else {
        cash_ += (size * price - commission);
    }
    
    // Update position
    _update_position(order, size, price);
}

void BackBroker::_update_position(std::shared_ptr<Order> order, double size, double price) {
    auto& position = positions_[order->data];
    if (!position) {
        position = std::make_shared<Position>();
    }
    
    if (order->isbuy()) {
        position->update(size, price);
    } else {
        position->update(-size, price);
    }
}

} // namespace backtrader