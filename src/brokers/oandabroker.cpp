#include "../../include/brokers/oandabroker.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <sstream>

namespace backtrader {
namespace brokers {

OandaBroker::OandaBroker(const Params& params) 
    : p(params), next_order_id_(1), next_trade_id_(1) {
    
    if (!p.store) {
        throw std::invalid_argument("Oanda store is required");
    }
    
    store_ = p.store;
    commission_ = p.commission;
    cash_ = p.cash;
    value_ = p.cash;
    
    // Initialize positions map
    positions_.clear();
}

std::string OandaBroker::next_order_id() {
    return "oanda_order_" + std::to_string(next_order_id_++);
}

std::shared_ptr<Order> OandaBroker::buy(std::shared_ptr<DataSeries> data, double size, 
                                       double price, const std::map<std::string, std::any>& kwargs) {
    return submit_order(data, size, price, true, OrderType::Market, kwargs);
}

std::shared_ptr<Order> OandaBroker::sell(std::shared_ptr<DataSeries> data, double size, 
                                        double price, const std::map<std::string, std::any>& kwargs) {
    return submit_order(data, size, price, false, OrderType::Market, kwargs);
}

std::shared_ptr<Order> OandaBroker::submit_order(std::shared_ptr<DataSeries> data, double size, 
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
    
    // Extract instrument from data or kwargs
    std::string instrument = getInstrumentFromData(data, kwargs);
    order->symbol = instrument;
    
    // Validate order
    if (!validate_order(order)) {
        order->status = OrderStatus::Rejected;
        order->reject_reason = "Insufficient margin or invalid parameters";
        return order;
    }
    
    // Submit to Oanda via store
    try {
        auto oanda_order = create_oanda_order_request(order, kwargs);
        auto result = store_->create_order(oanda_order);
        
        if (result.find("errorMessage") != result.end()) {
            order->status = OrderStatus::Rejected;
            order->reject_reason = std::any_cast<std::string>(result["errorMessage"]);
        } else {
            order->status = OrderStatus::Accepted;
            
            if (result.find("orderCreateTransaction") != result.end()) {
                auto transaction = std::any_cast<std::map<std::string, std::any>>(
                    result["orderCreateTransaction"]);
                
                if (transaction.find("id") != transaction.end()) {
                    order->exchange_id = std::any_cast<std::string>(transaction["id"]);
                }
            }
            
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

std::map<std::string, std::any> OandaBroker::create_oanda_order_request(
    std::shared_ptr<Order> order, const std::map<std::string, std::any>& kwargs) {
    
    std::map<std::string, std::any> request;
    
    // Order details
    std::map<std::string, std::any> order_data;
    order_data["instrument"] = order->symbol;
    order_data["units"] = std::to_string(static_cast<int>(order->size));
    
    if (order->order_type == OrderType::Market) {
        order_data["type"] = "MARKET";
    } else if (order->order_type == OrderType::Limit) {
        order_data["type"] = "LIMIT";
        order_data["price"] = std::to_string(order->price);
    } else if (order->order_type == OrderType::Stop) {
        order_data["type"] = "STOP";
        order_data["priceBound"] = std::to_string(order->price);
    }
    
    // Time in force
    auto tif_it = kwargs.find("timeInForce");
    if (tif_it != kwargs.end()) {
        order_data["timeInForce"] = std::any_cast<std::string>(tif_it->second);
    } else {
        order_data["timeInForce"] = "FOK"; // Fill or Kill for market orders
    }
    
    // Stop loss
    auto sl_it = kwargs.find("stopLoss");
    if (sl_it != kwargs.end()) {
        std::map<std::string, std::any> stop_loss;
        stop_loss["price"] = std::to_string(std::any_cast<double>(sl_it->second));
        order_data["stopLossOnFill"] = stop_loss;
    }
    
    // Take profit
    auto tp_it = kwargs.find("takeProfit");
    if (tp_it != kwargs.end()) {
        std::map<std::string, std::any> take_profit;
        take_profit["price"] = std::to_string(std::any_cast<double>(tp_it->second));
        order_data["takeProfitOnFill"] = take_profit;
    }
    
    request["order"] = order_data;
    
    return request;
}

bool OandaBroker::validate_order(std::shared_ptr<Order> order) {
    if (!order) {
        return false;
    }
    
    // For Oanda, we need to check margin requirements instead of cash
    // This is a simplified validation
    
    // Check if instrument is valid
    if (order->symbol.empty()) {
        return false;
    }
    
    // Check minimum trade size (Oanda typically has minimums like 1 unit)
    if (std::abs(order->size) < 1.0) {
        return false;
    }
    
    // Check maximum trade size
    if (std::abs(order->size) > p.max_units) {
        return false;
    }
    
    return true;
}

void OandaBroker::cancel(std::shared_ptr<Order> order) {
    if (!order || order->status != OrderStatus::Accepted) {
        return;
    }
    
    try {
        auto result = store_->cancel_order(order->exchange_id);
        
        if (result.find("orderCancelTransaction") != result.end()) {
            order->status = OrderStatus::Canceled;
            pending_orders_.erase(order->id);
        }
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Failed to cancel order: " << e.what() << std::endl;
        }
    }
}

std::vector<std::shared_ptr<Order>> OandaBroker::get_orders_open() {
    std::vector<std::shared_ptr<Order>> open_orders;
    
    for (const auto& pair : pending_orders_) {
        auto order = pair.second;
        if (order->status == OrderStatus::Accepted) {
            open_orders.push_back(order);
        }
    }
    
    return open_orders;
}

std::vector<std::shared_ptr<Order>> OandaBroker::get_orders_history() {
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

double OandaBroker::get_cash() {
    return cash_;
}

double OandaBroker::get_value() {
    update_portfolio_value();
    return value_;
}

Position OandaBroker::get_position(std::shared_ptr<DataSeries> data) {
    std::string instrument = getInstrumentFromData(data, {});
    
    auto it = positions_.find(instrument);
    if (it != positions_.end()) {
        return it->second;
    }
    
    // Return empty position
    Position pos;
    pos.symbol = instrument;
    pos.size = 0.0;
    pos.price = 0.0;
    pos.value = 0.0;
    return pos;
}

void OandaBroker::update_orders() {
    std::vector<std::string> completed_orders;
    
    for (auto& pair : pending_orders_) {
        auto order = pair.second;
        
        try {
            // Check order status with Oanda
            auto result = store_->get_order(order->exchange_id);
            
            if (result.find("order") != result.end()) {
                auto order_data = std::any_cast<std::map<std::string, std::any>>(result["order"]);
                
                if (order_data.find("state") != order_data.end()) {
                    std::string state = std::any_cast<std::string>(order_data["state"]);
                    
                    if (state == "FILLED") {
                        order->status = OrderStatus::Completed;
                        order->executed_time = std::chrono::system_clock::now();
                        
                        if (order_data.find("filledUnits") != order_data.end()) {
                            order->executed_size = std::stod(
                                std::any_cast<std::string>(order_data["filledUnits"]));
                        }
                        if (order_data.find("averageFillPrice") != order_data.end()) {
                            order->executed_price = std::stod(
                                std::any_cast<std::string>(order_data["averageFillPrice"]));
                        }
                        
                        // Create trade
                        create_trade_from_order(order);
                        completed_orders.push_back(order->id);
                        
                    } else if (state == "CANCELLED") {
                        order->status = OrderStatus::Canceled;
                        completed_orders.push_back(order->id);
                    }
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

void OandaBroker::update_portfolio_value() {
    try {
        auto account_info = store_->get_account();
        
        if (account_info.find("account") != account_info.end()) {
            auto account = std::any_cast<std::map<std::string, std::any>>(account_info["account"]);
            
            if (account.find("NAV") != account.end()) {
                value_ = std::stod(std::any_cast<std::string>(account["NAV"]));
            }
            
            if (account.find("balance") != account.end()) {
                cash_ = std::stod(std::any_cast<std::string>(account["balance"]));
            }
        }
        
        // Update positions
        auto positions = store_->get_positions();
        if (positions.find("positions") != positions.end()) {
            auto position_list = std::any_cast<std::vector<std::any>>(positions["positions"]);
            
            positions_.clear();
            for (const auto& pos_any : position_list) {
                auto pos_data = std::any_cast<std::map<std::string, std::any>>(pos_any);
                
                Position position;
                position.symbol = std::any_cast<std::string>(pos_data["instrument"]);
                
                if (pos_data.find("long") != pos_data.end()) {
                    auto long_data = std::any_cast<std::map<std::string, std::any>>(pos_data["long"]);
                    if (long_data.find("units") != long_data.end()) {
                        double long_units = std::stod(std::any_cast<std::string>(long_data["units"]));
                        position.size += long_units;
                    }
                }
                
                if (pos_data.find("short") != pos_data.end()) {
                    auto short_data = std::any_cast<std::map<std::string, std::any>>(pos_data["short"]);
                    if (short_data.find("units") != short_data.end()) {
                        double short_units = std::stod(std::any_cast<std::string>(short_data["units"]));
                        position.size += short_units; // short units are negative
                    }
                }
                
                if (std::abs(position.size) > 1e-8) {
                    positions_[position.symbol] = position;
                }
            }
        }
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Error updating portfolio value: " << e.what() << std::endl;
        }
    }
}

void OandaBroker::create_trade_from_order(std::shared_ptr<Order> order) {
    Trade trade;
    trade.id = "trade_" + std::to_string(next_trade_id_++);
    trade.order_id = order->id;
    trade.symbol = order->symbol;
    trade.size = order->executed_size;
    trade.price = order->executed_price;
    trade.value = std::abs(trade.size) * trade.price;
    trade.commission = 0.0;
    trade.datetime = order->executed_time;
    
    // Calculate commission (spread-based for Oanda)
    if (commission_) {
        trade.commission = commission_->get_commission(trade.size, trade.price);
    }
    
    // Update position
    update_position(trade);
    
    // Store trade
    trades_.push_back(trade);
}

void OandaBroker::update_position(const Trade& trade) {
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

std::string OandaBroker::getInstrumentFromData(std::shared_ptr<DataSeries> data, 
                                              const std::map<std::string, std::any>& kwargs) {
    // Try to get instrument from kwargs first
    auto instrument_it = kwargs.find("instrument");
    if (instrument_it != kwargs.end()) {
        return std::any_cast<std::string>(instrument_it->second);
    }
    
    // Try to get instrument from data metadata
    if (data && data->params.find("instrument") != data->params.end()) {
        return std::any_cast<std::string>(data->params.at("instrument"));
    }
    
    // Default fallback
    return "EUR_USD";
}

std::vector<Trade> OandaBroker::get_trades() {
    return trades_;
}

std::map<std::string, Position> OandaBroker::get_positions() {
    return positions_;
}

void OandaBroker::notify(std::shared_ptr<Order> order) {
    // Notification callback for order status changes
    if (p.debug) {
        std::cout << "Oanda Order notification: " << order->id 
                  << " status: " << static_cast<int>(order->status) << std::endl;
    }
}

} // namespace brokers
} // namespace backtrader