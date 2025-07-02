#pragma once

#include "LineRoot.h"
#include "feeds/DataFeed.h"
#include "indicators/IndicatorBase.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace backtrader {
namespace strategy {

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
    
    Order() : id(0), type(OrderType::MARKET), side(OrderSide::BUY), 
              status(OrderStatus::CREATED), size(0.0), price(0.0), 
              stop_price(0.0), executed_size(0.0), executed_price(0.0) {}
};

/**
 * @brief 持仓信息结构
 */
struct Position {
    double size;          // 持仓数量（正数为多头，负数为空头）
    double price;         // 平均成本价格
    double unrealized_pnl;// 未实现盈亏
    double realized_pnl;  // 已实现盈亏
    
    Position() : size(0.0), price(0.0), unrealized_pnl(0.0), realized_pnl(0.0) {}
    
    bool isLong() const { return size > 0.0; }
    bool isShort() const { return size < 0.0; }
    bool isEmpty() const { return size == 0.0; }
};

/**
 * @brief 策略基类
 * 
 * 提供策略开发的基础框架，包括：
 * - 数据访问接口
 * - 指标管理
 * - 订单管理
 * - 持仓管理
 * - 回调函数
 */
class StrategyBase {
protected:
    std::string name_;
    std::vector<std::shared_ptr<data::DataFeed>> data_feeds_;
    std::vector<std::shared_ptr<IndicatorBase>> indicators_;
    
    // 订单管理
    std::vector<Order> orders_;
    size_t next_order_id_;
    
    // 持仓管理
    std::unordered_map<std::string, Position> positions_;
    
    // 策略参数
    std::unordered_map<std::string, double> params_;
    
    // 统计信息
    double total_pnl_;
    double commission_;
    size_t trade_count_;
    
public:
    /**
     * @brief 构造函数
     * @param name 策略名称
     */
    explicit StrategyBase(const std::string& name = "Strategy")
        : name_(name), next_order_id_(1), total_pnl_(0.0), 
          commission_(0.001), trade_count_(0) {}
    
    virtual ~StrategyBase() = default;
    
    // 基础接口
    
    /**
     * @brief 获取策略名称
     * @return 策略名称
     */
    const std::string& getName() const { return name_; }
    
    /**
     * @brief 添加数据源
     * @param data_feed 数据源
     */
    void addDataFeed(std::shared_ptr<data::DataFeed> data_feed) {
        data_feeds_.push_back(data_feed);
    }
    
    /**
     * @brief 获取主数据源
     * @return 主数据源（第一个添加的数据源）
     */
    std::shared_ptr<data::DataFeed> getData() const {
        return data_feeds_.empty() ? nullptr : data_feeds_[0];
    }
    
    /**
     * @brief 获取指定索引的数据源
     * @param index 数据源索引
     * @return 数据源
     */
    std::shared_ptr<data::DataFeed> getData(size_t index) const {
        return (index < data_feeds_.size()) ? data_feeds_[index] : nullptr;
    }
    
    /**
     * @brief 添加指标
     * @param indicator 指标
     */
    void addIndicator(std::shared_ptr<IndicatorBase> indicator) {
        indicators_.push_back(indicator);
    }
    
    /**
     * @brief 获取指标
     * @param index 指标索引
     * @return 指标
     */
    std::shared_ptr<IndicatorBase> getIndicator(size_t index) const {
        return (index < indicators_.size()) ? indicators_[index] : nullptr;
    }
    
    // 参数管理
    
    /**
     * @brief 设置参数
     * @param key 参数名
     * @param value 参数值
     */
    void setParam(const std::string& key, double value) {
        params_[key] = value;
    }
    
    /**
     * @brief 获取参数
     * @param key 参数名
     * @param default_value 默认值
     * @return 参数值
     */
    double getParam(const std::string& key, double default_value = 0.0) const {
        auto it = params_.find(key);
        return (it != params_.end()) ? it->second : default_value;
    }
    
    // 订单管理
    
    /**
     * @brief 买入市价单
     * @param size 数量
     * @param data_index 数据源索引
     * @return 订单ID
     */
    size_t buy(double size = 1.0, size_t data_index = 0) {
        return createOrder(OrderType::MARKET, OrderSide::BUY, size, 0.0, data_index);
    }
    
    /**
     * @brief 卖出市价单
     * @param size 数量
     * @param data_index 数据源索引
     * @return 订单ID
     */
    size_t sell(double size = 1.0, size_t data_index = 0) {
        return createOrder(OrderType::MARKET, OrderSide::SELL, size, 0.0, data_index);
    }
    
    /**
     * @brief 买入限价单
     * @param price 限价
     * @param size 数量
     * @param data_index 数据源索引
     * @return 订单ID
     */
    size_t buyLimit(double price, double size = 1.0, size_t data_index = 0) {
        return createOrder(OrderType::LIMIT, OrderSide::BUY, size, price, data_index);
    }
    
    /**
     * @brief 卖出限价单
     * @param price 限价
     * @param size 数量
     * @param data_index 数据源索引
     * @return 订单ID
     */
    size_t sellLimit(double price, double size = 1.0, size_t data_index = 0) {
        return createOrder(OrderType::LIMIT, OrderSide::SELL, size, price, data_index);
    }
    
    /**
     * @brief 取消订单
     * @param order_id 订单ID
     * @return true if successful
     */
    bool cancelOrder(size_t order_id) {
        for (auto& order : orders_) {
            if (order.id == order_id && 
                (order.status == OrderStatus::CREATED || order.status == OrderStatus::SUBMITTED)) {
                order.status = OrderStatus::CANCELED;
                order.updated_time = std::chrono::system_clock::now();
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief 获取订单
     * @param order_id 订单ID
     * @return 订单指针
     */
    const Order* getOrder(size_t order_id) const {
        for (const auto& order : orders_) {
            if (order.id == order_id) {
                return &order;
            }
        }
        return nullptr;
    }
    
    /**
     * @brief 获取所有订单
     * @return 订单列表
     */
    const std::vector<Order>& getOrders() const {
        return orders_;
    }
    
    // 持仓管理
    
    /**
     * @brief 获取持仓
     * @param symbol 品种名称
     * @return 持仓信息
     */
    const Position& getPosition(const std::string& symbol = "default") const {
        static Position empty_position;
        auto it = positions_.find(symbol);
        return (it != positions_.end()) ? it->second : empty_position;
    }
    
    /**
     * @brief 获取持仓数量
     * @param symbol 品种名称
     * @return 持仓数量
     */
    double getPositionSize(const std::string& symbol = "default") const {
        return getPosition(symbol).size;
    }
    
    /**
     * @brief 是否持有多头头寸
     * @param symbol 品种名称
     * @return true if long
     */
    bool isLong(const std::string& symbol = "default") const {
        return getPosition(symbol).isLong();
    }
    
    /**
     * @brief 是否持有空头头寸
     * @param symbol 品种名称
     * @return true if short
     */
    bool isShort(const std::string& symbol = "default") const {
        return getPosition(symbol).isShort();
    }
    
    /**
     * @brief 是否空仓
     * @param symbol 品种名称
     * @return true if empty
     */
    bool isEmpty(const std::string& symbol = "default") const {
        return getPosition(symbol).isEmpty();
    }
    
    // 统计信息
    
    /**
     * @brief 获取总盈亏
     * @return 总盈亏
     */
    double getTotalPnL() const { return total_pnl_; }
    
    /**
     * @brief 获取交易次数
     * @return 交易次数
     */
    size_t getTradeCount() const { return trade_count_; }
    
    /**
     * @brief 设置手续费率
     * @param commission 手续费率
     */
    void setCommission(double commission) { commission_ = commission; }
    
    /**
     * @brief 获取手续费率
     * @return 手续费率
     */
    double getCommission() const { return commission_; }
    
    // 策略回调函数（子类需要实现）
    
    /**
     * @brief 策略初始化回调
     * 在策略开始运行前调用一次
     */
    virtual void init() {}
    
    /**
     * @brief 策略主逻辑回调
     * 每个数据点都会调用
     */
    virtual void next() = 0;
    
    /**
     * @brief 订单状态变化回调
     * @param order 订单信息
     */
    virtual void onOrder(const Order& order) {}
    
    /**
     * @brief 交易完成回调
     * @param order 订单信息
     */
    virtual void onTrade(const Order& order) {}
    
    /**
     * @brief 策略结束回调
     * 在策略运行结束后调用一次
     */
    virtual void stop() {}
    
    // 内部处理函数
    
    /**
     * @brief 处理一个数据点
     * 内部框架调用，不应该被用户直接调用
     */
    void processNext() {
        // 更新指标
        for (auto& indicator : indicators_) {
            indicator->calculate();
        }
        
        // 更新持仓盈亏
        updateUnrealizedPnL();
        
        // 调用策略逻辑
        next();
    }
    
    /**
     * @brief 初始化策略
     * 内部框架调用
     */
    void initialize() {
        init();
    }
    
    /**
     * @brief 结束策略
     * 内部框架调用
     */
    void finalize() {
        stop();
    }
    
protected:
    /**
     * @brief 创建订单
     * @param type 订单类型
     * @param side 订单方向
     * @param size 数量
     * @param price 价格
     * @param data_index 数据源索引
     * @return 订单ID
     */
    size_t createOrder(OrderType type, OrderSide side, double size, 
                      double price, size_t data_index = 0) {
        if (size <= 0.0 || data_index >= data_feeds_.size()) {
            return 0;
        }
        
        Order order;
        order.id = next_order_id_++;
        order.type = type;
        order.side = side;
        order.status = OrderStatus::CREATED;
        order.size = size;
        order.price = price;
        order.created_time = std::chrono::system_clock::now();
        order.updated_time = order.created_time;
        
        orders_.push_back(order);
        
        // 简化处理：立即执行市价单
        if (type == OrderType::MARKET) {
            executeOrder(orders_.size() - 1, data_index);
        }
        
        return order.id;
    }
    
    /**
     * @brief 执行订单
     * @param order_index 订单索引
     * @param data_index 数据源索引
     */
    void executeOrder(size_t order_index, size_t data_index) {
        if (order_index >= orders_.size() || data_index >= data_feeds_.size()) {
            return;
        }
        
        auto& order = orders_[order_index];
        auto data_feed = data_feeds_[data_index];
        
        if (!data_feed || data_feed->len() == 0) {
            order.status = OrderStatus::REJECTED;
            return;
        }
        
        // 获取当前价格
        double execution_price = data_feed->close()->get(0);
        if (isNaN(execution_price)) {
            order.status = OrderStatus::REJECTED;
            return;
        }
        
        // 执行订单
        order.executed_size = order.size;
        order.executed_price = execution_price;
        order.status = OrderStatus::COMPLETED;
        order.updated_time = std::chrono::system_clock::now();
        
        // 更新持仓
        updatePosition(order, "default");
        
        // 计算手续费
        double commission_cost = order.executed_size * order.executed_price * commission_;
        total_pnl_ -= commission_cost;
        
        trade_count_++;
        
        // 触发回调
        onOrder(order);
        onTrade(order);
    }
    
    /**
     * @brief 更新持仓
     * @param order 订单
     * @param symbol 品种名称
     */
    void updatePosition(const Order& order, const std::string& symbol) {
        Position& position = positions_[symbol];
        
        double order_value = order.executed_size * order.executed_price;
        
        if (order.side == OrderSide::BUY) {
            if (position.size >= 0) {
                // 增加多头持仓
                double total_value = position.size * position.price + order_value;
                position.size += order.executed_size;
                position.price = (position.size > 0) ? total_value / position.size : 0.0;
            } else {
                // 平空头持仓
                double close_size = std::min(order.executed_size, -position.size);
                double pnl = close_size * (position.price - order.executed_price);
                position.realized_pnl += pnl;
                total_pnl_ += pnl;
                
                position.size += order.executed_size;
                if (position.size > 0) {
                    position.price = order.executed_price;
                }
            }
        } else { // SELL
            if (position.size <= 0) {
                // 增加空头持仓
                double total_value = -position.size * position.price + order_value;
                position.size -= order.executed_size;
                position.price = (position.size < 0) ? total_value / (-position.size) : 0.0;
            } else {
                // 平多头持仓
                double close_size = std::min(order.executed_size, position.size);
                double pnl = close_size * (order.executed_price - position.price);
                position.realized_pnl += pnl;
                total_pnl_ += pnl;
                
                position.size -= order.executed_size;
                if (position.size < 0) {
                    position.price = order.executed_price;
                }
            }
        }
    }
    
    /**
     * @brief 更新未实现盈亏
     */
    void updateUnrealizedPnL() {
        if (data_feeds_.empty()) return;
        
        auto data_feed = data_feeds_[0];
        double current_price = data_feed->close()->get(0);
        
        if (isNaN(current_price)) return;
        
        for (auto& [symbol, position] : positions_) {
            if (position.size != 0.0) {
                if (position.size > 0) {
                    // 多头未实现盈亏
                    position.unrealized_pnl = position.size * (current_price - position.price);
                } else {
                    // 空头未实现盈亏
                    position.unrealized_pnl = -position.size * (position.price - current_price);
                }
            } else {
                position.unrealized_pnl = 0.0;
            }
        }
    }
};

} // namespace strategy
} // namespace backtrader