#pragma once

#include "Common.h"
#include "Order.h"
#include <string>
#include <vector>
#include <memory>

namespace backtrader {

/**
 * @brief Represents a completed trade consisting of entry and exit orders
 * 
 * A trade is formed when a position is opened and then closed, either partially
 * or completely. The trade tracks the profit/loss, commission, and duration.
 */
class Trade {
public:
    enum Status {
        Open,
        Closed
    };

private:
    Status status_;
    bool is_long_;
    double size_;
    double entry_price_;
    double exit_price_;
    double pnl_;
    double commission_;
    double pnl_comm_;
    int entry_bar_;
    int exit_bar_;
    int bar_len_;
    bool just_opened_;
    bool just_closed_;
    std::string data_name_;
    
    // Trade components
    std::vector<std::shared_ptr<Order>> entry_orders_;
    std::vector<std::shared_ptr<Order>> exit_orders_;
    
public:
    /**
     * @brief Constructor for a new trade
     * @param is_long True for long position, false for short
     * @param size Position size
     * @param entry_price Entry price
     * @param entry_bar Bar number when position was opened
     * @param data_name Name of the data feed
     */
    Trade(bool is_long, double size, double entry_price, int entry_bar, const std::string& data_name = "")
        : status_(Open),
          is_long_(is_long),
          size_(size),
          entry_price_(entry_price),
          exit_price_(NaN),
          pnl_(0.0),
          commission_(0.0),
          pnl_comm_(0.0),
          entry_bar_(entry_bar),
          exit_bar_(-1),
          bar_len_(0),
          just_opened_(true),
          just_closed_(false),
          data_name_(data_name) {}
    
    /**
     * @brief Close the trade
     * @param exit_price Exit price
     * @param exit_bar Bar number when position was closed
     * @param commission Total commission for the trade
     */
    void close(double exit_price, int exit_bar, double commission = 0.0) {
        if (status_ == Closed) {
            return; // Already closed
        }
        
        status_ = Closed;
        exit_price_ = exit_price;
        exit_bar_ = exit_bar;
        bar_len_ = exit_bar - entry_bar_ + 1;
        commission_ = commission;
        just_closed_ = true;
        just_opened_ = false;
        
        // Calculate PnL
        if (is_long_) {
            pnl_ = (exit_price_ - entry_price_) * size_;
        } else {
            pnl_ = (entry_price_ - exit_price_) * size_;
        }
        
        pnl_comm_ = pnl_ - commission_;
    }
    
    /**
     * @brief Update trade for current bar (called each bar while trade is open)
     * @param current_bar Current bar number
     */
    void update(int current_bar) {
        if (status_ == Open) {
            bar_len_ = current_bar - entry_bar_ + 1;
        }
        just_opened_ = false;
        just_closed_ = false;
    }
    
    // Getters
    Status getStatus() const { return status_; }
    bool isOpen() const { return status_ == Open; }
    bool isClosed() const { return status_ == Closed; }
    bool isLong() const { return is_long_; }
    bool isShort() const { return !is_long_; }
    double getSize() const { return size_; }
    double getEntryPrice() const { return entry_price_; }
    double getExitPrice() const { return exit_price_; }
    double getPnL() const { return pnl_; }
    double getCommission() const { return commission_; }
    double getPnLComm() const { return pnl_comm_; }
    int getEntryBar() const { return entry_bar_; }
    int getExitBar() const { return exit_bar_; }
    int getBarLen() const { return bar_len_; }
    bool justOpened() const { return just_opened_; }
    bool justClosed() const { return just_closed_; }
    const std::string& getDataName() const { return data_name_; }
    
    /**
     * @brief Calculate current unrealized PnL for open trades
     * @param current_price Current market price
     * @return Unrealized PnL
     */
    double getUnrealizedPnL(double current_price) const {
        if (status_ == Closed) {
            return pnl_comm_;
        }
        
        if (isNaN(current_price)) {
            return 0.0;
        }
        
        double unrealized_pnl;
        if (is_long_) {
            unrealized_pnl = (current_price - entry_price_) * size_;
        } else {
            unrealized_pnl = (entry_price_ - current_price) * size_;
        }
        
        return unrealized_pnl - commission_;
    }
    
    /**
     * @brief Get trade return as percentage
     * @return Trade return in percentage
     */
    double getReturn() const {
        if (status_ != Closed || entry_price_ == 0.0) {
            return 0.0;
        }
        
        return (pnl_ / (entry_price_ * std::abs(size_))) * 100.0;
    }
    
    /**
     * @brief Get trade information as string
     * @return Formatted trade information
     */
    std::string toString() const {
        std::string result = "Trade[";
        result += (is_long_ ? "LONG" : "SHORT");
        result += ", Size:" + std::to_string(size_);
        result += ", Entry:" + std::to_string(entry_price_);
        
        if (status_ == Closed) {
            result += ", Exit:" + std::to_string(exit_price_);
            result += ", PnL:" + std::to_string(pnl_comm_);
            result += ", Bars:" + std::to_string(bar_len_);
        } else {
            result += ", OPEN";
        }
        
        result += "]";
        return result;
    }
    
    /**
     * @brief Add entry order to trade
     * @param order Entry order
     */
    void addEntryOrder(std::shared_ptr<Order> order) {
        entry_orders_.push_back(order);
    }
    
    /**
     * @brief Add exit order to trade
     * @param order Exit order
     */
    void addExitOrder(std::shared_ptr<Order> order) {
        exit_orders_.push_back(order);
    }
    
    /**
     * @brief Get entry orders
     * @return Vector of entry orders
     */
    const std::vector<std::shared_ptr<Order>>& getEntryOrders() const {
        return entry_orders_;
    }
    
    /**
     * @brief Get exit orders
     * @return Vector of exit orders
     */
    const std::vector<std::shared_ptr<Order>>& getExitOrders() const {
        return exit_orders_;
    }
    
    /**
     * @brief Calculate trade efficiency (return per day)
     * @return Trade efficiency
     */
    double getEfficiency() const {
        if (status_ != Closed || bar_len_ <= 0) {
            return 0.0;
        }
        
        return getReturn() / bar_len_;
    }
    
    /**
     * @brief Check if trade was profitable
     * @return True if profitable, false otherwise
     */
    bool isProfitable() const {
        return pnl_comm_ > 0.0;
    }
    
    /**
     * @brief Get maximum adverse excursion (MAE) if tracked
     * @return MAE value
     */
    double getMAE() const {
        // This would require tracking the worst price during the trade
        // For now, return 0 - this could be implemented with additional tracking
        return 0.0;
    }
    
    /**
     * @brief Get maximum favorable excursion (MFE) if tracked
     * @return MFE value
     */
    double getMFE() const {
        // This would require tracking the best price during the trade
        // For now, return 0 - this could be implemented with additional tracking
        return 0.0;
    }
};

/**
 * @brief Trade history manager
 * 
 * Manages a collection of trades and provides aggregate statistics
 */
class TradeHistory {
private:
    std::vector<Trade> trades_;
    
public:
    /**
     * @brief Add a trade to history
     * @param trade Trade to add
     */
    void addTrade(const Trade& trade) {
        trades_.push_back(trade);
    }
    
    /**
     * @brief Get all trades
     * @return Vector of all trades
     */
    const std::vector<Trade>& getTrades() const {
        return trades_;
    }
    
    /**
     * @brief Get total number of trades
     * @return Number of trades
     */
    size_t getTradeCount() const {
        return trades_.size();
    }
    
    /**
     * @brief Get number of closed trades
     * @return Number of closed trades
     */
    size_t getClosedTradeCount() const {
        size_t count = 0;
        for (const auto& trade : trades_) {
            if (trade.isClosed()) {
                count++;
            }
        }
        return count;
    }
    
    /**
     * @brief Get total PnL from all closed trades
     * @return Total PnL
     */
    double getTotalPnL() const {
        double total = 0.0;
        for (const auto& trade : trades_) {
            if (trade.isClosed()) {
                total += trade.getPnLComm();
            }
        }
        return total;
    }
    
    /**
     * @brief Get winning trades
     * @return Vector of winning trades
     */
    std::vector<Trade> getWinningTrades() const {
        std::vector<Trade> winners;
        for (const auto& trade : trades_) {
            if (trade.isClosed() && trade.isProfitable()) {
                winners.push_back(trade);
            }
        }
        return winners;
    }
    
    /**
     * @brief Get losing trades
     * @return Vector of losing trades
     */
    std::vector<Trade> getLosingTrades() const {
        std::vector<Trade> losers;
        for (const auto& trade : trades_) {
            if (trade.isClosed() && !trade.isProfitable()) {
                losers.push_back(trade);
            }
        }
        return losers;
    }
    
    /**
     * @brief Clear all trades
     */
    void clear() {
        trades_.clear();
    }
};

} // namespace backtrader