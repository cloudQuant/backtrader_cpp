#include "../../include/observers/buysell.h"
#include <algorithm>

namespace backtrader {
namespace observers {

BuySell::BuySell(const Params& params) : p(params) {
    // Initialize lines for buy/sell markers
    lines.resize(2);
    lines[0].clear(); // buy signals
    lines[1].clear(); // sell signals
    
    // Set line names
    line_names.resize(2);
    line_names[0] = "buy";
    line_names[1] = "sell";
}

void BuySell::prenext() {
    // Called before next() during minimum period
    next();
}

void BuySell::next() {
    // Initialize with NaN (no signal)
    double buy_signal = std::numeric_limits<double>::quiet_NaN();
    double sell_signal = std::numeric_limits<double>::quiet_NaN();
    
    // Check if there are any orders to display
    if (!orders_.empty()) {
        // Process orders for current bar
        for (const auto& order : orders_) {
            if (order && order->status == OrderStatus::Completed) {
                // Check if this order was executed on current bar
                // This is simplified - in real implementation would check datetime
                
                if (order->size > 0) {
                    // Buy order
                    buy_signal = order->executed_price;
                } else if (order->size < 0) {
                    // Sell order
                    sell_signal = order->executed_price;
                }
            }
        }
        
        // Clear processed orders (simplified)
        orders_.clear();
    }
    
    // Add to lines
    lines[0].push_back(buy_signal);
    lines[1].push_back(sell_signal);
}

void BuySell::notify_order(std::shared_ptr<Order> order) {
    if (!order) {
        return;
    }
    
    // Store order for processing in next()
    if (order->status == OrderStatus::Completed) {
        orders_.push_back(order);
        
        // Also add to order history
        BuySellPoint point;
        point.order = order;
        point.bar_index = lines[0].size(); // Current bar index
        point.price = order->executed_price;
        point.size = order->executed_size;
        point.is_buy = order->size > 0;
        point.datetime = order->executed_time;
        
        order_history_.push_back(point);
    }
}

std::map<std::string, double> BuySell::get_analysis() const {
    std::map<std::string, double> analysis;
    
    int total_buys = 0;
    int total_sells = 0;
    double total_buy_volume = 0.0;
    double total_sell_volume = 0.0;
    double avg_buy_price = 0.0;
    double avg_sell_price = 0.0;
    
    for (const auto& point : order_history_) {
        if (point.is_buy) {
            total_buys++;
            total_buy_volume += std::abs(point.size);
            avg_buy_price += point.price * std::abs(point.size);
        } else {
            total_sells++;
            total_sell_volume += std::abs(point.size);
            avg_sell_price += point.price * std::abs(point.size);
        }
    }
    
    analysis["total_trades"] = static_cast<double>(order_history_.size());
    analysis["total_buys"] = static_cast<double>(total_buys);
    analysis["total_sells"] = static_cast<double>(total_sells);
    analysis["total_buy_volume"] = total_buy_volume;
    analysis["total_sell_volume"] = total_sell_volume;
    
    if (total_buy_volume > 0.0) {
        analysis["avg_buy_price"] = avg_buy_price / total_buy_volume;
    }
    
    if (total_sell_volume > 0.0) {
        analysis["avg_sell_price"] = avg_sell_price / total_sell_volume;
    }
    
    // Calculate buy/sell balance
    analysis["buy_sell_ratio"] = (total_buys > 0) ? 
        static_cast<double>(total_sells) / static_cast<double>(total_buys) : 0.0;
    
    // Calculate round trips (buy followed by sell)
    int round_trips = std::min(total_buys, total_sells);
    analysis["round_trips"] = static_cast<double>(round_trips);
    
    return analysis;
}

std::vector<BuySell::BuySellPoint> BuySell::get_buy_points() const {
    std::vector<BuySellPoint> buy_points;
    
    for (const auto& point : order_history_) {
        if (point.is_buy) {
            buy_points.push_back(point);
        }
    }
    
    return buy_points;
}

std::vector<BuySell::BuySellPoint> BuySell::get_sell_points() const {
    std::vector<BuySellPoint> sell_points;
    
    for (const auto& point : order_history_) {
        if (!point.is_buy) {
            sell_points.push_back(point);
        }
    }
    
    return sell_points;
}

std::vector<BuySell::BuySellPoint> BuySell::get_all_points() const {
    return order_history_;
}

void BuySell::reset() {
    orders_.clear();
    order_history_.clear();
    
    for (auto& line : lines) {
        line.clear();
    }
}

std::vector<double> BuySell::get_trade_prices() const {
    std::vector<double> prices;
    prices.reserve(order_history_.size());
    
    for (const auto& point : order_history_) {
        prices.push_back(point.price);
    }
    
    return prices;
}

std::vector<double> BuySell::get_trade_sizes() const {
    std::vector<double> sizes;
    sizes.reserve(order_history_.size());
    
    for (const auto& point : order_history_) {
        sizes.push_back(point.size);
    }
    
    return sizes;
}

double BuySell::get_last_buy_price() const {
    for (auto it = order_history_.rbegin(); it != order_history_.rend(); ++it) {
        if (it->is_buy) {
            return it->price;
        }
    }
    return 0.0;
}

double BuySell::get_last_sell_price() const {
    for (auto it = order_history_.rbegin(); it != order_history_.rend(); ++it) {
        if (!it->is_buy) {
            return it->price;
        }
    }
    return 0.0;
}

std::pair<double, double> BuySell::get_avg_buy_sell_prices() const {
    double total_buy_value = 0.0;
    double total_buy_size = 0.0;
    double total_sell_value = 0.0;
    double total_sell_size = 0.0;
    
    for (const auto& point : order_history_) {
        if (point.is_buy) {
            total_buy_value += point.price * std::abs(point.size);
            total_buy_size += std::abs(point.size);
        } else {
            total_sell_value += point.price * std::abs(point.size);
            total_sell_size += std::abs(point.size);
        }
    }
    
    double avg_buy = (total_buy_size > 0.0) ? total_buy_value / total_buy_size : 0.0;
    double avg_sell = (total_sell_size > 0.0) ? total_sell_value / total_sell_size : 0.0;
    
    return std::make_pair(avg_buy, avg_sell);
}

} // namespace observers
} // namespace backtrader