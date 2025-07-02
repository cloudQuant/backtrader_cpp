#include "strategy/Strategy.h"
#include "broker/Broker.h"

namespace backtrader {

std::shared_ptr<Position> Strategy::getPosition() { 
    // 从 broker 获取仓位信息
    if (broker_) {
        return broker_->getPosition();
    }
    return position_;
}

std::shared_ptr<Order> Strategy::buy(double size, double price) {
    if (!broker_) return nullptr;
    
    // 如果没有指定价格，使用当前收盘价
    if (price == 0.0 && data(0)) {
        price = data(0)->close(0);
    }
    
    auto order = std::make_shared<Order>(OrderType::Buy, size, price);
    return broker_->submitOrder(order);
}

std::shared_ptr<Order> Strategy::sell(double size, double price) {
    if (!broker_) return nullptr;
    
    // 如果没有指定价格，使用当前收盘价
    if (price == 0.0 && data(0)) {
        price = data(0)->close(0);
    }
    
    auto order = std::make_shared<Order>(OrderType::Sell, size, price);
    return broker_->submitOrder(order);
}

std::shared_ptr<Order> Strategy::close() {
    auto pos = getPosition();
    if (!broker_ || !pos || pos->isEmpty()) {
        return nullptr;
    }
    
    // 平仓：如果是多头则卖出，如果是空头则买入
    double size = std::abs(pos->getSize());
    if (pos->isLong()) {
        return sell(size);
    } else {
        return buy(size);
    }
}

void Strategy::updateCurrentPrice() {
    if (broker_ && data(0)) {
        broker_->setCurrentPrice(data(0)->close(0));
    }
}

} // namespace backtrader