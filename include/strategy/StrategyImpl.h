#pragma once

#include "Strategy.h"
#include "broker/Broker.h"

namespace backtrader {

// This file provides the inline implementations for Strategy methods
// that require complete Broker definition

inline std::shared_ptr<Position> Strategy::getPosition() { 
    // 从 broker 获取仓位信息
    if (broker_) {
        return broker_->getPosition();
    }
    return position_;
}

inline std::shared_ptr<Order> Strategy::buy(double size, double price) {
    if (!broker_) return nullptr;
    
    // 如果没有指定价格，使用当前收盘价
    if (price == 0.0 && data(0)) {
        price = data(0)->close(0);
    }
    
    auto order = std::make_shared<Order>(OrderType::Buy, size, price);
    return broker_->submitOrder(order);
}

inline std::shared_ptr<Order> Strategy::sell(double size, double price) {
    if (!broker_) return nullptr;
    
    // 如果没有指定价格，使用当前收盘价
    if (price == 0.0 && data(0)) {
        price = data(0)->close(0);
    }
    
    auto order = std::make_shared<Order>(OrderType::Sell, size, price);
    return broker_->submitOrder(order);
}

inline std::shared_ptr<Order> Strategy::close() {
    if (!broker_ || !position_ || position_->isEmpty()) {
        return nullptr;
    }
    
    // 平仓：如果是多头则卖出，如果是空头则买入
    double size = std::abs(position_->getSize());
    if (position_->isLong()) {
        return sell(size);
    } else {
        return buy(size);
    }
}

inline void Strategy::updateCurrentPrice() {
    if (broker_ && data(0)) {
        broker_->setCurrentPrice(data(0)->close(0));
    }
}

} // namespace backtrader