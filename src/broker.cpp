#include "broker.h"
#include "order.h"
#include "position.h"
#include "comminfo.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace backtrader {

BackBroker::BackBroker() : cash_(params.cash), value_(params.cash) {
}

void BackBroker::setcash(double cash) {
    cash_ = cash;
    value_ = cash;
}

double BackBroker::getcash() const {
    // For futures positions, Python includes unrealized P&L in cash value
    // but NOT the margin (margin is only included in value, not cash)
    double total_cash = cash_;
    
    // Add unrealized P&L for futures positions
    for (const auto& [data_ptr, position] : positions_) {
        if (position && position->size != 0) {
            // Get current price from data
            if (auto data = const_cast<DataSeries*>(data_ptr)) {
                double current_price = data->close(0);
                
                // Get commission info for this data
                std::shared_ptr<CommInfo> comminfo;
                
                // First try to find by matching raw pointer in data_refs_
                auto ref_it = data_refs_.find(data_ptr);
                if (ref_it != data_refs_.end() && ref_it->second) {
                    // Found the shared_ptr, now look for comminfo
                    auto comm_it = comminfo_.find(ref_it->second);
                    if (comm_it != comminfo_.end()) {
                        comminfo = comm_it->second;
                    } else if (default_comminfo_) {
                        comminfo = default_comminfo_;
                    }
                } else if (default_comminfo_) {
                    // No specific data ref found, use default
                    comminfo = default_comminfo_;
                }
                
                if (comminfo && !comminfo->stocklike) {
                    // For futures: add unrealized P&L to cash (but NOT margin)
                    double unrealized_pnl = (current_price - position->price) * position->size * comminfo->mult;
                    total_cash += unrealized_pnl;
                    
                    static int debug_count = 0;
                    if (++debug_count <= 5) {
                        std::cerr << "BackBroker::getcash() - FUTURES position: size=" << position->size 
                                  << ", entry_price=" << position->price
                                  << ", current_price=" << current_price
                                  << ", mult=" << comminfo->mult
                                  << ", unrealized_pnl=" << unrealized_pnl
                                  << ", raw_cash=" << cash_ 
                                  << ", total_cash=" << total_cash << std::endl;
                    }
                }
            }
        }
    }
    
    return total_cash;
}

double BackBroker::getvalue() const {
    // Portfolio value = cash + value of all positions
    double total_value = cash_;
    
    // Add value of all open positions
    for (const auto& [data_ptr, position] : positions_) {
        if (position && position->size != 0) {
            // Get current price from data
            if (auto data = const_cast<DataSeries*>(data_ptr)) {
                double current_price = data->close(0);
                
                // Get commission info for this data
                // Check if we have a data_ref that matches this data_ptr
                std::shared_ptr<CommInfo> comminfo;
                
                // First try to find by matching raw pointer in data_refs_
                auto ref_it = data_refs_.find(data_ptr);
                
                static int debug_count5 = 0;
                if (++debug_count5 <= 3) {
                    std::cerr << "BackBroker::getvalue() - data_refs_.size()=" << data_refs_.size() 
                              << ", looking for data_ptr=" << data_ptr;
                    if (ref_it != data_refs_.end()) {
                        std::cerr << " - FOUND in data_refs_, shared_ptr=" << ref_it->second.get();
                    } else {
                        std::cerr << " - NOT FOUND in data_refs_";
                    }
                    if (default_comminfo_) {
                        std::cerr << ", default_comminfo exists (stocklike=" 
                                  << default_comminfo_->stocklike << ")";
                    }
                    std::cerr << std::endl;
                }
                
                if (ref_it != data_refs_.end() && ref_it->second) {
                    // Found the shared_ptr, now look for comminfo
                    auto comm_it = comminfo_.find(ref_it->second);
                    if (comm_it != comminfo_.end()) {
                        comminfo = comm_it->second;
                    } else if (default_comminfo_) {
                        comminfo = default_comminfo_;
                    }
                } else if (default_comminfo_) {
                    // No specific data ref found, use default
                    comminfo = default_comminfo_;
                }
                
                double position_value = 0.0;
                if (comminfo) {
                    if (!comminfo->stocklike) {
                        // For futures: value includes margin AND unrealized P&L
                        // This matches Python's behavior
                        double unrealized_pnl = (current_price - position->price) * position->size * comminfo->mult;
                        double margin = std::abs(position->size) * comminfo->margin;
                        position_value = unrealized_pnl + margin;
                        
                        static int debug_count2 = 0;
                        if (++debug_count2 <= 5) {
                            std::cerr << "BackBroker::getvalue() - FUTURES position: size=" << position->size 
                                      << ", entry_price=" << position->price
                                      << ", current_price=" << current_price
                                      << ", mult=" << comminfo->mult
                                      << ", unrealized_pnl=" << unrealized_pnl
                                      << ", position_value=" << position_value << std::endl;
                        }
                    } else {
                        // For stocks: value = size * price
                        position_value = position->size * current_price;
                    }
                } else {
                    // Default to stock-like behavior
                    position_value = position->size * current_price;
                    
                    static int debug_count3 = 0;
                    if (++debug_count3 <= 5) {
                        std::cerr << "BackBroker::getvalue() - NO COMMINFO for data " << data_ptr 
                                  << ", using stock-like calculation" << std::endl;
                    }
                }
                
                total_value += position_value;
                
                static int debug_count = 0;
                if (++debug_count <= 5) {
                    std::cerr << "BackBroker::getvalue() - total_value=" << total_value 
                              << " (cash=" << cash_ << " + position_value=" << position_value << ")" << std::endl;
                }
            }
        }
    }
    
    return total_value;
}

std::shared_ptr<Position> BackBroker::getposition(std::shared_ptr<DataSeries> data) const {
    // std::cerr << "BackBroker::getposition - looking for data=" << data.get() << std::endl;
    // std::cerr << "BackBroker::getposition - positions_.size()=" << positions_.size() << std::endl;
    
    // Debug: print all position keys
    static int call_count = 0;
    if (++call_count % 10 == 0 || !positions_.empty()) {
        std::cerr << "BackBroker::getposition[" << call_count << "] - broker=" << this 
                  << ", Looking for: " << data.get() 
                  << ", positions_.size()=" << positions_.size() << std::endl;
        for (const auto& [key, pos] : positions_) {
            std::cerr << "  Key: " << key << ", Position size: " << pos->size << std::endl;
        }
    }
    
    auto it = positions_.find(data.get());
    if (it != positions_.end()) {
        std::cerr << "BackBroker::getposition - Found position for " << data.get() 
                  << ", size=" << it->second->size << std::endl;
        return it->second;
    }
    
    // std::cerr << "BackBroker::getposition - Position not found, returning empty" << std::endl;
    // Return empty position if not found
    auto position = std::make_shared<Position>();
    return position;
}

std::map<DataSeries*, std::shared_ptr<Position>> BackBroker::getpositions() const {
    return positions_;
}

std::shared_ptr<Order> BackBroker::submit(std::shared_ptr<Order> order) {
    std::cerr << "BackBroker::submit called, order size=" << order->size << std::endl;
    if (params.checksubmit && !_check_cash(order)) {
        order->status = OrderStatus::Rejected;
        std::cerr << "BackBroker::submit - Order rejected (cash check failed)" << std::endl;
        return order;
    }
    
    order->status = OrderStatus::Submitted;
    // Add to new_orders instead of pending_orders
    // These will be moved to pending_orders at the start of next bar
    new_orders_.push_back(order);
    orders_.push_back(order);
    
    std::cerr << "BackBroker::submit - Order submitted to new_orders, new_orders.size()=" << new_orders_.size() << std::endl;
    
    return order;
}

bool BackBroker::cancel(std::shared_ptr<Order> order) {
    // Check pending orders first
    auto it = std::find(pending_orders_.begin(), pending_orders_.end(), order);
    if (it != pending_orders_.end()) {
        pending_orders_.erase(it);
        order->status = OrderStatus::Canceled;
        return true;
    }
    
    // Also check new orders
    it = std::find(new_orders_.begin(), new_orders_.end(), order);
    if (it != new_orders_.end()) {
        new_orders_.erase(it);
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
    
    // For futures/forex (when margin > 0 and mult > 1), set stocklike to false
    if (margin > 0.0 && mult > 1.0) {
        comminfo->stocklike = false;
        comminfo->commtype = true;  // Fixed commission for futures
    }
    
    if (data) {
        comminfo_[data] = comminfo;
    } else {
        // Set as default for all data
        default_comminfo_ = comminfo;
    }
}

std::shared_ptr<CommInfo> BackBroker::getcommissioninfo(std::shared_ptr<DataSeries> data) const {
    auto it = comminfo_.find(data);
    if (it != comminfo_.end()) {
        // Found specific data CommInfo
        return it->second;
    }
    
    // Check for default comminfo
    if (default_comminfo_) {
        return default_comminfo_;
    }
    
    // Return default comminfo (stocklike=true)
    std::cerr << "BackBroker::getcommissioninfo - No CommInfo found, returning basic default" << std::endl;
    return std::make_shared<CommInfo>();
}

void BackBroker::next() {
    std::cerr << "BackBroker::next() called - pending_orders.size()=" << pending_orders_.size() 
              << ", new_orders.size()=" << new_orders_.size() << std::endl;
    
    // CRITICAL FIX: Move new orders to pending BEFORE processing
    // Orders submitted in bar N-1 should be executed in bar N
    if (!new_orders_.empty()) {
        std::cerr << "BackBroker::next() - Moving " << new_orders_.size() 
                  << " new orders to pending queue for execution THIS bar" << std::endl;
        pending_orders_.insert(pending_orders_.end(), new_orders_.begin(), new_orders_.end());
        new_orders_.clear();
    }
    
    // Now process the pending orders (includes orders from previous bar)
    _process_orders();
    
    _update_value();
}

void BackBroker::start() {
    // Initialize broker for new run
    pending_orders_.clear();
    new_orders_.clear();
    orders_.clear();
}

void BackBroker::stop() {
    // Cleanup after run
}

std::shared_ptr<Order> BackBroker::get_notification() {
    if (!notifications_.empty()) {
        auto order = notifications_.front();
        notifications_.erase(notifications_.begin());
        return order;
    }
    return nullptr;
}

bool BackBroker::has_notifications() const {
    bool has_notif = !notifications_.empty();
    if (has_notif) {
        std::cerr << "BackBroker::has_notifications() = true, size=" << notifications_.size() << std::endl;
    }
    return has_notif;
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
    std::cerr << "BackBroker::execute_order called, order size=" << order->size 
              << ", executed.size=" << order->executed.size << std::endl;
    if (!order->data || order->data->empty()) {
        std::cerr << "BackBroker::execute_order - No data or data is empty" << std::endl;
        return false;
    }
    
    std::cerr << "BackBroker::execute_order - about to call _get_order_price, ago=" << ago << std::endl;
    double price = _get_order_price(order, ago);
    std::cerr << "BackBroker::execute_order - _get_order_price returned " << price << std::endl;
    if (price <= 0.0 || std::isnan(price)) {
        return false;
    }
    
    if (!_can_execute(order, price, ago)) {
        std::cerr << "BackBroker::execute_order - _can_execute returned false" << std::endl;
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
    
    std::cerr << "BackBroker::execute_order - exec_size=" << exec_size 
              << " (order->size=" << order->size 
              << ", executed.size=" << order->executed.size << ")" << std::endl;
    
    // For sell orders, exec_size will be negative
    if (exec_size != 0.0) {
        _execute_order(order, slipped_price, exec_size);
        return true;
    }
    
    return false;
}

void BackBroker::_process_orders() {
    // std::cerr << "BackBroker::_process_orders() called with " << pending_orders_.size() << " pending orders" << std::endl;
    auto it = pending_orders_.begin();
    while (it != pending_orders_.end()) {
        auto order = *it;
        
        // std::cerr << "Processing order: size=" << order->size << ", status=" << static_cast<int>(order->status) << std::endl;
        
        if (execute_order(order)) {
            // std::cerr << "Order executed successfully" << std::endl;
            // For sell orders, both size and executed.size are negative
            std::cerr << "Order size check: executed.size=" << order->executed.size 
                      << ", order->size=" << order->size 
                      << ", abs comparison: " << std::abs(order->executed.size) 
                      << " >= " << std::abs(order->size) << std::endl;
            if (std::abs(order->executed.size) >= std::abs(order->size)) {
                order->status = OrderStatus::Completed;
                it = pending_orders_.erase(it);
            } else {
                order->status = OrderStatus::Partial;
                ++it;
            }
        } else {
            // std::cerr << "Order execution failed" << std::endl;
            ++it;
        }
    }
}

bool BackBroker::_check_cash(std::shared_ptr<Order> order) {
    // Get commission info for this data
    auto comminfo = getcommissioninfo(order->data);
    double price = _get_order_price(order, 0);
    
    double required_cash;
    if (comminfo && !comminfo->stocklike && comminfo->margin > 0) {
        // Futures mode: only need margin per contract
        required_cash = std::abs(order->size) * comminfo->margin;
    } else {
        // Stock mode: need full amount
        required_cash = std::abs(order->size) * price;
    }
    
    // Add commission
    double commission = comminfo ? comminfo->getcommission(order->size, price) : 0.0;
    required_cash += commission;
    
    return cash_ >= required_cash;
}

void BackBroker::_update_value() {
    double old_value = value_;
    value_ = cash_;
    
    // Debug output to track value changes
    static int update_count = 0;
    bool has_positions = false;
    
    // Add position values
    for (const auto& [data_ptr, position] : positions_) {
        if (position && position->size != 0.0) {
            has_positions = true;
            // Find the data series reference
            auto data_ref_it = data_refs_.find(data_ptr);
            if (data_ref_it != data_refs_.end() && data_ref_it->second) {
                // Get current close price
                double current_price = data_ref_it->second->close(0);
                if (!std::isnan(current_price)) {
                    // Get commission info to check if futures
                    auto comminfo = getcommissioninfo(data_ref_it->second);
                    
                    // Calculate position value
                    double position_value;
                    if (comminfo && !comminfo->stocklike && comminfo->margin > 0) {
                        // Futures: value is just the unrealized P&L
                        // The margin is not part of the position value - it's already deducted from cash
                        double unrealized_pnl = (current_price - position->price) * position->size * comminfo->mult;
                        position_value = unrealized_pnl;
                    } else {
                        // Stock: value = size * current_price
                        position_value = position->size * current_price;
                    }
                    value_ += position_value;
                    
                    // Debug output
                    static int debug_count = 0;
                    if (debug_count++ % 50 == 0) {
                        std::cerr << "BackBroker::_update_value - Position size=" << position->size 
                                  << ", entry_price=" << position->price
                                  << ", current_price=" << current_price 
                                  << ", mult=" << (comminfo ? comminfo->mult : 1.0)
                                  << ", position_value=" << position_value
                                  << ", total_value=" << value_ << std::endl;
                    }
                }
            }
        }
    }
    
    // Debug unexpected value changes
    if (update_count++ < 10 && !has_positions && std::abs(value_ - old_value) > 0.01) {
        std::cerr << "BackBroker::_update_value - WARNING: Value changed without positions! "
                  << "old_value=" << old_value << ", new_value=" << value_ 
                  << ", cash=" << cash_ << ", positions.size()=" << positions_.size() << std::endl;
    }
}

double BackBroker::_get_order_price(std::shared_ptr<Order> order, double ago) {
    if (!order->data || order->data->empty()) {
        return 0.0;
    }
    
    double price = 0.0;
    switch (order->type) {
        case OrderType::Market: {
            price = (*order->data->lines->getline(DataSeries::Open))[static_cast<int>(ago)];
            // Debug: Also print close price to verify which bar we're on
            double close_price = (*order->data->lines->getline(DataSeries::Close))[static_cast<int>(ago)];
            std::cerr << "BackBroker::_get_order_price - Market order, ago=" << ago 
                      << ", open price=" << price 
                      << ", close price=" << close_price 
                      << ", data ptr=" << order->data.get() << std::endl;
            break;
        }
        case OrderType::Close:
            price = (*order->data->lines->getline(DataSeries::Close))[static_cast<int>(ago)];
            break;
        case OrderType::Limit:
        case OrderType::Stop:
        case OrderType::StopLimit:
            price = order->price;
            break;
        default:
            price = (*order->data->lines->getline(DataSeries::Close))[static_cast<int>(ago)];
            break;
    }
    return price;
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
    // Commission calculated
    order->executed.comm += commission;
    
    // Update cash
    double old_cash = cash_;
    
    if (comminfo && !comminfo->stocklike && comminfo->margin > 0) {
        // Futures mode
        cash_ -= commission;  // Commission is always a cost
        
        // For futures, we need to handle margin and realized P&L
        auto& position = positions_[order->data.get()];
        double old_position_size = position ? position->size : 0.0;
        double new_position_size = old_position_size + size;
        
        if (old_position_size != 0.0) {
            // Check if this is a closing trade (opposite direction)
            bool is_closing = (old_position_size > 0 && size < 0) || (old_position_size < 0 && size > 0);
            if (is_closing) {
                // Calculate realized P&L for the closed portion
                double closed_size = std::min(std::abs(old_position_size), std::abs(size));
                double pnl = (price - position->price) * closed_size * comminfo->mult;
                if (old_position_size < 0) {
                    pnl = -pnl;  // Short position P&L is reversed
                }
                cash_ += pnl;
                
                // Return margin for closed portion
                double margin_returned = closed_size * comminfo->margin;
                cash_ += margin_returned;
                
                std::cerr << "BackBroker::_execute_order [FUTURES CLOSE] - "
                          << "closed_size=" << closed_size 
                          << ", entry_price=" << position->price
                          << ", exit_price=" << price
                          << ", P&L=" << pnl 
                          << ", margin_returned=" << margin_returned 
                          << ", cash before=" << (cash_ - pnl - margin_returned)
                          << ", cash after=" << cash_ << std::endl;
            }
        }
        
        // If opening or increasing position, deduct margin for new contracts
        if (std::abs(new_position_size) > std::abs(old_position_size)) {
            double margin_needed = (std::abs(new_position_size) - std::abs(old_position_size)) * comminfo->margin;
            cash_ -= margin_needed;
            std::cerr << "BackBroker::_execute_order [FUTURES OPEN] - margin_needed=" << margin_needed 
                      << ", cash before=" << (cash_ + margin_needed)
                      << ", cash after=" << cash_ << std::endl;
        }
        
        // Debug output
        std::cerr << "BackBroker::_execute_order [FUTURES] - size=" << size 
                  << ", price=" << price << ", commission=" << commission
                  << ", cash: " << old_cash << " -> " << cash_ << std::endl;
    } else {
        // Stock mode
        double cash_change = std::abs(size) * price;
        if (order->isbuy()) {
            cash_ -= (cash_change + commission);
        } else {
            cash_ += (cash_change - commission);
        }
        
        std::cerr << "BackBroker::_execute_order [STOCK] - size=" << size 
                  << ", price=" << price << ", value=" << cash_change 
                  << ", commission=" << commission
                  << ", cash: " << old_cash << " -> " << cash_ << std::endl;
    }
    
    // Update position
    _update_position(order, size, price);
    
    // Add to notification queue
    notifications_.push_back(order);
}

void BackBroker::_update_position(std::shared_ptr<Order> order, double size, double price) {
    std::cerr << "BackBroker::_update_position - broker=" << this 
              << ", order->data=" << order->data.get() << ", size=" << size << std::endl;
    
    auto& position = positions_[order->data.get()];
    if (!position) {
        position = std::make_shared<Position>();
        std::cerr << "BackBroker::_update_position - Created new position, positions_.size()=" << positions_.size() 
                  << ", initial size=" << position->size << std::endl;
    }
    
    // Store data reference for portfolio value calculation
    if (data_refs_.find(order->data.get()) == data_refs_.end()) {
        data_refs_[order->data.get()] = order->data;
    }
    
    // For both buy and sell orders, pass size directly
    // Buy orders: size is positive (e.g., 1.0)
    // Sell orders: size is negative (e.g., -1.0)
    std::cerr << "BackBroker::_update_position - Before update: position->size=" << position->size 
              << ", calling update(" << size << ", " << price << ")" << std::endl;
    
    // Make sure we're calling the right update method
    Position* pos_ptr = position.get();
    std::cerr << "BackBroker::_update_position - Position pointer: " << pos_ptr << std::endl;
    
    position->update(size, price);
    
    std::cerr << "BackBroker::_update_position - After update, position size=" << position->size << std::endl;
}

} // namespace backtrader