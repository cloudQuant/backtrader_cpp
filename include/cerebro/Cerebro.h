#pragma once

#include "strategy/Strategy.h"
#include "broker/Broker.h"
#include "feeds/DataFeed.h"
#include "feeds/SimpleDataFeed.h"
#include <memory>
#include <vector>
#include <type_traits>

namespace backtrader {

/**
 * @brief Cerebro类 - backtrader的核心引擎
 */
class Cerebro {
private:
    std::vector<std::shared_ptr<Strategy>> strategies_;
    std::vector<std::shared_ptr<data::SimpleDataFeed>> data_feeds_;
    std::shared_ptr<Broker> broker_;
    
public:
    Cerebro() : broker_(std::make_shared<Broker>()) {}
    
    // 添加策略
    void addStrategy(std::shared_ptr<Strategy> strategy) {
        strategy->setBroker(broker_.get());
        // 将已有数据源添加到策略
        for (auto& data : data_feeds_) {
            strategy->addData(data);
        }
        strategies_.push_back(strategy);
    }
    
    // 模板方法：添加策略类型
    template<typename StrategyType, typename... Args>
    void addstrategy(Args&&... args) {
        static_assert(std::is_base_of_v<Strategy, StrategyType>, "StrategyType must derive from Strategy");
        auto strategy = std::make_shared<StrategyType>(std::forward<Args>(args)...);
        addStrategy(strategy);
    }
    
    // 添加数据源
    void addData(std::shared_ptr<data::SimpleDataFeed> data) {
        data_feeds_.push_back(data);
        
        // 将数据源添加到所有策略
        for (auto& strategy : strategies_) {
            strategy->addData(data);
        }
    }
    
    // 从 CSV 数据构建 SimpleDataFeed 并添加
    template<typename CSVData>
    void adddata(const std::vector<CSVData>& csv_data, const std::string& name = "data") {
        auto data_feed = std::make_shared<data::SimpleDataFeed>(name);
        
        // 将 CSV 数据转换为 SimpleDataFeed 格式
        for (const auto& bar : csv_data) {
            data::SimpleDataFeed::OHLCVData ohlcv_data;
            ohlcv_data.date = bar.date;
            ohlcv_data.open = bar.open;
            ohlcv_data.high = bar.high;
            ohlcv_data.low = bar.low;
            ohlcv_data.close = bar.close;
            ohlcv_data.volume = bar.volume;
            ohlcv_data.openinterest = bar.openinterest;
            
            data_feed->addData(ohlcv_data);
        }
        
        addData(data_feed);
    }
    
    // 设置broker
    void setBroker(std::shared_ptr<Broker> broker) {
        broker_ = broker;
        for (auto& strategy : strategies_) {
            strategy->setBroker(broker_.get());
        }
    }
    
    // 获取broker
    std::shared_ptr<Broker> getBroker() { return broker_; }
    
    // 运行回测
    std::vector<std::shared_ptr<Strategy>> run() {
        if (strategies_.empty() || data_feeds_.empty()) {
            return strategies_;
        }
        
        // 设置订单通知回调
        for (auto& strategy : strategies_) {
            broker_->setOrderCallback([strategy](std::shared_ptr<Order> order) {
                strategy->notifyOrder(order);
            });
        }
        
        // 初始化策略
        for (auto& strategy : strategies_) {
            strategy->init();
        }
        
        // 调用所有策略的start方法
        for (auto& strategy : strategies_) {
            strategy->start();
        }
        
        // 模拟数据迭代
        size_t data_length = data_feeds_[0]->len();
        
        // 重置所有数据线到起始位置
        for (auto& data_feed : data_feeds_) {
            data_feed->reset();
        }
        
        bool first_next = true;
        
        for (size_t i = 0; i < data_length; ++i) {
            // 前进数据一步
            for (auto& data_feed : data_feeds_) {
                data_feed->forward();
            }
            
            // 执行待处理的订单（使用当前bar的开盘价）
            if (i > 0) {  // 第一个bar没有待执行订单
                double open_price = data_feeds_[0]->open(0);
                double datetime = data_feeds_[0]->datetime(0);
                broker_->processPendingOrders(open_price, datetime);
            }
            
            // 更新所有策略的当前数据索引
            for (auto& strategy : strategies_) {
                strategy->setCurrentIndex(i + 1);  // 从1开始计数
                strategy->updateCurrentPrice();    // 更新broker中的当前价格
                
                // 计算所有指标
                strategy->calculateIndicators();
            }
            
            // 期货模式日常现金调整（mark-to-market）
            // 暂时关闭，使用更简单的模型进行调试
            // if (i > 0) {  // 第一个bar不调整
            //     double close_price = data_feeds_[0]->close(0);
            //     broker_->dailyCashAdjustment(close_price);
            // }
            
            // 调用策略方法
            for (auto& strategy : strategies_) {
                if (first_next && i == 0) {
                    strategy->nextstart();
                } else {
                    strategy->next();
                }
            }
            first_next = false;
        }
        
        // 最终更新：确保使用最后一个bar的收盘价计算最终价值
        for (auto& strategy : strategies_) {
            strategy->updateCurrentPrice();
            if (broker_) {
                // 对期货模式，在最后一天进行最终的mark-to-market调整
                double final_close = data_feeds_[0]->close(0);
                broker_->dailyCashAdjustment(final_close);
                broker_->updateValue();
            }
        }
        
        // 调用所有策略的stop方法
        for (auto& strategy : strategies_) {
            strategy->stop();
        }
        
        return strategies_;
    }
    
    // 获取策略列表
    const std::vector<std::shared_ptr<Strategy>>& getStrategies() const {
        return strategies_;
    }
    
};

} // namespace backtrader