#include "../../include/brokers/ctpbroker.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <iomanip>

namespace backtrader {
namespace brokers {

CTPBroker::CTPBroker(const Params& params) 
    : p(params), next_order_id_(1), next_trade_id_(1), request_id_(1) {
    
    if (!p.store) {
        throw std::invalid_argument("CTP store is required");
    }
    
    store_ = p.store;
    commission_ = p.commission;
    cash_ = p.cash;
    value_ = p.cash;
    
    // Initialize positions map
    positions_.clear();
    
    // Set default parameters
    if (p.currency.empty()) {
        p.currency = "CNY";
    }
}

std::string CTPBroker::next_order_id() {
    return "ctp_order_" + std::to_string(next_order_id_++);
}

int CTPBroker::next_request_id() {
    return request_id_++;
}

std::shared_ptr<Order> CTPBroker::buy(std::shared_ptr<DataSeries> data, double size, 
                                     double price, const std::map<std::string, std::any>& kwargs) {
    return submit_order(data, size, price, true, OrderType::Limit, kwargs);
}

std::shared_ptr<Order> CTPBroker::sell(std::shared_ptr<DataSeries> data, double size, 
                                      double price, const std::map<std::string, std::any>& kwargs) {
    return submit_order(data, size, price, false, OrderType::Limit, kwargs);
}

std::shared_ptr<Order> CTPBroker::submit_order(std::shared_ptr<DataSeries> data, double size, 
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
    
    // Submit to CTP via store
    try {
        auto ctp_order = create_ctp_order_request(order, kwargs);
        auto result = store_->insert_order(ctp_order);
        
        if (result.find("ErrorID") != result.end()) {
            int error_id = std::any_cast<int>(result["ErrorID"]);
            if (error_id != 0) {
                order->status = OrderStatus::Rejected;
                if (result.find("ErrorMsg") != result.end()) {
                    order->reject_reason = std::any_cast<std::string>(result["ErrorMsg"]);
                } else {
                    order->reject_reason = "CTP Error ID: " + std::to_string(error_id);
                }
            } else {
                order->status = OrderStatus::Accepted;
                
                if (result.find("OrderRef") != result.end()) {
                    order->exchange_id = std::any_cast<std::string>(result["OrderRef"]);
                }
                
                // Store order
                orders_[order->id] = order;
                pending_orders_[order->id] = order;
            }
        }
        
    } catch (const std::exception& e) {
        order->status = OrderStatus::Rejected;
        order->reject_reason = e.what();
    }
    
    return order;
}

std::map<std::string, std::any> CTPBroker::create_ctp_order_request(
    std::shared_ptr<Order> order, const std::map<std::string, std::any>& kwargs) {
    
    std::map<std::string, std::any> request;
    
    // Basic order fields
    request["InstrumentID"] = order->symbol;
    request["OrderRef"] = order->id;
    request["UserID"] = p.user_id;
    request["BrokerID"] = p.broker_id;
    request["InvestorID"] = p.investor_id;
    request["ExchangeID"] = getExchangeFromInstrument(order->symbol);
    
    // Direction and offset
    if (order->size > 0) {
        request["Direction"] = "0"; // Buy
    } else {
        request["Direction"] = "1"; // Sell
    }
    
    // Offset flag (open/close position)
    auto offset_it = kwargs.find("offset");
    if (offset_it != kwargs.end()) {
        request["CombOffsetFlag"] = std::any_cast<std::string>(offset_it->second);
    } else {
        request["CombOffsetFlag"] = "0"; // Open position
    }
    
    // Hedge flag
    request["CombHedgeFlag"] = "1"; // Speculation
    
    // Volume
    request["VolumeTotalOriginal"] = static_cast<int>(std::abs(order->size));
    
    // Price and order type
    if (order->order_type == OrderType::Market) {
        request["OrderPriceType"] = "1"; // Any price (market order)
        request["LimitPrice"] = 0.0;
    } else if (order->order_type == OrderType::Limit) {
        request["OrderPriceType"] = "2"; // Limit price
        request["LimitPrice"] = order->price;
    }
    
    // Time condition
    request["TimeCondition"] = "3"; // GFD (Good For Day)
    
    // Volume condition
    request["VolumeCondition"] = "1"; // Any volume
    
    // Contingent condition
    request["ContingentCondition"] = "1"; // Immediately
    
    // Force close reason
    request["ForceCloseReason"] = "0"; // Not force close
    
    // Auto suspend
    request["IsAutoSuspend"] = 0;
    
    // User force close
    request["UserForceClose"] = 0;
    
    // Request ID
    request["RequestID"] = next_request_id();
    
    return request;
}

bool CTPBroker::validate_order(std::shared_ptr<Order> order) {
    if (!order) {
        return false;
    }
    
    // Check if instrument is valid
    if (order->symbol.empty()) {
        return false;
    }
    
    // Check minimum trade size (futures typically trade in lots)
    if (std::abs(order->size) < 1.0) {
        return false;
    }
    
    // Check if size is whole number for futures
    if (std::abs(order->size) != std::floor(std::abs(order->size))) {
        return false;
    }
    
    // Check maximum trade size
    if (std::abs(order->size) > p.max_volume) {
        return false;
    }
    
    // Check margin requirements (simplified)
    if (order->size > 0) {
        double required_margin = calculateMarginRequirement(order);
        if (required_margin > cash_) {
            return false;
        }
    }
    
    return true;
}

double CTPBroker::calculateMarginRequirement(std::shared_ptr<Order> order) {
    // Simplified margin calculation
    // In real implementation, this would use instrument specifications
    double contract_size = getContractSize(order->symbol);
    double margin_rate = getMarginRate(order->symbol);
    
    return order->price * contract_size * std::abs(order->size) * margin_rate;
}

double CTPBroker::getContractSize(const std::string& instrument) {
    // Simplified contract size lookup
    // In real implementation, this would query from CTP or configuration
    
    if (instrument.find("IF") == 0) return 300.0;    // CSI 300 index futures
    if (instrument.find("IC") == 0) return 200.0;    // CSI 500 index futures
    if (instrument.find("IH") == 0) return 300.0;    // SSE 50 index futures
    if (instrument.find("cu") == 0) return 5.0;      // Copper
    if (instrument.find("al") == 0) return 5.0;      // Aluminum
    if (instrument.find("ag") == 0) return 15.0;     // Silver
    if (instrument.find("au") == 0) return 1000.0;   // Gold
    
    return 1.0; // Default
}

double CTPBroker::getMarginRate(const std::string& instrument) {
    // Simplified margin rate lookup
    // In real implementation, this would query from CTP or configuration
    
    if (instrument.find("IF") == 0) return 0.12;     // Index futures typically 12%
    if (instrument.find("IC") == 0) return 0.12;
    if (instrument.find("IH") == 0) return 0.12;
    
    return 0.08; // Default 8% for commodities
}

std::string CTPBroker::getExchangeFromInstrument(const std::string& instrument) {
    // Map instrument to exchange
    if (instrument.find("IF") == 0 || instrument.find("IC") == 0 || instrument.find("IH") == 0) {
        return "CFFEX"; // China Financial Futures Exchange
    }
    if (instrument.find("cu") == 0 || instrument.find("al") == 0 || 
        instrument.find("ag") == 0 || instrument.find("au") == 0) {
        return "SHFE";  // Shanghai Futures Exchange
    }
    
    return "CFFEX"; // Default
}

void CTPBroker::cancel(std::shared_ptr<Order> order) {
    if (!order || order->status != OrderStatus::Accepted) {
        return;
    }
    
    try {
        std::map<std::string, std::any> cancel_request;
        cancel_request["OrderRef"] = order->exchange_id;
        cancel_request["InstrumentID"] = order->symbol;
        cancel_request["ExchangeID"] = getExchangeFromInstrument(order->symbol);
        cancel_request["UserID"] = p.user_id;
        cancel_request["BrokerID"] = p.broker_id;
        cancel_request["InvestorID"] = p.investor_id;
        cancel_request["RequestID"] = next_request_id();
        
        auto result = store_->cancel_order(cancel_request);
        
        if (result.find("ErrorID") != result.end()) {
            int error_id = std::any_cast<int>(result["ErrorID"]);
            if (error_id == 0) {
                order->status = OrderStatus::Canceled;
                pending_orders_.erase(order->id);
            }
        }
        
    } catch (const std::exception& e) {
        if (p.debug) {
            std::cerr << "Failed to cancel order: " << e.what() << std::endl;
        }
    }
}

std::vector<std::shared_ptr<Order>> CTPBroker::get_orders_open() {
    std::vector<std::shared_ptr<Order>> open_orders;
    
    for (const auto& pair : pending_orders_) {
        auto order = pair.second;
        if (order->status == OrderStatus::Accepted) {
            open_orders.push_back(order);
        }
    }
    
    return open_orders;
}

std::vector<std::shared_ptr<Order>> CTPBroker::get_orders_history() {
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

double CTPBroker::get_cash() {
    return cash_;
}

double CTPBroker::get_value() {
    update_portfolio_value();
    return value_;
}

Position CTPBroker::get_position(std::shared_ptr<DataSeries> data) {
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

void CTPBroker::update_orders() {
    // In CTP, order updates are typically received via callbacks
    // This method would process any pending order status updates
    
    // For now, implement a basic polling mechanism
    std::vector<std::string> completed_orders;
    
    for (auto& pair : pending_orders_) {
        auto order = pair.second;
        
        try {
            // Query order status from CTP
            auto result = store_->query_order(order->exchange_id);
            
            if (result.find("OrderStatus") != result.end()) {
                char status = std::any_cast<char>(result["OrderStatus"]);
                
                switch (status) {
                    case '0': // All traded
                        order->status = OrderStatus::Completed;
                        order->executed_time = std::chrono::system_clock::now();
                        
                        if (result.find("VolumeTraded") != result.end()) {
                            order->executed_size = std::any_cast<int>(result["VolumeTraded"]);
                            if (order->size < 0) {
                                order->executed_size = -order->executed_size;
                            }
                        }
                        
                        // Create trade
                        create_trade_from_order(order);
                        completed_orders.push_back(order->id);
                        break;
                        
                    case '5': // Canceled
                        order->status = OrderStatus::Canceled;
                        completed_orders.push_back(order->id);
                        break;
                        
                    case 'a': // Unknown
                    case 'b': // No trade queue
                    case 'c': // No trade, not in queue
                        // These might indicate rejection
                        order->status = OrderStatus::Rejected;
                        completed_orders.push_back(order->id);
                        break;
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

void CTPBroker::update_portfolio_value() {
    try {
        // Query trading account from CTP
        auto account_info = store_->query_trading_account();
        
        if (account_info.find("Balance") != account_info.end()) {
            cash_ = std::any_cast<double>(account_info["Balance"]);
        }
        
        if (account_info.find("Available") != account_info.end()) {
            value_ = std::any_cast<double>(account_info["Available"]);
        }
        
        // Query positions
        auto position_data = store_->query_investor_position();
        
        positions_.clear();
        
        if (position_data.find("positions") != position_data.end()) {
            auto position_list = std::any_cast<std::vector<std::any>>(position_data["positions"]);
            
            for (const auto& pos_any : position_list) {
                auto pos_data = std::any_cast<std::map<std::string, std::any>>(pos_any);
                
                Position position;
                position.symbol = std::any_cast<std::string>(pos_data["InstrumentID"]);
                
                char direction = std::any_cast<char>(pos_data["PosiDirection"]);
                int volume = std::any_cast<int>(pos_data["Position"]);
                double open_cost = std::any_cast<double>(pos_data["OpenCost"]);
                
                if (direction == '2') { // Long position
                    position.size = volume;
                } else if (direction == '3') { // Short position
                    position.size = -volume;
                }
                
                if (volume > 0) {
                    position.price = open_cost / volume;
                    position.value = position.size * position.price;
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

void CTPBroker::create_trade_from_order(std::shared_ptr<Order> order) {
    Trade trade;
    trade.id = "trade_" + std::to_string(next_trade_id_++);
    trade.order_id = order->id;
    trade.symbol = order->symbol;
    trade.size = order->executed_size;
    trade.price = order->executed_price;
    trade.value = std::abs(trade.size) * trade.price * getContractSize(trade.symbol);
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
}

void CTPBroker::update_position(const Trade& trade) {
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
    
    position.value = position.size * position.price * getContractSize(trade.symbol);
    position.last_update = trade.datetime;
    
    // Remove position if size is zero
    if (std::abs(position.size) < 1e-8) {
        positions_.erase(trade.symbol);
    }
}

std::string CTPBroker::getInstrumentFromData(std::shared_ptr<DataSeries> data, 
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
    return "IF2312"; // Default index future contract
}

std::vector<Trade> CTPBroker::get_trades() {
    return trades_;
}

std::map<std::string, Position> CTPBroker::get_positions() {
    return positions_;
}

void CTPBroker::notify(std::shared_ptr<Order> order) {
    // Notification callback for order status changes
    if (p.debug) {
        std::cout << "CTP Order notification: " << order->id 
                  << " status: " << static_cast<int>(order->status) << std::endl;
    }
}

} // namespace brokers
} // namespace backtrader