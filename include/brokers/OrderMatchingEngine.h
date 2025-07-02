#pragma once

#include "strategy/StrategyBase.h"
#include "feeds/DataFeed.h"
#include <queue>
#include <vector>
#include <unordered_map>
#include <functional>

namespace backtrader {
namespace broker {

using namespace strategy;
using namespace data;

/**
 * @brief 订单执行结果
 */
struct ExecutionResult {
    size_t order_id;
    double executed_price;
    double executed_size;
    double remaining_size;
    OrderStatus new_status;
    std::string reason;
    
    ExecutionResult() : order_id(0), executed_price(0.0), executed_size(0.0),
                       remaining_size(0.0), new_status(OrderStatus::CREATED) {}
};

/**
 * @brief 市场数据快照
 */
struct MarketSnapshot {
    double bid_price;
    double ask_price;
    double last_price;
    double high_price;
    double low_price;
    double volume;
    std::chrono::system_clock::time_point timestamp;
    
    MarketSnapshot() : bid_price(NaN), ask_price(NaN), last_price(NaN),
                      high_price(NaN), low_price(NaN), volume(0.0) {}
};

/**
 * @brief 滑点模型
 */
class SlippageModel {
public:
    virtual ~SlippageModel() = default;
    
    /**
     * @brief 计算滑点
     * @param order 订单
     * @param market_price 市场价格
     * @param volume 成交量
     * @return 实际执行价格
     */
    virtual double calculateSlippage(const Order& order, double market_price, double volume) = 0;
};

/**
 * @brief 固定滑点模型
 */
class FixedSlippageModel : public SlippageModel {
private:
    double slippage_pips_;
    
public:
    explicit FixedSlippageModel(double slippage_pips = 0.0) 
        : slippage_pips_(slippage_pips) {}
    
    double calculateSlippage(const Order& order, double market_price, double volume) override {
        if (order.side == OrderSide::BUY) {
            return market_price + slippage_pips_;
        } else {
            return market_price - slippage_pips_;
        }
    }
};

/**
 * @brief 百分比滑点模型
 */
class PercentageSlippageModel : public SlippageModel {
private:
    double slippage_percentage_;
    
public:
    explicit PercentageSlippageModel(double slippage_percentage = 0.0)
        : slippage_percentage_(slippage_percentage) {}
    
    double calculateSlippage(const Order& order, double market_price, double volume) override {
        double slippage = market_price * slippage_percentage_ / 100.0;
        
        if (order.side == OrderSide::BUY) {
            return market_price + slippage;
        } else {
            return market_price - slippage;
        }
    }
};

/**
 * @brief 订单匹配引擎
 * 
 * 负责处理订单的匹配、执行和管理
 * 支持多种订单类型和滑点模型
 */
class OrderMatchingEngine {
private:
    // 订单队列（按优先级排序）
    std::priority_queue<Order*, std::vector<Order*>, std::function<bool(Order*, Order*)>> pending_orders_;
    
    // 数据源
    std::shared_ptr<DataFeed> data_feed_;
    
    // 滑点模型
    std::unique_ptr<SlippageModel> slippage_model_;
    
    // 市场数据
    MarketSnapshot current_market_;
    
    // 配置参数
    bool allow_fractional_shares_;
    double min_order_size_;
    double max_order_size_;
    bool check_liquidity_;
    
    // 统计信息
    size_t total_orders_processed_;
    size_t orders_executed_;
    size_t orders_rejected_;
    double total_volume_executed_;
    
public:
    /**
     * @brief 构造函数
     * @param data_feed 数据源
     */
    explicit OrderMatchingEngine(std::shared_ptr<DataFeed> data_feed)
        : data_feed_(data_feed),
          pending_orders_(orderComparator),
          allow_fractional_shares_(true),
          min_order_size_(0.01),
          max_order_size_(1000000.0),
          check_liquidity_(false),
          total_orders_processed_(0),
          orders_executed_(0),
          orders_rejected_(0),
          total_volume_executed_(0.0) {
        
        // 默认使用固定滑点模型
        slippage_model_ = std::make_unique<FixedSlippageModel>(0.0);
    }
    
    /**
     * @brief 设置滑点模型
     * @param model 滑点模型
     */
    void setSlippageModel(std::unique_ptr<SlippageModel> model) {
        slippage_model_ = std::move(model);
    }
    
    /**
     * @brief 设置配置参数
     */
    void setConfiguration(bool allow_fractional_shares = true,
                         double min_order_size = 0.01,
                         double max_order_size = 1000000.0,
                         bool check_liquidity = false) {
        allow_fractional_shares_ = allow_fractional_shares;
        min_order_size_ = min_order_size;
        max_order_size_ = max_order_size;
        check_liquidity_ = check_liquidity;
    }
    
    /**
     * @brief 提交订单
     * @param order 订单
     * @return 执行结果
     */
    ExecutionResult submitOrder(Order& order) {
        ExecutionResult result;
        result.order_id = order.id;
        result.remaining_size = order.size;
        
        total_orders_processed_++;
        
        // 验证订单
        if (!validateOrder(order, result)) {
            order.status = OrderStatus::REJECTED;
            orders_rejected_++;
            return result;
        }
        
        // 更新市场数据
        updateMarketData();
        
        // 设置订单状态为已提交
        order.status = OrderStatus::SUBMITTED;
        order.updated_time = std::chrono::system_clock::now();
        
        // 根据订单类型处理
        switch (order.type) {
            case OrderType::MARKET:
                return executeMarketOrder(order);
            case OrderType::LIMIT:
                return handleLimitOrder(order);
            case OrderType::STOP:
                return handleStopOrder(order);
            case OrderType::STOP_LIMIT:
                return handleStopLimitOrder(order);
            default:
                result.new_status = OrderStatus::REJECTED;
                result.reason = "Unsupported order type";
                orders_rejected_++;
                return result;
        }
    }
    
    /**
     * @brief 处理待处理订单
     * 在每个时间步调用
     */
    void processPendingOrders() {
        updateMarketData();
        
        std::vector<Order*> orders_to_remove;
        std::vector<Order*> temp_orders;
        
        // 从优先队列中取出所有订单进行处理
        while (!pending_orders_.empty()) {
            Order* order = pending_orders_.top();
            pending_orders_.pop();
            temp_orders.push_back(order);
            
            ExecutionResult result = tryExecuteOrder(*order);
            
            if (result.new_status == OrderStatus::COMPLETED ||
                result.new_status == OrderStatus::REJECTED ||
                result.new_status == OrderStatus::CANCELED) {
                orders_to_remove.push_back(order);
            }
        }
        
        // 将未完成的订单重新加入队列
        for (Order* order : temp_orders) {
            bool should_remove = std::find(orders_to_remove.begin(), orders_to_remove.end(), order) 
                               != orders_to_remove.end();
            if (!should_remove) {
                pending_orders_.push(order);
            }
        }
    }
    
    /**
     * @brief 取消订单
     * @param order_id 订单ID
     * @return true if successful
     */
    bool cancelOrder(size_t order_id) {
        // 在实际实现中，需要维护一个订单映射
        // 这里简化处理
        return true;
    }
    
    /**
     * @brief 获取统计信息
     */
    struct Statistics {
        size_t total_orders;
        size_t executed_orders;
        size_t rejected_orders;
        double total_volume;
        double execution_rate;
        
        Statistics() : total_orders(0), executed_orders(0), rejected_orders(0),
                      total_volume(0.0), execution_rate(0.0) {}
    };
    
    Statistics getStatistics() const {
        Statistics stats;
        stats.total_orders = total_orders_processed_;
        stats.executed_orders = orders_executed_;
        stats.rejected_orders = orders_rejected_;
        stats.total_volume = total_volume_executed_;
        stats.execution_rate = (total_orders_processed_ > 0) ? 
                              static_cast<double>(orders_executed_) / total_orders_processed_ : 0.0;
        return stats;
    }
    
    /**
     * @brief 获取当前市场数据
     * @return 市场快照
     */
    const MarketSnapshot& getCurrentMarket() const {
        return current_market_;
    }
    
private:
    /**
     * @brief 订单比较器（用于优先队列）
     * @param a 订单A
     * @param b 订单B
     * @return true if a has lower priority than b
     */
    static bool orderComparator(Order* a, Order* b) {
        // 按创建时间排序，先进先出
        return a->created_time > b->created_time;
    }
    
    /**
     * @brief 验证订单
     * @param order 订单
     * @param result 执行结果
     * @return true if valid
     */
    bool validateOrder(const Order& order, ExecutionResult& result) {
        // 检查订单大小
        if (order.size < min_order_size_) {
            result.new_status = OrderStatus::REJECTED;
            result.reason = "Order size too small";
            return false;
        }
        
        if (order.size > max_order_size_) {
            result.new_status = OrderStatus::REJECTED;
            result.reason = "Order size too large";
            return false;
        }
        
        // 检查是否允许碎股
        if (!allow_fractional_shares_ && order.size != std::floor(order.size)) {
            result.new_status = OrderStatus::REJECTED;
            result.reason = "Fractional shares not allowed";
            return false;
        }
        
        // 检查价格有效性（限价单和止损限价单）
        if ((order.type == OrderType::LIMIT || order.type == OrderType::STOP_LIMIT) && 
            (order.price <= 0.0 || isNaN(order.price))) {
            result.new_status = OrderStatus::REJECTED;
            result.reason = "Invalid limit price";
            return false;
        }
        
        // 检查止损价格有效性
        if ((order.type == OrderType::STOP || order.type == OrderType::STOP_LIMIT) && 
            (order.stop_price <= 0.0 || isNaN(order.stop_price))) {
            result.new_status = OrderStatus::REJECTED;
            result.reason = "Invalid stop price";
            return false;
        }
        
        return true;
    }
    
    /**
     * @brief 更新市场数据
     */
    void updateMarketData() {
        if (!data_feed_ || data_feed_->len() == 0) {
            return;
        }
        
        auto ohlcv = data_feed_->getCurrentOHLCV();
        
        current_market_.last_price = ohlcv.close;
        current_market_.high_price = ohlcv.high;
        current_market_.low_price = ohlcv.low;
        current_market_.volume = ohlcv.volume;
        current_market_.timestamp = ohlcv.datetime;
        
        // 简化的买卖价差（实际应该从orderbook获取）
        double spread = ohlcv.close * 0.001;  // 0.1%的价差
        current_market_.bid_price = ohlcv.close - spread / 2.0;
        current_market_.ask_price = ohlcv.close + spread / 2.0;
    }
    
    /**
     * @brief 执行市价单
     * @param order 订单
     * @return 执行结果
     */
    ExecutionResult executeMarketOrder(Order& order) {
        ExecutionResult result;
        result.order_id = order.id;
        
        // 确定执行价格
        double execution_price;
        if (order.side == OrderSide::BUY) {
            execution_price = current_market_.ask_price;
        } else {
            execution_price = current_market_.bid_price;
        }
        
        if (isNaN(execution_price) || execution_price <= 0.0) {
            execution_price = current_market_.last_price;
        }
        
        if (isNaN(execution_price) || execution_price <= 0.0) {
            result.new_status = OrderStatus::REJECTED;
            result.reason = "No valid market price";
            return result;
        }
        
        // 应用滑点
        if (slippage_model_) {
            execution_price = slippage_model_->calculateSlippage(order, execution_price, order.size);
        }
        
        // 执行订单
        order.executed_price = execution_price;
        order.executed_size = order.size;
        order.status = OrderStatus::COMPLETED;
        order.updated_time = std::chrono::system_clock::now();
        
        result.executed_price = execution_price;
        result.executed_size = order.size;
        result.remaining_size = 0.0;
        result.new_status = OrderStatus::COMPLETED;
        
        orders_executed_++;
        total_volume_executed_ += order.size;
        
        return result;
    }
    
    /**
     * @brief 处理限价单
     * @param order 订单
     * @return 执行结果
     */
    ExecutionResult handleLimitOrder(Order& order) {
        ExecutionResult result;
        result.order_id = order.id;
        result.remaining_size = order.size;
        
        // 检查是否可以立即执行
        bool can_execute = false;
        double market_price = current_market_.last_price;
        
        if (order.side == OrderSide::BUY && market_price <= order.price) {
            can_execute = true;
        } else if (order.side == OrderSide::SELL && market_price >= order.price) {
            can_execute = true;
        }
        
        if (can_execute) {
            return executeAtPrice(order, order.price);
        } else {
            // 加入待处理队列
            pending_orders_.push(&order);
            result.new_status = OrderStatus::SUBMITTED;
            return result;
        }
    }
    
    /**
     * @brief 处理止损单
     * @param order 订单
     * @return 执行结果
     */
    ExecutionResult handleStopOrder(Order& order) {
        ExecutionResult result;
        result.order_id = order.id;
        result.remaining_size = order.size;
        
        // 检查是否触发止损
        bool stop_triggered = false;
        double market_price = current_market_.last_price;
        
        if (order.side == OrderSide::BUY && market_price >= order.stop_price) {
            stop_triggered = true;
        } else if (order.side == OrderSide::SELL && market_price <= order.stop_price) {
            stop_triggered = true;
        }
        
        if (stop_triggered) {
            // 转换为市价单执行
            order.type = OrderType::MARKET;
            return executeMarketOrder(order);
        } else {
            // 加入待处理队列
            pending_orders_.push(&order);
            result.new_status = OrderStatus::SUBMITTED;
            return result;
        }
    }
    
    /**
     * @brief 处理止损限价单
     * @param order 订单
     * @return 执行结果
     */
    ExecutionResult handleStopLimitOrder(Order& order) {
        ExecutionResult result;
        result.order_id = order.id;
        result.remaining_size = order.size;
        
        // 检查是否触发止损
        bool stop_triggered = false;
        double market_price = current_market_.last_price;
        
        if (order.side == OrderSide::BUY && market_price >= order.stop_price) {
            stop_triggered = true;
        } else if (order.side == OrderSide::SELL && market_price <= order.stop_price) {
            stop_triggered = true;
        }
        
        if (stop_triggered) {
            // 转换为限价单处理
            order.type = OrderType::LIMIT;
            return handleLimitOrder(order);
        } else {
            // 加入待处理队列
            pending_orders_.push(&order);
            result.new_status = OrderStatus::SUBMITTED;
            return result;
        }
    }
    
    /**
     * @brief 尝试执行订单
     * @param order 订单
     * @return 执行结果
     */
    ExecutionResult tryExecuteOrder(Order& order) {
        switch (order.type) {
            case OrderType::LIMIT:
                return handleLimitOrder(order);
            case OrderType::STOP:
                return handleStopOrder(order);
            case OrderType::STOP_LIMIT:
                return handleStopLimitOrder(order);
            default:
                ExecutionResult result;
                result.order_id = order.id;
                result.new_status = OrderStatus::REJECTED;
                result.reason = "Unsupported order type for pending execution";
                return result;
        }
    }
    
    /**
     * @brief 以指定价格执行订单
     * @param order 订单
     * @param price 执行价格
     * @return 执行结果
     */
    ExecutionResult executeAtPrice(Order& order, double price) {
        ExecutionResult result;
        result.order_id = order.id;
        
        // 应用滑点
        double execution_price = price;
        if (slippage_model_) {
            execution_price = slippage_model_->calculateSlippage(order, price, order.size);
        }
        
        // 执行订单
        order.executed_price = execution_price;
        order.executed_size = order.size;
        order.status = OrderStatus::COMPLETED;
        order.updated_time = std::chrono::system_clock::now();
        
        result.executed_price = execution_price;
        result.executed_size = order.size;
        result.remaining_size = 0.0;
        result.new_status = OrderStatus::COMPLETED;
        
        orders_executed_++;
        total_volume_executed_ += order.size;
        
        return result;
    }
};

} // namespace broker
} // namespace backtrader