#include "../../include/brokers/vcbroker.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <random>
#include <thread>

namespace backtrader {
namespace brokers {

VCBroker::VCBroker(const Params& params) 
    : p(params), next_order_id_(1), next_trade_id_(1) {
    
    commission_ = p.commission;
    cash_ = p.cash;
    value_ = p.cash;
    
    // Initialize positions map
    positions_.clear();
    
    // Initialize random number generator for slippage simulation
    rd_ = std::random_device{};
    gen_ = std::mt19937{rd_()};
}

std::string VCBroker::next_order_id() {
    return "vc_order_" + std::to_string(next_order_id_++);
}

std::shared_ptr<Order> VCBroker::buy(std::shared_ptr<DataSeries> data, double size, 
                                    double price, const std::map<std::string, std::any>& kwargs) {
    return submit_order(data, size, price, true, OrderType::Market, kwargs);
}

std::shared_ptr<Order> VCBroker::sell(std::shared_ptr<DataSeries> data, double size, 
                                     double price, const std::map<std::string, std::any>& kwargs) {
    return submit_order(data, size, price, false, OrderType::Market, kwargs);
}

std::shared_ptr<Order> VCBroker::submit_order(std::shared_ptr<DataSeries> data, double size, 
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
    
    // Accept order immediately (simulation)
    order->status = OrderStatus::Accepted;
    order->exchange_id = order->id; // Use internal ID as exchange ID
    
    // Store order
    orders_[order->id] = order;
    pending_orders_[order->id] = order;
    
    // For market orders, execute immediately
    if (order_type == OrderType::Market) {
        execute_market_order(order);
    }
    
    return order;
}

bool VCBroker::validate_order(std::shared_ptr<Order> order) {
    if (!order) {
        return false;
    }
    
    // Check if we have enough cash for buy orders
    if (order->size > 0) {
        double current_price = getCurrentPrice(order->data);
        double required_cash = order->size * current_price;
        
        if (commission_) {
            required_cash += commission_->get_commission(order->size, current_price);
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

void VCBroker::execute_market_order(std::shared_ptr<Order> order) {
    if (!order || order->status != OrderStatus::Accepted) {
        return;
    }
    
    // Get current market price
    double market_price = getCurrentPrice(order->data);
    
    // Apply slippage
    double execution_price = applySlippage(market_price, order->size > 0);
    
    // Simulate execution delay
    if (p.execution_delay > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(p.execution_delay));
    }
    
    // Execute the order
    order->status = OrderStatus::Completed;
    order->executed_time = std::chrono::system_clock::now();
    order->executed_size = order->size;
    order->executed_price = execution_price;
    
    // Create trade
    create_trade_from_order(order);
    
    // Remove from pending orders
    pending_orders_.erase(order->id);
    
    if (p.debug) {
        std::cout << "Executed market order: " << order->id 
                  << " size: " << order->executed_size 
                  << " price: " << order->executed_price << std::endl;
    }
}

double VCBroker::getCurrentPrice(std::shared_ptr<DataSeries> data) {
    if (!data || data->lines.empty() || data->lines[4].empty()) {
        return 0.0;
    }
    
    // Return the latest close price (line index 4)
    return data->lines[4].back();
}

double VCBroker::applySlippage(double price, bool is_buy) {
    if (p.slippage_percent <= 0.0) {
        return price;
    }
    
    // Generate random slippage
    std::uniform_real_distribution<double> dist(0.0, p.slippage_percent / 100.0);
    double slippage_factor = dist(gen_);
    
    if (is_buy) {
        // Buy orders get worse (higher) prices
        return price * (1.0 + slippage_factor);
    } else {
        // Sell orders get worse (lower) prices
        return price * (1.0 - slippage_factor);
    }
}

void VCBroker::cancel(std::shared_ptr<Order> order) {
    if (!order || order->status != OrderStatus::Accepted) {
        return;
    }
    
    // Simulate cancellation (always successful in backtesting)
    order->status = OrderStatus::Canceled;
    pending_orders_.erase(order->id);
    
    if (p.debug) {
        std::cout << "Canceled order: " << order->id << std::endl;
    }
}

std::vector<std::shared_ptr<Order>> VCBroker::get_orders_open() {
    std::vector<std::shared_ptr<Order>> open_orders;
    
    for (const auto& pair : pending_orders_) {
        auto order = pair.second;
        if (order->status == OrderStatus::Accepted) {
            open_orders.push_back(order);
        }
    }
    
    return open_orders;
}

std::vector<std::shared_ptr<Order>> VCBroker::get_orders_history() {
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

double VCBroker::get_cash() {
    return cash_;
}

double VCBroker::get_value() {
    update_portfolio_value();
    return value_;
}

Position VCBroker::get_position(std::shared_ptr<DataSeries> data) {
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

void VCBroker::update_orders() {
    // Check pending limit orders for execution
    std::vector<std::string> executed_orders;
    
    for (auto& pair : pending_orders_) {
        auto order = pair.second;
        
        if (order->order_type == OrderType::Limit && order->status == OrderStatus::Accepted) {
            double current_price = getCurrentPrice(order->data);
            
            bool should_execute = false;
            
            if (order->size > 0) {
                // Buy limit order: execute if current price <= limit price
                should_execute = (current_price <= order->price);
            } else {
                // Sell limit order: execute if current price >= limit price
                should_execute = (current_price >= order->price);
            }
            
            if (should_execute) {
                // Execute the order
                order->status = OrderStatus::Completed;
                order->executed_time = std::chrono::system_clock::now();
                order->executed_size = order->size;
                order->executed_price = order->price; // Limit orders execute at limit price
                
                // Create trade
                create_trade_from_order(order);
                executed_orders.push_back(order->id);
                
                if (p.debug) {
                    std::cout << "Executed limit order: " << order->id 
                              << " size: " << order->executed_size 
                              << " price: " << order->executed_price << std::endl;
                }
            }
        }
    }
    
    // Remove executed orders from pending
    for (const std::string& order_id : executed_orders) {
        pending_orders_.erase(order_id);
    }
}

void VCBroker::update_portfolio_value() {
    value_ = cash_;
    
    // Add value of all positions
    for (auto& pair : positions_) {
        auto& position = pair.second;
        
        // Get current market price for this position
        // This is simplified - in real implementation, we'd need to track the data series
        // For now, use the stored position price (unrealistic but functional)
        double current_price = position.price;
        
        position.value = position.size * current_price;
        value_ += std::abs(position.value);
    }
}

void VCBroker::create_trade_from_order(std::shared_ptr<Order> order) {
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

void VCBroker::update_position(const Trade& trade) {
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

std::string VCBroker::getSymbolFromData(std::shared_ptr<DataSeries> data, 
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
    
    // Generate a default symbol based on data memory address (for uniqueness)
    std::ostringstream oss;
    oss << "DATA_" << data.get();
    return oss.str();
}

std::vector<Trade> VCBroker::get_trades() {
    return trades_;
}

std::map<std::string, Position> VCBroker::get_positions() {
    return positions_;
}

void VCBroker::notify(std::shared_ptr<Order> order) {
    // Notification callback for order status changes
    if (p.debug) {
        std::cout << "VC Order notification: " << order->id 
                  << " status: " << static_cast<int>(order->status) << std::endl;
    }
}

// Portfolio statistics
double VCBroker::get_total_value() {
    update_portfolio_value();
    return value_;
}

double VCBroker::get_available_cash() {
    return cash_;
}

std::map<std::string, double> VCBroker::get_portfolio_summary() {
    std::map<std::string, double> summary;
    
    summary["total_value"] = get_total_value();
    summary["cash"] = cash_;
    summary["positions_count"] = static_cast<double>(positions_.size());
    summary["trades_count"] = static_cast<double>(trades_.size());
    
    double total_position_value = 0.0;
    for (const auto& pair : positions_) {
        total_position_value += std::abs(pair.second.value);
    }
    summary["positions_value"] = total_position_value;
    
    return summary;
}

void VCBroker::reset() {
    // Reset broker state (useful for backtesting)
    orders_.clear();
    pending_orders_.clear();
    trades_.clear();
    positions_.clear();
    
    cash_ = p.cash;
    value_ = p.cash;
    next_order_id_ = 1;
    next_trade_id_ = 1;
}

} // namespace brokers
} // namespace backtrader