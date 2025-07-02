#pragma once

#include <memory>

namespace backtrader {

/**
 * @brief 仓位类，跟踪策略的持仓情况
 */
class Position {
private:
    double size_;              // 仓位大小（正数为多头，负数为空头，0为无仓位）
    double price_;             // 平均成本价格
    double unrealized_pnl_;    // 未实现盈亏
    double realized_pnl_;      // 已实现盈亏
    double adjbase_;           // 期货调整基准价格（用于日常mark-to-market）
    
public:
    Position() : size_(0.0), price_(0.0), unrealized_pnl_(0.0), realized_pnl_(0.0), adjbase_(0.0) {}
    
    // 基本访问方法
    double getSize() const { return size_; }
    double getPrice() const { return price_; }
    double getUnrealizedPnL() const { return unrealized_pnl_; }
    double getRealizedPnL() const { return realized_pnl_; }
    double getAdjBase() const { return adjbase_; }
    void setAdjBase(double adjbase) { adjbase_ = adjbase; }
    
    // 检查仓位状态
    bool isLong() const { return size_ > 0.0; }
    bool isShort() const { return size_ < 0.0; }
    bool isEmpty() const { return size_ == 0.0; }
    
    // 更新仓位
    void update(double size, double price) {
        if (size_ == 0.0) {
            // 开新仓
            size_ = size;
            price_ = price;
            adjbase_ = price;  // 初始化调整基准价格
        } else if ((size_ > 0 && size > 0) || (size_ < 0 && size < 0)) {
            // 加仓 - 计算平均成本
            double total_value = size_ * price_ + size * price;
            size_ += size;
            if (size_ != 0.0) {
                price_ = total_value / size_;
                adjbase_ = price_;  // 更新调整基准价格为新的平均价格
            }
        } else {
            // 减仓或反向 - 计算已实现盈亏
            double reduction = std::min(std::abs(size), std::abs(size_));
            if (size_ > 0) {
                // 原来多头，现在卖出
                realized_pnl_ += reduction * (price - price_);
                size_ -= reduction;
            } else {
                // 原来空头，现在买入
                realized_pnl_ += reduction * (price_ - price);
                size_ += reduction;
            }
            
            // 如果还有剩余操作，按照新仓处理
            double remaining = std::abs(size) - reduction;
            if (remaining > 0.0) {
                if (size > 0) {
                    size_ += remaining;
                    price_ = price;  // 新仓价格
                    adjbase_ = price;  // 更新调整基准价格
                } else {
                    size_ -= remaining;
                    price_ = price;  // 新仓价格
                    adjbase_ = price;  // 更新调整基准价格
                }
            }
            
            // 如果仓位完全平掉，重置adjbase
            if (size_ == 0.0) {
                adjbase_ = 0.0;
            }
        }
    }
    
    // 更新未实现盈亏（基于当前市价）
    void updateUnrealizedPnL(double current_price) {
        if (size_ != 0.0) {
            unrealized_pnl_ = size_ * (current_price - price_);
        } else {
            unrealized_pnl_ = 0.0;
        }
    }
    
    // 平仓
    void close(double close_price) {
        if (size_ != 0.0) {
            if (size_ > 0) {
                realized_pnl_ += size_ * (close_price - price_);
            } else {
                realized_pnl_ += std::abs(size_) * (price_ - close_price);
            }
            size_ = 0.0;
            price_ = 0.0;
            unrealized_pnl_ = 0.0;
        }
    }
    
    // 重置仓位
    void reset() {
        size_ = 0.0;
        price_ = 0.0;
        unrealized_pnl_ = 0.0;
        realized_pnl_ = 0.0;
        adjbase_ = 0.0;
    }
    
    // 获取总盈亏
    double getTotalPnL() const {
        return realized_pnl_ + unrealized_pnl_;
    }
    
    // 获取市值（基于当前价格）
    double getValue(double current_price) const {
        if (size_ == 0.0) return 0.0;
        return std::abs(size_) * current_price;
    }
};

} // namespace backtrader