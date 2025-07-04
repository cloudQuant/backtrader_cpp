#include "../../include/brokers/ccxtbroker.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>

namespace backtrader {
namespace brokers {

CCXTBroker::CCXTBroker(const Params& params) 
    : p(params), next_order_id_(1), next_trade_id_(1) {
    
    if (!p.store) {
        throw std::invalid_argument("CCXT store is required");
    }
    
    store_ = p.store;
    commission_ = p.commission;
    cash_ = p.cash;
    value_ = p.cash;
    
    // Initialize positions map
    positions_.clear();
}

std::string CCXTBroker::next_order_id() {
    return "order_" + std::to_string(next_order_id_++);
}

std::shared_ptr<Order> CCXTBroker::buy(std::shared_ptr<DataSeries> data, double size, 
                                      double price, const std::map<std::string, std::any>& kwargs) {
    return submit_order(data, size, price, true, OrderType::Market, kwargs);
}

std::shared_ptr<Order> CCXTBroker::sell(std::shared_ptr<DataSeries> data, double size, 
                                       double price, const std::map<std::string, std::any>& kwargs) {
    return submit_order(data, size, price, false, OrderType::Market, kwargs);
}

std::shared_ptr<Order> CCXTBroker::submit_order(std::shared_ptr<DataSeries> data, double size, 
                                               double price, bool is_buy, OrderType order_type,
                                               const std::map<std::string, std::any>& kwargs) {
    
    if (!data) {
        throw std::invalid_argument("Data series is required");
    }
    
    if (size <= 0) {
        throw std::invalid_argument("Order size must be positive");
    }
    
    // Create order
    auto order = std::make_shared<Order>();
    order->id = next_order_id();
    order->data = data;
    order->size = is_buy ? size : -size;
    order->price = price;
    order->order_type = order_type;
    order->status = OrderStatus::Submitted;
    order->created_time = std::chrono::system_clock::now();
    
    // Extract symbol from data or kwargs
    std::string symbol = getSymbolFromData(data, kwargs);
    order->symbol = symbol;
    
    // Validate order
    if (!validate_order(order)) {
        order->status = OrderStatus::Rejected;
        order->reject_reason = "Insufficient funds or invalid parameters";
        return order;
    }
    
    // Submit to exchange via store
    try {
        std::string ccxt_order_type = (order_type == OrderType::Market) ? "market" : "limit";
        std::string side = is_buy ? "buy" : "sell";
        
        auto result = store_->create_order(symbol, ccxt_order_type, side, 
                                         std::abs(size), price);
        
        if (result.find("error") != result.end()) {
            order->status = OrderStatus::Rejected;
            order->reject_reason = std::any_cast<std::string>(result["error"]);
        } else {
            order->status = OrderStatus::Accepted;
            order->exchange_id = std::any_cast<std::string>(result["id"]);
            
            // Store order
            orders_[order->id] = order;
            pending_orders_[order->id] = order;
        }
        
    } catch (const std::exception& e) {
        order->status = OrderStatus::Rejected;
        order->reject_reason = e.what();
    }
    
    return order;
}

bool CCXTBroker::validate_order(std::shared_ptr<Order> order) {
    if (!order) {
        return false;
    }
    
    // Check if we have enough cash for buy orders
    if (order->size > 0) {
        double required_cash = order->size * order->price;
        if (commission_) {
            required_cash += commission_->get_commission(order->size, order->price);
        }
        
        if (required_cash > cash_) {
            return false;
        }
    }
    
    // Check if we have enough position for sell orders
    if (order->size < 0) {
        auto pos_it = positions_.find(order->symbol);
        if (pos_it != positions_.end()) {
            if (std::abs(order->size) > pos_it->second.size) {
                return false; // Cannot sell more than we own
            }
        } else if (std::abs(order->size) > 0) {
            return false; // No position to sell
        }
    }
    
    return true;
}

void CCXTBroker::cancel(std::shared_ptr<Order> order) {
    if (!order || order->status != OrderStatus::Accepted) {
        return;
    }
    
    try {
        auto result = store_->cancel_order(order->exchange_id, order->symbol);
        
        if (result.find("error") == result.end()) {
            order->status = OrderStatus::Canceled;
            pending_orders_.erase(order->id);
        }
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Failed to cancel order: " << e.what() << std::endl;
        }
    }
}

std::vector<std::shared_ptr<Order>> CCXTBroker::get_orders_open() {
    std::vector<std::shared_ptr<Order>> open_orders;
    
    for (const auto& pair : pending_orders_) {
        auto order = pair.second;
        if (order->status == OrderStatus::Accepted) {
            open_orders.push_back(order);
        }
    }
    
    return open_orders;
}

std::vector<std::shared_ptr<Order>> CCXTBroker::get_orders_history() {
    std::vector<std::shared_ptr<Order>> history;
    
    for (const auto& pair : orders_) {
        auto order = pair.second;
        if (order->status == OrderStatus::Completed || 
            order->status == OrderStatus::Canceled ||
            order->status == OrderStatus::Rejected) {
            history.push_back(order);
        }
    }
    
    return history;
}

double CCXTBroker::get_cash() {
    return cash_;
}

double CCXTBroker::get_value() {
    update_portfolio_value();
    return value_;
}

Position CCXTBroker::get_position(std::shared_ptr<DataSeries> data) {
    std::string symbol = getSymbolFromData(data, {});
    
    auto it = positions_.find(symbol);
    if (it != positions_.end()) {
        return it->second;
    }
    
    // Return empty position
    Position pos;
    pos.symbol = symbol;
    pos.size = 0.0;
    pos.price = 0.0;
    pos.value = 0.0;
    return pos;
}

void CCXTBroker::update_orders() {
    std::vector<std::string> completed_orders;
    
    for (auto& pair : pending_orders_) {
        auto order = pair.second;
        
        try {
            // Check order status on exchange
            auto result = store_->fetch_order(order->exchange_id, order->symbol);
            
            if (result.find("status") != result.end()) {
                std::string exchange_status = std::any_cast<std::string>(result["status"]);
                
                if (exchange_status == "closed" || exchange_status == "filled") {
                    // Order was filled
                    order->status = OrderStatus::Completed;
                    order->executed_time = std::chrono::system_clock::now();
                    
                    if (result.find("filled") != result.end()) {
                        order->executed_size = std::any_cast<double>(result["filled"]);
                    }
                    if (result.find("average") != result.end()) {
                        order->executed_price = std::any_cast<double>(result["average"]);
                    }
                    
                    // Create trade
                    create_trade_from_order(order);
                    completed_orders.push_back(order->id);
                    
                } else if (exchange_status == "canceled") {
                    order->status = OrderStatus::Canceled;
                    completed_orders.push_back(order->id);
                }
            }
            
        } catch (const std::exception& e) {
            if (p.debug) {
                std::cerr << "Error updating order " << order->id << ": " << e.what() << std::endl;
            }
        }
    }
    
    // Remove completed orders from pending
    for (const std::string& order_id : completed_orders) {
        pending_orders_.erase(order_id);
    }
}

void CCXTBroker::update_portfolio_value() {
    // Get current balance from exchange
    try {
        auto balance = store_->fetch_balance();
        
        if (balance.find("total") != balance.end()) {
            auto total_balances = std::any_cast<std::map<std::string, double>>(balance["total"]);
            
            value_ = 0.0;
            for (const auto& pair : total_balances) {
                value_ += pair.second;
            }
        }
        
        if (balance.find("free") != balance.end()) {
            auto free_balances = std::any_cast<std::map<std::string, double>>(balance["free"]);
            
            if (free_balances.find(p.base_currency) != free_balances.end()) {
                cash_ = free_balances[p.base_currency];
            }
        }
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Error updating portfolio value: " << e.what() << std::endl;
        }
    }
}

void CCXTBroker::create_trade_from_order(std::shared_ptr<Order> order) {
    Trade trade;
    trade.id = "trade_" + std::to_string(next_trade_id_++);
    trade.order_id = order->id;
    trade.symbol = order->symbol;
    trade.size = order->executed_size;
    trade.price = order->executed_price;
    trade.value = std::abs(trade.size) * trade.price;
    trade.commission = 0.0;
    trade.datetime = order->executed_time;
    
    // Calculate commission
    if (commission_) {
        trade.commission = commission_->get_commission(trade.size, trade.price);
    }
    
    // Update position
    update_position(trade);
    
    // Store trade
    trades_.push_back(trade);
    
    // Update cash
    if (trade.size > 0) {
        // Buy: decrease cash
        cash_ -= (trade.value + trade.commission);
    } else {
        // Sell: increase cash
        cash_ += (trade.value - trade.commission);
    }
}

void CCXTBroker::update_position(const Trade& trade) {
    auto& position = positions_[trade.symbol];
    position.symbol = trade.symbol;
    
    if (position.size == 0.0) {
        // New position
        position.size = trade.size;
        position.price = trade.price;
    } else {
        // Update existing position
        double total_value = position.size * position.price + trade.size * trade.price;
        position.size += trade.size;
        
        if (position.size != 0.0) {
            position.price = total_value / position.size;
        } else {
            position.price = 0.0;
        }
    }
    
    position.value = position.size * position.price;
    position.last_update = trade.datetime;
    
    // Remove position if size is zero
    if (std::abs(position.size) < 1e-8) {
        positions_.erase(trade.symbol);
    }
}

std::string CCXTBroker::getSymbolFromData(std::shared_ptr<DataSeries> data, 
                                         const std::map<std::string, std::any>& kwargs) {
    // Try to get symbol from kwargs first
    auto symbol_it = kwargs.find("symbol");
    if (symbol_it != kwargs.end()) {
        return std::any_cast<std::string>(symbol_it->second);
    }
    
    // Try to get symbol from data metadata
    if (data && data->params.find("symbol") != data->params.end()) {
        return std::any_cast<std::string>(data->params.at("symbol"));
    }
    
    // Default fallback
    return "BTC/USDT";
}

std::vector<Trade> CCXTBroker::get_trades() {
    return trades_;
}

std::map<std::string, Position> CCXTBroker::get_positions() {
    return positions_;
}

void CCXTBroker::notify(std::shared_ptr<Order> order) {
    // Notification callback for order status changes
    if (p.debug) {
        std::cout << "Order notification: " << order->id 
                  << " status: " << static_cast<int>(order->status) << std::endl;
    }
}

} // namespace brokers
} // namespace backtrader