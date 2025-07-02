#pragma once

#include "broker/Broker.h"
#include "Trade.h"
#include <memory>
#include <queue>
#include <map>

namespace backtrader {

/**
 * @brief Simple simulation broker for backtesting
 * 
 * Implements basic order execution with configurable commission,
 * slippage, and market impact models. Suitable for most backtesting
 * scenarios where realistic execution is needed.
 */
class SimpleBroker : public Broker {
public:
    /**
     * @brief Commission structure
     */
    struct Commission {
        enum Type {
            FIXED,       // Fixed amount per trade
            PERCENTAGE,  // Percentage of trade value
            PER_SHARE,   // Fixed amount per share
            TIERED       // Tiered based on volume
        };
        
        Type type = PERCENTAGE;
        double rate = 0.001;  // 0.1% default
        double minimum = 0.0;
        double maximum = std::numeric_limits<double>::max();
        
        // Tiered commission (for TIERED type)
        std::vector<std::pair<double, double>> tiers;  // {volume_threshold, rate}
    };
    
    /**
     * @brief Slippage model
     */
    struct Slippage {
        enum Type {
            NONE,        // No slippage
            FIXED,       // Fixed amount
            PERCENTAGE,  // Percentage of price
            VOLUME_BASED // Based on order size vs average volume
        };
        
        Type type = PERCENTAGE;
        double rate = 0.0005;  // 0.05% default
        double max_slippage = 0.01;  // Maximum 1% slippage
    };

private:
    Commission commission_;
    Slippage slippage_;
    
    // Portfolio state
    double cash_;
    double initial_cash_;
    std::map<std::string, double> positions_;  // symbol -> quantity
    std::map<std::string, double> avg_prices_; // symbol -> average entry price
    
    // Order management
    std::queue<std::shared_ptr<Order>> pending_orders_;
    std::vector<std::shared_ptr<Order>> order_history_;
    
    // Trade tracking
    TradeHistory trade_history_;
    std::vector<std::shared_ptr<Trade>> open_trades_;
    
    // Market data
    std::map<std::string, double> current_prices_;
    std::map<std::string, double> current_volumes_;
    
    // Performance tracking
    std::vector<double> portfolio_values_;
    double max_portfolio_value_;
    double max_drawdown_;
    
    // Risk management
    double max_position_size_;
    double max_portfolio_risk_;
    bool enable_margin_;
    double margin_ratio_;

public:
    explicit SimpleBroker(double initial_cash = 100000.0,
                         const Commission& commission = Commission{},
                         const Slippage& slippage = Slippage{})
        : Broker("SimpleBroker"),
          commission_(commission),
          slippage_(slippage),
          cash_(initial_cash),
          initial_cash_(initial_cash),
          max_portfolio_value_(initial_cash),
          max_drawdown_(0.0),
          max_position_size_(0.5),  // 50% max position
          max_portfolio_risk_(0.02), // 2% max risk per trade
          enable_margin_(false),
          margin_ratio_(2.0) {}
    
    /**
     * @brief Submit order to broker
     */
    std::shared_ptr<Order> submitOrder(OrderType type, const std::string& symbol, 
                                      double quantity, double price = 0.0) override {
        auto order = std::make_shared<Order>(type, symbol, quantity, price);
        
        // Validate order
        if (!validateOrder(order)) {
            order->setStatus(Order::Status::REJECTED);
            return order;
        }
        
        // Add to pending orders
        pending_orders_.push(order);
        order->setStatus(Order::Status::SUBMITTED);
        order_history_.push_back(order);
        
        return order;
    }
    
    /**
     * @brief Process market data update
     */
    void updateMarketData(const std::string& symbol, double price, double volume = 0.0) {
        current_prices_[symbol] = price;
        current_volumes_[symbol] = volume;
        
        // Process pending orders
        processPendingOrders();
        
        // Update portfolio value
        updatePortfolioValue();
    }
    
    /**
     * @brief Get current cash balance
     */
    double getCash() const override {
        return cash_;
    }
    
    /**
     * @brief Get current portfolio value
     */
    double getPortfolioValue() const override {
        double value = cash_;
        
        for (const auto& [symbol, quantity] : positions_) {
            auto it = current_prices_.find(symbol);
            if (it != current_prices_.end()) {
                value += quantity * it->second;
            }
        }
        
        return value;
    }
    
    /**
     * @brief Get position for symbol
     */
    double getPosition(const std::string& symbol) const override {
        auto it = positions_.find(symbol);
        return (it != positions_.end()) ? it->second : 0.0;
    }
    
    /**
     * @brief Get all positions
     */
    const std::map<std::string, double>& getAllPositions() const {
        return positions_;
    }
    
    /**
     * @brief Get trade history
     */
    const TradeHistory& getTradeHistory() const {
        return trade_history_;
    }
    
    /**
     * @brief Get order history
     */
    const std::vector<std::shared_ptr<Order>>& getOrderHistory() const {
        return order_history_;
    }
    
    /**
     * @brief Get portfolio value history
     */
    const std::vector<double>& getPortfolioHistory() const {
        return portfolio_values_;
    }
    
    /**
     * @brief Get maximum drawdown
     */
    double getMaxDrawdown() const {
        return max_drawdown_;
    }
    
    /**
     * @brief Set commission structure
     */
    void setCommission(const Commission& commission) {
        commission_ = commission;
    }
    
    /**
     * @brief Set slippage model
     */
    void setSlippage(const Slippage& slippage) {
        slippage_ = slippage;
    }
    
    /**
     * @brief Enable/disable margin trading
     */
    void setMarginEnabled(bool enabled, double ratio = 2.0) {
        enable_margin_ = enabled;
        margin_ratio_ = ratio;
    }
    
    /**
     * @brief Set risk management parameters
     */
    void setRiskManagement(double max_position_size, double max_portfolio_risk) {
        max_position_size_ = max_position_size;
        max_portfolio_risk_ = max_portfolio_risk;
    }
    
    /**
     * @brief Get performance statistics
     */
    struct PerformanceStats {
        double total_return;
        double max_drawdown;
        double sharpe_ratio;
        double win_rate;
        int total_trades;
        int winning_trades;
        double avg_win;
        double avg_loss;
        double profit_factor;
    };
    
    PerformanceStats getPerformanceStats() const {
        PerformanceStats stats{};
        
        double current_value = getPortfolioValue();
        stats.total_return = (current_value - initial_cash_) / initial_cash_;
        stats.max_drawdown = max_drawdown_;
        
        // Calculate trade statistics
        auto trades = trade_history_.getTrades();
        stats.total_trades = trades.size();
        
        double total_pnl = 0.0;
        double total_wins = 0.0;
        double total_losses = 0.0;
        
        for (const auto& trade : trades) {
            if (trade.isClosed()) {
                double pnl = trade.getPnLComm();
                total_pnl += pnl;
                
                if (pnl > 0) {
                    stats.winning_trades++;
                    total_wins += pnl;
                } else {
                    total_losses += std::abs(pnl);
                }
            }
        }
        
        if (stats.total_trades > 0) {
            stats.win_rate = static_cast<double>(stats.winning_trades) / stats.total_trades;
            stats.avg_win = (stats.winning_trades > 0) ? total_wins / stats.winning_trades : 0.0;
            stats.avg_loss = (stats.total_trades - stats.winning_trades > 0) ? 
                            total_losses / (stats.total_trades - stats.winning_trades) : 0.0;
            stats.profit_factor = (stats.avg_loss > 0) ? stats.avg_win / stats.avg_loss : 0.0;
        }
        
        // Simplified Sharpe ratio calculation
        if (portfolio_values_.size() > 1) {
            std::vector<double> returns;
            for (size_t i = 1; i < portfolio_values_.size(); ++i) {
                double ret = (portfolio_values_[i] - portfolio_values_[i-1]) / portfolio_values_[i-1];
                returns.push_back(ret);
            }
            
            if (!returns.empty()) {
                double mean_return = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
                double variance = 0.0;
                for (double ret : returns) {
                    variance += std::pow(ret - mean_return, 2);
                }
                variance /= returns.size();
                double std_dev = std::sqrt(variance);
                
                stats.sharpe_ratio = (std_dev > 0) ? mean_return / std_dev * std::sqrt(252) : 0.0;
            }
        }
        
        return stats;
    }

private:
    bool validateOrder(std::shared_ptr<Order> order) {
        // Check if we have current price for the symbol
        if (current_prices_.find(order->getSymbol()) == current_prices_.end()) {
            return false;
        }
        
        double current_price = current_prices_[order->getSymbol()];
        double order_value = std::abs(order->getQuantity()) * current_price;
        
        // Check available cash for buy orders
        if (order->getQuantity() > 0) {  // Buy order
            double required_cash = order_value + calculateCommission(order_value);
            
            if (!enable_margin_ && required_cash > cash_) {
                return false;
            }
            
            // Check maximum position size
            double current_value = getPortfolioValue();
            if (order_value > current_value * max_position_size_) {
                return false;
            }
        }
        
        // Check available position for sell orders
        if (order->getQuantity() < 0) {  // Sell order
            double current_position = getPosition(order->getSymbol());
            if (std::abs(order->getQuantity()) > current_position) {
                return false;  // Cannot sell more than we own
            }
        }
        
        return true;
    }
    
    void processPendingOrders() {
        std::queue<std::shared_ptr<Order>> remaining_orders;
        
        while (!pending_orders_.empty()) {
            auto order = pending_orders_.front();
            pending_orders_.pop();
            
            if (executeOrder(order)) {
                order->setStatus(Order::Status::FILLED);
            } else {
                remaining_orders.push(order);
            }
        }
        
        pending_orders_ = remaining_orders;
    }
    
    bool executeOrder(std::shared_ptr<Order> order) {
        const std::string& symbol = order->getSymbol();
        double quantity = order->getQuantity();
        
        auto price_it = current_prices_.find(symbol);
        if (price_it == current_prices_.end()) {
            return false;
        }
        
        double execution_price = price_it->second;
        
        // Apply slippage
        execution_price = applySlippage(execution_price, quantity);
        
        double trade_value = std::abs(quantity) * execution_price;
        double commission = calculateCommission(trade_value);
        
        // Update cash
        if (quantity > 0) {  // Buy
            cash_ -= trade_value + commission;
        } else {  // Sell
            cash_ += trade_value - commission;
        }
        
        // Update position
        positions_[symbol] += quantity;
        if (positions_[symbol] == 0.0) {
            positions_.erase(symbol);
            avg_prices_.erase(symbol);
        } else {
            // Update average price for position
            updateAveragePrice(symbol, quantity, execution_price);
        }
        
        // Create trade record
        createTrade(order, execution_price, commission);
        
        return true;
    }
    
    double applySlippage(double price, double quantity) {
        if (slippage_.type == Slippage::NONE) {
            return price;
        }
        
        double slippage_amount = 0.0;
        
        switch (slippage_.type) {
            case Slippage::FIXED:
                slippage_amount = slippage_.rate;
                break;
                
            case Slippage::PERCENTAGE:
                slippage_amount = price * slippage_.rate;
                break;
                
            case Slippage::VOLUME_BASED:
                // Simplified volume-based slippage
                slippage_amount = price * slippage_.rate * std::min(1.0, std::abs(quantity) / 1000.0);
                break;
                
            default:
                break;
        }
        
        // Apply maximum slippage limit
        slippage_amount = std::min(slippage_amount, price * slippage_.max_slippage);
        
        // Slippage is adverse to the trade direction
        if (quantity > 0) {  // Buy order - price goes up
            return price + slippage_amount;
        } else {  // Sell order - price goes down
            return price - slippage_amount;
        }
    }
    
    double calculateCommission(double trade_value) {
        switch (commission_.type) {
            case Commission::FIXED:
                return commission_.rate;
                
            case Commission::PERCENTAGE:
                return std::max(commission_.minimum, 
                              std::min(commission_.maximum, trade_value * commission_.rate));
                
            case Commission::PER_SHARE:
                // Would need share count, simplified to percentage for now
                return trade_value * commission_.rate;
                
            case Commission::TIERED:
                // Simplified tiered commission
                for (const auto& [threshold, rate] : commission_.tiers) {
                    if (trade_value <= threshold) {
                        return trade_value * rate;
                    }
                }
                return trade_value * commission_.rate;
                
            default:
                return 0.0;
        }
    }
    
    void updateAveragePrice(const std::string& symbol, double quantity, double price) {
        double current_position = positions_[symbol] - quantity;  // Position before this trade
        double current_avg = avg_prices_[symbol];
        
        if (current_position == 0.0) {
            avg_prices_[symbol] = price;
        } else if ((current_position > 0 && quantity > 0) || (current_position < 0 && quantity < 0)) {
            // Adding to position
            double total_cost = current_position * current_avg + quantity * price;
            avg_prices_[symbol] = total_cost / positions_[symbol];
        }
        // For reducing position, keep the same average price
    }
    
    void createTrade(std::shared_ptr<Order> order, double execution_price, double commission) {
        // For simplicity, create one trade per order
        // In reality, orders might be partially filled over multiple trades
        
        bool is_long = order->getQuantity() > 0;
        double size = std::abs(order->getQuantity());
        
        Trade trade(is_long, size, execution_price, static_cast<int>(order_history_.size()), order->getSymbol());
        trade.close(execution_price, static_cast<int>(order_history_.size()), commission);
        
        trade_history_.addTrade(trade);
    }
    
    void updatePortfolioValue() {
        double current_value = getPortfolioValue();
        portfolio_values_.push_back(current_value);
        
        // Update maximum portfolio value and drawdown
        if (current_value > max_portfolio_value_) {
            max_portfolio_value_ = current_value;
        } else {
            double drawdown = (max_portfolio_value_ - current_value) / max_portfolio_value_;
            max_drawdown_ = std::max(max_drawdown_, drawdown);
        }
    }
};

} // namespace backtrader