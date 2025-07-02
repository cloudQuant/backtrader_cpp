#pragma once

#include "Order.h"
#include "Position.h"
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include <iostream>

namespace backtrader {

/**
 * @brief 简化版Broker类，用于策略测试
 */
class Broker {
private:
    double cash_;
    double value_;
    std::vector<std::shared_ptr<Order>> orders_;
    std::vector<std::shared_ptr<Order>> pending_orders_;  // 待执行订单
    std::function<void(std::shared_ptr<Order>)> order_callback_;
    std::unordered_map<std::string, std::shared_ptr<Position>> positions_;
    
    // 手续费参数
    double commission_;        // 手续费率或固定金额
    double mult_;             // 合约乘数（期货模式）
    double margin_;           // 保证金（期货模式）
    bool stocklike_;          // 是否为股票模式
    
    // 当前价格（用于执行价格计算）
    double current_price_;
    
public:
    Broker(double cash = 10000.0) 
        : cash_(cash), value_(cash), 
          commission_(0.0), mult_(1.0), margin_(0.0), stocklike_(true),
          current_price_(100.0) {
        // 初始化默认仓位
        positions_["default"] = std::make_shared<Position>();
    }
    
    // 基本功能
    double getCash() const { return cash_; }
    double getValue() const { return value_; }
    
    void setCash(double cash) { cash_ = cash; }
    void setValue(double value) { value_ = value; }
    
    // 手续费设置 - 兼容股票和期货模式
    void setCommission(double commission = 0.0, double mult = 1.0, double margin = 0.0) {
        commission_ = commission;
        mult_ = mult;
        margin_ = margin;
        stocklike_ = (margin == 0.0);  // 有保证金则为期货模式
    }
    
    // 仓位管理
    std::shared_ptr<Position> getPosition(const std::string& symbol = "default") {
        auto it = positions_.find(symbol);
        if (it != positions_.end()) {
            return it->second;
        }
        // 创建新仓位
        positions_[symbol] = std::make_shared<Position>();
        return positions_[symbol];
    }
    
    // 设置当前价格（由Cerebro调用）
    void setCurrentPrice(double price) { current_price_ = price; }
    
    // 期货模式日常现金调整（mark-to-market）
    void dailyCashAdjustment(double current_price) {
        if (stocklike_) return;  // 只适用于期货模式
        
        for (auto& pair : positions_) {
            auto position = pair.second;
            if (position->getSize() != 0.0) {
                // 计算现金调整：size * (current_price - adjbase) * mult
                double old_adjbase = position->getAdjBase();
                if (old_adjbase != 0.0) {  // 只有当adjbase有效时才调整
                    double adjustment = position->getSize() * (current_price - old_adjbase) * mult_;
                    cash_ += adjustment;
                    
                    // 更新调整基准价格
                    position->setAdjBase(current_price);
                }
            }
        }
    }
    
    // 订单管理
    std::shared_ptr<Order> submitOrder(std::shared_ptr<Order> order) {
        order->submit();
        order->accept();
        orders_.push_back(order);
        
        // 不立即执行，等待下一个bar的开盘价
        pending_orders_.push_back(order);
        
        return order;
    }
    
    // 执行待处理的订单（在新bar开始时调用）
    void processPendingOrders(double open_price, double datetime = 0.0) {
        for (auto& order : pending_orders_) {
            executeOrder(order, open_price, datetime);
        }
        pending_orders_.clear();
    }
    
    void executeOrder(std::shared_ptr<Order> order, double exec_price, double datetime = 0.0) {
        // 使用传入的执行价格（下一个bar的开盘价）
        order->execute(exec_price, datetime);
        
        // 计算手续费
        double commission = calculateCommission(order->getSize(), exec_price);
        
        
        // 更新仓位和现金
        auto position = getPosition();
        
        
        if (stocklike_) {
            // 股票模式：传统处理
            if (order->isBuy()) {
                position->update(order->getSize(), exec_price);
                cash_ -= exec_price * order->getSize() + commission;
            } else {
                position->update(-order->getSize(), exec_price);
                cash_ += exec_price * order->getSize() - commission;
            }
        } else {
            // 期货模式：在交易时计算P&L到现金，管理保证金，扣除手续费
            double current_size = position->getSize();
            double old_abs_size = std::abs(current_size);
            
            // 如果有现有仓位，计算从adjbase到执行价格的P&L
            if (current_size != 0.0) {
                double adjbase = position->getAdjBase();
                if (adjbase != 0.0) {
                    double pnl = current_size * (exec_price - adjbase) * mult_;
                    cash_ += pnl;
                    
                }
            }
            
            // 更新仓位（这会自动更新adjbase到执行价格）
            if (order->isBuy()) {
                position->update(order->getSize(), exec_price);
            } else {
                position->update(-order->getSize(), exec_price);
            }
            
            // 计算新的保证金要求变化
            double new_abs_size = std::abs(position->getSize());
            double margin_change = (new_abs_size - old_abs_size) * margin_;
            cash_ -= margin_change;  // 增加仓位需要更多保证金，减少仓位释放保证金
            
            
            // 扣除手续费
            cash_ -= commission;
        }
        
        
        // 更新组合价值
        updateValue();
        
        // 触发回调
        if (order_callback_) {
            order_callback_(order);
        }
    }
    
    void setOrderCallback(std::function<void(std::shared_ptr<Order>)> callback) {
        order_callback_ = callback;
    }
    
    // 更新组合价值
    void updateValue() {
        value_ = cash_;
        for (const auto& pair : positions_) {
            if (stocklike_) {
                // 股票模式：市值 = 股数 × 当前价格
                pair.second->updateUnrealizedPnL(current_price_);
                value_ += pair.second->getValue(current_price_);
            } else {
                // 期货模式：价值 = 现金 + 保证金占用
                // 在期货模式下，mark-to-market的盈亏已经体现在现金中
                // 组合价值只需要加上保证金占用即可
                double position_size = std::abs(pair.second->getSize());
                value_ += position_size * margin_;
            }
        }
    }
    
    // 获取订单历史
    const std::vector<std::shared_ptr<Order>>& getOrders() const {
        return orders_;
    }
    
private:
    double getCurrentTime() {
        // 返回一个简单的时间戳
        return 20060102.0;  // 模拟日期
    }
    
    // 计算手续费
    double calculateCommission(double size, double price) {
        double abs_size = std::abs(size);
        
        if (stocklike_) {
            // 股票模式：按成交金额的百分比
            return abs_size * price * commission_;
        } else {
            // 期货模式：固定金额
            return abs_size * commission_;
        }
    }
};

} // namespace backtrader