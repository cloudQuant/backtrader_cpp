#pragma once

#include "broker/Order.h"
#include "broker/Position.h"
#include "indicators/IndicatorBase.h"
#include "feeds/DataFeed.h"
#include "feeds/SimpleDataFeed.h"
#include "LineRoot.h"
#include "Utils.h"
#include <memory>
#include <vector>
#include <string>
#include <iostream>

namespace backtrader {

// Forward declarations
class Broker;
class Cerebro;

/**
 * @brief 策略基类
 */
class Strategy {
protected:
    std::string name_;
    std::vector<std::shared_ptr<data::SimpleDataFeed>> data_feeds_;
    std::vector<std::shared_ptr<IndicatorBase>> indicators_;
    Broker* broker_;  // Broker指针，由Cerebro设置
    std::shared_ptr<Position> position_;  // 策略仓位
    size_t data_length_;  // 数据长度
    size_t current_index_;  // 当前数据索引
    
public:
    Strategy() : name_("Strategy"), broker_(nullptr), 
                 position_(std::make_shared<Position>()), 
                 data_length_(0), current_index_(0) {}
    virtual ~Strategy() = default;
    
    // 设置broker（由Cerebro调用）
    void setBroker(Broker* broker) { broker_ = broker; }
    Broker* broker() { return broker_; }
    
    // 数据访问
    std::shared_ptr<data::SimpleDataFeed> data(size_t index = 0) {
        if (index < data_feeds_.size()) {
            return data_feeds_[index];
        }
        return nullptr;
    }
    
    void addData(std::shared_ptr<data::SimpleDataFeed> data) {
        data_feeds_.push_back(data);
        if (data) {
            data_length_ = data->len();
        }
    }
    
    // 获取数据长度
    size_t len() const { return current_index_; }
    
    // 设置当前数据索引（由Cerebro调用）
    void setCurrentIndex(size_t index) { current_index_ = index; }
    
    // 指标管理
    void addIndicator(std::shared_ptr<IndicatorBase> indicator) {
        indicators_.push_back(indicator);
    }
    
    // 仓位管理
    std::shared_ptr<Position> getPosition();
    
    // 订单方法（通过broker执行）
    virtual std::shared_ptr<Order> buy(double size = 1.0, double price = 0.0);
    virtual std::shared_ptr<Order> sell(double size = 1.0, double price = 0.0);
    virtual std::shared_ptr<Order> close();
    
    // 生命周期回调方法
    virtual void init() {}    // 策略初始化（添加指标等）
    virtual void start() {}   // 策略开始前调用
    virtual void stop() {}    // 策略结束后调用
    virtual void next() {}    // 每个数据点调用
    virtual void nextstart() { next(); } // 第一次有足够数据时调用
    virtual void prenext() {} // 在有足够数据前调用
    
    // 更新当前价格（由Cerebro调用）
    void updateCurrentPrice();
    
    // 计算所有指标（由Cerebro调用）
    void calculateIndicators() {
        static int debug_count = 0;
        if (debug_count < 3) {
            std::cout << "*** calculateIndicators: indicator count=" << indicators_.size() << " (count=" << debug_count << ")" << std::endl;
            debug_count++;
        }
        
        for (auto& indicator : indicators_) {
            if (indicator) {
                indicator->calculate();
            }
        }
    }
    
    // 通知回调
    virtual void notifyOrder(std::shared_ptr<Order> order) {}
    virtual void notifyTrade() {}
    virtual void notifyData(std::shared_ptr<data::DataFeed> data, int status) {}
    
    // 获取策略名称
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
};

} // namespace backtrader