/**
 * @file Order.h
 * @brief Order management classes and enums
 * 
 * This file contains the order-related structures and enums that are used
 * throughout the backtrader system for order management.
 */

#pragma once

#include "Common.h"
#include <chrono>
#include <string>

namespace backtrader {

/**
 * @brief 订单类型枚举
 */
enum class OrderType {
    MARKET,     // 市价单
    LIMIT,      // 限价单
    STOP,       // 止损单
    STOP_LIMIT  // 止损限价单
};

/**
 * @brief 订单方向枚举
 */
enum class OrderSide {
    BUY,        // 买入
    SELL        // 卖出
};

/**
 * @brief 订单状态枚举
 */
enum class OrderStatus {
    CREATED,    // 已创建
    SUBMITTED,  // 已提交
    PARTIAL,    // 部分成交
    COMPLETED,  // 已完成
    CANCELED,   // 已取消
    REJECTED    // 已拒绝
};

/**
 * @brief 订单结构
 */
struct Order {
    size_t id;
    OrderType type;
    OrderSide side;
    OrderStatus status;
    double size;          // 数量
    double price;         // 价格（限价单使用）
    double stop_price;    // 止损价格（止损单使用）
    double executed_size; // 已成交数量
    double executed_price;// 平均成交价格
    std::chrono::system_clock::time_point created_time;
    std::chrono::system_clock::time_point updated_time;
    std::string data_name; // 数据源名称
    
    Order() : id(0), type(OrderType::MARKET), side(OrderSide::BUY), 
              status(OrderStatus::CREATED), size(0.0), price(0.0), 
              stop_price(0.0), executed_size(0.0), executed_price(0.0) {}
    
    /**
     * @brief 检查订单是否完全成交
     */
    bool isCompleted() const {
        return status == OrderStatus::COMPLETED;
    }
    
    /**
     * @brief 检查订单是否部分成交
     */
    bool isPartial() const {
        return status == OrderStatus::PARTIAL;
    }
    
    /**
     * @brief 检查订单是否被取消
     */
    bool isCanceled() const {
        return status == OrderStatus::CANCELED;
    }
    
    /**
     * @brief 检查订单是否被拒绝
     */
    bool isRejected() const {
        return status == OrderStatus::REJECTED;
    }
    
    /**
     * @brief 检查订单是否为买单
     */
    bool isBuyOrder() const {
        return side == OrderSide::BUY;
    }
    
    /**
     * @brief 检查订单是否为卖单
     */
    bool isSellOrder() const {
        return side == OrderSide::SELL;
    }
    
    /**
     * @brief 检查订单是否为市价单
     */
    bool isMarketOrder() const {
        return type == OrderType::MARKET;
    }
    
    /**
     * @brief 检查订单是否为限价单
     */
    bool isLimitOrder() const {
        return type == OrderType::LIMIT;
    }
    
    /**
     * @brief 获取剩余未成交数量
     */
    double getRemainingSize() const {
        return size - executed_size;
    }
    
    /**
     * @brief 更新订单执行信息
     * @param exec_size 本次成交数量
     * @param exec_price 本次成交价格
     */
    void addExecution(double exec_size, double exec_price) {
        if (exec_size <= 0.0) return;
        
        // 更新平均成交价格
        double total_value = executed_size * executed_price + exec_size * exec_price;
        executed_size += exec_size;
        executed_price = (executed_size > 0.0) ? (total_value / executed_size) : 0.0;
        
        // 更新订单状态
        if (executed_size >= size) {
            status = OrderStatus::COMPLETED;
        } else if (executed_size > 0.0) {
            status = OrderStatus::PARTIAL;
        }
        
        updated_time = std::chrono::system_clock::now();
    }
    
    /**
     * @brief 设置订单状态
     * @param new_status 新状态
     */
    void setStatus(OrderStatus new_status) {
        status = new_status;
        updated_time = std::chrono::system_clock::now();
    }
    
    /**
     * @brief 克隆订单（用于部分成交处理）
     * @return 新的订单对象
     */
    Order clone() const {
        Order cloned = *this;
        cloned.id = 0; // 新订单需要新的ID
        cloned.status = OrderStatus::CREATED;
        cloned.executed_size = 0.0;
        cloned.executed_price = 0.0;
        cloned.created_time = std::chrono::system_clock::now();
        cloned.updated_time = cloned.created_time;
        return cloned;
    }
    
    /**
     * @brief 获取订单描述字符串
     */
    std::string toString() const {
        std::string result = "Order[ID:" + std::to_string(id);
        result += ", Type:";
        switch (type) {
            case OrderType::MARKET: result += "MARKET"; break;
            case OrderType::LIMIT: result += "LIMIT"; break;
            case OrderType::STOP: result += "STOP"; break;
            case OrderType::STOP_LIMIT: result += "STOP_LIMIT"; break;
        }
        result += ", Side:";
        result += (side == OrderSide::BUY) ? "BUY" : "SELL";
        result += ", Size:" + std::to_string(size);
        result += ", Price:" + std::to_string(price);
        result += ", Status:";
        switch (status) {
            case OrderStatus::CREATED: result += "CREATED"; break;
            case OrderStatus::SUBMITTED: result += "SUBMITTED"; break;
            case OrderStatus::PARTIAL: result += "PARTIAL"; break;
            case OrderStatus::COMPLETED: result += "COMPLETED"; break;
            case OrderStatus::CANCELED: result += "CANCELED"; break;
            case OrderStatus::REJECTED: result += "REJECTED"; break;
        }
        result += "]";
        return result;
    }
};

/**
 * @brief 持仓信息结构 (Order module)
 */
struct OrderPosition {
    double size;          // 持仓数量（正数为多头，负数为空头）
    double price;         // 平均成本价格
    double unrealized_pnl;// 未实现盈亏
    double realized_pnl;  // 已实现盈亏
    std::string data_name; // 数据源名称
    
    OrderPosition() : size(0.0), price(0.0), unrealized_pnl(0.0), realized_pnl(0.0) {}
    
    OrderPosition(const std::string& name) : size(0.0), price(0.0), 
                                             unrealized_pnl(0.0), realized_pnl(0.0), 
                                             data_name(name) {}
    
    bool isLong() const { return size > 0.0; }
    bool isShort() const { return size < 0.0; }
    bool isEmpty() const { return size == 0.0; }
    
    /**
     * @brief 更新持仓
     * @param trade_size 交易数量（正数为买入，负数为卖出）
     * @param trade_price 交易价格
     */
    void update(double trade_size, double trade_price) {
        if (trade_size == 0.0) return;
        
        double new_size = size + trade_size;
        
        if (size == 0.0) {
            // 新开仓
            size = new_size;
            price = trade_price;
        } else if ((size > 0 && trade_size > 0) || (size < 0 && trade_size < 0)) {
            // 加仓
            double total_value = size * price + trade_size * trade_price;
            size = new_size;
            price = (size != 0.0) ? (total_value / size) : 0.0;
        } else {
            // 减仓或反向
            if (std::abs(trade_size) <= std::abs(size)) {
                // 减仓
                double closed_pnl;
                if (size > 0) {
                    closed_pnl = -trade_size * (trade_price - price);
                } else {
                    closed_pnl = -trade_size * (price - trade_price);
                }
                realized_pnl += closed_pnl;
                size = new_size;
                if (size == 0.0) {
                    price = 0.0;
                }
            } else {
                // 反向操作
                double close_size = -size;
                double close_pnl;
                if (size > 0) {
                    close_pnl = close_size * (trade_price - price);
                } else {
                    close_pnl = close_size * (price - trade_price);
                }
                realized_pnl += close_pnl;
                
                // 反向开仓
                size = new_size;
                price = trade_price;
            }
        }
    }
    
    /**
     * @brief 计算未实现盈亏
     * @param current_price 当前价格
     */
    void updateUnrealizedPnL(double current_price) {
        if (size == 0.0 || isNaN(current_price)) {
            unrealized_pnl = 0.0;
            return;
        }
        
        if (size > 0) {
            unrealized_pnl = size * (current_price - price);
        } else {
            unrealized_pnl = size * (price - current_price);
        }
    }
    
    /**
     * @brief 获取总盈亏
     */
    double getTotalPnL() const {
        return realized_pnl + unrealized_pnl;
    }
    
    /**
     * @brief 获取持仓描述字符串
     */
    std::string toString() const {
        std::string result = "Position[";
        if (size > 0) {
            result += "LONG";
        } else if (size < 0) {
            result += "SHORT";
        } else {
            result += "EMPTY";
        }
        result += ", Size:" + std::to_string(size);
        result += ", Price:" + std::to_string(price);
        result += ", UnrealizedPnL:" + std::to_string(unrealized_pnl);
        result += ", RealizedPnL:" + std::to_string(realized_pnl);
        result += "]";
        return result;
    }
};

} // namespace backtrader