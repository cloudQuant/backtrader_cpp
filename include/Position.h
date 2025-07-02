#pragma once

#include "Common.h"
#include <string>

namespace backtrader {

/**
 * @brief 持仓类，表示交易品种的当前持仓状态
 * 
 * 跟踪持仓大小、价格、已实现和未实现盈亏
 */
class Position {
public:
    double size;                // 持仓大小（正数为多头，负数为空头）
    double price;               // 平均持仓价格
    double unrealized_pnl;      // 未实现盈亏
    double realized_pnl;        // 已实现盈亏
    
private:
    std::string data_name_;     // 数据名称
    double total_cost_;         // 总成本
    
public:
    /**
     * @brief 构造函数
     * @param data_name 数据名称
     */
    explicit Position(const std::string& data_name = "")
        : size(0.0), price(0.0), unrealized_pnl(0.0), realized_pnl(0.0),
          data_name_(data_name), total_cost_(0.0) {}
    
    /**
     * @brief 检查持仓是否为空
     * @return true if position is empty
     */
    bool isEmpty() const {
        return std::abs(size) < 1e-8;
    }
    
    /**
     * @brief 检查是否为多头持仓
     * @return true if long position
     */
    bool isLong() const {
        return size > 1e-8;
    }
    
    /**
     * @brief 检查是否为空头持仓
     * @return true if short position
     */
    bool isShort() const {
        return size < -1e-8;
    }
    
    /**
     * @brief 更新持仓
     * @param size_change 持仓变化量
     * @param price 成交价格
     */
    void update(double size_change, double price) {
        if (std::abs(size_change) < 1e-8) {
            return;  // 没有变化
        }
        
        double old_size = size;
        double new_size = old_size + size_change;
        
        if (std::abs(old_size) < 1e-8) {
            // 开仓
            size = new_size;
            this->price = price;
            total_cost_ = new_size * price;
        } else if ((old_size > 0 && size_change > 0) || (old_size < 0 && size_change < 0)) {
            // 加仓
            total_cost_ += size_change * price;
            size = new_size;
            this->price = total_cost_ / size;
        } else {
            // 减仓或平仓
            double close_amount = std::min(std::abs(size_change), std::abs(old_size));
            double pnl_per_unit = (old_size > 0) ? (price - this->price) : (this->price - price);
            realized_pnl += close_amount * pnl_per_unit;
            
            size = new_size;
            if (std::abs(size) < 1e-8) {
                // 完全平仓
                size = 0.0;
                this->price = 0.0;
                total_cost_ = 0.0;
            } else {
                // 部分平仓，平均价格不变
                total_cost_ = size * this->price;
            }
        }
    }
    
    /**
     * @brief 更新未实现盈亏
     * @param current_price 当前市场价格
     */
    void updateUnrealizedPnL(double current_price) {
        if (std::abs(size) < 1e-8) {
            unrealized_pnl = 0.0;
        } else {
            if (size > 0) {
                // 多头持仓
                unrealized_pnl = size * (current_price - price);
            } else {
                // 空头持仓
                unrealized_pnl = std::abs(size) * (price - current_price);
            }
        }
    }
    
    /**
     * @brief 获取总盈亏
     * @return 已实现盈亏 + 未实现盈亏
     */
    double getTotalPnL() const {
        return realized_pnl + unrealized_pnl;
    }
    
    /**
     * @brief 获取数据名称
     * @return 数据名称
     */
    const std::string& getDataName() const {
        return data_name_;
    }
    
    /**
     * @brief 重置持仓
     */
    void reset() {
        size = 0.0;
        price = 0.0;
        unrealized_pnl = 0.0;
        realized_pnl = 0.0;
        total_cost_ = 0.0;
    }
};

} // namespace backtrader